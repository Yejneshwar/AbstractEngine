#include "GraphicsCore.h"
#include "Platform/Metal/MetalShader.h"
#include "Platform/Metal/MetalContext.h"

#include <fstream>
#include <Logger.h>

#include "Foundation/Foundation.hpp"
#include "Metal/Metal.hpp"

#include "IOS/FileUtils.h"

namespace Graphics {
    MetalShader::MetalShader(const std::string& filepath, bool cache)
        : Shader(GUI::Utils::getResourcePath(filepath)) {
            this->CompileOrGetSpirVBinaries(m_ShaderSources);
            
            for(auto&& [stage, spirVSource] : m_SPIRV){
                auto msl_source = Shader::CompileSpirVToMSL(stage, spirVSource);
                // Output MSL
                // std::cout << "Generated MSL Shader: " << m_Name << "\n" << msl_source << std::endl;
                CompileShader(stage, msl_source);
            }
            
            MTL::RenderPipelineDescriptor* newPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
            
            if (!m_Name.empty())
            {
                std::string label = m_Name + "_pipeline";
                newPipelineDescriptor->setLabel(NS::String::string(label.c_str(), NS::UTF8StringEncoding));
            }
            
            newPipelineDescriptor->setVertexFunction(m_VertexFunction);
            newPipelineDescriptor->setFragmentFunction(m_FragmentFunction);
            newPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
            newPipelineDescriptor->colorAttachments()->object(1)->setPixelFormat(MTL::PixelFormatR32Sint);
            newPipelineDescriptor->colorAttachments()->object(2)->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
            newPipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
            newPipelineDescriptor->setStencilAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
            
            // Get the descriptor for the first color attachment.
            MTL::RenderPipelineColorAttachmentDescriptor* colorAttachment = newPipelineDescriptor->colorAttachments()->object(0);

            // Enable blending.
            colorAttachment->setBlendingEnabled(true);

            // Set the blend operation to Add.
            colorAttachment->setRgbBlendOperation(MTL::BlendOperationAdd);
            colorAttachment->setAlphaBlendOperation(MTL::BlendOperationAdd);

            // Configure the blend factors for standard transparency.
            colorAttachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
            colorAttachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
            colorAttachment->setSourceAlphaBlendFactor(MTL::BlendFactorSourceAlpha);
            colorAttachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
            
            m_PipelineDescriptor = newPipelineDescriptor;
            
            NS::Error* pError = nullptr;
            m_PipelineState = MetalContext::GetCurrentDevice()->newRenderPipelineState(newPipelineDescriptor, &pError);
            if (!m_PipelineState) {
                std::cerr << "Failed to create pipeline state: "
                          << pError->localizedDescription()->utf8String() << std::endl;
            }
            
    }

    MetalShader::MetalShader(const std::string& name, const ShaderSources& shaderSources)
        : Shader(""), m_PipelineDescriptor(MTL::RenderPipelineDescriptor::alloc()->init()){
        GRAPHICS_CORE_ASSERT(false, "DONT USE YET!! WHAT TO DO WITH m_FileName?")
        m_Name = name;
        // In Metal, vertex and fragment shaders are often in the same file.
        // We'll concatenate them for this implementation.

        //CompileShader(source);
    }

    MetalShader::MetalShader(const std::string& MSLSrc, const ShaderFunctionNames& shaderFunctionNames)
    : Shader(""), m_PipelineDescriptor(MTL::RenderPipelineDescriptor::alloc()->init()) {
        m_Name = "Unknown";
        
        for(auto functionName : shaderFunctionNames) {
            this->CompileShader(functionName.first, MSLSrc, functionName.second);
        }
    }

    MetalShader::~MetalShader() {
    }

    bool MetalShader::CompileMSLLibrary(const std::string& MSLSrc) {
        MTL::Device* pDevice = MetalContext::GetCurrentDevice();
        NS::Error* pError = nullptr;
        
        
        // Convert the C++ source to NSString
        NS::String* sourceString = NS::String::string(MSLSrc.c_str(), NS::UTF8StringEncoding);
        
        // Set up Metal compile options
        MTL::CompileOptions* pOptions = MTL::CompileOptions::alloc()->init();
        pOptions->setLanguageVersion(MTL::LanguageVersion2_0); // Adjust version as needed
        
        // Compile the shader source
        m_Library = pDevice->newLibrary(sourceString, pOptions, &pError);
        
        pOptions->release();
        
        if (!m_Library)
        {
            std::cerr << "Shader compilation failed : "
                    << pError->localizedDescription()->utf8String() << std::endl;
            return false;
        }
        return true;
    }

    void MetalShader::CompileShader(const std::string& MSLSrc, const std::string& vertexFunctionName, const std::string& fragmentFunctionName) {
        bool compileResult = this->CompileMSLLibrary(MSLSrc);
        if(!compileResult) {
            std::cerr << "Unified Shader compilation failed " << std::endl;
            
            std::cout << "\n\nSource\n\n" << MSLSrc << std::endl;
            GRAPHICS_CORE_ASSERT(false);
            return;
        }
        
        MTL::Function* vertexFunction = m_Library->newFunction(NS::String::string(vertexFunctionName.c_str(), NS::UTF8StringEncoding));
        MTL::Function* fragmentFunction = m_Library->newFunction(NS::String::string(fragmentFunctionName.c_str(), NS::UTF8StringEncoding));
        if (!vertexFunction || !fragmentFunction) {
            std::cerr << "Failed to find function '" << (vertexFunction ? fragmentFunctionName : vertexFunctionName) << std::endl;
            GRAPHICS_CORE_ASSERT(false);
            return;
        }
        
        m_PipelineDescriptor->setVertexFunction(vertexFunction);
        m_PipelineDescriptor->setFragmentFunction(fragmentFunction);
        
        m_VertexFunction = vertexFunction;
        m_FragmentFunction = fragmentFunction;
        
        m_Library->release();
        
    }

    void MetalShader::CompileShader(ShaderStage stage, const std::string& source, const std::string& functionName) {
            bool compileResult = this->CompileMSLLibrary(source);
            if(!compileResult) {
                std::cerr << "Shader compilation failed for stage " << static_cast<int>(stage)
                        << std::endl;
                
                std::cout << "\n\nSource\n\n" << source << std::endl;
                GRAPHICS_CORE_ASSERT(false);
                return;
            }
        
        // Choose the entry point based on the stage
        NS::String* NSfunctionName = nullptr;
        switch (stage)
        {
            case ShaderStage::VERTEX_SHADER:
                NSfunctionName = NS::String::string(functionName.c_str(), NS::UTF8StringEncoding);
                break;
            case ShaderStage::FRAGMENT_SHADER:
                NSfunctionName = NS::String::string(functionName.c_str(), NS::UTF8StringEncoding);
                break;
            default:
                std::cerr << "Unsupported shader stage passed to CompileShader." << std::endl;
                GRAPHICS_CORE_ASSERT(false);
                return;
        }
        
        MTL::Function* function = m_Library->newFunction(NSfunctionName);
        if (!function) {
            std::cerr << "Failed to find function '" << NSfunctionName->utf8String()
                    << "' in compiled library for stage " << static_cast<int>(stage) << std::endl;
            GRAPHICS_CORE_ASSERT(false);
            return;
        }
        
        if (stage == ShaderStage::VERTEX_SHADER){
            m_PipelineDescriptor->setVertexFunction(function);
            m_VertexFunction = function;
        }
        else if (stage == ShaderStage::FRAGMENT_SHADER)
        {
            m_PipelineDescriptor->setFragmentFunction(function);
            m_FragmentFunction = function;
        }
        
        m_Library->release();
    }

    void MetalShader::Bind() const {
//        if(m_PipelineState == nullptr) {
//            id<MTLDevice> device = (__bridge id<MTLDevice>)MetalContext::GetCurrentDevice();
//            NSError *error = nil;
//            id<MTLRenderPipelineState> PipelineState = [device newRenderPipelineStateWithDescriptor:(__bridge MTLRenderPipelineDescriptor*)m_PipelineDescriptor error:&error];
//            if (!PipelineState) {
//                std::cerr << "Failed to create pipeline state: " << [[error localizedDescription] UTF8String] << std::endl;
//                throw;
//            }
//        };
//        
//        id<MTLRenderCommandEncoder> pEncoder = (__bridge id<MTLRenderCommandEncoder>)MetalContext::GetCurrentRenderCommandEncoder();
//        [pEncoder setRenderPipelineState:(__bridge id<MTLRenderPipelineState>)m_PipelineState];
        
        MetalContext::SetPipelineStateDecsriptor(m_PipelineDescriptor);
        
        // Binding is handled by setting the pipeline state which contains the shaders
    }

    void MetalShader::Unbind() const {
        // Unbinding is handled by the render pipeline
    }

    void MetalShader::SetInt(const std::string& name, int value) {
        // Implementation will require a command encoder
    }

    void MetalShader::SetIntArray(const std::string& name, int* values, uint32_t count) {
        // Implementation will require a command encoder
    }

    void MetalShader::SetFloat(const std::string& name, float value) {
        // Implementation will require a command encoder
    }

    void MetalShader::SetFloat2(const std::string& name, const glm::vec2& value) {
        // Implementation will require a command encoder
    }

    void MetalShader::SetFloat3(const std::string& name, const glm::vec3& value) {
        // Implementation will require a command encoder
    }

    void MetalShader::SetFloat4(const std::string& name, const glm::vec4& value) {
        // Implementation will require a command encoder
    }

    void MetalShader::SetMat4(const std::string& name, const glm::mat4& value) {
        // Implementation will require a command encoder
    }
    
    const uint32_t& MetalShader::GetId() const {
        static uint32_t placeholder = 0;
        return placeholder; // Metal does not use IDs in the same way as OpenGL
    }
}
