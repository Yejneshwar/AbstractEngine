#pragma once

#include "Renderer/Buffer.h"
#include <Metal/Metal.hpp>

#ifdef __OBJC__
@protocol MTLBuffer;
#endif

namespace Graphics {

	// Dynamic buffers (created without initial data) keep one copy per
	// in-flight frame so the CPU can write while the GPU reads earlier
	// frames. Buffers that are not rewritten every frame are kept coherent
	// via a content version: a stale ring slot catches up (memcpy from the
	// newest slot) the first time it is bound.
	class MetalVertexBuffer : public VertexBuffer
	{
	public:
		MetalVertexBuffer(uint32_t size, std::string& label, bool retained = false);
		MetalVertexBuffer(float* vertices, uint32_t size, std::string& label);
		virtual ~MetalVertexBuffer();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void ResizeBuffer(uint32_t size) override;

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

		virtual const BufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }
	private:
		static constexpr uint32_t kRingSize = 3; // == MetalContext::kMaxFramesInFlight

		uint32_t CurrentSlot() const;
		MTL::Buffer* CurrentBuffer() const;

		uint32_t m_Size = 0;
		BufferLayout m_Layout;
		bool isStatic = true;
		MTL::Buffer* m_Buffers[kRingSize] = {};
		uint64_t m_ContentVersion = 0;
		mutable uint64_t m_SlotVersions[kRingSize] = {};
		uint32_t m_LatestSlot = 0;
		uint32_t m_ContentBytes = 0;
	};

	class MetalIndexBuffer : public IndexBuffer
{
public:
    MetalIndexBuffer(uint32_t* indices, uint32_t count, std::string& label);
    MetalIndexBuffer(uint32_t count, std::string& label, bool retained = false);
    virtual ~MetalIndexBuffer();

    virtual void Bind() const override;
    virtual void Unbind() const override;
    virtual void SetData(const uint32_t* data, uint32_t count, uint32_t offset = 0) override;
    virtual void ResizeBuffer(uint32_t count) override;

    virtual uint32_t GetCount() const override { return m_Count; }

    MTL::Buffer* GetBuffer() const;
	private:
		static constexpr uint32_t kRingSize = 3; // == MetalContext::kMaxFramesInFlight

		uint32_t CurrentSlot() const;

		uint32_t m_Count;
		bool isStatic = true;
		MTL::Buffer* m_Buffers[kRingSize] = {};
		uint64_t m_ContentVersion = 0;
		mutable uint64_t m_SlotVersions[kRingSize] = {};
		uint32_t m_LatestSlot = 0;
		uint32_t m_ContentBytes = 0;
	};

}
