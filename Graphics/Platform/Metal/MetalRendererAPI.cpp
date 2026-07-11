#include "GraphicsCore.h"
#include "Platform/Metal/MetalRendererAPI.h"

#include <Foundation/Foundation.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <Metal/Metal.hpp>

#include "MetalContext.h"

#include "MetalVertexArray.h"
#include "MetalBuffer.h"

#include "Logger.h"

#include <map>
#include <stdexcept>

#include <dispatch/dispatch.h>

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

    // ---------------------------------------------------------------------
    // Pipeline-state cache.
    // newRenderPipelineState() is one of the most expensive Metal calls; it
    // used to run on EVERY draw call, every frame. Pipeline states are fully
    // determined here by (shader pipeline descriptor, vertex layout), both of
    // which are stable objects that live for the whole run, so cache by
    // pointer identity. Shader recreation allocates new descriptors (old ones
    // are never freed), so stale keys can never alias a new descriptor.
    // ---------------------------------------------------------------------
    static std::map<std::pair<const void*, const void*>, MTL::RenderPipelineState*> s_PipelineStateCache;

    static MTL::RenderPipelineState* GetOrCreatePipelineState(MTL::RenderPipelineDescriptor* descriptor,
                                                              MTL::VertexDescriptor* vertexDescriptor)
    {
        const auto key = std::make_pair((const void*)descriptor, (const void*)vertexDescriptor);
        auto it = s_PipelineStateCache.find(key);
        if (it != s_PipelineStateCache.end())
            return it->second;

        if (vertexDescriptor)
            descriptor->setVertexDescriptor(vertexDescriptor);

        NS::Error* error = nullptr;
        MTL::RenderPipelineState* pipelineState = MetalContext::GetCurrentDevice()->newRenderPipelineState(descriptor, &error);
        if (!pipelineState) {
            std::cerr << "Failed to create pipeline state: " << error->localizedDescription()->utf8String() << std::endl;
            throw std::runtime_error("Failed to create Metal pipeline state");
        }

        s_PipelineStateCache[key] = pipelineState;
        return pipelineState;
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
        // Depth/stencil states are immutable; build the two variants once.
        static MTL::DepthStencilState* s_DepthEnabledState = nullptr;
        static MTL::DepthStencilState* s_DepthDisabledState = nullptr;

        if (!s_DepthEnabledState) {
            MTL::DepthStencilDescriptor* desc = MTL::DepthStencilDescriptor::alloc()->init();

            desc->setDepthCompareFunction(MTL::CompareFunctionLess);
            desc->setDepthWriteEnabled(true);
            s_DepthEnabledState = MetalContext::GetCurrentDevice()->newDepthStencilState(desc);

            desc->setDepthCompareFunction(MTL::CompareFunctionAlways);
            desc->setDepthWriteEnabled(false);
            s_DepthDisabledState = MetalContext::GetCurrentDevice()->newDepthStencilState(desc);

            desc->release();
        }

        MetalContext::GetCurrentRenderCommandEncoder()->setDepthStencilState(enable ? s_DepthEnabledState : s_DepthDisabledState);
	}

	void MetalRendererAPI::PolygonSmooth(bool enable) {

	}

	void MetalRendererAPI::ClearBuffers(){
		
	}

    // Allows the CPU to encode up to kMaxFramesInFlight frames while the GPU
    // works; per-frame dynamic buffers are ring-buffered against the frame
    // index so nothing in flight is overwritten.
    static dispatch_semaphore_t s_FrameSemaphore = nullptr;

    void MetalRendererAPI::BeginLoop(){
        if (!s_FrameSemaphore)
            s_FrameSemaphore = dispatch_semaphore_create(MetalContext::kMaxFramesInFlight);
        dispatch_semaphore_wait(s_FrameSemaphore, DISPATCH_TIME_FOREVER);
        MetalContext::AdvanceFrameInFlight();

        MTL::CommandBuffer* CommadBuffer = Graphics::MetalContext::GetCurrentCommandQueue()->commandBuffer();
        CommadBuffer->setLabel(NS::String::string("Application Command Buffer", NS::UTF8StringEncoding));

        Graphics::MetalContext::SetCommandBuffer(CommadBuffer);
    }

    void MetalRendererAPI::EndLoop(){
        MTL::CommandBuffer* CommadBuffer = Graphics::MetalContext::GetCurrentCommandBuffer();
        // No waitUntilCompleted here: the semaphore in BeginLoop provides
        // back-pressure, and the GPU runs ahead-of/behind the CPU freely.
        CommadBuffer->addCompletedHandler([](MTL::CommandBuffer*) {
            dispatch_semaphore_signal(s_FrameSemaphore);
        });
        CommadBuffer->commit();
        Graphics::MetalContext::SetLastCommittedCommandBuffer(CommadBuffer);
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
        MetalVertexArray* metalArray = dynamic_cast<MetalVertexArray*>(vertexArray.get());
        MTL::RenderPipelineState* PipelineState = GetOrCreatePipelineState(
            MetalContext::GetCurrentPipelineStateDecsriptor(), metalArray->GetVertexDescriptor());

        metalArray->Bind();

        MTL::RenderCommandEncoder* pEncoder = MetalContext::GetCurrentRenderCommandEncoder();

        pEncoder->setRenderPipelineState(PipelineState);
        pEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle, start, count);
	}

	void MetalRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
	{
        MetalVertexArray* metalArray = dynamic_cast<MetalVertexArray*>(vertexArray.get());
        MetalIndexBuffer* metalIndexBuffer = dynamic_cast<MetalIndexBuffer*>(metalArray->GetIndexBuffer().get());
        MTL::Buffer* indexBuffer = metalIndexBuffer->GetBuffer();

        if (indexCount <= 0) indexCount = metalIndexBuffer->GetCount();

        MTL::RenderPipelineState* PipelineState = GetOrCreatePipelineState(
            MetalContext::GetCurrentPipelineStateDecsriptor(), metalArray->GetVertexDescriptor());

        metalArray->Bind();

        MTL::RenderCommandEncoder* pEncoder = MetalContext::GetCurrentRenderCommandEncoder();

        pEncoder->setRenderPipelineState(PipelineState);
        pEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, indexCount, MTL::IndexTypeUInt32, indexBuffer, 0);
	}

	void MetalRendererAPI::DrawIndexedRange(const Ref<VertexArray>& vertexArray, uint32_t indexCount, uint32_t indexByteOffset)
	{
        MetalVertexArray* metalArray = dynamic_cast<MetalVertexArray*>(vertexArray.get());
        MetalIndexBuffer* metalIndexBuffer = dynamic_cast<MetalIndexBuffer*>(metalArray->GetIndexBuffer().get());
        MTL::Buffer* indexBuffer = metalIndexBuffer->GetBuffer();

        MTL::RenderPipelineState* PipelineState = GetOrCreatePipelineState(
            MetalContext::GetCurrentPipelineStateDecsriptor(), metalArray->GetVertexDescriptor());

        metalArray->Bind();

        MTL::RenderCommandEncoder* pEncoder = MetalContext::GetCurrentRenderCommandEncoder();

        pEncoder->setRenderPipelineState(PipelineState);
        pEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, indexCount, MTL::IndexTypeUInt32, indexBuffer, indexByteOffset);
	}

	void MetalRendererAPI::DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
	{
        MetalVertexArray* metalArray = dynamic_cast<MetalVertexArray*>(vertexArray.get());
        MTL::RenderPipelineState* PipelineState = GetOrCreatePipelineState(
            MetalContext::GetCurrentPipelineStateDecsriptor(), metalArray->GetVertexDescriptor());

        metalArray->Bind();

        MTL::RenderCommandEncoder* pEncoder = MetalContext::GetCurrentRenderCommandEncoder();

        pEncoder->setRenderPipelineState(PipelineState);
        pEncoder->drawPrimitives(MTL::PrimitiveTypeLine, (uint32_t)0, vertexCount);
	}

	void MetalRendererAPI::DrawLinesIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
	{
        MetalVertexArray* metalArray = dynamic_cast<MetalVertexArray*>(vertexArray.get());
        MetalIndexBuffer* metalIndexBuffer = dynamic_cast<MetalIndexBuffer*>(metalArray->GetIndexBuffer().get());
        MTL::Buffer* indexBuffer = metalIndexBuffer->GetBuffer();

        if (indexCount <= 0) indexCount = metalIndexBuffer->GetCount();

        MTL::RenderPipelineState* PipelineState = GetOrCreatePipelineState(
            MetalContext::GetCurrentPipelineStateDecsriptor(), metalArray->GetVertexDescriptor());

        metalArray->Bind();

        MTL::RenderCommandEncoder* pEncoder = MetalContext::GetCurrentRenderCommandEncoder();

        pEncoder->setRenderPipelineState(PipelineState);
        pEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeLine, indexCount, MTL::IndexTypeUInt32, indexBuffer, 0);
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
        MTL::RenderPipelineDescriptor* pipelineDescriptor = MetalContext::GetCurrentPipelineStateDecsriptor();
        MTL::RenderPassDescriptor* renderPassDescriptor = MetalContext::GetCurrentRenderPassDescriptor();

        // Match the pipeline's attachment formats to the bound render pass.
        // These are stable per framebuffer, so the resulting pipeline state is
        // served from the cache after the first draw.
        MTL::RenderPassColorAttachmentDescriptorArray* colorAttachments = renderPassDescriptor->colorAttachments();
        for (int i = 0; i < 8; ++i)
        {
            MTL::RenderPassColorAttachmentDescriptor* attachment = colorAttachments->object(i);
            pipelineDescriptor->colorAttachments()->object(i)->setPixelFormat(attachment->texture()->pixelFormat());
        }
        pipelineDescriptor->setDepthAttachmentPixelFormat(renderPassDescriptor->depthAttachment()->texture()->pixelFormat());
        pipelineDescriptor->setStencilAttachmentPixelFormat(renderPassDescriptor->stencilAttachment()->texture()->pixelFormat());

        MTL::RenderPipelineState* PipelineState = GetOrCreatePipelineState(pipelineDescriptor, nullptr);

        MTL::RenderCommandEncoder* pEncoder = MetalContext::GetCurrentRenderCommandEncoder();

        pEncoder->setRenderPipelineState(PipelineState);
        pEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle, 0, 6, 1, 0);
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
