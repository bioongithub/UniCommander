#import <Cocoa/Cocoa.h>
#include "cocoa_window.h"

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    return YES;
}
@end

CocoaWindow::CocoaWindow()  = default;
CocoaWindow::~CocoaWindow() = default;

bool CocoaWindow::create(const std::string& title, int width, int height)
{
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    m_delegate = [[AppDelegate alloc] init];
    [NSApp setDelegate:m_delegate];

    NSRect frame = NSMakeRect(0, 0, width, height);
    NSUInteger style = NSWindowStyleMaskTitled
                     | NSWindowStyleMaskClosable
                     | NSWindowStyleMaskResizable
                     | NSWindowStyleMaskMiniaturizable;

    m_window = [[NSWindow alloc] initWithContentRect:frame
                                           styleMask:style
                                             backing:NSBackingStoreBuffered
                                               defer:NO];

    [m_window setTitle:[NSString stringWithUTF8String:title.c_str()]];
    [m_window center];
    return m_window != nullptr;
}

void CocoaWindow::show()
{
    if (m_window)
        [m_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

void CocoaWindow::run()
{
    [NSApp run];
}

void CocoaWindow::close()
{
    if (m_window)
        [m_window close];
}

// Factory
std::unique_ptr<Window> createWindow()
{
    return std::make_unique<CocoaWindow>();
}
