#include "Platform/Metal/MetalContext.h"
#include <GraphicsCore.h>

#import "Foundation/Foundation.h"
#import "Metal/Metal.hpp"

#if TARGET_OS_OSX
    #import <AppKit/NSView.h>
#elif TARGET_OS_IOS
    #import <UIKit/UIKit.h>
#endif

namespace Graphics {

    MetalContext::MetalContext(void* nativeView)
        : m_NativeView(nativeView)
    {
        GRAPHICS_CORE_ASSERT(nativeView, "Native view is null!");
    }

    MetalContext::~MetalContext()
    {
        
    }

    void MetalContext::Init()
    {
        m_Device = MTL::CreateSystemDefaultDevice()->retain();
        GRAPHICS_CORE_ASSERT(m_Device, "Failed to create Metal device!");
        if (![NSThread isMainThread]) {
            NSLog(@"FATAL ERROR: UI initialization must occur on the main thread. Terminating.");
            abort(); // Forcefully and immediately terminates the app.
        }

        // Set up Metal layer
        CAMetalLayer* metalLayer = [CAMetalLayer layer];
        metalLayer.device = (__bridge id<MTLDevice>)m_Device;
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metalLayer.framebufferOnly = YES;

        #if TARGET_OS_OSX
            NSView* view = (__bridge NSView*)m_NativeView;
            metalLayer.contentsScale = view.window.backingScaleFactor;
            view.wantsLayer = YES;
            view.layer = metalLayer;
        #elif TARGET_OS_IOS
            UIView* view = (__bridge UIView*)m_NativeView;
            if (![view.layer isKindOfClass:[CAMetalLayer class]])
            {
                // This is a programmer error. The UIView passed to the context
                // was not configured correctly to be backed by a CAMetalLayer.
                GRAPHICS_CORE_ASSERT(false, "UIView's layer is not a CAMetalLayer. To fix this, either use an MTKView or override the view's `+layerClass` method to return `[CAMetalLayer class]`.");
            }
            metalLayer.contentsScale = view.contentScaleFactor;
            // iOS fix: do NOT assign to view.layer (read-only)
            // Instead, cast existing layer (must be a CAMetalLayer from subclass or MTKView)
            CAMetalLayer* existingLayer = (CAMetalLayer*)view.layer;
            existingLayer.device = metalLayer.device;
            existingLayer.pixelFormat = metalLayer.pixelFormat;
            existingLayer.framebufferOnly = metalLayer.framebufferOnly;
            existingLayer.contentsScale = metalLayer.contentsScale;

            metalLayer = existingLayer;
        #endif

        metalLayer.drawableSize = metalLayer.bounds.size;
        
        // Store the layer
        m_MetalLayer = (__bridge_retained void*)metalLayer;
        m_CommandQueue = m_Device->newCommandQueue();
        m_CommandQueue->setLabel(NS::String::string("Application Queue", NS::UTF8StringEncoding));

        GRAPHICS_CORE_ASSERT(m_CommandQueue, "Failed to create command queue!");
    }

    void MetalContext::SwapBuffers()
    {
        // Present drawable (this is where you'd do command encoding in real use)
        if (!m_MetalLayer) return;
        
        if (m_CurrentDrawable) {
            CFRelease(m_CurrentDrawable);
            m_CurrentDrawable = nil; // It's good practice to nullify the pointer after release.
        }

        m_CurrentDrawable = (__bridge_retained void*)[(__bridge CAMetalLayer*)m_MetalLayer nextDrawable];
        if (m_CurrentDrawable == nil) {
            NSLog(@"Current Drawable is null");
            return;
        };
//
//        // Example: Fill screen with a color (stub rendering)
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
    }

    void MetalContext::setActiveRenderCommandBuffer(MTL::CommandBuffer* commandBuffer) {
        if(commandBuffer == nullptr && m_CurrentCommandBuffer) {
//            m_CurrentCommandBuffer->release();
        }
        m_CurrentCommandBuffer = commandBuffer;
    }

    void MetalContext::setActiveRenderCommandEncoder(MTL::RenderCommandEncoder* encoder) {
        if(encoder == nullptr && m_CurrentCommandEncoder) {
//            m_CurrentCommandEncoder->release();
        }
        m_CurrentCommandEncoder = encoder;
    }

    void MetalContext::setActiveComputeCommandEncoder(MTL::ComputeCommandEncoder* encoder) {
        if(encoder == nullptr && m_CurrentComputeCommandEncoder) {
//            m_CurrentComputeCommandEncoder->release();
        }
        m_CurrentComputeCommandEncoder = encoder;
    }

    void MetalContext::setActiveRenderPassDescriptor(MTL::RenderPassDescriptor* descriptor) {
        m_CurrentRenderPassDescriptor = descriptor;
    }

    void MetalContext::setActivePipelineStateDescriptor(MTL::RenderPipelineDescriptor* descriptor) {
        m_CurrentPipelineStateDecsriptor = descriptor;
    }

}
