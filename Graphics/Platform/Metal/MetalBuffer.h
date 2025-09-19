#pragma once

#include "Renderer/Buffer.h"
#include <Metal/Metal.hpp>

#ifdef __OBJC__
@protocol MTLBuffer;
#endif

namespace Graphics {

	class MetalVertexBuffer : public VertexBuffer
	{
	public:
		MetalVertexBuffer(uint32_t size, std::string& label);
		MetalVertexBuffer(float* vertices, uint32_t size, std::string& label);
		virtual ~MetalVertexBuffer();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void ResizeBuffer(uint32_t size) override;
		
		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

		virtual const BufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }
	private:
        uint32_t m_Size;
		BufferLayout m_Layout;
		bool isStatic = true;
        MTL::Buffer* m_Buffer;
	};

	class MetalIndexBuffer : public IndexBuffer
{
public:
    MetalIndexBuffer(uint32_t* indices, uint32_t count, std::string& label);
    MetalIndexBuffer(uint32_t count, std::string& label);
    virtual ~MetalIndexBuffer();
    
    virtual void Bind() const override;
    virtual void Unbind() const override;
    virtual void SetData(const uint32_t* data, uint32_t count, uint32_t offset = 0) override;
    
    virtual uint32_t GetCount() const override { return m_Count; }
    
    MTL::Buffer* GetBuffer() const { return m_Buffer; }
	private:
		uint32_t m_Count;
		bool isStatic = true;
        MTL::Buffer* m_Buffer;
	};

}
