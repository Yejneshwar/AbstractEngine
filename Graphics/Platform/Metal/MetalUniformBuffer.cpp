#include "MetalUniformBuffer.h"

#include "MetalContext.h"


namespace Graphics {

    MetalUniformBuffer::MetalUniformBuffer(uint32_t size, uint32_t binding, std::string& label)
        : m_Size(size), m_Binding(binding)
    {
        for (uint32_t i = 0; i < kSlots; i++)
        {
            m_Buffers[i] = MetalContext::GetCurrentDevice()->newBuffer(size, MTL::ResourceStorageModeShared);
            if (!label.empty())
            {
                m_Buffers[i]->setLabel(NS::String::string(label.c_str(), NS::UTF8StringEncoding));
            }
        }
    }

    MetalUniformBuffer::~MetalUniformBuffer()
    {
        for (uint32_t i = 0; i < kSlots; i++)
        {
            if (m_Buffers[i])
            {
                m_Buffers[i]->release();
                m_Buffers[i] = nullptr;
            }
        }
    }

    void MetalUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
    {
        // Rotate to a fresh slot so draws already encoded against the
        // previous contents (in this frame or in flight) are unaffected.
        m_Slot = (m_Slot + 1) % kSlots;
        MTL::Buffer* buffer = m_Buffers[m_Slot];

        if (buffer && (offset + size <= m_Size))
        {
            uint8_t* bufferContents = static_cast<uint8_t*>(buffer->contents());
            memcpy(bufferContents + offset, data, size);
        }

        MTL::RenderCommandEncoder* pEncoder = MetalContext::GetCurrentRenderCommandEncoder();
        if (!pEncoder)
            return;

        // Bind the buffer to the vertex and fragment shader stages.
        pEncoder->setVertexBuffer(buffer, 0, m_Binding);
        pEncoder->setFragmentBuffer(buffer, 0, m_Binding);
    }

}
