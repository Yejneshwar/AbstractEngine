#pragma once

#include <string>
#include <map>

#include <glm/glm.hpp>
#include "GraphicsCore.h"

namespace Graphics {

    enum ShaderStage {
        VERTEX_SHADER = 0x1,
        FRAGMENT_SHADER = 0x2,
        GEOMETRY_SHADER = 0x3,
        COMPUTE_SHADER = 0x4,
        UNKNOWN = 0x0
    };

    typedef std::map<ShaderStage, ShaderStage> ShaderSources;
#ifdef BUILDING_METAL
    typedef std::map<ShaderStage, std::string> ShaderFunctionNames;
#endif

    namespace Utils {
        static ShaderStage ShaderTypeFromString(const std::string& type)
        {
            if (type == "vertex")
                return VERTEX_SHADER;
            if (type == "fragment" || type == "pixel")
                return FRAGMENT_SHADER;
            if (type == "geometry")
                return GEOMETRY_SHADER;
            if (type == "compute")
                return COMPUTE_SHADER;
            
            GRAPHICS_CORE_ASSERT(false, "Unknown shader type!");
            return UNKNOWN;
        }
    
        static const char* ShaderStageToString(ShaderStage stage)
        {
            switch (stage)
            {
                case VERTEX_SHADER:   return "VERTEX_SHADER";
                case FRAGMENT_SHADER: return "FRAGMENT_SHADER";
                case GEOMETRY_SHADER: return "GEOMETRY_SHADER";
                case COMPUTE_SHADER: return "COMPUTE_SHADER";
            }
            GRAPHICS_CORE_ASSERT(false);
            return nullptr;
        }
    }

	class Shader
	{
    protected:
        static std::string extractNameFromPath(const std::string& filePath) {
            if(filePath.empty()) return "Unknown";
            // Extract name from filepath
            auto lastSlash = filePath.find_last_of("/\\");
            lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
            auto lastDot = filePath.rfind('.');
            auto count = lastDot == std::string::npos ? filePath.size() - lastSlash : lastDot - lastSlash;
            return filePath.substr(lastSlash, count);
        }
        
        static std::string CompileSpirVToMSL(ShaderStage stage, const std::vector<uint32_t>& shaderData);
        
        struct ShaderProgramSource
        {
            int lineOffset = -1;
            std::string Source;
        };

        using ShaderProgramSources = std::unordered_map<ShaderStage, ShaderProgramSource>;

        static std::string ReadFile(const std::string& filepath, uint32_t* num_lines = nullptr);
        static void PreProcessIncludes(ShaderProgramSources& source);
        static ShaderProgramSources PreProcess(const std::string& source);
        static void Reflect(ShaderStage stage, const std::vector<uint32_t>& shaderData);
        
        void CompileOrGetSpirVBinaries(const ShaderProgramSources& shaderSources);
        std::unordered_map<ShaderStage, std::vector<uint32_t>> m_SPIRV;
        
        ShaderProgramSources m_ShaderSources;
        
        std::string m_FilePath = "";
        std::string m_Name = "Unknown";
        
	public:
        Shader(const std::string& filepath);
        Shader() = default;
		virtual ~Shader() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void SetInt(const std::string& name, int value) = 0;
		virtual void SetIntArray(const std::string& name, int* values, uint32_t count) = 0;
		virtual void SetFloat(const std::string& name, float value) = 0;
		virtual void SetFloat2(const std::string& name, const glm::vec2& value) = 0;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) = 0;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) = 0;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;

		virtual const std::string& GetName() const = 0;
		virtual const uint32_t& GetId() const = 0;

		virtual const uint32_t& GetVertexAttributeLocation(const std::string& name) const = 0;

		static Ref<Shader> Create(const std::string& filepath, bool cache = true);
		static Ref<Shader> Create(const std::string& name, const ShaderSources& shaderSources);
#ifdef BUILDING_METAL
        static Ref<Shader> CreateFromMSL(const std::string& MSLSrc, const ShaderFunctionNames& shaderFunctionNames);
#endif
	};

	class ShaderLibrary
	{
	public:
		void Add(const std::string& name, const Ref<Shader>& shader);
		void Add(const Ref<Shader>& shader);
		Ref<Shader> Load(const std::string& filepath);
		Ref<Shader> Load(const std::string& name, const std::string& filepath);

		Ref<Shader> Get(const std::string& name);

		bool Exists(const std::string& name) const;
	private:
		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
	};

}
