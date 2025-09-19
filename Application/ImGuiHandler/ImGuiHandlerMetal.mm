#include "ImGuiHandler.h"
#include "imgui_impl_metal.h"

#import "Foundation/Foundation.h"
#include "Metal/Metal.h"

#if TARGET_OS_OSX
#import <AppKit/NSView.h>
#elif TARGET_OS_IOS
#import <UIKit/UIKit.h>
#endif

#include "Platform/Metal/MetalContext.h"

#include "Events/Input.h"

#include "Events/EventTypes/ApplicationEvent.h"
#include "Events/EventTypes/MouseEvent.h"
#include "Events/EventTypes/KeyEvent.h"

id<MTLCommandBuffer> commandBuffer;
MTLRenderPassDescriptor* renderPassDescriptor;
id<CAMetalDrawable> drawable;

UIView* nativeView;

void ImGuiHandler::NewFrame() {
    
    //        id<MTLCommandBuffer> commandBuffer = [(__bridge id<MTLCommandQueue>)m_CommandQueue commandBuffer];
    //        MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
    //        id<CAMetalDrawable> drawable = (__bridge id<CAMetalDrawable>)m_CurrentDrawable;
    //
    //        passDesc.colorAttachments[0].texture = drawable.texture;
    //        passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
    //        passDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.2, 0.3, 0.3, 1.0);
    //        passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
    //
    //        id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
    //        [encoder endEncoding];
    //        [commandBuffer presentDrawable:(__bridge id<CAMetalDrawable>)m_CurrentDrawable];
    //        [commandBuffer commit];
    
    id<MTLCommandQueue> commandQueue = (__bridge id<MTLCommandQueue>)Graphics::MetalContext::GetCurrentCommandQueue();
    commandBuffer = [commandQueue commandBuffer];
    commandBuffer.label = @"ImGui_command_buffer";
    renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
    renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    
    if (renderPassDescriptor == nil)
    {
        [commandBuffer commit];
        return;
    }
    
    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    ImGui::NewFrame();
}

inline void ImGuiHandler::Render() {
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    
    if (renderPassDescriptor == nil)
    {
        // Handle case where descriptor couldn't be created
        [commandBuffer commit];
        return;
    }
    
    
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.2, 0.3, 0.3, 1.0);
    
    id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    [renderEncoder pushDebugGroup:@"Dear ImGui rendering"];
    ImGui_ImplMetal_RenderDrawData(draw_data, commandBuffer, renderEncoder);
    [renderEncoder popDebugGroup];
    [renderEncoder endEncoding];
    
    // Present
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}

ImGuiHandler::ImGuiHandler(void* window, const char* glsl_version) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    nativeView = (__bridge UIView*)Graphics::MetalContext::GetNativeView();
    id<MTLDevice> device = (__bridge id<MTLDevice>)Graphics::MetalContext::GetCurrentDevice();
    
    CAMetalLayer* metalLayer = (CAMetalLayer*)nativeView.layer;
    
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.DisplaySize.x = metalLayer.drawableSize.width;
    io.DisplaySize.y = metalLayer.drawableSize.height;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    
#ifdef IMGUI_DOCKING_BRANCH_ENABLED
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigViewportsNoTaskBarIcon = true;
#endif
    
    ImGui::StyleColorsDark();
    
    ImGuiStyle& style = ImGui::GetStyle();
    
    
    ImGui::StyleColorsDark();
    
    ImGui_ImplMetal_Init(device);
}

void ImGuiHandler::Update(const ImGuiUpdateFn& updateFn) {
    void* drawablePtr = Graphics::MetalContext::GetCurrentDrawable();
    if (!drawablePtr) {
        return;
    }
    drawable = (__bridge id<CAMetalDrawable>)Graphics::MetalContext::GetCurrentDrawable();
    CAMetalLayer* metalLayer = (CAMetalLayer*)nativeView.layer;
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.DisplaySize.x = metalLayer.drawableSize.width;
    io.DisplaySize.y = metalLayer.drawableSize.height;
    
    this->NewFrame();
    
#ifdef IMGUI_DOCKING_BRANCH_ENABLED
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockspace_flags);
#endif
    
    updateFn();
    
    this->Render();
}

ImGuiHandler::~ImGuiHandler() {
    ImGui_ImplMetal_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiHandler::OnAttach()
{
}

void ImGuiHandler::OnDetach()
{
}

void ImGuiHandler::OnUpdateLayer()
{
}

void ImGuiHandler::OnDrawUpdate()
{
}

void ImGuiHandler::OnEvent(Application::Event& event)
{
    std::cout << event.ToString() << std::endl;
    UIEvent* uiEvent = (__bridge UIEvent*)event.m_NativeEvent;
    UITouch *anyTouch = uiEvent.allTouches.anyObject;
    CGPoint touchLocation = [anyTouch locationInView:nativeView];
    ImGuiIO &io = ImGui::GetIO();
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    io.AddMousePosEvent(touchLocation.x, touchLocation.y);
    
    BOOL hasActiveTouch = NO;
    for (UITouch *touch in uiEvent.allTouches)
    {
        if (touch.phase != UITouchPhaseEnded && touch.phase != UITouchPhaseCancelled)
        {
            hasActiveTouch = YES;
            break;
        }
    }
    io.AddMouseButtonEvent(0, hasActiveTouch);
}

void ImGuiHandler::OnSelection(int objectId, bool state)
{
}

void ImGuiHandler::OnImGuiRender()
{
}
