#pragma once

#import "Renderer/VertexArray.h"
#include "Metal/Metal.hpp"
#import <vector>

namespace Graphics {

class MetalVertexArray : public VertexArray {
public:
    MetalVertexArray();
    virtual ~MetalVertexArray();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
    virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer, const Ref<Shader>& shaderInput) override;
    virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) override;

    virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const override { return m_VertexBuffers; }
    virtual const Ref<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

    MTL::VertexDescriptor* GetVertexDescriptor() const { return m_VertexDescriptor; }

private:
    MTL::VertexDescriptor* m_VertexDescriptor = nullptr;
    std::vector<Ref<VertexBuffer>> m_VertexBuffers;
    Ref<IndexBuffer> m_IndexBuffer = nullptr;
    bool previousVertexBufferGetsLocations = false;
    uint32_t m_VertexBufferIndex = 0;
};

} // namespace Graphics
