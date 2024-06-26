#pragma once

#include "Renderer/Shader.h"
#include <glm/glm.hpp>

// TODO: REMOVE!
typedef unsigned int GLenum;

namespace Graphics {

	class OpenGLShader : public Shader
	{
	public:
		OpenGLShader(const std::string& filepath, bool cache);
		OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
		virtual ~OpenGLShader();

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
		virtual const uint32_t& GetId() const override { return m_RendererID; }

		virtual const uint32_t& GetVertexAttributeLocation(const std::string& name) const override;

		void UploadUniformInt(const std::string& name, int value);
		void UploadUniformIntArray(const std::string& name, int* values, uint32_t count);

		void UploadUniformFloat(const std::string& name, float value);
		void UploadUniformFloat2(const std::string& name, const glm::vec2& value);
		void UploadUniformFloat3(const std::string& name, const glm::vec3& value);
		void UploadUniformFloat4(const std::string& name, const glm::vec4& value);

		void UploadUniformMat3(const std::string& name, const glm::mat3& matrix);
		void UploadUniformMat4(const std::string& name, const glm::mat4& matrix);
	private:

		//-1 coz of #type in shader
		struct ShaderProgramSource
		{
			int lineOffset = -1;
			std::string Source;
		};

		using ShaderSources = std::unordered_map<GLenum, ShaderProgramSource>;

		std::string ReadFile(const std::string& filepath, uint32_t* num_lines = nullptr);
		void PreProcessIncludes(ShaderSources& source);
		ShaderSources PreProcess(const std::string& source);

		std::string ResetLineOffset(const std::string& source, const std::string& filename, int lineOffset);

		void CompileOrGetVulkanBinaries(const ShaderSources& shaderSources);
		void CompileOrGetOpenGLBinaries();
		void CompileOrGetOpenGLBinaries(const ShaderSources& shaderSources);
		void CreateProgram();
		void FillVertexAttributeLocations(const ShaderSources& shaderSources);
		void Reflect(GLenum stage, const std::vector<uint32_t>& shaderData);
	private:
		uint32_t m_RendererID;
		std::string m_FilePath;
		std::string m_Name;
		bool m_EnableCache;

		std::unordered_map<std::string, int> m_VertexAttributeLocationCache;
		
		std::unordered_map<GLenum, std::vector<uint32_t>> m_VulkanSPIRV;
		std::unordered_map<GLenum, std::vector<uint32_t>> m_OpenGLSPIRV;

		std::unordered_map<GLenum, std::string> m_OpenGLSourceCode;
	};

}
