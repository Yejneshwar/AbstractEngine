#include "GraphicsCore.h"
#include "Platform/OpenGL/OpenGLShader.h"

#include <fstream>
#include <glad/gl.h>

#include <glm/gtc/type_ptr.hpp>

#include <spirv_glsl.hpp>
#include <GLFW/glfw3.h>

#include "Logger.h"
#include "OpenGLShader.h"
//Note: Keep bindings and locations explicit even in opengl?


namespace Graphics {

	namespace Utils {

		static GLenum ShaderStageToGLEnum(const ShaderStage& stage)
		{
			switch (stage) {
				case VERTEX_SHADER:
					return GL_VERTEX_SHADER;
				case FRAGMENT_SHADER:
					return GL_FRAGMENT_SHADER;
				case GEOMETRY_SHADER:
					return GL_GEOMETRY_SHADER;
			}

			GRAPHICS_CORE_ASSERT(false, "Unknown shader type!");
			return 0;
		}

		static const char* GetCacheDirectory()
		{
			// TODO: make sure the assets directory is valid
			return "Resource/Shaders/cache/opengl";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::string cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);

			LOG_DEBUG_STREAM << "ShaderCache : " << std::filesystem::absolute(cacheDirectory);
		}

		static const char* GLShaderStageCachedOpenGLFileExtension(ShaderStage stage)
		{
			switch (stage)
			{
			case VERTEX_SHADER:    return ".cached_opengl.vert";
			case FRAGMENT_SHADER:  return ".cached_opengl.frag";
			case GEOMETRY_SHADER:  return ".cached_opengl.geom";
			}
			GRAPHICS_CORE_ASSERT(false);
			return "";
		}

		static const char* GLShaderStageCachedVulkanFileExtension(ShaderStage stage)
		{
			switch (stage)
			{
			case VERTEX_SHADER:    return ".cached_vulkan.vert";
			case FRAGMENT_SHADER:  return ".cached_vulkan.frag";
			case GEOMETRY_SHADER:  return ".cached_vulkan.geom";
			}
			GRAPHICS_CORE_ASSERT(false);
			return "";
		}


	}

	OpenGLShader::OpenGLShader(const std::string& filepath, bool cache)
		: m_FilePath(filepath), m_EnableCache(cache)
	{


		Utils::CreateCacheDirectoryIfNeeded();

		std::string source = ReadFile(filepath);


		Shader::ShaderProgramSources shaderProgramSources = PreProcess(source);

		PreProcessIncludes(shaderProgramSources);

		LOG_DEBUG_STREAM << "//////////////////////////////////////Compiling shader " << filepath;
		{
			//Timer timer;
			try {
				//CompileOrGetVulkanBinaries(shaderProgramSources);
				CompileOrGetOpenGLBinaries(shaderProgramSources);
#if IS_LOG_TRACE
				auto& shaderDataOpenGL = m_OpenGLSPIRV;
				auto& shaderDataVulkan = m_VulkanSPIRV;
				LOG_TRACE_STREAM << "//////////////////////////////////////Vulkan reflection";
				for (auto&& [stage, data] : shaderDataVulkan)
					Reflect(stage, data);
				LOG_TRACE_STREAM << "//////////////////////////////////////OpenGL reflection";
					for (auto&& [stage, data] : shaderDataOpenGL)
						Reflect(stage, data);
#endif
				CreateProgram();
				FillVertexAttributeLocations(shaderProgramSources);

			}
			catch (std::runtime_error e) {
				LOG_DEBUG_STREAM << "Shader error : " << e.what();
				LOG_DEBUG_STREAM << "//////////////////////////////////////End Compilation";
				return;
			}


			LOG_DEBUG_STREAM << "//////////////////////////////////////End Compilation";


			//HZ_CORE_WARN("Shader creation took {0} ms", timer.ElapsedMillis());
		}

		// Extract name from filepath
		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = filepath.rfind('.');
		auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
		m_Name = filepath.substr(lastSlash, count);
	}

	OpenGLShader::OpenGLShader(const std::string& name, const ShaderSources& shaderSources)
		: m_Name(name)
	{


		ShaderProgramSources shaderProgramsources;

		for (auto&& [stage, src] : shaderSources) {
			shaderProgramsources[stage] = { 0, src };
		}


		CompileOrGetVulkanBinaries(shaderProgramsources);
		CompileOrGetOpenGLBinaries(shaderProgramsources);
		CreateProgram();
	}

	OpenGLShader::~OpenGLShader()
	{
		glDeleteProgram(m_RendererID);
	}

	void OpenGLShader::CompileOrGetVulkanBinaries(const OpenGLShader::ShaderProgramSources& shaderProgramSources)
	{
		GLuint program = glCreateProgram();

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		const bool optimize = true;
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

		auto& shaderData = m_VulkanSPIRV;
		shaderData.clear();
		for (auto&& [stage, program] : shaderProgramSources)
		{
			std::filesystem::path shaderFilePath = m_FilePath;
			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + Utils::GLShaderStageCachedVulkanFileExtension(stage));

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open() && m_EnableCache)
			{
				in.seekg(0, std::ios::end);
				auto size = in.tellg();
				in.seekg(0, std::ios::beg);

				auto& data = shaderData[stage];
				data.resize(size / sizeof(uint32_t));
				in.read((char*)data.data(), size);
			}
			else
			{
				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(program.Source, Utils::ShaderStageToShaderC(stage), m_FilePath.c_str(), options);
				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					LOG_FATAL_STREAM << "\n" << Utils::ResetLineOffset(module.GetErrorMessage(), m_FilePath, program.lineOffset);;
					throw(std::runtime_error("Error in compiling shader for Vulkan"));
					GRAPHICS_CORE_ASSERT(false);
				}

				shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					auto& data = shaderData[stage];
					out.write((char*)data.data(), data.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}
		}

	}


	/**
	* Uses vulkan binaries if already compiled
	* Can have issues with : gl_VertexID
	*/
	void OpenGLShader::CompileOrGetOpenGLBinaries()
	{
		auto& shaderData = m_OpenGLSPIRV;

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
		const bool optimize = false;
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

		shaderData.clear();
		m_OpenGLSourceCode.clear();
		for (auto&& [stage, spirv] : m_VulkanSPIRV)
		{
			std::filesystem::path shaderFilePath = m_FilePath;
			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + Utils::GLShaderStageCachedOpenGLFileExtension(stage));

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open() && m_EnableCache)
			{
				in.seekg(0, std::ios::end);
				auto size = in.tellg();
				in.seekg(0, std::ios::beg);

				auto& data = shaderData[stage];
				data.resize(size / sizeof(uint32_t));
				in.read((char*)data.data(), size);
			}
			else
			{
				spirv_cross::CompilerGLSL glslCompiler(spirv);
				m_OpenGLSourceCode[stage] = glslCompiler.compile();
				auto& source = m_OpenGLSourceCode[stage];

				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, Utils::ShaderStageToShaderC(stage), m_FilePath.c_str());
				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					LOG_FATAL_STREAM << module.GetErrorMessage();
					GRAPHICS_CORE_ASSERT(false);
				}

				shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					auto& data = shaderData[stage];
					out.write((char*)data.data(), data.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}
		}
	}

	void OpenGLShader::CompileOrGetOpenGLBinaries(const ShaderProgramSources& shaderProgramSources)
	{
		auto& shaderData = m_OpenGLSPIRV;

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
		const bool optimize = false;
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

		shaderData.clear();
		m_OpenGLSourceCode.clear();
		for (auto&& [stage, program] : shaderProgramSources)
		{
			std::filesystem::path shaderFilePath = m_FilePath;
			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + Utils::GLShaderStageCachedOpenGLFileExtension(stage));

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open() && m_EnableCache)
			{
				in.seekg(0, std::ios::end);
				auto size = in.tellg();
				in.seekg(0, std::ios::beg);

				auto& data = shaderData[stage];
				data.resize(size / sizeof(uint32_t));
				in.read((char*)data.data(), size);
			}
			else
			{
				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(program.Source, Utils::ShaderStageToShaderC(stage), m_FilePath.c_str(), options);
				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					LOG_FATAL_STREAM << "\n" << Utils::ResetLineOffset(module.GetErrorMessage(), m_FilePath, program.lineOffset);
					LOG_FATAL_STREAM << "Stage :" << Utils::ShaderStageToString(stage);
					throw(std::runtime_error("Error in compiling shader for OPENGL"));

					GRAPHICS_CORE_ASSERT(false);
				}

				shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					auto& data = shaderData[stage];
					out.write((char*)data.data(), data.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}
		}
	}

	void OpenGLShader::CreateProgram()
	{
		GLuint program = glCreateProgram();

		std::vector<GLuint> shaderIDs;
		for (auto&& [stage, spirv] : m_OpenGLSPIRV)
		{
			LOG_TRACE_STREAM << "Creating " << Utils::ShaderStageToString(stage) << " shader";
			GLuint shaderID = shaderIDs.emplace_back(glCreateShader(Utils::ShaderStageToGLEnum(stage)));
			GLenum error = glGetError();
			if (error != GL_NO_ERROR) {
				// Handle the error appropriately
				LOG_FATAL_STREAM << "Error creating " << Utils::ShaderStageToString(stage) << " shader : " << error;
				// Additional error handling code
			}
			glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, spirv.data(), spirv.size() * sizeof(uint32_t));
			//std::cout << "Shader ID: " << shaderID << std::endl;
			glSpecializeShader(shaderID, "main", 0, nullptr, nullptr);
			glAttachShader(program, shaderID);

		}

		glLinkProgram(program);

		GLint isLinked;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());
			//HZ_CORE_ERROR("Shader linking failed ({0}):\n{1}", m_FilePath, infoLog.data());

			glDeleteProgram(program);

			for (auto id : shaderIDs)
				glDeleteShader(id);

			const auto& string = std::format("Shader linking failed ({}):\n{}", infoLog.data(), program);
			throw(std::runtime_error(string));
			return;
		}

		for (auto id : shaderIDs)
		{
			glDetachShader(program, id);
			glDeleteShader(id);
		}

		m_RendererID = program;
	}


    void OpenGLShader::FillVertexAttributeLocations(const ShaderProgramSources& shaderProgramSources){
		for (auto&& [stage, program] : shaderProgramSources)
		{
			if(stage != VERTEX_SHADER)
				continue;
			
			spirv_cross::Compiler compiler(m_OpenGLSPIRV[stage]);
			spirv_cross::ShaderResources resources = compiler.get_shader_resources();

			for (const auto& resource : resources.stage_inputs)
			{
				const auto& bufferType = compiler.get_type(resource.base_type_id);
				uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
				assert(!resource.name.empty(), "Resource name is empty");
				m_VertexAttributeLocationCache[resource.name] = location;
			}
		}
	}

	void OpenGLShader::Bind() const
	{
		glUseProgram(m_RendererID);
	}

	void OpenGLShader::Unbind() const
	{


		glUseProgram(0);
	}

	const uint32_t& OpenGLShader::GetVertexAttributeLocation(const std::string& name) const
    {
		auto it = m_VertexAttributeLocationCache.find(name);
		assert(it != m_VertexAttributeLocationCache.end(), "Vertex attribute not found");
		return it->second;
    }

	void OpenGLShader::SetInt(const std::string& name, int value)
	{


		UploadUniformInt(name, value);
	}

	void OpenGLShader::SetIntArray(const std::string& name, int* values, uint32_t count)
	{
		UploadUniformIntArray(name, values, count);
	}

	void OpenGLShader::SetFloat(const std::string& name, float value)
	{


		UploadUniformFloat(name, value);
	}

	void OpenGLShader::SetFloat2(const std::string& name, const glm::vec2& value)
	{


		UploadUniformFloat2(name, value);
	}

	void OpenGLShader::SetFloat3(const std::string& name, const glm::vec3& value)
	{


		UploadUniformFloat3(name, value);
	}

	void OpenGLShader::SetFloat4(const std::string& name, const glm::vec4& value)
	{


		UploadUniformFloat4(name, value);
	}

	void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& value)
	{


		UploadUniformMat4(name, value);
	}

void OpenGLShader::UploadUniformInt(const std::string& name, int value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1i(location, value);
	}

	void OpenGLShader::UploadUniformIntArray(const std::string& name, int* values, uint32_t count)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1iv(location, count, values);
	}

	void OpenGLShader::UploadUniformFloat(const std::string& name, float value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1f(location, value);
	}

	void OpenGLShader::UploadUniformFloat2(const std::string& name, const glm::vec2& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform2f(location, value.x, value.y);
	}

	void OpenGLShader::UploadUniformFloat3(const std::string& name, const glm::vec3& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform3f(location, value.x, value.y, value.z);
	}

	void OpenGLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}

	void OpenGLShader::UploadUniformMat3(const std::string& name, const glm::mat3& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void OpenGLShader::UploadUniformMat4(const std::string& name, const glm::mat4& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

}
