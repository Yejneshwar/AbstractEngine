#include "Starter.h"
#import <Foundation/Foundation.h>
#include <memory>

#if TARGET_OS_IOS
#import <UIKit/UIKit.h>
#elif TARGET_OS_OSX
#import <AppKit/AppKit.h>
#endif

// --- SceneDelegate Interface (iOS Only) ---
// Manages a single instance of the app's UI (a "scene" or "window").
#if TARGET_OS_IOS
@interface SceneDelegate : UIResponder <UIWindowSceneDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (strong, nonatomic) CADisplayLink *displayLink;
@property (nonatomic, assign) GUI::TestGUI* cppApp;

@end
#endif


// --- Application Delegate Interface ---
#if TARGET_OS_IOS
@interface AppDelegate : UIResponder <UIApplicationDelegate>
// No properties are needed
@end

#elif TARGET_OS_OSX
@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong, nonatomic) NSWindow *window;
@property (strong, nonatomic) NSTimer *displayLink;
@property (nonatomic, assign) GUI::TestGUI* cppApp;
@end
#endif


#pragma mark - SceneDelegate Implementation (iOS Only)
#if TARGET_OS_IOS
@implementation SceneDelegate

// Entry point for UI setup.
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions {
    if (![scene isKindOfClass:[UIWindowScene class]]) {
        return;
    }
    UIWindowScene *windowScene = (UIWindowScene *)scene;
    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
    
    self.cppApp = GUI::CreateApplication(GUI::ApplicationCommandLineArgs(), (__bridge_retained void*)self.window);
    
    [self.window makeKeyAndVisible];
    
    // Set up the render loop using CADisplayLink for screen-synchronized updates.
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(renderLoop)];
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
}

- (void)renderLoop {
    if (self.cppApp) {
        self.cppApp->RunLoop();
    }
}

// Called when the scene is disconnected. This is a good place for cleanup.
- (void)sceneDidDisconnect:(UIScene *)scene {
    // Invalidate the display link to stop the render loop.
    [self.displayLink invalidate];
    self.displayLink = nil;
    
    // Clean up the C++ application instance.
    if (self.cppApp) {
        delete self.cppApp;
        self.cppApp = nullptr;
    }
}

@end
#endif // TARGET_OS_IOS


#pragma mark - AppDelegate Implementation
@implementation AppDelegate

// --- iOS-Specific Implementation ---
#if TARGET_OS_IOS
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // App-level setup
    return YES;
}

#pragma mark - UISceneSession Lifecycle

// This method tells iOS how to create a new scene.
- (UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options {
    return [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole:connectingSceneSession.role];
}

// Called when a scene is discarded by the user.
- (void)application:(UIApplication *)application didDiscardSceneSessions:(NSSet<UISceneSession *> *)sceneSessions {
    // Release any resources associated with the discarded scenes here.
}

#endif // TARGET_OS_IOS


// --- macOS-Specific Implementation ---
#if TARGET_OS_OSX
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    NSRect windowRect = NSMakeRect(0, 0, 1024, 768);
    
    self.window = [[NSWindow alloc] initWithContentRect:windowRect
                                              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window center];
    [self.window setTitle:@"Abstract Engine"];
    
    // Create the C++ Application, passing the native NSWindow to it.
    self.cppApp = GUI::CreateApplication(GUI::ApplicationCommandLineArgs(), (__bridge_retained void*)self.window);
    
    [self.window makeKeyAndOrderFront:nil];
    [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
    
    // Set up a timer for the render loop.
    self.displayLink = [NSTimer scheduledTimerWithTimeInterval:(1.0 / 60.0) // Target 60 FPS
                                                        target:self
                                                      selector:@selector(renderLoop:)
                                                      userInfo:nil
                                                       repeats:YES];
}

- (void)renderLoop:(NSTimer *)timer {
    if (self.cppApp) {
        self.cppApp->RunLoop();
    }
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}
#endif // TARGET_OS_OSX

@end


// --- Main Application Entry Point ---
int main(int argc, char * argv[]) {
    @autoreleasepool {
#if TARGET_OS_IOS
        // Bootstrap the iOS application. Automatically handles the SceneDelegate.
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
#elif TARGET_OS_OSX
        // Bootstrap the macOS application.
        NSApplication *application = [NSApplication sharedApplication];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [application setDelegate:delegate];
        return NSApplicationMain(argc, (const char **)argv);
#endif
    }
}
