#include "MetalTexture.h"

#include "MetalContext.h"

#include <iostream>
#include <cassert>
#include <string>
#include <algorithm>

//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Logger.h"
#include "IOS/FileUtils.h"

namespace Graphics {

    namespace Utils {
    
        // Helper to translate our abstract format to a Metal MTLPixelFormat
        static MTL::PixelFormat TextureFormatToMTL(TextureFormat format)
        {
            switch (format)
            {
                case TextureFormat::RGBA8:           return MTL::PixelFormatRGBA8Unorm;
                case TextureFormat::RGBA16FLOAT:     return MTL::PixelFormatRGBA16Float;
                case TextureFormat::RGBA32FLOAT:     return MTL::PixelFormatRGBA32Float;
            }
        }
    
    }

    MetalTexture2D::MetalTexture2D(uint32_t width, uint32_t height, TextureFormat format, uint32_t mipLevels) :
        Texture2D(format), m_Width(width), m_Height(height), m_MipLevels(mipLevels ? mipLevels : 1), m_InternalFormat(Utils::TextureFormatToMTL(format))
    {
        MTL::TextureDescriptor* descriptor = MTL::TextureDescriptor::alloc()->init();
        descriptor->setPixelFormat(m_InternalFormat);
        descriptor->setWidth(width);
        descriptor->setHeight(height);
        descriptor->setMipmapLevelCount(m_MipLevels);
        descriptor->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderWrite);

        MTL::SamplerDescriptor* samplerDescriptor = MTL::SamplerDescriptor::alloc()->init();
        samplerDescriptor->setMagFilter(MTL::SamplerMinMagFilterLinear);
        samplerDescriptor->setMinFilter(MTL::SamplerMinMagFilterLinear);
        if (m_MipLevels > 1) {
            // Mipped textures are sampled with explicit LOD (prefiltered
            // environment); wrap horizontally so equirect U seams filter.
            samplerDescriptor->setMipFilter(MTL::SamplerMipFilterLinear);
            samplerDescriptor->setSAddressMode(MTL::SamplerAddressModeRepeat);
            samplerDescriptor->setLodMaxClamp((float)(m_MipLevels - 1));
        }

        MTL::Device* device = MetalContext::GetCurrentDevice();

        m_RendererID = device->newTexture(descriptor);
        m_SamplerState = device->newSamplerState(samplerDescriptor);

        descriptor->release();
        samplerDescriptor->release();

        m_IsLoaded = true;
    }

    MetalTexture2D::MetalTexture2D(const std::string& path) :
        Texture2D(TextureFormat::RGBA32FLOAT), m_Path(GUI::Utils::getResourcePath(path))
    {
        LOG_TRACE_STREAM << "Loading texture " << m_Path;
        int width, height, channels;
        stbi_set_flip_vertically_on_load(1);
        stbi_uc* data = nullptr;
        data = stbi_load(m_Path.c_str(), &width, &height, &channels, 0);

        if (data)
        {
            m_IsLoaded = true;
            m_Width = width;
            m_Height = height;

            MTL::PixelFormat pixelFormat;
            if (channels == 4)
            {
                pixelFormat = MTL::PixelFormatRGBA8Unorm;
            }
            else if (channels == 3)
            {
                pixelFormat = MTL::PixelFormatRGBA8Unorm;
            }
            else
            {
                LOG_FATAL_STREAM << "Unsupported channel count";
                stbi_image_free(data);
                return;
            }
            
            MTL::TextureDescriptor* descriptor = MTL::TextureDescriptor::alloc()->init();
            descriptor->setPixelFormat(pixelFormat);
            descriptor->setWidth(width);
            descriptor->setHeight(height);
            descriptor->setUsage(MTL::TextureUsageShaderRead);
            
            MTL::SamplerDescriptor* samplerDescriptor = MTL::SamplerDescriptor::alloc()->init();
            samplerDescriptor->setMagFilter(MTL::SamplerMinMagFilterLinear);
            samplerDescriptor->setMinFilter(MTL::SamplerMinMagFilterLinear);

            MTL::Device* device = MetalContext::GetCurrentDevice();
            
            m_RendererID = device->newTexture(descriptor);
            m_SamplerState = device->newSamplerState(samplerDescriptor);
            m_InternalFormat = pixelFormat;

            // Create a Metal buffer with the image data
            MTL::Region region = MTL::Region::Make2D(0, 0, width, height);
            m_RendererID->replaceRegion(region, 0, data, width * (channels == 4 ? 4 : 3));

            stbi_image_free(data);
            
            descriptor->release();
            samplerDescriptor->release();
        }
        else {
            LOG_FATAL_STREAM << "Texture data not loaded";
            GRAPHICS_CORE_ASSERT(false, "Texture Loading failed");
        }
    }

    MetalTexture2D::~MetalTexture2D()
    {
        // No explicit delete required; Metal handles it.
    }

    static uint32_t BytesPerPixel(MTL::PixelFormat format)
    {
        switch (format)
        {
            case MTL::PixelFormatRGBA32Float: return 16;
            case MTL::PixelFormatRGBA16Float: return 8;
            case MTL::PixelFormatRGBA8Unorm:  return 4;
            default:                          return 4;
        }
    }

    void MetalTexture2D::SetData(void* data, uint32_t size)
    {
        uint32_t bpp = BytesPerPixel(m_InternalFormat);
        assert(size == m_Width * m_Height * bpp && "Data must be entire texture!");

        MTL::Region region = MTL::Region::Make2D(0, 0, m_Width, m_Height);
        m_RendererID->replaceRegion(region, 0, data, m_Width * bpp);
    }

    void MetalTexture2D::SetMipData(void* data, uint32_t size, uint32_t mip)
    {
        assert(mip < m_MipLevels);
        const uint32_t mipWidth = std::max(1u, m_Width >> mip);
        const uint32_t mipHeight = std::max(1u, m_Height >> mip);
        const uint32_t bpp = BytesPerPixel(m_InternalFormat);
        assert(size == mipWidth * mipHeight * bpp && "Data must be the entire mip level!");

        MTL::Region region = MTL::Region::Make2D(0, 0, mipWidth, mipHeight);
        m_RendererID->replaceRegion(region, mip, data, mipWidth * bpp);
    }

    void MetalTexture2D::Resize(uint32_t width, uint32_t height)
    {
        m_Width = width;
        m_Height = height;

        MTL::TextureDescriptor* descriptor = MTL::TextureDescriptor::alloc()->init();
        descriptor->setPixelFormat(m_InternalFormat);
        descriptor->setWidth(width);
        descriptor->setHeight(height);
        descriptor->setMipmapLevelCount(m_MipLevels);
        descriptor->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderWrite);

        MTL::SamplerDescriptor* samplerDescriptor = MTL::SamplerDescriptor::alloc()->init();
        samplerDescriptor->setMagFilter(MTL::SamplerMinMagFilterLinear);
        samplerDescriptor->setMinFilter(MTL::SamplerMinMagFilterLinear);

        MTL::Device* device = MetalContext::GetCurrentDevice();

        m_RendererID->release();
        m_SamplerState->release();

        m_RendererID = device->newTexture(descriptor);
        m_SamplerState = device->newSamplerState(samplerDescriptor);
        
        descriptor->release();
        samplerDescriptor->release();
    }

    void MetalTexture2D::Bind(uint32_t slot) const
    {
        MetalContext::GetCurrentRenderCommandEncoder()->setFragmentTexture(m_RendererID, slot);
        MetalContext::GetCurrentRenderCommandEncoder()->setFragmentSamplerState(m_SamplerState, slot);
    }

    void MetalTexture2D::Blit(uintptr_t srcTexture) {
        MTL::BlitCommandEncoder* blitEncoder =  MetalContext::GetCurrentCommandBuffer()->blitCommandEncoder();
        blitEncoder->copyFromTexture((MTL::Texture*)srcTexture, m_RendererID);
        blitEncoder->endEncoding();
    }
}
