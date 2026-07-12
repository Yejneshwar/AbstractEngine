#pragma once

#include "GraphicsCore.h"
#include <cstdint>
#include <string>

namespace Graphics {

	class UniformBuffer
	{
	public:
		virtual ~UniformBuffer() {}
		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

		// (Re)attach the current contents to the active encoder without
		// uploading. On Metal every render encoder starts with empty bindings,
		// so buffers whose data didn't change this frame still need a Bind
		// after the framebuffer is bound. GL bindings persist — no-op there.
		virtual void Bind() {}

		static Ref<UniformBuffer> Create(uint32_t size, uint32_t binding, std::string label = "");
	};

}
