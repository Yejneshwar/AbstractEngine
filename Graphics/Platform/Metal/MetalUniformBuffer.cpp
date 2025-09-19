#include "MetalUniformBuffer.h"

#include "MetalContext.h"


namespace Graphics {

    MetalUniformBuffer::MetalUniformBuffer(uint32_t size, uint32_t binding, std::string& label)
        : m_Size(size), m_Binding(binding)
    {
        m_Buffer = MetalContext::GetCurrentDevice()->newBuffer(size, MTL::ResourceStorageModeShared);
        if (!label.empty())
        {
            m_Buffer->setLabel(NS::String::string(label.c_str(), NS::UTF8StringEncoding));
        }
    }

    MetalUniformBuffer::~MetalUniformBuffer()
    {
    }

    void MetalUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
    {
        if (m_Buffer && (offset + size <= m_Size))
        {
            uint8_t* bufferContents = static_cast<uint8_t*>(m_Buffer->contents());
            memcpy(bufferContents + offset, data, size);
        }
        
        MTL::RenderCommandEncoder* pEncoder = MetalContext::GetCurrentRenderCommandEncoder();

        // Bind the buffer to the vertex and fragment shader stages.
        pEncoder->setVertexBuffer(m_Buffer, 0, m_Binding);
        pEncoder->setFragmentBuffer(m_Buffer, 0, m_Binding);
    }

}
