#include "GraphicsCore.h"
#include "Platform/Metal/MetalRendererAPI.h"

#include <Foundation/Foundation.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <Metal/Metal.hpp>

#include "MetalContext.h"

#include "MetalVertexArray.h"
#include "MetalBuffer.h"

#include "Logger.h"

namespace Graphics {
	
	void MetalMessageCallback(
		unsigned source,
		unsigned type,
		unsigned id,
		unsigned severity,
		int length,
		const char* message,
		const void* userParam)
	{
		//switch (severity)
		//{
		//	case GL_DEBUG_SEVERITY_HIGH:         HZ_CORE_CRITICAL(message); return;
		//	case GL_DEBUG_SEVERITY_MEDIUM:       HZ_CORE_ERROR(message); return;
		//	case GL_DEBUG_SEVERITY_LOW:          HZ_CORE_WARN(message); return;
		//	case GL_DEBUG_SEVERITY_NOTIFICATION: HZ_CORE_TRACE(message); return;
		//}
		//
		//GRAPHICS_CORE_ASSERT(false, "Unknown severity level!");
	}

	void MetalRendererAPI::Init()
	{
		
        std::cout << "METAL API INIT" << std::endl;


	}

	void MetalRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		
	}

	void MetalRendererAPI::SetClearColor(const glm::vec4& color)
	{
		
	}

	void MetalRendererAPI::Clear(float alpha)
	{

	}

	void MetalRendererAPI::ClearStencil()
	{

	}

	void MetalRendererAPI::DepthTest(bool enable) {
        // Create a descriptor to configure the depth and stencil state
        MTL::DepthStencilDescriptor* depthStencilDesc = MTL::DepthStencilDescriptor::alloc()->init();

        if (enable) {
            // Enable depth testing: Fragments pass if they are closer to the camera.
            depthStencilDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
            // Allow writing new, closer fragment depth values to the depth buffer.
            depthStencilDesc->setDepthWriteEnabled(true);
        } else {
            // Disable depth testing: All fragments pass the test.
            depthStencilDesc->setDepthCompareFunction(MTL::CompareFunctionAlways);
            // Prevent writing to the depth buffer.
            depthStencilDesc->setDepthWriteEnabled(false);
        }

        MTL::DepthStencilState* depthStencilState = MetalContext::GetCurrentDevice()->newDepthStencilState(depthStencilDesc);
        // Set the state on the command encoder for subsequent draw calls
        MetalContext::GetCurrentRenderCommandEncoder()->setDepthStencilState(depthStencilState);
        
        depthStencilDesc->release();
        depthStencilState->release();
	}

	void MetalRendererAPI::PolygonSmooth(bool enable) {

	}

	void MetalRendererAPI::ClearBuffers(){
		
	}

    void MetalRendererAPI::BeginLoop(){
        MTL::CommandBuffer* CommadBuffer = Graphics::MetalContext::GetCurrentCommandQueue()->commandBuffer();
        CommadBuffer->setLabel(NS::String::string("Application Command Buffer", NS::UTF8StringEncoding));
        
        Graphics::MetalContext::SetCommandBuffer(CommadBuffer);
    }

    void MetalRendererAPI::EndLoop(){
        MTL::CommandBuffer* CommadBuffer = Graphics::MetalContext::GetCurrentCommandBuffer();
        CommadBuffer->commit();
        CommadBuffer->waitUntilCompleted();
        Graphics::MetalContext::SetCommandBuffer(nullptr);
    }
    

	void MetalRendererAPI::EnableStencil() {

	}

	void MetalRendererAPI::DisableStencil() {

	}



	void MetalRendererAPI::SetStencilFunc(unsigned int func ,bool ref,uint8_t mask) {
		
	}

	void MetalRendererAPI::SetStencilOp(unsigned int sfail, unsigned int dpfail, unsigned int dppass) {
		
	}

	void MetalRendererAPI::DrawNonIndexed(const Ref<VertexArray>& vertexArray, uint32_t count, uint32_t start)
	{
        MTL::RenderPipelineDescriptor* currentDescriptor = MetalContext::GetCurrentPipelineStateDecsriptor();
        MetalVertexArray* metalArray = dynamic_cast<MetalVertexArray*>(vertexArray.get());
        
        currentDescriptor->setVertexDescriptor(metalArray->GetVertexDescriptor());
        
        MTL::Device* device = MetalContext::GetCurrentDevice();
        NS::Error* error = nullptr;
        MTL::RenderPipelineState* PipelineState = device->newRenderPipelineState(currentDescriptor, &error);
        if (!PipelineState) {
            std::cerr << "Failed to create pipeline state: " << error->localizedDescription() << std::endl;
            throw;
        }
        
        metalArray->Bind();
        
        MTL::RenderCommandEncoder* pEncoder = MetalContext::GetCurrentRenderCommandEncoder();
        
        pEncoder->setRenderPipelineState(PipelineState);
        pEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle, start, count);
        
        PipelineState->release();

	}

	void MetalRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
	{
        MTL::RenderPipelineDescriptor* currentDescriptor = MetalContext::GetCurrentPipelineStateDecsriptor();
        MetalVertexArray* metalArray = dynamic_cast<MetalVertexArray*>(vertexArray.get());
        
        currentDescriptor->setVertexDescriptor(metalArray->GetVertexDescriptor());
        
        MetalIndexBuffer* metalIndexBuffer = dynamic_cast<MetalIndexBuffer*>(metalArray->GetIndexBuffer().get());
        MTL::Buffer* indexBuffer = metalIndexBuffer->GetBuffer();
        
        if (indexCount <= 0) indexCount = metalIndexBuffer->GetCount();
        
        MTL::Device* device = MetalContext::GetCurrentDevice();
        NS::Error* error = nullptr;
        MTL::RenderPipelineState* PipelineState = device->newRenderPipelineState(currentDescriptor, &error);
        if (!PipelineState) {
            std::cerr << "Failed to create pipeline state: " << error->localizedDescription() << std::endl;
            throw;
        }
        
        metalArray->Bind();
        
        MTL::RenderCommandEncoder* pEncoder = MetalContext::GetCurrentRenderCommandEncoder();
        
        pEncoder->setRenderPipelineState(PipelineState);
        pEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, indexCount, MTL::IndexTypeUInt32, indexBuffer, 0);
        
        PipelineState->release();
	}

	void MetalRendererAPI::DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
	{
        MTL::RenderPipelineDescriptor* currentDescriptor = MetalContext::GetCurrentPipelineStateDecsriptor();
        MetalVertexArray* metalArray = dynamic_cast<MetalVertexArray*>(vertexArray.get());
        
        currentDescriptor->setVertexDescriptor(metalArray->GetVertexDescriptor());
        
        MTL::Device* device = MetalContext::GetCurrentDevice();
        NS::Error* error = nullptr;
        MTL::RenderPipelineState* PipelineState = device->newRenderPipelineState(currentDescriptor, &error);
        if (!PipelineState) {
            std::cerr << "Failed to create pipeline state: " << error->localizedDescription() << std::endl;
            throw;
        }
        
        metalArray->Bind();
        
        MTL::RenderCommandEncoder* pEncoder = MetalContext::GetCurrentRenderCommandEncoder();
        
        pEncoder->setRenderPipelineState(PipelineState);
        pEncoder->drawPrimitives(MTL::PrimitiveTypeLine, (uint32_t)0, vertexCount);
        
        PipelineState->release();
	}

	void MetalRendererAPI::DrawLinesIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
	{
        MTL::RenderPipelineDescriptor* currentDescriptor = MetalContext::GetCurrentPipelineStateDecsriptor();
        MetalVertexArray* metalArray = dynamic_cast<MetalVertexArray*>(vertexArray.get());
        
        currentDescriptor->setVertexDescriptor(metalArray->GetVertexDescriptor());
        
        MetalIndexBuffer* metalIndexBuffer = dynamic_cast<MetalIndexBuffer*>(metalArray->GetIndexBuffer().get());
        MTL::Buffer* indexBuffer = metalIndexBuffer->GetBuffer();
        
        if (indexCount <= 0) indexCount = metalIndexBuffer->GetCount();
        
        MTL::Device* device = MetalContext::GetCurrentDevice();
        NS::Error* error = nullptr;
        MTL::RenderPipelineState* PipelineState = device->newRenderPipelineState(currentDescriptor, &error);
        if (!PipelineState) {
            std::cerr << "Failed to create pipeline state: " << error->localizedDescription() << std::endl;
            throw;
        }
        
        metalArray->Bind();
        
        MTL::RenderCommandEncoder* pEncoder = MetalContext::GetCurrentRenderCommandEncoder();
        
        pEncoder->setRenderPipelineState(PipelineState);
        pEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeLine, indexCount, MTL::IndexTypeUInt32, indexBuffer, 0);
        
        PipelineState->release();
	}

	void MetalRendererAPI::DrawWireFrameCube(const std::vector<glm::dvec3>& cube, const float& thickness) {
//		glLineWidth(thickness);
//		glColor3f(1.0,1.0,1.0);
//		glBegin(GL_LINES);
//		glVertex3d(0, 0, 0);
//		glVertex3d(-0.3, 0.5, 0.5);
//		std::cout << cube.at(0).x << std::endl;
//		glVertex3d(cube.at(0).x, cube.at(0).y, cube.at(0).z); glVertex3d(cube.at(1).x, cube.at(1).y, cube.at(1).z);
//		glVertex3d(cube.at(0).x, cube.at(0).y, cube.at(0).z); glVertex3d(cube.at(3).x, cube.at(3).y, cube.at(3).z);
//		glVertex3d(cube.at(0).x, cube.at(0).y, cube.at(0).z); glVertex3d(cube.at(4).x, cube.at(4).y, cube.at(4).z);
//		glVertex3d(cube.at(1).x, cube.at(1).y, cube.at(1).z); glVertex3d(cube.at(2).x, cube.at(2).y, cube.at(2).z);
//		glVertex3d(cube.at(1).x, cube.at(1).y, cube.at(1).z); glVertex3d(cube.at(5).x, cube.at(5).y, cube.at(5).z);
//		glVertex3d(cube.at(2).x, cube.at(2).y, cube.at(2).z); glVertex3d(cube.at(3).x, cube.at(3).y, cube.at(3).z);
//		glVertex3d(cube.at(2).x, cube.at(2).y, cube.at(2).z); glVertex3d(cube.at(6).x, cube.at(6).y, cube.at(6).z);
//		glVertex3d(cube.at(3).x, cube.at(3).y, cube.at(3).z); glVertex3d(cube.at(7).x, cube.at(7).y, cube.at(7).z);
//		glVertex3d(cube.at(4).x, cube.at(4).y, cube.at(4).z); glVertex3d(cube.at(5).x, cube.at(5).y, cube.at(5).z);
//		glVertex3d(cube.at(4).x, cube.at(4).y, cube.at(4).z); glVertex3d(cube.at(7).x, cube.at(7).y, cube.at(7).z);
//		glVertex3d(cube.at(5).x, cube.at(5).y, cube.at(5).z); glVertex3d(cube.at(6).x, cube.at(6).y, cube.at(6).z);
//		glVertex3d(cube.at(6).x, cube.at(6).y, cube.at(6).z); glVertex3d(cube.at(7).x, cube.at(7).y, cube.at(7).z);
//		glEnd();
	}

	void MetalRendererAPI::DrawGridTriangles(){
        MTL::Device* device = MetalContext::GetCurrentDevice();
        MTL::RenderPipelineDescriptor* pipelineDescriptor = MetalContext::GetCurrentPipelineStateDecsriptor();
        MTL::RenderPassDescriptor* renderPassDescriptor = MetalContext::GetCurrentRenderPassDescriptor();
        
        // Get the array of color attachment descriptors
        MTL::RenderPassColorAttachmentDescriptorArray* colorAttachments = renderPassDescriptor->colorAttachments();

        // Loop through the attachments. You typically know how many you are using.
        // Let's assume a maximum or a known count, e.g., 8.
        for (int i = 0; i < 8; ++i)
        {
            
            // Get the descriptor for the current attachment index
            MTL::RenderPassColorAttachmentDescriptor* attachment = colorAttachments->object(i);

            // An attachment is considered active if it has a texture assigned.
            pipelineDescriptor->colorAttachments()->object(i)->setPixelFormat(attachment->texture()->pixelFormat());
            
            
        }
        pipelineDescriptor->setDepthAttachmentPixelFormat(renderPassDescriptor->depthAttachment()->texture()->pixelFormat());
        pipelineDescriptor->setStencilAttachmentPixelFormat(renderPassDescriptor->stencilAttachment()->texture()->pixelFormat());
        
        NS::Error *error = nullptr;
        MTL::RenderPipelineState* PipelineState = device->newRenderPipelineState(pipelineDescriptor, &error);
        if (!PipelineState) {
            std::cerr << "Failed to create pipeline state: " << error->localizedDescription() << std::endl;
            throw;
        }
        
        MTL::RenderCommandEncoder* pEncoder = MetalContext::GetCurrentRenderCommandEncoder();

        pEncoder->setRenderPipelineState(PipelineState);
        pEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle, 0, 6, 1, 0);
        
        PipelineState->release();
	}

	void MetalRendererAPI::SetLineWidth(float width)
	{

	}

	void MetalRendererAPI::SetRendererMode(int mode)
	{

	}

	void MetalRendererAPI::SetRendererModeToDefault()
	{
        
    }

    void MetalRendererAPI::DrawLinesInstancedBaseInstance(const Ref<VertexArray> &vertexArray, uint32_t filrst, uint32_t vertexCount, uint32_t instanceCount, uint32_t baseInstance) {
    }


}
