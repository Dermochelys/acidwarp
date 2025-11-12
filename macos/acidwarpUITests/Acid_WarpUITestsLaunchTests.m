//
//  Acid_WarpUITestsLaunchTests.m
//  Acid WarpUITests
//
//  Created by Matthew Zavislak on 10/4/25.
//

#import <XCTest/XCTest.h>

@interface Acid_WarpUITestsLaunchTests : XCTestCase

@end

@implementation Acid_WarpUITestsLaunchTests

+ (BOOL)runsForEachTargetApplicationUIConfiguration {
    return YES;
}

- (void)setUp {
    self.continueAfterFailure = NO;
}

- (void)testLaunch {
    XCUIApplication *app = [[XCUIApplication alloc] init];
    // Disable idle waiting since Acid Warp continuously animates
    app.waitForIdleTimeout = 0;
    [app launch];

    // Insert steps here to perform after app launch but before taking a screenshot,
    // such as logging into a test account or navigating somewhere in the app

    XCTAttachment *attachment = [XCTAttachment attachmentWithScreenshot:XCUIScreen.mainScreen.screenshot];
    attachment.name = @"Launch Screen";
    attachment.lifetime = XCTAttachmentLifetimeKeepAlways;
    [self addAttachment:attachment];
}

@end
