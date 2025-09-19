#include "IOSWindow.h"
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

#include "Events/Input.h"

#include "Events/EventTypes/ApplicationEvent.h"
#include "Events/EventTypes/MouseEvent.h"
#include "Events/EventTypes/KeyEvent.h"

#include "Window/Platform/InputManager/InputManager.h"

// A custom UIView subclass that tells iOS its backing layer should be a CAMetalLayer.
@interface MetalView : UIView
@property (nonatomic, assign) Application::WindowData* windowData;
@property (nonatomic, strong) UIPinchGestureRecognizer *pinchGestureRecognizer;
@end

@implementation MetalView
+ (Class)layerClass {
    return [CAMetalLayer class];
}

#pragma mark - Initialization

// Add an initializer to enable multi-touch.
- (instancetype)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Crucial: Enable multi-touch for the view.
        self.multipleTouchEnabled = YES;
        
        // Initialize and add the pinch gesture recognizer
        _pinchGestureRecognizer = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(handlePinch:)];
        [self addGestureRecognizer:_pinchGestureRecognizer];
    }
    return self;
}

// Also handle initialization from Storyboards or XIBs.
- (void)awakeFromNib {
    [super awakeFromNib];
    self.multipleTouchEnabled = YES;
}

- (void)layoutSubviews {
    [super layoutSubviews];

    // Get a strongly-typed reference to the layer by casting self.layer.
    CAMetalLayer *metalLayer = (CAMetalLayer *)self.layer;
    
    // Use the view's contentScaleFactor. This is more robust than using
    // [UIScreen mainScreen].nativeScale as it automatically adapts if the
    // view is moved to a display with a different scale.
    CGFloat scale = self.contentScaleFactor;

    // Calculate the new size in pixels.
    CGSize newSize = CGSizeMake(self.bounds.size.width * scale, self.bounds.size.height * scale);

    // Update the layer's drawableSize only if it has actually changed.
    if (!CGSizeEqualToSize(metalLayer.drawableSize, newSize)) {
        metalLayer.drawableSize = newSize;
    }
}

// Helper function to dispatch events
- (void)dispatchMouseEvent:(Application::Event&)event {
    if (self.windowData && self.windowData->EventCallback) {
        self.windowData->EventCallback(event);
    }
}
#pragma mark - Pinch Gesture Handler

// The new handler for pinch gestures
- (void)handlePinch:(UIPinchGestureRecognizer *)recognizer {
    if (recognizer.state == UIGestureRecognizerStateBegan || recognizer.state == UIGestureRecognizerStateChanged) {
        // The scale property gives the multiplicative zoom factor since the gesture started.
        // We want the *change* in scale since the last frame, not the total.
        // So we calculate a delta from the neutral 1.0 scale.
        float zoomDelta = recognizer.scale - 1.0f;
        
        // We can map this zoom delta to the Y-axis of a mouse scroll event.
        // The sensitivity can be adjusted by changing the multiplier.
        const float zoomSensitivity = 50.0f;
        Application::MouseScrolledEvent scrollEvent(0.0f, zoomDelta * zoomSensitivity);
        [self dispatchMouseEvent:scrollEvent];
        
        // IMPORTANT: Reset the recognizer's scale to 1.0.
        // This ensures that the next time this method is called, the scale value
        // represents the change from *now* rather than from the beginning of the gesture.
        // This effectively turns a cumulative scale into a per-frame delta.
        recognizer.scale = 1.0f;
    }
}

#pragma mark - Touch Handlers

// Called when a finger first touches the screen
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UITouch *touch = [touches anyObject];
    CGPoint location = [touch locationInView:self];
    NSLog(@"Tap count: %ld", [touch tapCount]);
    NSLog(@"Number of touches: %ld", [touches count]);
    // Update the C++ InputManager singleton for polling
    Application::InputManager::OnTouchDown((float)location.x, (float)location.y, touches.count);
    // We'll use '0' for the left mouse button equivalent.
    Application::MouseButtonPressedEvent pressEvent((touches.count == 1) ? 0 : 1, (__bridge_retained void*)event);
    [self dispatchMouseEvent:pressEvent];
}

// Called when a finger moves across the screen
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UITouch *touch = [touches anyObject];
    CGPoint location = [touch locationInView:self];

    
    // Flip the Y-coordinate if necessary
    float yPos = self.bounds.size.height - location.y;
    
    // Update the C++ InputManager singleton for polling
    Application::InputManager::OnTouchMoved((float)location.x, (float)location.y);

    Application::MouseMovedEvent moveEvent((float)location.x, (float)location.y, (__bridge_retained void*)event);
    [self dispatchMouseEvent:moveEvent];
}

// Called when a finger is lifted from the screen
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UITouch *touch = [touches anyObject];
    CGPoint location = [touch locationInView:self];
    
    // Update the C++ InputManager singleton for polling
    Application::InputManager::OnTouchUp((float)location.x, (float)location.y);

    Application::MouseButtonReleasedEvent releaseEvent(0, (__bridge_retained void*)event);
    [self dispatchMouseEvent:releaseEvent];
}

// Also handle touches being cancelled (e.g., by a system event)
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self touchesEnded:touches withEvent:event];
}

#pragma mark - Keyboard Handlers

- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    for (UIPress *press in presses) {
        Application::KeyCode keyCode = (Application::KeyCode)press.key.keyCode;
        Application::InputManager::OnKeyPress(keyCode);
    }
    [super pressesBegan:presses withEvent:event];
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    for (UIPress *press in presses) {
        Application::KeyCode keyCode = (Application::KeyCode)press.key.keyCode;
        Application::InputManager::OnKeyRelease(keyCode);
    }
    [super pressesEnded:presses withEvent:event];
}

- (void)pressesCancelled:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    [self pressesEnded:presses withEvent:event];
}
@end

namespace Application {

class iOSWindowImpl {
public:
    UIWindow* window;
    UIViewController* viewController;
    MetalView* view;

    iOSWindowImpl(const WindowProps& props, void* nativeWindow) {
        if (![NSThread isMainThread]) {
            NSLog(@"FATAL ERROR: UI initialization must occur on the main thread. Terminating.");
            abort(); // Forcefully and immediately terminates the app.
        }
        
        CGRect screenBounds = [[UIScreen mainScreen] bounds];
        window = (__bridge UIWindow*)nativeWindow;

        viewController = [[UIViewController alloc] init];
        viewController.view.backgroundColor = [UIColor blackColor];
        
        view = [[MetalView alloc] initWithFrame:screenBounds];
        view.translatesAutoresizingMaskIntoConstraints = NO;
        view.backgroundColor = [UIColor whiteColor];
        
//        [viewController setView:view];
        [viewController.view addSubview:view];

        // Get the safe area layout guide from the controller's view
        UILayoutGuide *safeArea = viewController.view.safeAreaLayoutGuide;

        // Activate constraints to pin the MetalView to the safe area
        [view.topAnchor constraintEqualToAnchor:safeArea.topAnchor].active = YES;
        [view.bottomAnchor constraintEqualToAnchor:safeArea.bottomAnchor].active = YES;
        [view.leadingAnchor constraintEqualToAnchor:safeArea.leadingAnchor].active = YES;
        [view.trailingAnchor constraintEqualToAnchor:safeArea.trailingAnchor].active = YES;
        
//        // 1. Create the view controller. It comes with a default container view.
//        viewController = [[UIViewController alloc] init];
//        UIView* containerView = viewController.view;
//        containerView.backgroundColor = [UIColor whiteColor];
//        
//        // 2. Create your MetalView and add it as a SUBVIEW.
//        //    No need for a frame, constraints will define its size.
//        view = [[MetalView alloc] initWithFrame:screenBounds];
//        [containerView addSubview:view];
//
//        // 3. Set up Auto Layout constraints to make the MetalView fill the container.
//        //    This is the critical fix for the layout collapse.
//        view.translatesAutoresizingMaskIntoConstraints = NO;
//        UILayoutGuide *safeArea = viewController.view.safeAreaLayoutGuide;
//
//        [NSLayoutConstraint activateConstraints:@[
//            [view.topAnchor constraintEqualToAnchor:safeArea.topAnchor],
//            [view.bottomAnchor constraintEqualToAnchor:safeArea.bottomAnchor],
//            [view.leadingAnchor constraintEqualToAnchor:safeArea.leadingAnchor],
//            [view.trailingAnchor constraintEqualToAnchor:safeArea.trailingAnchor]
//        ]];

        [window setRootViewController:viewController];
    }

    ~iOSWindowImpl() {
        [view removeFromSuperview];
#if !(__has_feature(objc_arc))
        [viewController release];
        [view release];
        [window release];
#endif
    }
};

    static iOSWindowImpl* s_Impl = nullptr;

    IOSWindow::IOSWindow(const WindowProps& props, void* nativeWindow) {
        s_Impl = new iOSWindowImpl(props, nativeWindow);
        s_Impl->view.windowData = &m_Data;

        // Create graphics context from UIView (likely Metal)
        m_GraphicsContext = Graphics::GraphicsContext::Create((__bridge void*)s_Impl->view);
        m_GraphicsContext->Init();
        
    }

    void IOSWindow::SetEventCallback(const EventCallbackFn &callback) {
        m_Data.EventCallback = callback;
    }

    void IOSWindow::SetVSync(bool enabled) {

    }

    void IOSWindow::SetPolygonSmooth(bool enabled) {

    }

    bool IOSWindow::IsVSync() const {
        return false;
    }

    bool IOSWindow::IsPolygonSmooth() const {
        return false;
    }

    void *IOSWindow::GetNativeWindow() const {
        return (__bridge_retained void*)s_Impl->window;
    }

    int IOSWindow::GetMonitorCount() const {
        return [[UIScreen screens] count];
    }

    const char* IOSWindow::GetPrimaryMonitorName() const {
        return "iOS Screen";
    }

    Graphics::Ref<Graphics::GraphicsContext> IOSWindow::GetRenderContext() const {
        return m_GraphicsContext;
    }


    uint32_t IOSWindow::GetHeight() const {
    return 0;
    }


    uint32_t IOSWindow::GetWidth() const {
    return 0;
    }


    void IOSWindow::OnUpdate() {
        m_GraphicsContext->SwapBuffers();
    }

    IOSWindow::~IOSWindow() {
        delete s_Impl;
        s_Impl = nullptr;
    }
}
