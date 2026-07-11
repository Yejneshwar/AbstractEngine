#include "IOSWindow.h"

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

#include "Events/Input.h"

#include "Events/EventTypes/ApplicationEvent.h"
#include "Events/EventTypes/MouseEvent.h"
#include "Events/EventTypes/KeyEvent.h"
#include "Events/EventTypes/GestureEvent.h"

#include "Window/Platform/InputManager/InputManager.h"

static Application::GesturePhase GesturePhaseFromRecognizer(UIGestureRecognizer* recognizer) {
    switch (recognizer.state) {
        case UIGestureRecognizerStateBegan:     return Application::GesturePhase::Began;
        case UIGestureRecognizerStateEnded:     return Application::GesturePhase::Ended;
        case UIGestureRecognizerStateCancelled:
        case UIGestureRecognizerStateFailed:    return Application::GesturePhase::Cancelled;
        default:                                return Application::GesturePhase::Changed;
    }
}

static Application::PointerType PointerTypeFromTouch(UITouch* touch) {
    switch (touch.type) {
        case UITouchTypePencil:          return Application::PointerType::Pencil;
        case UITouchTypeIndirectPointer: return Application::PointerType::IndirectPointer;
        default:                         return Application::PointerType::Touch;
    }
}

static float PressureFromTouch(UITouch* touch) {
    if (touch.maximumPossibleForce > 0.0) {
        return (float)(touch.force / touch.maximumPossibleForce);
    }
    return 1.0f;
}

// Map UIKeyboardHIDUsage codes to the macOS kVK_* codes the rest of the
// engine (KeyCodes, DebugLayer shortcuts) already uses, so a hardware
// keyboard behaves identically on iPad and Mac.
static int KeyCodeFromHIDUsage(UIKeyboardHIDUsage usage) {
    switch (usage) {
        // Letters (HID A=4): kVK codes are scattered.
        case UIKeyboardHIDUsageKeyboardA: return 0;   case UIKeyboardHIDUsageKeyboardB: return 11;
        case UIKeyboardHIDUsageKeyboardC: return 8;   case UIKeyboardHIDUsageKeyboardD: return 2;
        case UIKeyboardHIDUsageKeyboardE: return 14;  case UIKeyboardHIDUsageKeyboardF: return 3;
        case UIKeyboardHIDUsageKeyboardG: return 5;   case UIKeyboardHIDUsageKeyboardH: return 4;
        case UIKeyboardHIDUsageKeyboardI: return 34;  case UIKeyboardHIDUsageKeyboardJ: return 38;
        case UIKeyboardHIDUsageKeyboardK: return 40;  case UIKeyboardHIDUsageKeyboardL: return 37;
        case UIKeyboardHIDUsageKeyboardM: return 46;  case UIKeyboardHIDUsageKeyboardN: return 45;
        case UIKeyboardHIDUsageKeyboardO: return 31;  case UIKeyboardHIDUsageKeyboardP: return 35;
        case UIKeyboardHIDUsageKeyboardQ: return 12;  case UIKeyboardHIDUsageKeyboardR: return 15;
        case UIKeyboardHIDUsageKeyboardS: return 1;   case UIKeyboardHIDUsageKeyboardT: return 17;
        case UIKeyboardHIDUsageKeyboardU: return 32;  case UIKeyboardHIDUsageKeyboardV: return 9;
        case UIKeyboardHIDUsageKeyboardW: return 13;  case UIKeyboardHIDUsageKeyboardX: return 7;
        case UIKeyboardHIDUsageKeyboardY: return 16;  case UIKeyboardHIDUsageKeyboardZ: return 6;
        // Digits
        case UIKeyboardHIDUsageKeyboard1: return 18;  case UIKeyboardHIDUsageKeyboard2: return 19;
        case UIKeyboardHIDUsageKeyboard3: return 20;  case UIKeyboardHIDUsageKeyboard4: return 21;
        case UIKeyboardHIDUsageKeyboard5: return 23;  case UIKeyboardHIDUsageKeyboard6: return 22;
        case UIKeyboardHIDUsageKeyboard7: return 26;  case UIKeyboardHIDUsageKeyboard8: return 28;
        case UIKeyboardHIDUsageKeyboard9: return 25;  case UIKeyboardHIDUsageKeyboard0: return 29;
        // Function keys
        case UIKeyboardHIDUsageKeyboardF1: return 122;  case UIKeyboardHIDUsageKeyboardF2: return 120;
        case UIKeyboardHIDUsageKeyboardF3: return 99;   case UIKeyboardHIDUsageKeyboardF4: return 118;
        case UIKeyboardHIDUsageKeyboardF5: return 96;   case UIKeyboardHIDUsageKeyboardF6: return 97;
        case UIKeyboardHIDUsageKeyboardF7: return 98;   case UIKeyboardHIDUsageKeyboardF8: return 100;
        case UIKeyboardHIDUsageKeyboardF9: return 101;  case UIKeyboardHIDUsageKeyboardF10: return 109;
        case UIKeyboardHIDUsageKeyboardF11: return 103; case UIKeyboardHIDUsageKeyboardF12: return 111;
        // Navigation / editing
        case UIKeyboardHIDUsageKeyboardLeftArrow: return 123;
        case UIKeyboardHIDUsageKeyboardRightArrow: return 124;
        case UIKeyboardHIDUsageKeyboardDownArrow: return 125;
        case UIKeyboardHIDUsageKeyboardUpArrow: return 126;
        case UIKeyboardHIDUsageKeyboardReturnOrEnter: return 36;
        case UIKeyboardHIDUsageKeyboardEscape: return 53;
        case UIKeyboardHIDUsageKeyboardDeleteOrBackspace: return 51;
        case UIKeyboardHIDUsageKeyboardTab: return 48;
        case UIKeyboardHIDUsageKeyboardSpacebar: return 49;
        // Modifiers
        case UIKeyboardHIDUsageKeyboardLeftShift:
        case UIKeyboardHIDUsageKeyboardRightShift: return 56;
        case UIKeyboardHIDUsageKeyboardLeftControl:
        case UIKeyboardHIDUsageKeyboardRightControl: return 59;
        case UIKeyboardHIDUsageKeyboardLeftAlt:
        case UIKeyboardHIDUsageKeyboardRightAlt: return 58;
        case UIKeyboardHIDUsageKeyboardLeftGUI:
        case UIKeyboardHIDUsageKeyboardRightGUI: return 55;
        default: return (int)usage; // fall through with the raw HID usage
    }
}

// A custom UIView subclass that tells iOS its backing layer should be a CAMetalLayer.
@interface MetalView : UIView <UIGestureRecognizerDelegate>
@property (nonatomic, assign) Application::WindowData* windowData;
@property (nonatomic, strong) UIPinchGestureRecognizer* pinchGestureRecognizer;
@property (nonatomic, strong) UIRotationGestureRecognizer* rotationGestureRecognizer;
@property (nonatomic, strong) UIPanGestureRecognizer* twoFingerPanGestureRecognizer;
@property (nonatomic, strong) UIPanGestureRecognizer* scrollGestureRecognizer;
@property (nonatomic, strong) UIHoverGestureRecognizer* hoverGestureRecognizer;
// The single touch (Pencil preferred) currently driving the pointer stream.
@property (nonatomic, weak) UITouch* primaryTouch;
@end

@implementation MetalView

    std::chrono::high_resolution_clock::time_point touchStart, touchEnd;

+ (Class)layerClass {
    return [CAMetalLayer class];
}

#pragma mark - Initialization

// Add an initializer to enable multi-touch.
- (instancetype)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Crucial: Enable multi-touch for the view.
        self.multipleTouchEnabled = YES;

        // Pinch-to-zoom
        _pinchGestureRecognizer = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(handlePinch:)];
        _pinchGestureRecognizer.delegate = self;
        [self addGestureRecognizer:_pinchGestureRecognizer];

        // Two-finger rotation
        _rotationGestureRecognizer = [[UIRotationGestureRecognizer alloc] initWithTarget:self action:@selector(handleRotation:)];
        _rotationGestureRecognizer.delegate = self;
        [self addGestureRecognizer:_rotationGestureRecognizer];

        // Two-finger pan
        _twoFingerPanGestureRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handleTwoFingerPan:)];
        _twoFingerPanGestureRecognizer.minimumNumberOfTouches = 2;
        _twoFingerPanGestureRecognizer.maximumNumberOfTouches = 2;
        _twoFingerPanGestureRecognizer.delegate = self;
        [self addGestureRecognizer:_twoFingerPanGestureRecognizer];

        // Trackpad / mouse scroll (indirect pointer, zero-touch pan)
        if (@available(iOS 13.4, *)) {
            _scrollGestureRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handleScroll:)];
            _scrollGestureRecognizer.allowedScrollTypesMask = UIScrollTypeMaskAll;
            _scrollGestureRecognizer.maximumNumberOfTouches = 0;
            _scrollGestureRecognizer.delegate = self;
            [self addGestureRecognizer:_scrollGestureRecognizer];
        }

        // Hover: Apple Pencil hover and the iPadOS trackpad pointer.
        _hoverGestureRecognizer = [[UIHoverGestureRecognizer alloc] initWithTarget:self action:@selector(handleHover:)];
        [self addGestureRecognizer:_hoverGestureRecognizer];
    }
    return self;
}

// Let pinch/rotate/pan run together so e.g. a pinch that drifts also pans.
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    return YES;
}

// Also handle initialization from Storyboards or XIBs.
- (void)awakeFromNib {
    [super awakeFromNib];
    self.multipleTouchEnabled = YES;
}

// Required for hardware-keyboard presses to be delivered to this view.
- (BOOL)canBecomeFirstResponder {
    return YES;
}

- (void)didMoveToWindow {
    [super didMoveToWindow];
    if (self.window) {
        [self becomeFirstResponder];
    }
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
#pragma mark - Gesture Handlers

- (void)handlePinch:(UIPinchGestureRecognizer *)recognizer {
    CGPoint center = [recognizer locationInView:self];
    // Reset scale each callback so `scale` is the per-event multiplicative delta.
    Application::PinchGestureEvent pinchEvent((float)recognizer.scale,
        GesturePhaseFromRecognizer(recognizer), (float)center.x, (float)center.y);
    recognizer.scale = 1.0;
    [self dispatchMouseEvent:pinchEvent];
}

- (void)handleRotation:(UIRotationGestureRecognizer *)recognizer {
    CGPoint center = [recognizer locationInView:self];
    // UIKit rotation is radians, clockwise positive; the engine convention is
    // counter-clockwise positive (matching AppKit).
    Application::RotateGestureEvent rotateEvent(-(float)recognizer.rotation,
        GesturePhaseFromRecognizer(recognizer), (float)center.x, (float)center.y);
    recognizer.rotation = 0.0;
    [self dispatchMouseEvent:rotateEvent];
}

- (void)handleTwoFingerPan:(UIPanGestureRecognizer *)recognizer {
    CGPoint center = [recognizer locationInView:self];
    CGPoint translation = [recognizer translationInView:self];
    [recognizer setTranslation:CGPointMake(0, 0) inView:self];
    Application::PanGestureEvent panEvent((float)translation.x, (float)translation.y,
        GesturePhaseFromRecognizer(recognizer), (float)center.x, (float)center.y);
    [self dispatchMouseEvent:panEvent];
}

// Trackpad / mouse-wheel scrolling on iPadOS (indirect pointer).
- (void)handleScroll:(UIPanGestureRecognizer *)recognizer {
    CGPoint translation = [recognizer translationInView:self];
    [recognizer setTranslation:CGPointMake(0, 0) inView:self];
    // Scale point deltas down to wheel-like offsets (consumers multiply up).
    Application::MouseScrolledEvent scrollEvent((float)translation.x * 0.1f, (float)translation.y * 0.1f,
        /*precise*/ true, /*momentum*/ false);
    [self dispatchMouseEvent:scrollEvent];
}

// Pencil hover and iPadOS trackpad pointer moves (no button held).
- (void)handleHover:(UIHoverGestureRecognizer *)recognizer {
    if (recognizer.state == UIGestureRecognizerStateEnded ||
        recognizer.state == UIGestureRecognizerStateCancelled) {
        return;
    }
    CGPoint location = [recognizer locationInView:self];
    Application::InputManager::OnMouseMoved((float)location.x, (float)location.y);
    Application::MouseMovedEvent moveEvent((float)location.x, (float)location.y, nullptr,
        Application::PointerType::IndirectPointer);
    [self dispatchMouseEvent:moveEvent];
}

#pragma mark - Touch Handlers

// A single tracked touch (Pencil preferred) drives the pointer/button stream;
// multi-finger interaction is owned entirely by the gesture recognizers.
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    if (self.primaryTouch != nil) {
        // A second finger while the primary is down: gestures take over.
        return;
    }

    // Prefer the Pencil if it's among the new touches (palm rejection).
    UITouch* chosen = nil;
    for (UITouch* touch in touches) {
        if (touch.type == UITouchTypePencil) { chosen = touch; break; }
    }
    if (!chosen) chosen = [touches anyObject];
    self.primaryTouch = chosen;

    self.windowData->m_mousePressStartLeft = std::chrono::high_resolution_clock::now();
    CGPoint location = [chosen locationInView:self];

    Application::InputManager::OnTouchDown((float)location.x, (float)location.y, 1);

    Application::MouseButtonPressedEvent pressEvent(0, (__bridge_retained void*)event,
        PointerTypeFromTouch(chosen), PressureFromTouch(chosen));
    [self dispatchMouseEvent:pressEvent];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UITouch* primary = self.primaryTouch;
    if (primary == nil || ![touches containsObject:primary]) {
        return;
    }

    CGPoint location = [primary locationInView:self];
    Application::InputManager::OnTouchMoved((float)location.x, (float)location.y);

    Application::MouseMovedEvent moveEvent((float)location.x, (float)location.y, (__bridge_retained void*)event,
        PointerTypeFromTouch(primary), PressureFromTouch(primary));
    [self dispatchMouseEvent:moveEvent];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UITouch* primary = self.primaryTouch;
    if (primary == nil || ![touches containsObject:primary]) {
        return;
    }
    self.primaryTouch = nil;

    self.windowData->m_mousePressEndLeft = std::chrono::high_resolution_clock::now();
    CGPoint location = [primary locationInView:self];

    Application::InputManager::OnTouchUp((float)location.x, (float)location.y);

    Application::MouseButtonReleasedEvent releaseEvent(0,
        std::chrono::duration_cast<std::chrono::milliseconds>(self.windowData->m_mousePressEndLeft - self.windowData->m_mousePressStartLeft),
        (__bridge_retained void*)event, PointerTypeFromTouch(primary));
    [self dispatchMouseEvent:releaseEvent];
}

// Also handle touches being cancelled (e.g. when a gesture recognizer claims
// them, or by a system event) — release the synthetic button so drags end.
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self touchesEnded:touches withEvent:event];
}

#pragma mark - Keyboard Handlers

- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    for (UIPress *press in presses) {
        if (!press.key) continue;
        Application::KeyCode keyCode = (Application::KeyCode)KeyCodeFromHIDUsage(press.key.keyCode);
        Application::InputManager::OnKeyPress(keyCode);
        Application::KeyPressedEvent keyEvent(keyCode, 0);
        [self dispatchMouseEvent:keyEvent];
    }
    [super pressesBegan:presses withEvent:event];
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    for (UIPress *press in presses) {
        if (!press.key) continue;
        Application::KeyCode keyCode = (Application::KeyCode)KeyCodeFromHIDUsage(press.key.keyCode);
        Application::InputManager::OnKeyRelease(keyCode);
        Application::KeyReleasedEvent keyEvent(keyCode);
        [self dispatchMouseEvent:keyEvent];
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

    Graphics::GraphicsContext* IOSWindow::GetRenderContext() const {
        return m_GraphicsContext.get();
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
