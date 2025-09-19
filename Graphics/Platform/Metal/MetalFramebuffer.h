#pragma once

#include "Renderer/Framebuffer.h"
#include <vector>
#include <cassert>

#include <Metal/Metal.hpp>

namespace Graphics {

    class MetalFramebuffer : public Framebuffer
    {
    public:
        MetalFramebuffer(const FramebufferSpecification& spec);
        virtual ~MetalFramebuffer();

        void Invalidate();

        virtual void Bind() override;
        virtual void Unbind() override;

        virtual void Resize(uint32_t width, uint32_t height) override;
        virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;

        virtual void ClearAttachment(uint32_t attachmentIndex, int value) override;

        virtual void DrawToAllColorBuffers() override;

        virtual void SetDrawBuffer(uint8_t index) override;

        virtual void BindColorAttachmentAsTexture(uint32_t index, uint32_t slot) override;

        virtual uintptr_t GetColorAttachmentRendererID(uint32_t index = 0) const override { assert(index < m_ColorAttachments.size()); return reinterpret_cast<uintptr_t>(m_ColorAttachments[index]); }

        virtual uintptr_t GetDepthAttachmentRendererID() const override { return reinterpret_cast<uintptr_t>(m_DepthAttachment); }

        virtual uintptr_t GetStencilAttachmentRendererID() const override { return reinterpret_cast<uintptr_t>(m_StencilAttachment); }

        virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }

        virtual const size_t GetColorAttachmentCount() const override { return m_ColorAttachments.size(); }

        virtual const uint32_t getID() const override { return m_RendererID; }

        virtual void BlitBuffers(uint32_t src, uint32_t srcX0, uint32_t srcY0, uint32_t srcX1, uint32_t srcY1, uint32_t dstX0, uint32_t dstY0, uint32_t dstX1, uint32_t dstY1, uint32_t mask, uint16_t filter) override;
        
        virtual void BlitToColorAttachment(int index, uintptr_t srcTexture) override;
        
        MTL::RenderPassDescriptor* GetRenderPassDescriptor() const;

    private:
        uint32_t m_RendererID = 0;
        FramebufferSpecification m_Specification;

        MTL::Device* m_Device = nullptr;

        std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
        std::vector<MTL::Texture*> m_ColorResolveAttachments;
        FramebufferTextureSpecification m_DepthAttachmentSpecification = FramebufferTextureFormat::DEPTH32FLOAT;

        std::vector<MTL::Texture*> m_ColorAttachments = {};
        MTL::Texture* m_DepthAttachment = nullptr;
        MTL::Texture* m_StencilAttachment = nullptr;
        
        MTL::RenderCommandEncoder* m_RenderCommandEncoder = nullptr;
        MTL::RenderCommandEncoder* m_PreviousRenderCommandEncoder = nullptr;
    };

}
