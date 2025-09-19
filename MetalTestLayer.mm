//
//  MetalTestLayer.mm
//  TestGui
//
//  Created by Yejneshwar on 30/08/25.
//

#include "MetalTestLayer.h"
#include <glm/gtc/type_ptr.hpp>
#include "imgui.h"

#include "Platform/Metal/MetalContext.h"
#include <Metal/Metal.h>


// Define a struct for our vertex data that matches the shader's expectation
struct Vertex {
    glm::vec3 position;
    float pad1 = 0.0f;
    glm::vec3 color;
    float pad2 = 0.0f;
};

// Metal shader source code written in MSL (Metal Shading Language)
const char* shaderSrc = R"(
    #include <metal_stdlib>
    using namespace metal;

    // CORRECTED: Removed the [[attribute(n)]] decorators.
    // They are not used when reading from a buffer with [[vertex_id]].
    struct Vertex {
        float3 position;
        float3 color;
    };

    struct VertexOut {
        float4 position [[position]];
        half4  color;
    };

    vertex VertexOut vertex_main(const device Vertex* vertex_array [[buffer(0)]],
                                 uint vertex_id [[vertex_id]]) {
        VertexOut out;
        out.position = float4(vertex_array[vertex_id].position, 1.0);
        out.color = half4(half3(vertex_array[vertex_id].color), half(1.0f));
        return out;
    }

    fragment half4 fragment_main(VertexOut in [[stage_in]]) {
        return in.color;
    }
)";

// Texture dimensions
const uint32_t _textureWidth = 1200;
const uint32_t _textureHeight = 720;

MetalTestLayer::MetalTestLayer() {
    
}

MetalTestLayer::~MetalTestLayer() {
    
}

void MetalTestLayer::OnAttach() {
    // 1. --- DEVICE AND COMMAND QUEUE SETUP ---
    _pDevice = Graphics::MetalContext::GetCurrentDevice();
    _pCommandQueue = Graphics::MetalContext::GetCurrentCommandQueue();
    

    // 2. --- VERTEX DATA SETUP ---
    // Define the triangle's vertices (position and color)
    const Vertex vertices[] = {
        { { 0.0f,  0.75f, 0.0f}, 0.0f, {1.0f, 0.0f, 0.0f} }, // Top vertex, Red
        { {-0.75f, -0.75f, 0.0f}, 0.0f, {0.0f, 1.0f, 0.0f} }, // Left vertex, Green
        { { 0.75f, -0.75f, 0.0f}, 0.0f, {0.0f, 0.0f, 1.0f} }  // Right vertex, Blue
    };
    std::cout << "Vertex size: " << sizeof(Vertex) << std::endl;
    _pVertexBuffer = (__bridge_retained void*)[(__bridge id<MTLDevice>)_pDevice newBufferWithBytes:vertices length:sizeof(vertices) options:MTLResourceStorageModeShared];

    // 3. --- SHADER AND PIPELINE SETUP ---
    NSError *error = nil; // Objective-C style error handling
    
    // Convert C-string shader source to NSString
    NSString* shaderString = [NSString stringWithUTF8String:shaderSrc];
    id<MTLLibrary> pLibrary = [(__bridge id<MTLDevice>)_pDevice newLibraryWithSource:shaderString options:nil error:&error];
    
    if (!pLibrary) {
        std::cerr << "Failed to create library: " << [[error localizedDescription] UTF8String] << std::endl;
        return;
    }
    
    id<MTLFunction> pVertexFn = [pLibrary newFunctionWithName:@"vertex_main"];
    id<MTLFunction> pFragmentFn = [pLibrary newFunctionWithName:@"fragment_main"];
    
    // Using a native MTLRenderPipelineDescriptor object
    MTLRenderPipelineDescriptor *pPipelineDesc = [MTLRenderPipelineDescriptor new];
    pPipelineDesc.vertexFunction = pVertexFn;
    pPipelineDesc.fragmentFunction = pFragmentFn;
    pPipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    
    _pPipelineState = (__bridge_retained void*)[(__bridge id<MTLDevice>)_pDevice newRenderPipelineStateWithDescriptor:pPipelineDesc error:&error];
    if (!_pPipelineState) {
        std::cerr << "Failed to create pipeline state: " << [[error localizedDescription] UTF8String] << std::endl;
        return;
    }
    
    // 4. --- RENDER TARGET TEXTURE SETUP ---
    MTLTextureDescriptor *pTextureDesc = [MTLTextureDescriptor new];
    pTextureDesc.width = _textureWidth;
    pTextureDesc.height = _textureHeight;
    pTextureDesc.pixelFormat = MTLPixelFormatBGRA8Unorm;
    pTextureDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    pTextureDesc.storageMode = MTLStorageModeShared;
    
    _pRenderTargetTexture = (__bridge_retained void*)[(__bridge id<MTLDevice>)_pDevice newTextureWithDescriptor:pTextureDesc];
}

void MetalTestLayer::OnEvent(Application::Event &event) {

}

void MetalTestLayer::OnSelection(int objectId, bool state) {

}

void MetalTestLayer::OnImGuiRender() {
    
    ImGui::Begin("Metal Test");
    ImGui::Image((ImTextureID)_pRenderTargetTexture, ImVec2(1200, 720));
    ImGui::End();
    
}


void MetalTestLayer::OnDrawUpdate() {
    if (_pRenderTargetTexture == nil) {
        std::cout << "ERROR: _pRenderTargetTexture is nil before rendering!" << std::endl;
        return;
    }
    std::cout << "Draw Update" << std::endl;
    
    id<MTLCommandBuffer> pCommandBuffer = [(__bridge id<MTLCommandQueue>)_pCommandQueue commandBuffer];
    pCommandBuffer.label = @"texture_command_buffer";
            
    MTLRenderPassDescriptor *pRenderPassDesc = [MTLRenderPassDescriptor new];
    pRenderPassDesc.colorAttachments[0].texture = (__bridge id<MTLTexture>)_pRenderTargetTexture;
    pRenderPassDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
    pRenderPassDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
    pRenderPassDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.1, 0.1, 0.1, 1.0);
    id<MTLRenderCommandEncoder> pEncoder = [pCommandBuffer renderCommandEncoderWithDescriptor:pRenderPassDesc];
    
    [pEncoder setCullMode:MTLCullModeNone];
    
    // Encoding commands with message passing
    [pEncoder setRenderPipelineState:(__bridge id<MTLRenderPipelineState>)_pPipelineState];
    [pEncoder setVertexBuffer:(__bridge id<MTLBuffer>)_pVertexBuffer offset:0 atIndex:0];
    [pEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    [pEncoder endEncoding];
    
    [pCommandBuffer commit];
    [pCommandBuffer waitUntilCompleted];
}


void MetalTestLayer::OnUpdateLayer() {
    std::cout << "Layer Update";
}


void MetalTestLayer::OnDetach() {
}


