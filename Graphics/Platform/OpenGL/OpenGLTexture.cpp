
#include "OpenGLTexture.h"


#include <cstdint>
#include <string>
#include <cassert>
#include <iostream>
#include <algorithm>

#include "Logger.h"
#include "stb_image.h"


namespace Graphics {

	namespace Utils {
		GLenum TextureFormatToGLFormat(TextureFormat format) {
			switch (format) {
			case TextureFormat::RGBA8:
				return GL_RGBA8;
			case TextureFormat::RGBA16FLOAT:
				return GL_RGBA16F;
			case TextureFormat::RGBA32FLOAT:
				return GL_RGBA32F;
			}

			throw;
		}
	}

	OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height, TextureFormat format, uint32_t mipLevels)
		: Texture2D(format), m_Width(width), m_Height(height), m_MipLevels(mipLevels ? mipLevels : 1), m_InternalFormat(Utils::TextureFormatToGLFormat(format))
	{
		m_DataFormat = GL_RGBA;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, m_MipLevels, m_InternalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, m_MipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	// Pixel upload type/size for our internal formats.
	static GLenum GLDataTypeForInternalFormat(GLenum internalFormat)
	{
		return (internalFormat == GL_RGBA32F || internalFormat == GL_RGBA16F) ? GL_FLOAT : GL_UNSIGNED_BYTE;
	}

	static uint32_t GLBytesPerPixel(GLenum internalFormat, GLenum dataFormat)
	{
		const uint32_t channels = (dataFormat == GL_RGBA) ? 4 : 3;
		// Float formats are uploaded from float client data (GL_FLOAT).
		return (internalFormat == GL_RGBA32F || internalFormat == GL_RGBA16F) ? channels * 4 : channels;
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::string& path)
		: Texture2D(TextureFormat::RGBA32FLOAT), m_Path(path)
	{
		LOG_DEBUG_STREAM << "Loading texture";
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* data = nullptr;
		{
			data = stbi_load(path.c_str(), &width, &height, &channels, 0);
		}

		if (data)
		{
			m_IsLoaded = true;

			m_Width = width;
			m_Height = height;

			GLenum internalFormat = 0, dataFormat = 0;
			if (channels == 4)
			{
				internalFormat = GL_RGBA8;
				dataFormat = GL_RGBA;
			}
			else if (channels == 3)
			{
				internalFormat = GL_RGB8;
				dataFormat = GL_RGB;
			}

			m_InternalFormat = internalFormat;
			m_DataFormat = dataFormat;
			
			assert(internalFormat & dataFormat && "Format not supported!");

			glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
			glTextureStorage2D(m_RendererID, 1, internalFormat, m_Width, m_Height);

			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);

			stbi_image_free(data);
		}
		else {
			LOG_FATAL_STREAM << "texture data not loaded";
			assert("Texture Loading failed");
		}
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture2D::SetData(void* data, uint32_t size)
	{
		uint32_t bpp = GLBytesPerPixel(m_InternalFormat, m_DataFormat);
		assert(size == m_Width * m_Height * bpp && "Data must be entire texture!");
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GLDataTypeForInternalFormat(m_InternalFormat), data);
	}

	void OpenGLTexture2D::SetMipData(void* data, uint32_t size, uint32_t mip)
	{
		assert(mip < m_MipLevels);
		const uint32_t mipWidth = std::max(1u, m_Width >> mip);
		const uint32_t mipHeight = std::max(1u, m_Height >> mip);
		const uint32_t bpp = GLBytesPerPixel(m_InternalFormat, m_DataFormat);
		assert(size == mipWidth * mipHeight * bpp && "Data must be the entire mip level!");
		glTextureSubImage2D(m_RendererID, mip, 0, 0, mipWidth, mipHeight, m_DataFormat, GLDataTypeForInternalFormat(m_InternalFormat), data);
	}

	void OpenGLTexture2D::Resize(uint32_t width, uint32_t height)
	{
		// If the dimensions are the same, do nothing.
		if (m_Width == width && m_Height == height)
			return;

		m_Width = width;
		m_Height = height;

		// First, delete the old texture from the GPU
		glDeleteTextures(1, &m_RendererID);

		// Create a new texture with the same target
		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);

		// Now, allocate the immutable storage for the NEW texture object
		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

		// IMPORTANT: Re-apply any texture parameters
		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	void OpenGLTexture2D::Bind(uint32_t slot) const
	{

		glBindTextureUnit(slot, m_RendererID);
	}
	void OpenGLTexture2D::Blit(uintptr_t srcTexture)
	{
		// Same-size copy (used to fold the JFA composite back into the
		// display texture, mirroring MetalTexture2D::Blit).
		glCopyImageSubData((GLuint)srcTexture, GL_TEXTURE_2D, 0, 0, 0, 0,
		                   m_RendererID, GL_TEXTURE_2D, 0, 0, 0, 0,
		                   m_Width, m_Height, 1);
	}
}