#pragma once

#include "Renderer/Renderer.h"
#include "Renderer/GraphicsContext.h"

#include <Metal/Metal.hpp>

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
    
    static Ref<MetalContext> Get() { return std::static_pointer_cast<MetalContext>(Renderer::GetContext()); }
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
    };

}
