// Created by Anthropic Claude in a vibe-coding session.
// There are likely cleaner and/or more modern ways to accomplish this
// functionality, but it considered "good enough" as-is.

#import <Cocoa/Cocoa.h>
#import "acidwarp/AboutMenu.h"
#import "AboutWindowController.h"

// Add a category to NSObject so any object in the responder chain can handle this
@interface NSObject (AcidWarpAbout)
- (void)showCustomAboutWindow:(id)sender;
@end

void setupCustomAboutMenu(void) {
    @autoreleasepool {
        // Find the About menu item that SDL created
        NSMenu *mainMenu = [[NSApplication sharedApplication] mainMenu];
        if (!mainMenu) return;

        NSMenu *appMenu = [[mainMenu itemAtIndex:0] submenu];
        if (!appMenu) return;

        // Find the About menu item (usually first item)
        for (NSMenuItem *item in [appMenu itemArray]) {
            NSString *title = [item title];
            if ([title containsString:@"About"]) {
                // Set a simple action that will show our window
                [item setAction:@selector(showCustomAboutWindow:)];
                [item setTarget:[NSApp delegate]];
                break;
            }
        }
    }
}

@implementation NSObject (AcidWarpAbout)

- (void)showCustomAboutWindow:(id)sender {
    [[AboutWindowController sharedController] showWindow];
}

@end
