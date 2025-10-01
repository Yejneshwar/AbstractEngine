#include "MacOSWindow.h"

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>

#import <QuartzCore/QuartzCore.h>

#include "Events/Input.h"
#include "Events/EventTypes/ApplicationEvent.h"
#include "Events/EventTypes/MouseEvent.h"
#include "Events/EventTypes/KeyEvent.h"
#include "Window/Platform/InputManager/InputManager.h"

// A custom NSView subclass that tells macOS its backing layer should be a CAMetalLayer.
@interface MetalView : NSView
@property (nonatomic, assign) Application::WindowData* windowData;
@end

@implementation MetalView

+ (Class)layerClass {
    return [CAMetalLayer class];
}

#pragma mark - Initialization

- (instancetype)initWithFrame:(NSRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Crucial: Set the view to be layer-backed and specify the layer type.
        self.wantsLayer = YES;
    }
    return self;
}

// Override to specify a CAMetalLayer.
- (CALayer *)makeBackingLayer {
    return [CAMetalLayer layer];
}

// macOS needs to know the view can become the first responder to receive key events.
- (BOOL)acceptsFirstResponder {
    return YES;
}

// This method is called when the view's frame changes.
- (void)resizeSubviewsWithOldSize:(NSSize)oldSize {
    [super resizeSubviewsWithOldSize:oldSize];
    
    CAMetalLayer *metalLayer = (CAMetalLayer *)self.layer;
    
    // On macOS, we get the scale factor from the window.
    CGFloat scale = self.window.backingScaleFactor;
    
    // Calculate the new size in pixels.
    CGSize newSize = CGSizeMake(self.bounds.size.width * scale, self.bounds.size.height * scale);
    
    // Update the layer's drawableSize only if it has actually changed.
    if (!CGSizeEqualToSize(metalLayer.drawableSize, newSize)) {
        metalLayer.drawableSize = newSize;
        
        // Dispatch a window resize event
        Application::WindowResizeEvent event((uint32_t)newSize.width, (uint32_t)newSize.height);
        [self dispatchEvent:event];
    }
}

// Helper function to dispatch events
- (void)dispatchEvent:(Application::Event&)event {
    if (self.windowData && self.windowData->EventCallback) {
        self.windowData->EventCallback(event);
    }
}

#pragma mark - Mouse Handlers

// Helper to get mouse location and dispatch a move event.
- (void)handleMouseMove:(NSEvent *)event {
    NSPoint locationInWindow = [event locationInWindow];
    NSPoint locationInView = [self convertPoint:locationInWindow fromView:nil];
    
    // Update the C++ InputManager singleton for polling.
    Application::InputManager::OnMouseMoved((float)locationInView.x, -(float)locationInView.y);
    
    // Dispatch the C++ event.
    Application::MouseMovedEvent moveEvent((float)locationInView.x, -(float)locationInView.y, (__bridge_retained void*)event);
    [self dispatchEvent:moveEvent];
}


- (void)mouseDown:(NSEvent *)event {
    self.windowData->m_mousePressStartLeft = std::chrono::high_resolution_clock::now();
    [self handleMouseMove:event]; // Update position on click
    Application::InputManager::OnMouseDown(0);
    Application::MouseButtonPressedEvent pressEvent(0, (__bridge_retained void*)event); // 0 for left mouse button
    [self dispatchEvent:pressEvent];
}

- (void)mouseUp:(NSEvent *)event {
    self.windowData->m_mousePressEndLeft = std::chrono::high_resolution_clock::now();
    [self handleMouseMove:event]; // Update position on release
    Application::InputManager::OnMouseUp(0);
    Application::MouseButtonReleasedEvent releaseEvent(0,
                                                       std::chrono::duration_cast<std::chrono::milliseconds>(self.windowData->m_mousePressEndLeft - self.windowData->m_mousePressStartLeft),
                                                       (__bridge_retained void*)event
                                                       );
    [self dispatchEvent:releaseEvent];
}

- (void)mouseDragged:(NSEvent *)event {
    [self handleMouseMove:event];
}

- (void)rightMouseDown:(NSEvent *)event {
    self.windowData->m_mousePressStartRight = std::chrono::high_resolution_clock::now();
    [self handleMouseMove:event];
    Application::InputManager::OnMouseDown(1);
    Application::MouseButtonPressedEvent pressEvent(1, (__bridge_retained void*)event); // 1 for right mouse button
    [self dispatchEvent:pressEvent];
}

- (void)rightMouseUp:(NSEvent *)event {
    self.windowData->m_mousePressEndRight = std::chrono::high_resolution_clock::now();
    [self handleMouseMove:event];
    Application::InputManager::OnMouseUp(1);
    Application::MouseButtonReleasedEvent releaseEvent(1,
                                                       std::chrono::duration_cast<std::chrono::milliseconds>(self.windowData->m_mousePressEndRight - self.windowData->m_mousePressStartRight),
                                                       (__bridge_retained void*)event
                                                       );
    [self dispatchEvent:releaseEvent];
}


// Handles scroll wheel, trackpad scrolling, and trackpad pinch-to-zoom gestures.
- (void)scrollWheel:(NSEvent *)event {
    float dx = [event scrollingDeltaX];
    float dy = [event scrollingDeltaY];
    
    Application::MouseScrolledEvent scrollEvent(dx, dy);
    [self dispatchEvent:scrollEvent];
}

#pragma mark - Keyboard Handlers

- (void)keyDown:(NSEvent *)event {
    if ([event isARepeat]) {
        // NOTE: Your engine architecture may want to handle repeat events differently.
        // For now, we treat it as a new press for simplicity.
    }
    Application::KeyCode keyCode = (Application::KeyCode)[event keyCode];
    Application::InputManager::OnKeyPress(keyCode);
    
    Application::KeyPressedEvent pressEvent(keyCode, [event isARepeat] ? 1 : 0);
    [self dispatchEvent:pressEvent];
}

- (void)keyUp:(NSEvent *)event {
    Application::KeyCode keyCode = (Application::KeyCode)[event keyCode];
    Application::InputManager::OnKeyRelease(keyCode);
    
    Application::KeyReleasedEvent releaseEvent(keyCode);
    [self dispatchEvent:releaseEvent];
}

- (void)flagsChanged:(NSEvent *)event {
    // This handles modifier keys like Shift, Ctrl, Option, Command.
    // You can add logic here if you need to track modifier key state changes independently.
}

@end

namespace Application {
    
    class MacOSWindowImpl {
    public:
        NSWindow* window;
        MetalView* view;
        
        MacOSWindowImpl(const WindowProps& props, void* nativeWindow) {
            if (![NSThread isMainThread]) {
                NSLog(@"FATAL ERROR: UI initialization must occur on the main thread. Terminating.");
                abort();
            }
            
            NSRect contentRect = NSMakeRect(0, 0, props.Width, props.Height);
            
            NSWindowStyleMask styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
            
            window = (__bridge NSWindow*)nativeWindow;
            
            view = [[MetalView alloc] initWithFrame:contentRect];
            
            [window setContentView:view];
            [window makeFirstResponder:view]; // Make the view ready for keyboard input
            [window makeKeyAndOrderFront:nil];
        }
        
        ~MacOSWindowImpl() {
            [window close];
#if !(__has_feature(objc_arc))
            [view release];
            [window release];
#endif
        }
    };
    
    static MacOSWindowImpl* s_Impl = nullptr;
    
    MacOSWindow::MacOSWindow(const WindowProps& props, void* nativeWindow) {
        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;
        
        s_Impl = new MacOSWindowImpl(props, nativeWindow);
        s_Impl->view.windowData = &m_Data;
        
        // Create graphics context from NSView
        m_GraphicsContext = Graphics::GraphicsContext::Create((__bridge void*)s_Impl->view);
        m_GraphicsContext->Init();
    }
    
    MacOSWindow::~MacOSWindow() {
        delete s_Impl;
        s_Impl = nullptr;
    }
    
    void MacOSWindow::OnUpdate() {
        // On macOS, events are typically processed through the run loop.
        // For a game loop, you might need to poll for events explicitly.
        @autoreleasepool {
            NSEvent *event;
            while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES])) {
                [NSApp sendEvent:event];
            }
        }
        m_GraphicsContext->SwapBuffers();
    }
    
    void MacOSWindow::SetEventCallback(const EventCallbackFn &callback) {
        m_Data.EventCallback = callback;
    }
    
    void MacOSWindow::SetVSync(bool enabled) {
        // VSync implementation would be in the Metal graphics context.
    }
    
    bool MacOSWindow::IsVSync() const {
        return false;
    }
    
    void MacOSWindow::SetPolygonSmooth(bool enabled) {
        // This is typically a graphics context/pipeline state, not a window property.
    }
    
    bool MacOSWindow::IsPolygonSmooth() const {
        return false;
    }
    
    void *MacOSWindow::GetNativeWindow() const {
        return (__bridge_retained void*)s_Impl->window;
    }
    
    uint32_t MacOSWindow::GetWidth() const {
        return [s_Impl->view bounds].size.width;
    }
    
    uint32_t MacOSWindow::GetHeight() const {
        return [s_Impl->view bounds].size.height;
    }
    
    Graphics::GraphicsContext* MacOSWindow::GetRenderContext() const {
        return m_GraphicsContext.get();
    }
    
    int MacOSWindow::GetMonitorCount() const {
        return [[NSScreen screens] count];
    }
    
    const char* MacOSWindow::GetPrimaryMonitorName() const {
        NSScreen* primaryScreen = [[NSScreen screens] objectAtIndex:0];
        NSString* screenName = @"Primary Screen"; // Default
        if (@available(macOS 10.15, *)) {
            screenName = [primaryScreen localizedName];
        }
        return [screenName UTF8String];
    }
    
} // namespace Application
