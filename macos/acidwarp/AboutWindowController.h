// Created by Anthropic Claude in a vibe-coding session.
// There are likely cleaner and/or more modern ways to accomplish this
// functionality, but it considered "good enough" as-is.

#import <Cocoa/Cocoa.h>

@interface AboutWindowController : NSWindowController

+ (instancetype)sharedController;
- (void)showWindow;

@end
