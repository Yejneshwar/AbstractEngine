#pragma once

#include "Renderer/UniformBuffer.h"
#include <Metal/Metal.hpp>

namespace Graphics {

    class MetalUniformBuffer : public UniformBuffer
    {
    public:
        MetalUniformBuffer(uint32_t size, uint32_t binding, std::string& label);
        virtual ~MetalUniformBuffer();

        virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

    private:
        MTL::Buffer* m_Buffer = nullptr;
        uint32_t m_Size = 0;
        uint32_t m_Binding;
    };
}
