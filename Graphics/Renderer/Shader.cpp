#include "GraphicsCore.h"
#include "Renderer/Shader.h"

#include "Renderer/Renderer.h"
#include <fstream>

#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>
#include <shaderc/shaderc.hpp>

#ifdef BUILDING_METAL
#include "Platform/Metal/MetalShader.h"
#include "IOS/FileUtils.h"
#include <spirv_cross.hpp>
#include <spirv_msl.hpp>
#else
#include "Platform/OpenGL/OpenGLShader.h"
#endif

namespace Utils {
    static shaderc_shader_kind ShaderStageToShaderC(Graphics::ShaderStage stage)
    {
        switch (stage)
        {
            case Graphics::ShaderStage::VERTEX_SHADER:   return shaderc_glsl_vertex_shader;
            case Graphics::ShaderStage::FRAGMENT_SHADER: return shaderc_glsl_fragment_shader;
            case Graphics::ShaderStage::GEOMETRY_SHADER: return shaderc_glsl_geometry_shader;
            case Graphics::ShaderStage::COMPUTE_SHADER: return shaderc_glsl_compute_shader;
            case Graphics::ShaderStage::UNKNOWN: throw("Unknown shader stage");
        }
        GRAPHICS_CORE_ASSERT(false);
        return (shaderc_shader_kind)0;
    }

    std::string ResetLineOffset(const std::string& source, const std::string& filename, int lineOffset)
    {
        //Sample error
        //./Resources/Shaders/MeshNormals.glsl:96: error: 'pixelFac' : undeclared identifier
        
        std::string result;
        std::istringstream stream(source);
        
        //Find the line number in error message
        std::string line;
        
        while (std::getline(stream, line))
        {
            if (line.find(filename) != std::string::npos)
            {
                std::string::size_type pos = line.find(":");
                if (pos != std::string::npos)
                {
                    std::string::size_type pos2 = line.find(":", pos + 1);
                    if (pos2 != std::string::npos)
                    {
                        std::string lineNumber = line.substr(pos + 1, pos2 - pos - 1);
                        int errorLine = std::stoi(lineNumber);
                        errorLine -= lineOffset;
                        line.replace(pos + 1, pos2 - pos - 1, std::to_string(errorLine));
                    }
                }
            }
            result += line + "\n";
        }
        
        return result;
    }

}

namespace Graphics {
    
    std::string Shader::CompileSpirVToMSL(ShaderStage stage, const std::vector<uint32_t>& shaderData) {
        spirv_cross::CompilerMSL compiler(shaderData);
        
        spirv_cross::CompilerMSL::Options msl_options;
        msl_options.platform = spirv_cross::CompilerMSL::Options::iOS;
        compiler.set_msl_options(msl_options);
        
        try{
            // Compile to MSL
            return compiler.compile();
        }
        catch (const spirv_cross::CompilerError& e) {
            std::cerr << "SPIRV-Cross Compiler Error: " << e.what() << std::endl;
            GRAPHICS_CORE_ASSERT(false);
        }
        catch (const std::exception& e) {
            std::cerr << "General Error: " << e.what() << std::endl;
            GRAPHICS_CORE_ASSERT(false);
        }
        
        throw;
    }

    std::string Shader::ReadFile(const std::string& filepath, uint32_t* num_lines)
    {
        std::string result;
        std::ifstream in(filepath, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
        if (in)
        {
            in.seekg(0, std::ios::end);
            size_t size = in.tellg();
            if (size != -1)
            {
                result.resize(size);
                in.seekg(0, std::ios::beg);
                in.read(&result[0], size);
                
                in.seekg(0, std::ios::beg);
                // new lines will be skipped unless we stop it from happening:
                in.unsetf(std::ios_base::skipws);
                // count the newlines with an algorithm specialized for counting:
                size_t newlines = std::count(
                                            std::istream_iterator<char>(in),
                                            std::istream_iterator<char>(),
                                            '\n');
                LOG_TRACE_STREAM << "Read " << newlines << " lines from file " << filepath;
                if (num_lines)
                    *num_lines = newlines;
            }
            else
            {
                LOG_FATAL_STREAM << "Could not read from file " << filepath;
            }
        }
        else
        {
            LOG_FATAL_STREAM << "Could not open shader file " << filepath;
            GRAPHICS_CORE_ASSERT(false)
        }
        
        return result;
    }
    
    void Shader::PreProcessIncludes(ShaderProgramSources& shaderSources)
    {
        for (auto&& [stage, program] : shaderSources)
        {
            auto& source = program.Source;
            while (source.find("#include ") != source.npos)
                {
                    const auto pos = source.find("#include ");
                    const auto p1 = source.find('<', pos);
                    const auto p2 = source.find('>', pos);
                    if (p1 == source.npos || p2 == source.npos || p2 <= p1)
                    {
                        LOG_FATALF("Error while loading shader program: %s\n", source.c_str());
                        return;
                    }
                    const std::string name = source.substr(p1 + 1, p2 - p1 - 1);
                    uint32_t num_lines;
#ifdef BUILDING_METAL
                    const std::string include = ReadFile(GUI::Utils::getResourcePath(name).c_str(), &num_lines);
#else
                    const std::string include = ReadFile(name.c_str(), &num_lines);
#endif
                    source.replace(pos, p2 - pos + 1, include.c_str());
                    program.lineOffset += num_lines;
                }
        }
    }
    
    Shader::ShaderProgramSources Shader::PreProcess(const std::string& source)
    {
        
        Shader::ShaderProgramSources shaderSources;
        
        const char* typeToken = "#type";
        size_t typeTokenLength = strlen(typeToken);
        size_t pos = source.find(typeToken, 0); //Start of shader type declaration line
        uint32_t lineOffsetFromPreviousStage = 0;
        while (pos != std::string::npos)
        {
            size_t eol = source.find_first_of("\r\n", pos); //End of shader type declaration line
            GRAPHICS_CORE_ASSERT(eol != std::string::npos, "Syntax error");
            size_t begin = pos + typeTokenLength + 1; //Start of shader type name (after "#type " keyword)
            std::string type = source.substr(begin, eol - begin);
            GRAPHICS_CORE_ASSERT(Utils::ShaderTypeFromString(type), "Invalid shader type specified");
            
            size_t nextLinePos = source.find_first_not_of("\r\n", eol); //Start of shader code after shader type declaration line
            GRAPHICS_CORE_ASSERT(nextLinePos != std::string::npos, "Syntax error");
            pos = source.find(typeToken, nextLinePos); //Start of next shader type declaration line
            
            shaderSources[Utils::ShaderTypeFromString(type)].lineOffset -= lineOffsetFromPreviousStage;
            shaderSources[Utils::ShaderTypeFromString(type)].Source = (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
            std::istringstream iss(shaderSources[Utils::ShaderTypeFromString(type)].Source);
            iss.unsetf(std::ios_base::skipws);
            
            lineOffsetFromPreviousStage = std::count(
                                                    std::istream_iterator<char>(iss),
                                                    std::istream_iterator<char>(),
                                                    '\n') + 1; //Last newlines are not counted
        }
        
        //LOG_DEBUG_STREAM << "Vertex Shader ###### \n" << shaderSources[GL_VERTEX_SHADER];
        //LOG_DEBUG_STREAM << "Fragment Shader ###### \n" << shaderSources[GL_FRAGMENT_SHADER];
        
        
        return shaderSources;
    }

    void Shader::CompileOrGetSpirVBinaries(const ShaderProgramSources& shaderSources)
    {
        auto& shaderData = m_SPIRV;
        
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
        const bool optimize = false;
        if (optimize)
            options.SetOptimizationLevel(shaderc_optimization_level_performance);
        
        shaderData.clear();
        for (auto&& [stage, program] : shaderSources)
        {
                shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(program.Source, ::Utils::ShaderStageToShaderC(stage), m_FilePath.c_str(), options);
                if (module.GetCompilationStatus() != shaderc_compilation_status_success)
                {
                    LOG_FATAL_STREAM << "\n" << ::Utils::ResetLineOffset(module.GetErrorMessage(), m_FilePath, program.lineOffset);
                    LOG_FATAL_STREAM << "Stage :" << Utils::ShaderStageToString(stage);
                    throw(std::runtime_error("Error in compiling shader to SPIRV"));
                    
                    GRAPHICS_CORE_ASSERT(false);
                }
                
                shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());
        }
    }

    void Shader::Reflect(ShaderStage stage, const std::vector<uint32_t>& shaderData)
    {
        spirv_cross::Compiler compiler(shaderData);
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();
        
        std::cout << std::format("OpenGLShader::Reflect - {}", Utils::ShaderStageToString(stage)) << std::endl;
        std::cout << std::format("    {} uniform buffers ", resources.uniform_buffers.size()) << std::endl;
        std::cout << std::format("    {} resources ", resources.sampled_images.size()) << std::endl;
        std::cout << std::format("    {} inputs ", resources.stage_inputs.size()) << std::endl;
        std::cout << std::format("    {} outputs ", resources.stage_outputs.size()) << std::endl;
        
        
        //HZ_CORE_TRACE("Uniform buffers:");
        for (const auto& resource : resources.uniform_buffers)
        {
            const auto& bufferType = compiler.get_type(resource.base_type_id);
            uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            int memberCount = bufferType.member_types.size();
            
            std::cout << "  " << resource.name << std::endl;
            std::cout << "    Size = " << bufferSize << std::endl;
            std::cout << "    Binding = " << binding << std::endl;
            std::cout << "    Members = " << memberCount << std::endl;
        }
        
        std::cout << std::endl << "  Inputs : " << std::endl;
        for (const auto& resource : resources.stage_inputs)
        {
            const auto& bufferType = compiler.get_type(resource.base_type_id);
            uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
            
            
            std::cout << "    " << (resource.name.empty() ? "Unknown - input" : resource.name) << std::endl;
            std::cout << "        Location = " << location << std::endl;
        }
        
        std::cout << std::endl << "  Outputs : " << std::endl;
        for (const auto& resource : resources.stage_outputs)
        {
            const auto& bufferType = compiler.get_type(resource.base_type_id);
            uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
            
            
            std::cout << "    " << (resource.name.empty() ? "Unknown - output" : resource.name) << std::endl;
            std::cout << "        Location = " << location << std::endl;
        }
    }

	Ref<Shader> Shader::Create(const std::string& filepath, bool cache)
	{
#ifdef BUILDING_METAL
        return CreateRef<MetalShader>(filepath, cache);
#else
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    GRAPHICS_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLShader>(filepath, cache);
		}

		GRAPHICS_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
#endif
	}

	Ref<Shader> Shader::Create(const std::string& name, const ShaderSources& shaderSources)
	{
#ifdef BUILDING_METAL
        return CreateRef<MetalShader>(name, shaderSources);
#else
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    GRAPHICS_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLShader>(name, vertexSrc, fragmentSrc);
		}

		GRAPHICS_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
#endif
	}

#ifdef BUILDING_METAL
    Ref<Shader> Shader::CreateFromMSL(const std::string& MSLSrc, const ShaderFunctionNames& shaderFunctionNames)
    {
        return CreateRef<MetalShader>(MSLSrc, shaderFunctionNames);
    }
#endif

    Shader::Shader(const std::string& filepath)
    : m_FilePath(filepath), m_Name("Unknown") {
        if(filepath.empty()) return;
        m_Name = extractNameFromPath(m_FilePath);
        std::string source = Shader::ReadFile(filepath);
        m_ShaderSources = Shader::PreProcess(source);
        Shader::PreProcessIncludes(m_ShaderSources);
    }

	void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader)
	{
		GRAPHICS_CORE_ASSERT(!Exists(name), "Shader already exists!");
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Add(const Ref<Shader>& shader)
	{
		auto& name = shader->GetName();
		Add(name, shader);
	}

	Ref<Shader> ShaderLibrary::Load(const std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Load(const std::string& name, const std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(name, shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Get(const std::string& name)
	{
		GRAPHICS_CORE_ASSERT(Exists(name), "Shader not found!");
		return m_Shaders[name];
	}

	bool ShaderLibrary::Exists(const std::string& name) const
	{
		return m_Shaders.find(name) != m_Shaders.end();
	}

}
