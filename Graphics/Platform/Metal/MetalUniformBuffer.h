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
        // Each SetData writes a fresh slot and binds it, so earlier draws in
        // flight (or in the same frame, e.g. per-viewport camera data) keep
        // reading the contents they were encoded with. Sized to cover
        // kMaxFramesInFlight frames of several updates each.
        static constexpr uint32_t kSlots = 12;

        MTL::Buffer* m_Buffers[kSlots] = {};
        uint32_t m_Slot = 0;
        uint32_t m_Size = 0;
        uint32_t m_Binding;
    };
}
