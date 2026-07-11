#include "ImGuiHandler.h"
#include "imgui_impl_metal.h"

#import "Foundation/Foundation.h"
#include "Metal/Metal.h"

#if TARGET_OS_OSX
#import <AppKit/NSView.h>
#import <QuartzCore/QuartzCore.h>
#include "imgui_impl_osx.h"
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

_VIEW_* nativeView;

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
#if TARGET_OS_OSX
    ImGui_ImplOSX_NewFrame(nativeView);
#endif
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

    // The drawable can be smaller than the size this frame was built for
    // (live resize, device rotation, or layer contentsScale != backingScaleFactor).
    // Metal validation aborts on any scissor rect exceeding the render target,
    // so clamp every clip rect to the real target size before encoding.
    id<MTLTexture> targetTexture = renderPassDescriptor.colorAttachments[0].texture;
    if (targetTexture != nil)
    {
        const ImVec2 off = draw_data->DisplayPos;
        const float scaleX = draw_data->FramebufferScale.x > 0.0f ? draw_data->FramebufferScale.x : 1.0f;
        const float scaleY = draw_data->FramebufferScale.y > 0.0f ? draw_data->FramebufferScale.y : 1.0f;
        const float maxX = off.x + (float)targetTexture.width / scaleX;
        const float maxY = off.y + (float)targetTexture.height / scaleY;
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            ImDrawList* cmdList = draw_data->CmdLists[n];
            for (int c = 0; c < cmdList->CmdBuffer.Size; c++)
            {
                ImDrawCmd& cmd = cmdList->CmdBuffer[c];
                if (cmd.ClipRect.x > maxX) cmd.ClipRect.x = maxX;
                if (cmd.ClipRect.y > maxY) cmd.ClipRect.y = maxY;
                if (cmd.ClipRect.z > maxX) cmd.ClipRect.z = maxX;
                if (cmd.ClipRect.w > maxY) cmd.ClipRect.w = maxY;
            }
        }
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
    
    nativeView = (__bridge _VIEW_*)Graphics::MetalContext::GetNativeView();
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
#if TARGET_OS_OSX
    ImGui_ImplOSX_Init(nativeView);
#endif
}

void ImGuiHandler::Update(const ImGuiUpdateFn& updateFn) {
    void* drawablePtr = Graphics::MetalContext::GetCurrentDrawable();
    if (!drawablePtr) {
        return;
    }
    drawable = (__bridge id<CAMetalDrawable>)drawablePtr;
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
#if TARGET_OS_OSX
    ImGui_ImplOSX_Shutdown();
#endif
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
#if TARGET_OS_IOS
    // Pass events to imgui
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
#endif
}

void ImGuiHandler::OnSelection(int objectId, bool state)
{
}

void ImGuiHandler::OnImGuiRender()
{
}
