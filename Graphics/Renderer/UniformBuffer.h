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
		
		static Ref<UniformBuffer> Create(uint32_t size, uint32_t binding, std::string label = "");
	};

}
