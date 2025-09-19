#pragma once

#include "Renderer/ComputeShader.h"
#include <Metal/Metal.hpp>

namespace Graphics {

	class MetalComputeShader : public ComputeShader, public Shader
	{
	public:
        MetalComputeShader(const std::filesystem::path& filepath);
        MetalComputeShader(const std::string& Src, const std::string& functionName, bool isMSL = false);
//		virtual ~MetalComputeShader();

        virtual uint32_t GetRendererID() const override { return 0; }
        
        virtual void Bind() override;
        
        virtual void Unbind() override;
        
        virtual void BindTexture(uintptr_t texture, int slot) override;
        
        virtual void SetInt(int* ptr, int slot) override;
        
        virtual void Dispatch(uint32_t width, uint32_t height, uint32_t depth) override;
        
        virtual void SetIntArray(const std::string &name, int *values, uint32_t count) override {};
        
        virtual void SetFloat(const std::string &name, float value) override {};
        
        virtual void SetFloat2(const std::string &name, const glm::vec2 &value) override {};
        
        virtual void SetFloat3(const std::string &name, const glm::vec3 &value) override {};
        
        virtual void SetFloat4(const std::string &name, const glm::vec4 &value) override {};
        
        virtual void SetMat4(const std::string &name, const glm::mat4 &value) override {};
        
        virtual const std::string &GetName() const override {};
        
        virtual const uint32_t &GetId() const override {};
        
        virtual const uint32_t &GetVertexAttributeLocation(const std::string &name) const override {};
        
        virtual void Bind() const override {}
        
        virtual void Unbind() const override {}
        
        virtual void SetInt(const std::string &name, int value) override {}
        
	private:
        void CreateComputeShader(const std::string& MSLSrc, const std::string& functionName);
        MTL::Library* m_Library = nullptr;
        MTL::ComputeCommandEncoder* m_ComputeCommandEncoder = nullptr;
        MTL::Function* m_ComputeFunction = nullptr;
        MTL::ComputePipelineState* m_ComputePipelineState = nullptr;
        bool m_IsLoaded = false;
        std::string m_Path;
    };


}
