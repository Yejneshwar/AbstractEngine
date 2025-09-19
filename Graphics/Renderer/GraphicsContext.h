#pragma once
#include "GraphicsCore.h"
namespace Graphics {

	class GraphicsContext
	{
	public:
        GraphicsContext() = default;
		virtual ~GraphicsContext() = default;

		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;

		static Ref<GraphicsContext> Create(void* window);
	};

}
