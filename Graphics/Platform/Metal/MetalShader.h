#pragma once

#include "Renderer/Shader.h"
#include <glm/glm.hpp>

#include <Metal/Metal.hpp>

namespace Graphics {

    class MetalShader : public Shader {
    public:
        MetalShader(const std::string& filepath, bool cache);
        MetalShader(const std::string& name, const ShaderSources& shaderSources);
        MetalShader(const std::filesystem::path& MSLFilePath, const ShaderFunctionNames& shaderFunctionNames);
        MetalShader(const std::string& MSLSrc, const ShaderFunctionNames& shaderFunctionNames);

        std::string CompileSpirVToMSL(ShaderStage, const std::vector<uint32_t>& shaderData);
        virtual ~MetalShader();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual void SetInt(const std::string& name, int value) override;
        virtual void SetIntArray(const std::string& name, int* values, uint32_t count) override;
        virtual void SetFloat(const std::string& name, float value) override;
        virtual void SetFloat2(const std::string& name, const glm::vec2& value) override;
        virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
        virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;
        virtual void SetMat4(const std::string& name, const glm::mat4& value) override;

        virtual const std::string& GetName() const override { return m_Name; }
        virtual const uint32_t& GetId() const override;
        
        const uint32_t &GetVertexAttributeLocation(const std::string &name) const override {}
        

        // Metal-specific methods
        MTL::Function* GetVertexFunction() const { return m_VertexFunction; }
        MTL::Function* GetFragmentFunction() const { return m_FragmentFunction; }

    private:
        std::string ReadFile(const std::string& filepath);
        bool CompileMSLLibrary(const std::string& MSLSrc);
        void CompileShader(ShaderStage stage,const std::string& source, const std::string& functionName = "main0");
        void CompileShader(const std::string& MSLSrc, const std::string& vertexFunctionName, const std::string& fragmentFunctionName);

        MTL::Library* m_Library = nullptr;
        MTL::Function* m_VertexFunction = nullptr;
        MTL::Function* m_FragmentFunction = nullptr;
        MTL::RenderPipelineDescriptor* m_PipelineDescriptor = nullptr;
        MTL::RenderPipelineState* m_PipelineState = nullptr;
    };

}
