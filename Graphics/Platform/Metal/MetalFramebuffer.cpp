#include "MetalFrameBuffer.h"
#include "MetalContext.h"

#include <Logger.h>
#include <cassert>

#include <Metal/Metal.hpp>
#include <Foundation/Foundation.hpp>

namespace Graphics {

    static const uint32_t s_MaxFramebufferSize = 8192;

    namespace Utils {

        // Helper to translate our abstract format to a Metal MTLPixelFormat
        static MTL::PixelFormat FBTextureFormatToMTL(FramebufferTextureFormat format)
        {
            switch (format)
            {
                case FramebufferTextureFormat::RGBA8:           return MTL::PixelFormatRGBA8Unorm;
                case FramebufferTextureFormat::RED_INTEGER:     return MTL::PixelFormatR32Sint;
                // Note: Metal does not have a single-channel blue integer format.
                // This would require custom handling, possibly using an RGBA format and ignoring other channels.
                // For this implementation, we'll assert, as it's not a direct mapping.
                case FramebufferTextureFormat::BLUE_INTEGER:    GRAPHICS_CORE_ASSERT(false, "MTLPixelFormat for BLUE_INTEGER not supported directly."); return MTL::PixelFormatInvalid;
#if TARGET_OS_OSX
                case FramebufferTextureFormat::DEPTH24STENCIL8: return MTL::PixelFormatDepth24Unorm_Stencil8;
#else
                case FramebufferTextureFormat::DEPTH24STENCIL8: return MTL::PixelFormatInvalid;
                case FramebufferTextureFormat::DEPTH32FLOAT: return MTL::PixelFormatDepth32Float;
                case FramebufferTextureFormat::DEPTH16UNORM: return MTL::PixelFormatDepth16Unorm;
                case FramebufferTextureFormat::DEPTH32STENCIL8: return MTL::PixelFormatDepth32Float_Stencil8;
#endif
                case FramebufferTextureFormat::None:            return MTL::PixelFormatInvalid;
            }

            GRAPHICS_CORE_ASSERT(false, "Unknown FramebufferTextureFormat!");
            return MTL::PixelFormatInvalid;
        }

        // Helper to determine if a format is a depth/stencil format
        static bool IsDepthFormat(FramebufferTextureFormat format)
        {
            switch (format)
            {
                case FramebufferTextureFormat::DEPTH24STENCIL8: return true;
                case FramebufferTextureFormat::DEPTH32FLOAT: return true;
                case FramebufferTextureFormat::DEPTH16UNORM: return true;
                case FramebufferTextureFormat::DEPTH32STENCIL8: return true;
                default:                                        return false;
            }
        }

    } // namespace Utils


    // --- Constructor ---
    // Initializes the framebuffer specification and creates the underlying Metal textures.
    MetalFramebuffer::MetalFramebuffer(const FramebufferSpecification& spec)
        : m_Specification(spec)
    {
        // Get the main Metal device from our graphics context.
        m_Device = MetalContext::GetCurrentDevice();

        for (auto attachmentSpec : m_Specification.Attachments.Attachments)
        {
            if (!Utils::IsDepthFormat(attachmentSpec.TextureFormat))
                m_ColorAttachmentSpecifications.emplace_back(attachmentSpec);
            else
                m_DepthAttachmentSpecification = attachmentSpec;
        }

        Invalidate();
    }

    // --- Destructor ---
    // In an ARC environment, releasing the id<> pointers is handled automatically.
    // The member vectors will clean themselves up.
    MetalFramebuffer::~MetalFramebuffer()
    {
        // All id<MTLTexture> objects are managed by ARC and will be released.
    }

    // --- Invalidate ---
    // This is the core function that creates or recreates all necessary Metal textures
    // based on the framebuffer's specification.
    void MetalFramebuffer::Invalidate()
    {
        for(auto attachment : m_ColorAttachments) attachment->release();
        for(auto attachment : m_ColorResolveAttachments) attachment->release();
        if(m_DepthAttachment != NULL) m_DepthAttachment->release();
//        if(m_StencilAttachment != NULL) m_StencilAttachment->release();
        
        m_ColorAttachments.clear();
        m_ColorResolveAttachments.clear();
        m_DepthAttachment = NULL;
        m_StencilAttachment = NULL; // Stencil is often part of the depth texture in Metal

        bool multisample = m_Specification.Samples > 1;

        // 1. Create Color Attachments
        if (!m_ColorAttachmentSpecifications.empty())
        {
            m_ColorAttachments.resize(m_ColorAttachmentSpecifications.size());
            if (multisample) {
                m_ColorResolveAttachments.resize(m_ColorAttachmentSpecifications.size());
            }

            for (size_t i = 0; i < m_ColorAttachmentSpecifications.size(); ++i)
            {
                const auto& spec = m_ColorAttachmentSpecifications[i];
                MTL::PixelFormat format = Utils::FBTextureFormatToMTL(spec.TextureFormat);

                // Create the primary color texture (which may be multisampled)
                MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();
                textureDescriptor->setWidth(m_Specification.Width);
                textureDescriptor->setHeight(m_Specification.Height);
                textureDescriptor->setPixelFormat(format);
                textureDescriptor->setTextureType(multisample ? MTL::TextureType2DMultisample : MTL::TextureType2D);
                textureDescriptor->setSampleCount(m_Specification.Samples);
                textureDescriptor->setStorageMode((spec.TextureFormat == FramebufferTextureFormat::RED_INTEGER) ? (MTL::StorageModeShared) : (MTL::StorageModePrivate)); // GPU only is most performant
                textureDescriptor->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);

                m_ColorAttachments[i] = m_Device->newTexture(textureDescriptor);
                textureDescriptor->release();
                std::string label = "FB_ColorAttachment_" + std::to_string(i);
                m_ColorAttachments[i]->setLabel(NS::String::string(label.c_str(), NS::UTF8StringEncoding));


                // If multisampling, create a non-multisampled "resolve" texture.
                // The GPU will automatically copy the resolved image here at the end of the render pass.
                if (multisample)
                {
                    MTL::TextureDescriptor* resolveDescriptor = MTL::TextureDescriptor::alloc()->init();
                    resolveDescriptor->setWidth(m_Specification.Width);
                    resolveDescriptor->setHeight(m_Specification.Height);
                    resolveDescriptor->setPixelFormat(format);
                    resolveDescriptor->setTextureType(MTL::TextureType2D);
                    resolveDescriptor->setSampleCount(1);
                    resolveDescriptor->setStorageMode(MTL::StorageModePrivate);
                    resolveDescriptor->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);

                    m_ColorResolveAttachments[i] = m_Device->newTexture(resolveDescriptor);
                    resolveDescriptor->release();
                    std::string label = "FB_ColorResolveAttachment_" + std::to_string(i);
                    m_ColorResolveAttachments[i]->setLabel(NS::String::string(label.c_str(), NS::UTF8StringEncoding));
                }
            }
        }

        // 2. Create Depth Attachment (if specified)
        if (m_DepthAttachmentSpecification.TextureFormat != FramebufferTextureFormat::None)
        {
            MTL::PixelFormat depthFormat = Utils::FBTextureFormatToMTL(m_DepthAttachmentSpecification.TextureFormat);

            MTL::TextureDescriptor* depthTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
            depthTextureDescriptor->setWidth(m_Specification.Width);
            depthTextureDescriptor->setHeight(m_Specification.Height);
            depthTextureDescriptor->setPixelFormat(depthFormat);
            depthTextureDescriptor->setTextureType(multisample ? MTL::TextureType2DMultisample : MTL::TextureType2D);
            depthTextureDescriptor->setSampleCount(m_Specification.Samples);
            depthTextureDescriptor->setStorageMode(MTL::StorageModePrivate);
            depthTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);

            m_DepthAttachment = m_Device->newTexture(depthTextureDescriptor);
            depthTextureDescriptor->release();
            m_DepthAttachment->setLabel(NS::String::string("FB_DepthAttachment", NS::UTF8StringEncoding));

            // In Metal, the stencil buffer is part of the depth texture if the format supports it.
            // e.g., MTLPixelFormatDepth24Unorm_Stencil8 or MTLPixelFormatDepth32Float_Stencil8
#if TARGET_OS_OSX
            if(depthFormat == MTL::PixelFormatDepth24Unorm_Stencil8 || depthFormat == MTL::PixelFormatDepth32Float_Stencil8) {
                m_StencilAttachment = m_DepthAttachment;
            }
#else
            if(depthFormat == MTL::PixelFormatDepth32Float_Stencil8) {
                m_StencilAttachment = m_DepthAttachment;
            }
#endif
        }
    }

    // --- Bind/Unbind ---
    // In Metal, there is no persistent bound framebuffer. Instead, a MTLRenderPassDescriptor
    // is configured and used to create a MTLRenderCommandEncoder for a specific command buffer.
    // These functions are thus conceptually different. `Bind` can be interpreted as "get a configured
    // render pass descriptor for this framebuffer". `Unbind` is a no-op, as the action of
    // unbinding is `[encoder endEncoding]`, which is handled by the renderer.
    void MetalFramebuffer::Bind()
    {
        auto renderPassDescriptor = this->GetRenderPassDescriptor();
        m_RenderCommandEncoder = MetalContext::GetCurrentCommandBuffer()->renderCommandEncoder(renderPassDescriptor);
        MetalContext::SetCommandEncoder(m_RenderCommandEncoder);
        MetalContext::SetRenderPassDescriptor(renderPassDescriptor);
    }

    void MetalFramebuffer::Unbind()
    {
        m_RenderCommandEncoder->endEncoding();
        MetalContext::SetCommandEncoder(nullptr);
    }

    // --- Resize ---
    // Resizes the framebuffer by recreating all its texture attachments with new dimensions.
    void MetalFramebuffer::Resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
        {
            LOG_WARN_STREAM << "Attempted to resize framebuffer to " << width << ", " << height;
            return;
        }
        m_Specification.Width = width;
        m_Specification.Height = height;
        
        Invalidate();
    }

    // --- ReadPixel ---
    // Reads a single pixel value from a color attachment. This is a synchronous and slow operation.
    // It requires creating a temporary buffer and a blit command to copy the data from GPU to CPU.
    int MetalFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
    {
        assert(attachmentIndex < m_ColorAttachments.size());

        MTL::Texture* readTexture = (m_Specification.Samples > 1) ? m_ColorResolveAttachments[attachmentIndex] : m_ColorAttachments[attachmentIndex];
        
        // Ensure the texture format is readable as an integer.
        const auto& spec = m_ColorAttachmentSpecifications[attachmentIndex];
        if (spec.TextureFormat != FramebufferTextureFormat::RED_INTEGER) {
            LOG_ERROR_STREAM << "ReadPixel is only implemented for RED_INTEGER format.";
            return -1;
        }
        
        const NS::UInteger textureWidth = readTexture->width();
        const NS::UInteger textureHeight = readTexture->height();
        // Bounds check
        if (x < 0 || y < 0 || x >= textureWidth || y >= textureHeight)
        {
            // Coordinates are outside the texture's bounds, return -1.
            return -1;
        }

        int pixelData = -1;
        MTL::Region region = MTL::Region::Make2D(x, y, 1, 1);
        
        // For RED_INTEGER (R32_SINT), each pixel is 4 bytes (sizeof(int)).
        // The 'bytesPerRow' parameter is the stride of the source texture.
        NS::UInteger bytesPerRow = textureWidth * sizeof(int);

        // Directly copy the pixel data from the texture into our variable.
        // This is a direct memory access, no GPU commands needed.
        readTexture->getBytes(&pixelData, bytesPerRow, region, 0);

        return pixelData;
    }

    // --- ClearAttachment ---
    // In Metal, clearing is typically done via the `loadAction` of a render pass.
    // This implementation simulates an immediate clear by creating and running a
    // dedicated render pass just for the clear operation.
    void MetalFramebuffer::ClearAttachment(uint32_t attachmentIndex, int value)
    {
        assert(attachmentIndex < m_ColorAttachments.size());

//        MTLRenderPassDescriptor* passDescriptor = [MTLRenderPassDescriptor new];
//        passDescriptor.colorAttachments[0].texture = (__bridge id<MTLTexture>)m_ColorAttachments[attachmentIndex];
//        passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
//        passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
//        
//        // The clear color must match the attachment's format.
//        // Here we assume we are clearing a RED_INTEGER attachment.
//        float floatValue = static_cast<float>(value);
//        passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(floatValue, 1.0, 1.0, 1.0);
//
//        id<MTLCommandQueue> commandQueue = (__bridge id<MTLCommandQueue>)MetalContext::GetCurrentCommandQueue();
//        id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
//        id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
//        [encoder endEncoding]; // No drawing needed, the clear happens on begin.
//        [commandBuffer commit];
    }

    // --- Getters for Renderer ---
    // These methods provide access to the underlying Metal objects so the renderer can use them.
//    id<MTLTexture> MetalFramebuffer::GetColorAttachment(uint32_t index) const
//    {
//        assert(index < m_ColorAttachments.size());
//        // If we are multisampling, the shader-readable texture is the resolve texture.
//        if (m_Specification.Samples > 1) {
//            return m_ColorResolveAttachments[index];
//        }
//        return m_ColorAttachments[index];
//    }
//
//    id<MTLTexture> MetalFramebuffer::GetDepthAttachment() const
//    {
//        return m_DepthAttachment;
//    }

    // --- GetRenderPassDescriptor ---
    // This is the primary way a renderer will interact with the framebuffer. It returns a
    // fully configured descriptor ready to be used to create a render command encoder.
    MTL::RenderPassDescriptor* MetalFramebuffer::GetRenderPassDescriptor() const
    {
        MTL::RenderPassDescriptor* descriptor = MTL::RenderPassDescriptor::renderPassDescriptor();
        bool multisample = m_Specification.Samples > 1;

        // Configure Color Attachments
        for (size_t i = 0; i < m_ColorAttachments.size(); ++i) {
            MTL::RenderPassColorAttachmentDescriptor* colorAttachment = descriptor->colorAttachments()->object(i);
            colorAttachment->setTexture(m_ColorAttachments[i]);
            colorAttachment->setLoadAction(MTL::LoadActionClear);
            colorAttachment->setClearColor(MTL::ClearColor::Make(0.0, 0.0, 0.0, 1.0));

            if (multisample) {
                colorAttachment->setStoreAction(MTL::StoreActionMultisampleResolve);
                colorAttachment->setResolveTexture(m_ColorResolveAttachments[i]);
            } else {
                colorAttachment->setStoreAction(MTL::StoreActionStore);
            }
        }

        // Configure Depth Attachment
        if (m_DepthAttachment) {
            MTL::RenderPassDepthAttachmentDescriptor* depthAttachment = descriptor->depthAttachment();
            depthAttachment->setTexture(m_DepthAttachment);
            depthAttachment->setLoadAction(MTL::LoadActionClear);
            depthAttachment->setStoreAction(MTL::StoreActionDontCare);
            depthAttachment->setClearDepth(1.0);
        }
        
        // Configure Stencil Attachment (if it exists)
        if (m_StencilAttachment) {
            MTL::RenderPassStencilAttachmentDescriptor* stencilAttachment = descriptor->stencilAttachment();
            stencilAttachment->setTexture(m_StencilAttachment);
            stencilAttachment->setLoadAction(MTL::LoadActionClear);
            stencilAttachment->setStoreAction(MTL::StoreActionDontCare);
            stencilAttachment->setClearStencil(0);
        }

        return descriptor;
    }

    // --- BlitBuffers ---
    // This function is not implemented as it requires a source framebuffer object.
    // In a full Metal application, this would take another `MetalFramebuffer*` as source
    // and use a MTLBlitCommandEncoder to perform the copy.
    void MetalFramebuffer::BlitBuffers(uint32_t src, uint32_t srcX0, uint32_t srcY0, uint32_t srcX1, uint32_t srcY1, uint32_t dstX0, uint32_t dstY0, uint32_t dstX1, uint32_t dstY1, uint32_t mask, uint16_t filter)
    {
//        // Calculate the dimensions of the source and destination rectangles.
//        const uint32_t srcWidth = srcX1 - srcX0;
//        const uint32_t srcHeight = srcY1 - srcY0;
//        const uint32_t dstWidth = dstX1 - dstX0;
//        const uint32_t dstHeight = dstY1 - dstY0;
//
//        // Metal's MTLBlitCommandEncoder does not support scaling with filtering.
//        // This function implements the direct, non-scaling blit. If scaling is
//        // required, a separate render pass (drawing a textured quad) is necessary.
//        if (srcWidth != dstWidth || srcHeight != dstHeight)
//        {
//            LOG_WARN_STREAM << "MetalFramebuffer::BlitBuffers: Scaled blits are not supported in this path. The source and destination dimensions must be identical.";
//            return;
//        }
//
//        // Exit if there is nothing to blit.
//        if (srcWidth == 0 || srcHeight == 0 || mask == 0)
//        {
//            return;
//        }
//
//        // Obtain a command buffer from the command queue.
//        id<MTLCommandBuffer> commandBuffer = [GetContext()->GetCommandQueue() commandBuffer];
//        commandBuffer.label = @"FramebufferBlit";
//
//        // Create a blit command encoder to issue copy commands.
//        id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
//        blitEncoder.label = @"FramebufferRegionBlit";
//
//        // Define the source and destination regions for the copy operation.
//        MTLOrigin sourceOrigin = MTLOriginMake(srcX0, srcY0, 0);
//        MTLSize copySize = MTLSizeMake(srcWidth, srcHeight, 1);
//        MTLOrigin destOrigin = MTLOriginMake(dstX0, dstY0, 0);
//
//        // --- Blit Color Buffer ---
//        if (mask & GL_COLOR_BUFFER_BIT)
//        {
//            // Assuming the blit targets the first color attachment (index 0).
//            id<MTLTexture> srcTexture = [src GetColorTexture:0];
//            id<MTLTexture> dstTexture = [self GetColorTexture:0];
//
//            if (srcTexture && dstTexture)
//            {
//                [blitEncoder copyFromTexture:srcTexture
//                                 sourceSlice:0
//                                 sourceLevel:0
//                                sourceOrigin:sourceOrigin
//                                  sourceSize:copySize
//                                   toTexture:dstTexture
//                            destinationSlice:0
//                            destinationLevel:0
//                           destinationOrigin:destOrigin];
//            }
//        }
//
//        // --- Blit Depth and/or Stencil Buffers ---
//        // In Metal, depth and stencil data are often in a single combined texture.
//        // This single copy command handles depth-only, stencil-only, and combined formats.
//        if (mask & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
//        {
//            id<MTLTexture> srcTexture = [src GetDepthStencilTexture];
//            id<MTLTexture> dstTexture = [self GetDepthStencilTexture];
//
//            if (srcTexture && dstTexture)
//            {
//                [blitEncoder copyFromTexture:srcTexture
//                                 sourceSlice:0
//                                 sourceLevel:0
//                                sourceOrigin:sourceOrigin
//                                  sourceSize:copySize
//                                   toTexture:dstTexture
//                            destinationSlice:0
//                            destinationLevel:0
//                           destinationOrigin:destOrigin];
//            }
//        }
//
//        // Finalize encoding and commit the commands to the GPU for execution.
//        [blitEncoder endEncoding];
    }

    void MetalFramebuffer::BlitToColorAttachment(int index, uintptr_t srcTexture) {
        MTL::BlitCommandEncoder* blitEncoder =  MetalContext::GetCurrentCommandBuffer()->blitCommandEncoder();
        blitEncoder->copyFromTexture((MTL::Texture*)srcTexture, m_ColorAttachments[index]);
        blitEncoder->endEncoding();
    }

    void MetalFramebuffer::DrawToAllColorBuffers() {
//        m_RenderCommandEncoder->endEncoding();
//        
//        m_RenderCommandEncoder = m_PreviousRenderCommandEncoder;
//        MetalContext::SetCommandEncoder(m_RenderCommandEncoder);
//        this->Bind();
    };

    void MetalFramebuffer::SetDrawBuffer(uint8_t index) {
//        assert(index < m_ColorAttachments.size());
//        m_PreviousRenderCommandEncoder = m_RenderCommandEncoder;
//        m_RenderCommandEncoder->endEncoding();
//        MTL::RenderPassDescriptor* descriptor = MTL::RenderPassDescriptor::renderPassDescriptor();
//        bool multisample = m_Specification.Samples > 1;
//        
//        MTL::RenderPassColorAttachmentDescriptor* colorAttachment = descriptor->colorAttachments()->object(0);
//        colorAttachment->setTexture(m_ColorAttachments[index]);
//        colorAttachment->setLoadAction(MTL::LoadActionClear);
//        colorAttachment->setClearColor(MTL::ClearColor::Make(0.0, 0.0, 0.0, 1.0));
//
//        if (multisample) {
//            colorAttachment->setStoreAction(MTL::StoreActionMultisampleResolve);
//            colorAttachment->setResolveTexture(m_ColorResolveAttachments[index]);
//        } else {
//            colorAttachment->setStoreAction(MTL::StoreActionStore);
//        }
//        
//        m_RenderCommandEncoder = MetalContext::GetCurrentCommandBuffer()->renderCommandEncoder(descriptor);
//        MetalContext::SetCommandEncoder(m_RenderCommandEncoder);
    };
    void MetalFramebuffer::BindColorAttachmentAsTexture(uint32_t index, uint32_t slot) {
        MTL::Device* device = MetalContext::GetCurrentDevice();
        
        MTL::SamplerDescriptor* samplerDescriptor = MTL::SamplerDescriptor::alloc()->init();
        samplerDescriptor->setMagFilter(MTL::SamplerMinMagFilterLinear);
        samplerDescriptor->setMinFilter(MTL::SamplerMinMagFilterLinear);
        MTL::SamplerState* sampler = device->newSamplerState(samplerDescriptor);
        
        MetalContext::GetCurrentRenderCommandEncoder()->setFragmentTexture(m_ColorAttachments[index], slot);
        MetalContext::GetCurrentRenderCommandEncoder()->setFragmentSamplerState(sampler, slot);
    };
}
