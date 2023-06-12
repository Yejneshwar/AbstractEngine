#include "GraphicsCore.h"
#include "Renderer/RenderCommand.h"

namespace Graphics {

	Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();

}