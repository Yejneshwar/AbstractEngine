#include "Core/Base.h"
#include "Window/Window.h"

#if BUILDING_METAL

#if TARGET_OS_OSX
#include "Platform/MacOSWindow.h"
#elif TARGET_OS_IOS
#include "Platform/IOSWindow.h"
#endif // TARGET_OS_OSX

#else
#include "Platform/WindowsWindow.h"
#endif // BUILDING_METAL

namespace Application {

	Graphics::Scope<Window> Window::Create(const WindowProps& props, void* nativeWindow)
	{
#if BUILDING_METAL
#if TARGET_OS_OSX
        return Graphics::CreateScope<MacOSWindow>(props, nativeWindow);
#else
        return Graphics::CreateScope<IOSWindow>(props, nativeWindow);
#endif
#else
		return Graphics::CreateScope<WindowsWindow>(props);
#endif
	}

}
