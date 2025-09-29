#include "Platform/OpenGL/OpenGLComputeShader.h"
#include "glad/gl.h"


namespace Graphics {

	OpenGLComputeShader::OpenGLComputeShader(const std::filesystem::path& filepath) : Shader(filepath.string())
	{
		this->CompileOrGetSpirVBinaries(m_ShaderSources);

		auto&& spirVSource = m_SPIRV[ShaderStage::COMPUTE_SHADER];

		this->CreateComputeShader();

	}

	OpenGLComputeShader::OpenGLComputeShader(const std::string& Src, const std::string& functionName)
	{
	}

	void OpenGLComputeShader::CreateComputeShader()
	{
		// Get the compiled SPIR-V binary for the compute stage.
		const auto& spirVBinary = m_SPIRV.at(ShaderStage::COMPUTE_SHADER);


		// 1. Create a program object.
		// This will be the handle to our final compute shader program.
		GLuint program = glCreateProgram();

		// 2. Create a shader object for the compute stage.
		GLuint shader = glCreateShader(GL_COMPUTE_SHADER);

		// 3. Load the SPIR-V binary into the shader object.
		glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, spirVBinary.data(), spirVBinary.size() * sizeof(uint32_t));

		// 4. Specialize the shader. This tells OpenGL the entry point function (usually "main").
		// This step is mandatory for SPIR-V shaders.
		glSpecializeShader(shader, "main", 0, nullptr, nullptr);

		//////////// BEGIN - If compiling glsl directly /////////////////
		//const auto& sourceProgram = m_ShaderSources.at(ShaderStage::COMPUTE_SHADER);
		//const char* shaderCode = sourceProgram.Source.c_str();
		//glShaderSource(shader, 1, &shaderCode, NULL);
		//glCompileShader(shader);
		//////////// END - If compiling glsl directly //////////////////

		// 5. Check for compilation/specialization errors.
		GLint isCompiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
		if (isCompiled == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

			// Clean up before throwing an error.
			glDeleteShader(shader);
			glDeleteProgram(program);

			// Log the error and stop execution.
			std::cerr << "Compute shader specialization failed for " << m_FilePath << ":\n" << infoLog.data() << std::endl;
			// Or throw an exception, assert, etc.
			return;
		}

		// 6. Attach the compiled shader to the program and link it.
		glAttachShader(program, shader);
		glLinkProgram(program);

		// 7. Check for linking errors.
		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

			// Clean up before throwing an error.
			glDeleteProgram(program);
			glDeleteShader(shader);

			// Log the error and stop execution.
			std::cerr << "Compute shader linking failed for " << m_FilePath << ":\n" << infoLog.data() << std::endl;
			return;
		}

		// 8. Detach and delete the shader object.
		// It's no longer needed after a successful link.
		glDetachShader(program, shader);
		glDeleteShader(shader);

		// Assign the successfully created program to our member variable.
		m_RendererID = program;
	}

	void OpenGLComputeShader::Bind()
	{
		glUseProgram(m_RendererID);
	}

	void OpenGLComputeShader::Unbind()
	{
		glUseProgram(0);
	}

	void OpenGLComputeShader::BindTexture(uintptr_t texture, int slot)
	{
		GLint internalFormat;

		// 1. Bind the texture to its target
		glBindTexture(GL_TEXTURE_2D, texture);

		// 2. Query the internal format for mipmap level 0
		glGetTexLevelParameteriv(
			GL_TEXTURE_2D,       // The texture target
			0,                   // The mipmap level to query
			GL_TEXTURE_INTERNAL_FORMAT, // The parameter you want to get
			&internalFormat      // Pointer to the integer to store the result
		);

		// 'internalFormat' now holds the texture's format, e.g., GL_RGBA8, GL_R32F, etc.
		// You can unbind the texture
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindImageTexture(
			slot,                            // The binding slot (image unit)
			(GLuint)texture,                 // The texture ID
			0,                               // Mipmap level
			GL_FALSE,                        // Not layered
			0,                               // Layer
			GL_READ_WRITE,                   // Access type
			internalFormat                       // Format must match the texture's internal format
		);
	}

	void OpenGLComputeShader::SetInt(int* ptr, int slot)
	{
		// Set an integer uniform value at a specific location.
		// The program must be bound before this call.
		glUniform1i(slot, *ptr);
	}

	void OpenGLComputeShader::Dispatch(uint32_t width, uint32_t height, uint32_t depth)
	{
		// Execute the compute shader.
		// 'width', 'height', and 'depth' specify the number of work groups to launch.
		glDispatchCompute(width, height, depth);

		// Ensure that memory operations are complete before proceeding.
		// This is crucial if the results are needed immediately by subsequent OpenGL calls.
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
}