
#include "OpenGLTexture.h"


#include <cstdint>
#include <string>
#include <cassert>
#include <iostream>

#include "Logger.h"
#include "stb_image.h"


namespace Graphics {

	OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height)
	{

		m_InternalFormat = GL_RGBA8;
		m_DataFormat = GL_RGBA;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);	
		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);
		
		glTexParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		glTexParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::string& path)
		: m_Path(path)
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

		uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
		assert(size == m_Width * m_Height * bpp && "Data must be entire texture!");
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
	}

	void OpenGLTexture2D::Resize(uint32_t width, uint32_t height)
	{
		m_Width = width;
		m_Height = height;
		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);
	}

	void OpenGLTexture2D::Bind(uint32_t slot) const
	{

		glBindTextureUnit(slot, m_RendererID);
	}
}