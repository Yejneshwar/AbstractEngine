
#include "Window/Window.h"


namespace Application {

    struct WindowSettings {
        bool VSync = true;
        bool PolygonSmooth = false;
        bool fullScreen = false;
        int monitorCount;
    };

    struct WindowData
    {
        std::string Title;
        unsigned int Width, Height;
        WindowSettings m_Settings;
        std::chrono::high_resolution_clock::time_point m_mousePressStartLeft;
        std::chrono::high_resolution_clock::time_point m_mousePressEndLeft;
        
        std::chrono::high_resolution_clock::time_point m_mousePressStartRight;
        std::chrono::high_resolution_clock::time_point m_mousePressEndRight;
        
        Window::EventCallbackFn EventCallback;
    };

    class IOSWindow : public Window {
    public:
        IOSWindow(const WindowProps& props, void* nativeWindow);
        virtual ~IOSWindow();

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
        Graphics::Ref<Graphics::GraphicsContext> m_GraphicsContext;
        void* m_Window;

        WindowData m_Data;
    };

}
