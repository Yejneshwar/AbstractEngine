#pragma once

#include "Renderer/Framebuffer.h"
#include <vector>
#include <cassert>

namespace Graphics {

	class OpenGLFramebuffer : public Framebuffer
	{
	public:
		OpenGLFramebuffer(const FramebufferSpecification& spec);
		virtual ~OpenGLFramebuffer();

		void Invalidate();

		virtual void Bind() override;
		virtual void Unbind() override;

		virtual void Resize(uint32_t width, uint32_t height) override;
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;

		virtual void ClearAttachment(uint32_t attachmentIndex, int value) override;

		virtual void DrawToAllColorBuffers() override;

		virtual void SetDrawBuffer(uint8_t index) override;

		virtual void BindColorAttachmentAsTexture(uint32_t index, uint32_t slot) override;

		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override { assert(index < m_ColorAttachments.size()); return m_ColorAttachments[index]; }

		virtual uint32_t GetDepthAttachmentRendererID() const override { return m_DepthAttachment; }

		virtual uint32_t GetStencilAttachmentRendererID() const override { return m_StencilAttachment; }

		virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }

		virtual const size_t GetColorAttachmentCount() const override { 
			return m_ColorAttachments.size();
		}

		virtual const uint32_t getID() const override { return m_RendererID; }

		virtual void BlitBuffers(uint32_t src, uint32_t srcX0, uint32_t srcY0, uint32_t srcX1, uint32_t srcY1, uint32_t dstX0, uint32_t dstY0, uint32_t dstX1, uint32_t dstY1, uint32_t mask, uint16_t filter) override;
	private:
		uint32_t m_RendererID = 0;
		FramebufferSpecification m_Specification;

		std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
		FramebufferTextureSpecification m_DepthAttachmentSpecification = FramebufferTextureFormat::None;

		std::vector<uint32_t> m_ColorAttachments = {};
		uint32_t m_DepthAttachment = 100;
		uint32_t m_StencilAttachment = 1;
	};

}
