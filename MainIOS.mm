#include "Starter.h"
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#include <memory>

// --- The Application Delegate ---
// This class is the core of the fix. It manages the app's lifecycle.
@interface AppDelegate : UIResponder <UIApplicationDelegate, UIWindowSceneDelegate>

@property (nonatomic, assign) GUI::TestGUI* cppApp;
@property (strong, nonatomic) CADisplayLink *displayLink;
@end

@implementation AppDelegate

// This is called when the app finishes launching.
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    UIWindow* nativeWindow = [[UIWindow alloc] initWithFrame:screenBounds];
    
    // Create the C++ Application, passing the native UIWindow to it.
    self.cppApp = GUI::CreateApplication(GUI::ApplicationCommandLineArgs(), (__bridge_retained void*)nativeWindow);
    
    // Make the window visible.
    [nativeWindow makeKeyAndVisible];

    // Set up the render loop using CADisplayLink for screen-synchronized updates.
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(renderLoop)];
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
    return YES;
}

// The main render loop, called by CADisplayLink.
- (void)renderLoop {
    self.cppApp->RunLoop();
}

@end


// --- Main Application Entry Point ---
// This replaces your C++ main() and correctly starts the iOS application.
int main(int argc, char * argv[]) {
    @autoreleasepool {
        // This function starts the application run loop and sets up the delegate.
        // It does not return until the application exits.
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
