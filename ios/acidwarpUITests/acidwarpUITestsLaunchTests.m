//
//  acidwarpUITestsLaunchTests.m
//  acidwarpUITests
//
//  Created by Matthew Zavislak on 9/9/25.
//

#import <XCTest/XCTest.h>

@interface acidwarpUITestsLaunchTests : XCTestCase

@end

@implementation acidwarpUITestsLaunchTests

+ (BOOL)runsForEachTargetApplicationUIConfiguration {
    return YES;
}

- (void)setUp {
    self.continueAfterFailure = NO;
}

- (void)testLaunch {
    XCUIApplication *app = [[XCUIApplication alloc] init];
    [app launch];

    // Insert steps here to perform after app launch but before taking a screenshot,
    // such as logging into a test account or navigating somewhere in the app

    // This test runs for each UI configuration (Light/Dark mode) due to
    // runsForEachTargetApplicationUIConfiguration returning YES.
    // Detect the actual appearance mode to generate unique screenshot names.
    NSString *appearance = app.userInterfaceStyle == XCUIUserInterfaceStyleDark ? @"Dark" : @"Light";

    // Use a static counter for each appearance to handle multiple runs
    static NSMutableDictionary *counters = nil;
    if (!counters) {
        counters = [NSMutableDictionary dictionary];
    }

    NSNumber *count = counters[appearance];
    int currentCount = count ? [count intValue] + 1 : 1;
    counters[appearance] = @(currentCount);

    NSString *screenshotName;
    if (currentCount == 1) {
        screenshotName = [NSString stringWithFormat:@"Launch Screen %@", appearance];
    } else {
        screenshotName = [NSString stringWithFormat:@"Launch Screen %@ %d", appearance, currentCount];
    }

    XCTAttachment *attachment = [XCTAttachment attachmentWithScreenshot:XCUIScreen.mainScreen.screenshot];
    attachment.name = screenshotName;
    attachment.lifetime = XCTAttachmentLifetimeKeepAlways;
    [self addAttachment:attachment];
}

@end
