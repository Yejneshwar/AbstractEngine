#pragma once

#include "Renderer/Renderer.h"
#include "Renderer/GraphicsContext.h"

#include <Metal/Metal.hpp>

#if TARGET_OS_OSX
#define _VIEW_ NSView
#elif TARGET_OS_IOS
#define _VIEW_ UIView
#endif

namespace Graphics {

class MetalContext : public GraphicsContext {
public:
    // nativeView: NSView* (macOS) or UIView* (iOS), passed as void*
    MetalContext(void* nativeView);
    virtual ~MetalContext();
    virtual void Init() override;
    virtual void SwapBuffers() override;
    MTL::Device* GetDevice() { return m_Device; }
    void setActiveRenderCommandBuffer(MTL::CommandBuffer* commandBuffer);
    void setActiveRenderCommandEncoder(MTL::RenderCommandEncoder* encoder);
    void setActiveComputeCommandEncoder(MTL::ComputeCommandEncoder* encoder);
    void setActiveRenderPassDescriptor(MTL::RenderPassDescriptor* descriptor);
    void setActivePipelineStateDescriptor(MTL::RenderPipelineDescriptor* descriptor);
    
    static MetalContext* Get() { return static_cast<MetalContext*>(Renderer::GetContext()); }
    static MTL::Device* GetCurrentDevice() { return Get()->GetDevice(); }
    static MTL::CommandQueue* GetCurrentCommandQueue() { return Get()->m_CommandQueue; }
    static void* GetCurrentDrawable() { return Get()->m_CurrentDrawable; }
    static void* GetMetalLayer() { return Get()->m_MetalLayer; }
    static void* GetNativeView() { return Get()->m_NativeView; }
    static void SetCommandBuffer(MTL::CommandBuffer* commandBuffer) { Get()->setActiveRenderCommandBuffer(commandBuffer); }
    static void SetCommandEncoder(MTL::RenderCommandEncoder* encoder) { Get()->setActiveRenderCommandEncoder(encoder); }
    static void SetComputeCommandEncoder(MTL::ComputeCommandEncoder* encoder) { Get()->setActiveComputeCommandEncoder(encoder); }
    static void SetPipelineStateDecsriptor(MTL::RenderPipelineDescriptor* descriptor) { Get()->setActivePipelineStateDescriptor(descriptor); }
    static void SetRenderPassDescriptor(MTL::RenderPassDescriptor* descriptor) { Get()->setActiveRenderPassDescriptor(descriptor); }
    static MTL::CommandBuffer* GetCurrentCommandBuffer() { return Get()->m_CurrentCommandBuffer; }
    static MTL::RenderCommandEncoder* GetCurrentRenderCommandEncoder() { return Get()->m_CurrentCommandEncoder; }
    static MTL::ComputeCommandEncoder* GetCurrentComputeCommandEncoder() { return Get()->m_CurrentComputeCommandEncoder; }
    static MTL::RenderPassDescriptor* GetCurrentRenderPassDescriptor() { return Get()->m_CurrentRenderPassDescriptor; }
    static MTL::RenderPipelineDescriptor* GetCurrentPipelineStateDecsriptor() { return Get()->m_CurrentPipelineStateDecsriptor; }

    // --- Frame pipelining -------------------------------------------------
    // The CPU may run up to this many frames ahead of the GPU. Per-frame
    // dynamic buffers keep one copy per in-flight frame so the CPU never
    // overwrites memory the GPU is still reading.
    static constexpr uint32_t kMaxFramesInFlight = 3;
    static uint32_t GetFrameInFlightIndex() { return Get()->m_FrameInFlightIndex; }
    static void AdvanceFrameInFlight() { Get()->m_FrameInFlightIndex = (Get()->m_FrameInFlightIndex + 1) % kMaxFramesInFlight; }

    // The most recently committed frame command buffer (retained). Used by
    // rare synchronous operations (pixel readback for picking, updates to
    // single-copy buffers) to wait for the GPU without stalling every frame.
    static void SetLastCommittedCommandBuffer(MTL::CommandBuffer* commandBuffer) {
        MTL::CommandBuffer*& last = Get()->m_LastCommittedCommandBuffer;
        if (commandBuffer) commandBuffer->retain();
        if (last) last->release();
        last = commandBuffer;
    }
    static void WaitForGpuIdle() {
        MTL::CommandBuffer* last = Get()->m_LastCommittedCommandBuffer;
        if (last) last->waitUntilCompleted();
    }

    private:
        //MetalView
        void* m_NativeView;

        MTL::Device* m_Device;
        MTL::CommandQueue* m_CommandQueue = nullptr;
        void* m_MetalLayer = nullptr;
        void* m_CurrentDrawable = nullptr;
        MTL::CommandBuffer* m_CurrentCommandBuffer = nullptr;
        MTL::RenderCommandEncoder* m_CurrentCommandEncoder = nullptr;
        MTL::ComputeCommandEncoder* m_CurrentComputeCommandEncoder = nullptr;
        MTL::RenderPassDescriptor* m_CurrentRenderPassDescriptor = nullptr;
        MTL::RenderPipelineDescriptor* m_CurrentPipelineStateDecsriptor = nullptr;
        uint32_t m_FrameInFlightIndex = 0;
        MTL::CommandBuffer* m_LastCommittedCommandBuffer = nullptr;
    };

}
