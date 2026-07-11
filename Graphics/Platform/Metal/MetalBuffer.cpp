#include "Platform/Metal/MetalBuffer.h"

#include "Platform/Metal/MetalContext.h"

namespace Graphics {

// ---------------------------------------------------------------------------
// MetalVertexBuffer
// ---------------------------------------------------------------------------

MetalVertexBuffer::MetalVertexBuffer(uint32_t size, std::string& label, bool retained)
{
    // Dynamic buffer: one copy per in-flight frame.
    // Retained buffer: a single copy; updates synchronize with the GPU.
    isStatic = retained;
    const uint32_t copies = retained ? 1 : kRingSize;
    for (uint32_t i = 0; i < copies; i++) {
        m_Buffers[i] = MetalContext::GetCurrentDevice()->newBuffer(size, MTL::ResourceStorageModeShared);
        if (!label.empty()) m_Buffers[i]->setLabel(NS::String::string(label.c_str(), NS::UTF8StringEncoding));
    }

    m_Size = size;
}

MetalVertexBuffer::MetalVertexBuffer(float* vertices, uint32_t size, std::string& label)
{
    // Static buffer: single copy, uploaded once.
    isStatic = true;
    m_Buffers[0] = MetalContext::GetCurrentDevice()->newBuffer(vertices, size, MTL::ResourceStorageModeShared);
    if (!label.empty()) m_Buffers[0]->setLabel(NS::String::string(label.c_str(), NS::UTF8StringEncoding));

    m_Size = size;
    m_ContentBytes = size;
}

MetalVertexBuffer::~MetalVertexBuffer()
{
    const uint32_t copies = isStatic ? 1 : kRingSize;
    for (uint32_t i = 0; i < copies; i++) {
        if (m_Buffers[i]) {
            m_Buffers[i]->release();
            m_Buffers[i] = nullptr;
        }
    }
}

uint32_t MetalVertexBuffer::CurrentSlot() const
{
    return isStatic ? 0 : (MetalContext::GetFrameInFlightIndex() % kRingSize);
}

MTL::Buffer* MetalVertexBuffer::CurrentBuffer() const
{
    const uint32_t slot = CurrentSlot();
    // A slot that missed one or more SetData calls catches up from the
    // newest copy before the GPU reads it (only sporadically-updated
    // buffers ever hit this; per-frame batches are always current).
    if (!isStatic && m_SlotVersions[slot] != m_ContentVersion && m_ContentBytes) {
        memcpy(m_Buffers[slot]->contents(), m_Buffers[m_LatestSlot]->contents(), m_ContentBytes);
        m_SlotVersions[slot] = m_ContentVersion;
    }
    return m_Buffers[slot];
}

void MetalVertexBuffer::Bind() const
{
    MTL::Buffer* buffer = CurrentBuffer();
    MTL::RenderCommandEncoder* pEncoder = MetalContext::GetCurrentRenderCommandEncoder();
    pEncoder->setVertexBuffer(buffer, 0, 30);
    pEncoder->setFragmentBuffer(buffer, 0, 30);
}

void MetalVertexBuffer::Unbind() const
{
    // No-op in Metal.
}

void MetalVertexBuffer::ResizeBuffer(uint32_t newSize)
{
    // Content is not preserved: growth only happens for dynamic batches,
    // which re-upload their full contents on every flush.
    const uint32_t copies = isStatic ? 1 : kRingSize;
    for (uint32_t i = 0; i < copies; i++) {
        if (m_Buffers[i]) m_Buffers[i]->release();
        m_Buffers[i] = MetalContext::GetCurrentDevice()->newBuffer(newSize, MTL::ResourceStorageModeShared);
    }
    m_Size = newSize;
    m_ContentBytes = 0;
    m_ContentVersion++;
}

void MetalVertexBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
{
    const uint32_t slot = CurrentSlot();
    MTL::Buffer* buffer = m_Buffers[slot];
    assert(buffer != nullptr);

    const NS::UInteger bufLen = buffer->length();
    assert((NS::UInteger)offset + (NS::UInteger)size <= bufLen);

    if (isStatic) {
        // Single-copy buffer: make sure no in-flight frame is still reading.
        MetalContext::WaitForGpuIdle();
    }

    uint8_t* dst = (uint8_t*)buffer->contents() + offset;
    memcpy(dst, data, size);

    m_ContentVersion++;
    m_SlotVersions[slot] = m_ContentVersion;
    m_LatestSlot = slot;
    if (offset + size > m_ContentBytes)
        m_ContentBytes = offset + size;

#if TARGET_OS_OSX
    // Only needed for Managed storage; harmless check for Shared
    if(buffer->storageMode() == MTL::StorageModeManaged) {
        buffer->didModifyRange(NS::Range::Make(offset, size));
    }
#endif
}

// ---------------------------------------------------------------------------
// MetalIndexBuffer
// ---------------------------------------------------------------------------

	MetalIndexBuffer::MetalIndexBuffer(uint32_t* indices, uint32_t count, std::string& label) : m_Count(count), isStatic(true)
	{
		// Create a Metal buffer with initial data
        m_Buffers[0] = MetalContext::GetCurrentDevice()->newBuffer(indices, count * sizeof(uint32_t), MTL::ResourceStorageModeShared);
        if (!label.empty()) m_Buffers[0]->setLabel(NS::String::string(label.c_str(), NS::UTF8StringEncoding));
        m_ContentBytes = count * sizeof(uint32_t);
	}

	MetalIndexBuffer::MetalIndexBuffer(uint32_t count, std::string& label, bool retained) : m_Count(count), isStatic(retained)
	{
		// Dynamic buffer: one copy per in-flight frame.
		// Retained buffer: a single copy; updates synchronize with the GPU.
        const uint32_t copies = retained ? 1 : kRingSize;
        for (uint32_t i = 0; i < copies; i++) {
            m_Buffers[i] = MetalContext::GetCurrentDevice()->newBuffer(count * sizeof(uint32_t), MTL::ResourceStorageModeShared);
            if (!label.empty()) m_Buffers[i]->setLabel(NS::String::string(label.c_str(), NS::UTF8StringEncoding));
        }
	}

	MetalIndexBuffer::~MetalIndexBuffer()
	{
        const uint32_t copies = isStatic ? 1 : kRingSize;
        for (uint32_t i = 0; i < copies; i++) {
            if (m_Buffers[i]) {
                m_Buffers[i]->release();
                m_Buffers[i] = nullptr;
            }
        }
	}

	uint32_t MetalIndexBuffer::CurrentSlot() const
	{
		return isStatic ? 0 : (MetalContext::GetFrameInFlightIndex() % kRingSize);
	}

	MTL::Buffer* MetalIndexBuffer::GetBuffer() const
	{
		const uint32_t slot = CurrentSlot();
		if (!isStatic && m_SlotVersions[slot] != m_ContentVersion && m_ContentBytes) {
			memcpy(m_Buffers[slot]->contents(), m_Buffers[m_LatestSlot]->contents(), m_ContentBytes);
			m_SlotVersions[slot] = m_ContentVersion;
		}
		return m_Buffers[slot];
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

	void MetalIndexBuffer::ResizeBuffer(uint32_t count)
	{
		const uint32_t copies = isStatic ? 1 : kRingSize;
		for (uint32_t i = 0; i < copies; i++) {
			if (m_Buffers[i]) m_Buffers[i]->release();
			m_Buffers[i] = MetalContext::GetCurrentDevice()->newBuffer(count * sizeof(uint32_t), MTL::ResourceStorageModeShared);
		}
		m_Count = count;
		m_ContentBytes = 0;
		m_ContentVersion++;
	}

	void MetalIndexBuffer::SetData(const uint32_t* data, uint32_t count, uint32_t offset)
	{
        if (isStatic) {
            // Single-copy buffer: make sure no in-flight frame is still reading.
            MetalContext::WaitForGpuIdle();
        }

        const uint32_t slot = CurrentSlot();
        MTL::Buffer* buffer = m_Buffers[slot];

        assert(buffer != nullptr && "MTLBuffer is nil");

        const size_t bufLen = buffer->length();
        const size_t size = count * sizeof(uint32_t);
        assert(offset + size <= bufLen && "Write out of bounds");

        // Get a raw pointer to the buffer's memory.
        uint8_t* buffer_ptr = static_cast<uint8_t*>(buffer->contents());

        // Apply the byte offset and copy the data.
        memcpy(buffer_ptr + offset, data, size);

        m_ContentVersion++;
        m_SlotVersions[slot] = m_ContentVersion;
        m_LatestSlot = slot;
        if (offset + size > m_ContentBytes)
            m_ContentBytes = (uint32_t)(offset + size);
	}

} // namespace Graphics
