#include "Platform/Metal/MetalBuffer.h"

#include "Platform/Metal/MetalContext.h"

namespace Graphics {

MetalVertexBuffer::MetalVertexBuffer(uint32_t size, std::string& label)
{
    m_Buffer = MetalContext::GetCurrentDevice()->newBuffer(size, MTL::ResourceStorageModeShared);
    
    if(!label.empty()) m_Buffer->setLabel(NS::String::string(label.c_str(), NS::UTF8StringEncoding));
    
    m_Size = size;                   // add this member if you don't have it
}

MetalVertexBuffer::MetalVertexBuffer(float* vertices, uint32_t size, std::string& label)
{
    m_Buffer = MetalContext::GetCurrentDevice()->newBuffer(vertices, size, MTL::ResourceStorageModeShared);
    
    if(!label.empty()) m_Buffer->setLabel(NS::String::string(label.c_str(), NS::UTF8StringEncoding));

    m_Size = size;
}

MetalVertexBuffer::~MetalVertexBuffer()
{
    if (m_Buffer) {
        // We used __bridge_retained, so release here.
        m_Buffer->release();
        m_Buffer = nullptr;
    }
}

void MetalVertexBuffer::Bind() const
{
    // No-op by design in Metal. Prefer a method that takes an encoder + index.
    // Example you could add:
    //   void SetOnEncoder(id<MTLRenderCommandEncoder> enc, uint32_t index, uint32_t offset) const {
    //       [enc setVertexBuffer:ToMTLBuffer(m_Buffer) offset:offset atIndex:index];
    //   }
    MTL::RenderCommandEncoder* pEncoder = MetalContext::GetCurrentRenderCommandEncoder();
    pEncoder->setVertexBuffer(m_Buffer, 0, 30);
    pEncoder->setFragmentBuffer(m_Buffer, 0, 30);
}

void MetalVertexBuffer::Unbind() const
{
    // No-op in Metal.
}

void MetalVertexBuffer::ResizeBuffer(uint32_t newSize)
{
    MTL::Buffer* newBuffer = MetalContext::GetCurrentDevice()->newBuffer(newSize, MTL::ResourceStorageModeShared);

    MTL::Buffer* oldBuffer = m_Buffer;
    if (oldBuffer) {
        // Preserve existing data (up to min sizes) when in Shared storage.
        const NS::UInteger copySize = (NS::UInteger)std::min<uint32_t>(m_Size, newSize);
        if (copySize > 0) {
            memcpy(newBuffer->contents(), oldBuffer->contents(), copySize);
        }
        m_Buffer->release();
    }

    m_Buffer = newBuffer;
    m_Size = newSize;
}

void MetalVertexBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
{
    assert(m_Buffer != nullptr);

    const NS::UInteger bufLen = m_Buffer->length();
    assert((NS::UInteger)offset + (NS::UInteger)size <= bufLen);

    uint8_t* dst = (uint8_t*)m_Buffer->contents() + offset;
    memcpy(dst, data, size);

#if TARGET_OS_OSX
    // Only needed for Managed storage; harmless check for Shared
    if (buffer.storageMode == MTLStorageModeManaged) {
        [buffer didModifyRange:NSMakeRange(offset, size)];
    }
#endif
}

	// MetalIndexBuffer Implementation
	MetalIndexBuffer::MetalIndexBuffer(uint32_t* indices, uint32_t count, std::string& label) : m_Count(count), isStatic(true)
	{
		// Create a Metal buffer with initial data
        m_Buffer = MetalContext::GetCurrentDevice()->newBuffer(indices, count * sizeof(uint32_t), MTL::ResourceStorageModeShared);
	}

	MetalIndexBuffer::MetalIndexBuffer(uint32_t count, std::string& label) : m_Count(count), isStatic(false)
	{
		// Create a Metal buffer with no initial data
        m_Buffer = MetalContext::GetCurrentDevice()->newBuffer(count * sizeof(uint32_t), MTL::ResourceStorageModeShared);
	}

	MetalIndexBuffer::~MetalIndexBuffer()
	{
		// Destructor cleanup if necessary
	}

	void MetalIndexBuffer::Bind() const
	{
		// Bind the index buffer to the GPU pipeline
		// Binding is generally handled within command encoders in Metal
	}

	void MetalIndexBuffer::Unbind() const
	{
		// Unbind the index buffer from the GPU pipeline
		// Again, unbinding is generally handled within command encoders
	}

	void MetalIndexBuffer::SetData(const uint32_t* data, uint32_t count, uint32_t offset)
	{
        assert(!isStatic);
        
//        id<MTLBuffer> buffer = ToMTLBuffer(m_Buffer);
//
//        NSCAssert(buffer != nil, @"MTLBuffer is nil");
//        const NSUInteger bufLen = buffer.length;
//        const NSUInteger size = count * sizeof(uint32_t);
//        NSCAssert((NSUInteger)offset + (NSUInteger)size <= bufLen, @"Write out of bounds");
//
//        uint32_t* dst = (uint32_t*)[buffer contents] + offset;
//        memcpy(dst, data, size);
        
        MTL::Buffer* buffer = m_Buffer;

        assert(buffer != nullptr && "MTLBuffer is nil");

        const size_t bufLen = buffer->length();
        const size_t size = count * sizeof(uint32_t);
        assert(offset + size <= bufLen && "Write out of bounds");

        // Get a raw pointer to the buffer's memory.
        uint8_t* buffer_ptr = static_cast<uint8_t*>(buffer->contents());

        // Apply the byte offset and copy the data.
        memcpy(buffer_ptr + offset, data, size);
	}

} // namespace Graphics
