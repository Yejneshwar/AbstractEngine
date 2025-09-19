#include "MetalTexture.h"

#include "MetalContext.h"

#include <iostream>
#include <cassert>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
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
                case TextureFormat::RGBA32FLOAT:     return MTL::PixelFormatRGBA32Float;
            }
        }
    
    }

    MetalTexture2D::MetalTexture2D(uint32_t width, uint32_t height, TextureFormat format) :
        m_Width(width), m_Height(height), m_InternalFormat(Utils::TextureFormatToMTL(format))
    {
        MTL::TextureDescriptor* descriptor = MTL::TextureDescriptor::alloc()->init();
        descriptor->setPixelFormat(m_InternalFormat);
        descriptor->setWidth(width);
        descriptor->setHeight(height);
        descriptor->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderWrite);
        
        MTL::SamplerDescriptor* samplerDescriptor = MTL::SamplerDescriptor::alloc()->init();
        samplerDescriptor->setMagFilter(MTL::SamplerMinMagFilterLinear);
        samplerDescriptor->setMinFilter(MTL::SamplerMinMagFilterLinear);
        
        MTL::Device* device = MetalContext::GetCurrentDevice();

        m_RendererID = device->newTexture(descriptor);
        m_SamplerState = device->newSamplerState(samplerDescriptor);
        
        descriptor->release();
        samplerDescriptor->release();

        m_IsLoaded = true;
    }

    MetalTexture2D::MetalTexture2D(const std::string& path) :
        m_Path(GUI::Utils::getResourcePath(path))
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

    void MetalTexture2D::SetData(void* data, uint32_t size)
    {
        uint32_t bpp = (m_InternalFormat == MTL::PixelFormatRGBA8Unorm) ? 4 : 3;
        assert(size == m_Width * m_Height * bpp && "Data must be entire texture!");

        MTL::Region region = MTL::Region::Make2D(0, 0, m_Width, m_Height);
        m_RendererID->replaceRegion(region, 0, data, m_Width * bpp);
    }

    void MetalTexture2D::Resize(uint32_t width, uint32_t height)
    {
        m_Width = width;
        m_Height = height;

        MTL::TextureDescriptor* descriptor = MTL::TextureDescriptor::alloc()->init();
        descriptor->setPixelFormat(m_InternalFormat);
        descriptor->setWidth(width);
        descriptor->setHeight(height);
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
