#include "Core/Base.h"
#include "Window/Window.h"

#ifdef BUILDING_METAL
#include "Platform/MacOSWindow.h"
#include "Platform/IOSWindow.h"
#else
#include "Platform/WindowsWindow.h"
#endif

namespace Application {

	Graphics::Scope<Window> Window::Create(const WindowProps& props, void* nativeWindow)
	{
#ifdef BUILDING_METAL
#if TARGET_OS_OSX
        return Graphics::CreateScope<MacOSWindow>(props);
#else
        return Graphics::CreateScope<IOSWindow>(props, nativeWindow);
#endif
#else
		return Graphics::CreateScope<WindowsWindow>(props);
#endif
	}

}