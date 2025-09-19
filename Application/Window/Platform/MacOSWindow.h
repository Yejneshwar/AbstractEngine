#include "Window/Window.h"


namespace Application {

    class MacOSWindow : public Window {
    public:
        MacOSWindow(const WindowProps& props);
		virtual ~MacOSWindow();

		virtual void OnUpdate() override;

		virtual uint32_t GetWidth() const override;
		virtual uint32_t GetHeight() const override;

		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) override;
		virtual void SetVSync(bool enabled) override;
		virtual void SetPolygonSmooth(bool enabled) override;
		virtual bool IsVSync() const override;
		virtual bool IsPolygonSmooth() const override;

		virtual void* GetNativeWindow() const override;

		virtual int GetMonitorCount() const override;
        virtual const char* GetPrimaryMonitorName() const override;
        
        Graphics::Ref<Graphics::GraphicsContext> GetRenderContext() const override;
        
    private:
        Graphics::Scope<Graphics::GraphicsContext> m_GraphicsContext;
    };

}
