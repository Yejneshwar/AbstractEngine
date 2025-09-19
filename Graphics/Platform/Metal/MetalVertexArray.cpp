#import "Platform/Metal/MetalVertexArray.h"
#import "Renderer/Shader.h"
#import "Renderer/Buffer.h"

namespace Graphics {

static MTL::VertexFormat ShaderDataTypeToMTLVertexFormat(ShaderDataType type)
{
    switch (type)
    {
        case ShaderDataType::Float:   return MTL::VertexFormatFloat;
        case ShaderDataType::Float2:  return MTL::VertexFormatFloat2;
        case ShaderDataType::Float3:  return MTL::VertexFormatFloat3;
        case ShaderDataType::Float4:  return MTL::VertexFormatFloat4;
        case ShaderDataType::Int:     return MTL::VertexFormatInt;
        case ShaderDataType::Int2:    return MTL::VertexFormatInt2;
        case ShaderDataType::Int3:    return MTL::VertexFormatInt3;
        case ShaderDataType::Int4:    return MTL::VertexFormatInt4;
        case ShaderDataType::Bool:    return MTL::VertexFormatInt;
        case ShaderDataType::Mat3:
        case ShaderDataType::Mat4:
            // Not directly supported as-is; needs to be split into multiple attributes
            break;
    }

    GRAPHICS_CORE_ASSERT(false, "Unknown or unsupported ShaderDataType for Metal!");
    return MTL::VertexFormatInvalid;
}

MetalVertexArray::MetalVertexArray()
{
    m_VertexDescriptor = MTL::VertexDescriptor::alloc()->init();
}

MetalVertexArray::~MetalVertexArray()
{
    m_VertexDescriptor = nil;
}

void MetalVertexArray::Bind() const {
    for (auto vertexBuffer : m_VertexBuffers){
        vertexBuffer->Bind();
    }
}
void MetalVertexArray::Unbind() const {
    
}

void MetalVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
{
    GRAPHICS_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");

    if (m_VertexBuffers.empty()) {
        previousVertexBufferGetsLocations = false;
    } else {
        assert(!previousVertexBufferGetsLocations && "Previous VB gets locations, so must this");
    }

    const auto& layout = vertexBuffer->GetLayout();
    uint32_t bufferIndex = m_VertexBufferIndex++;
    uint32_t attributeIndex = 0;

    for (const auto& element : layout)
    {
        MTL::VertexAttributeDescriptor* attr = m_VertexDescriptor->attributes()->object(attributeIndex);
        attr->setFormat(ShaderDataTypeToMTLVertexFormat(element.Type));
        attr->setOffset(element.Offset);
        attr->setBufferIndex(30);

        MTL::VertexBufferLayoutDescriptor* bufferLayout = m_VertexDescriptor->layouts()->object(30);
        bufferLayout->setStride(layout.GetStride());
        bufferLayout->setStepFunction(element.Instanced ? MTL::VertexStepFunctionPerInstance : MTL::VertexStepFunctionPerVertex);
        bufferLayout->setStepRate(element.Divisor);

        attributeIndex++;
    }

    m_VertexBuffers.push_back(vertexBuffer);
}

void MetalVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer, const Ref<Shader>& shaderInput)
{
    // This version can eventually support reflection-based attribute mapping using shaderInput
    AddVertexBuffer(vertexBuffer);
}

void MetalVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
{
    if (m_IndexBuffer.get() != nullptr) {
        m_IndexBuffer->Unbind();
        m_IndexBuffer->~IndexBuffer();
    }

    m_IndexBuffer = indexBuffer;
}

} // namespace Graphics
