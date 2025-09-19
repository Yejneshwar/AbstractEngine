#include "MetalComputeShader.h"

#include "MetalContext.h"

#include <iostream>
#include <cassert>
#include <string>

#include "Logger.h"
#include "IOS/FileUtils.h"

namespace Graphics {
    MetalComputeShader::MetalComputeShader(const std::filesystem::path& filepath) : Shader(GUI::Utils::getResourcePath(filepath)) {
        this->CompileOrGetSpirVBinaries(m_ShaderSources);
        
        auto&& spirVSource = m_SPIRV[ShaderStage::COMPUTE_SHADER];
        auto msl_source = Shader::CompileSpirVToMSL(ShaderStage::COMPUTE_SHADER, spirVSource);
        this->CreateComputeShader(msl_source, "main0");
    }

    MetalComputeShader::MetalComputeShader(const std::string& Src, const std::string& functionName, bool isMSL) :
        m_ComputeCommandEncoder(nullptr) {
            assert(isMSL && "Not implemented!");
            this->CreateComputeShader(Src, functionName);
    }
    
    void MetalComputeShader::CreateComputeShader(const std::string& MSLSrc, const std::string& functionName) {
        MTL::Device* pDevice = MetalContext::GetCurrentDevice();
        NS::Error* pError = nullptr;
        
        
        // Convert the C++ source to NSString
        NS::String* sourceString = NS::String::string(MSLSrc.c_str(), NS::UTF8StringEncoding);
        
        // Set up Metal compile options
        MTL::CompileOptions* pOptions = MTL::CompileOptions::alloc()->init();
        pOptions->setLanguageVersion(MTL::LanguageVersion2_0); // Adjust version as needed
        
        // Compile the shader source
        MTL::Library* pLibrary = pDevice->newLibrary(sourceString, pOptions, &pError);
        
        pOptions->release();
        
        if (!pLibrary)
        {
            std::cerr << "Shader compilation failed : "
            << pError->localizedDescription()->utf8String() << std::endl;
            return false;
        }
        // Retain the compiled library (you can store one per stage if needed)
        m_Library = pLibrary;
        
        m_ComputeFunction = m_Library->newFunction(NS::String::string(functionName.c_str(), NS::UTF8StringEncoding));
        
        if (!m_ComputeFunction) {
            std::cerr << "Failed to find function '"
            << functionName
            << std::endl;
            GRAPHICS_CORE_ASSERT(false);
            return;
        }
        
        m_ComputePipelineState = MetalContext::GetCurrentDevice()->newComputePipelineState(m_ComputeFunction, &pError);
        if (!m_ComputePipelineState) {
            std::cerr << "Failed to create pipeline state: "
            << pError->localizedDescription()->utf8String() << std::endl;
        }
        
    }

    void MetalComputeShader::Bind() {
        m_ComputeCommandEncoder = MetalContext::GetCurrentCommandBuffer()->computeCommandEncoder();
        m_ComputeCommandEncoder->setComputePipelineState(m_ComputePipelineState);
        MetalContext::SetComputeCommandEncoder(m_ComputeCommandEncoder);
    }

    void MetalComputeShader::Unbind() {
        m_ComputeCommandEncoder->endEncoding();
        m_ComputeCommandEncoder = nullptr;
        MetalContext::SetComputeCommandEncoder(nullptr);
    }

    void MetalComputeShader::BindTexture(uintptr_t texture, int slot) {
        m_ComputeCommandEncoder->setTexture((MTL::Texture*)texture, slot);
    }


    void MetalComputeShader::SetInt(int* ptr, int slot) {
        m_ComputeCommandEncoder->setBytes(ptr, sizeof(int), slot);
    }

    void MetalComputeShader::Dispatch(uint32_t width, uint32_t height, uint32_t depth) {
        MTL::Size threadsPerGrid = MTL::Size(width, height, depth);
        
        // Define the threadgroup size. This can be optimized.
        MTL::Size threadgroupSize = MTL::Size(16, 16, 1);

        // 1. Calculate the number of threadgroups needed to cover the entire grid.
        MTL::Size threadgroupCount = MTL::Size(
            (width + threadgroupSize.width - 1) / threadgroupSize.width,
            (height + threadgroupSize.height - 1) / threadgroupSize.height,
            depth);
        
        m_ComputeCommandEncoder->dispatchThreadgroups(threadgroupCount, threadgroupSize);
    }


}
