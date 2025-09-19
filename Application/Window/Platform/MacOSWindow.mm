#include "MacOSWindow.h"

namespace Application {
    MacOSWindow::MacOSWindow(const WindowProps& props) {

        
    }

    void MacOSWindow::SetEventCallback(const EventCallbackFn &callback) {

    }

    void MacOSWindow::SetVSync(bool enabled) {

    }

    void MacOSWindow::SetPolygonSmooth(bool enabled) {

    }

    bool MacOSWindow::IsVSync() const {

    }

    bool MacOSWindow::IsPolygonSmooth() const {

    }

    void* MacOSWindow::GetNativeWindow() const {

    }

    int MacOSWindow::GetMonitorCount() const {

    }

    const char *MacOSWindow::GetPrimaryMonitorName() const {

    }

    Graphics::Ref<Graphics::GraphicsContext> MacOSWindow::GetRenderContext() const {
    return nullptr;
    }


    uint32_t MacOSWindow::GetHeight() const {
    return 0;
    }


    uint32_t MacOSWindow::GetWidth() const {
    return 0;
    }


    void MacOSWindow::OnUpdate() {
    }


    MacOSWindow::~MacOSWindow() {
    }

}
