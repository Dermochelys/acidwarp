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
    // Use a static counter to handle multiple test runs
    static int screenshotCounter = 0;
    screenshotCounter++;

    NSString *screenshotName;
    if (screenshotCounter == 1) {
        screenshotName = @"Launch Screen";
    } else {
        screenshotName = [NSString stringWithFormat:@"Launch Screen %d", screenshotCounter];
    }

    XCTAttachment *attachment = [XCTAttachment attachmentWithScreenshot:XCUIScreen.mainScreen.screenshot];
    attachment.name = screenshotName;
    attachment.lifetime = XCTAttachmentLifetimeKeepAlways;
    [self addAttachment:attachment];
}

@end
