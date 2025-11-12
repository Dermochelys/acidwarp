//
//  Acid_WarpUITests.m
//  Acid WarpUITests
//
//  Created by Matthew Zavislak on 10/4/25.
//

#import <XCTest/XCTest.h>

@interface Acid_WarpUITests : XCTestCase

@end

@implementation Acid_WarpUITests

- (void)setUp {
    // Put setup code here. This method is called before the invocation of each test method in the class.

    // In UI tests it is usually best to stop immediately when a failure occurs.
    self.continueAfterFailure = NO;

    // In UI tests it's important to set the initial state - such as interface orientation - required for your tests before they run. The setUp method is a good place to do this.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
}

- (void)takeScreenshotNamed:(NSString *)name {
    XCTAttachment *attachment = [XCTAttachment attachmentWithScreenshot:XCUIScreen.mainScreen.screenshot];
    attachment.name = name;
    attachment.lifetime = XCTAttachmentLifetimeKeepAlways;
    [self addAttachment:attachment];
}

- (void)testAppLaunchAndInteraction {
    // Launch the application
    XCUIApplication *app = [[XCUIApplication alloc] init];
    [app launch];

    // Wait for app initialization
    sleep(5);

    // Capture startup screenshot
    [self takeScreenshotNamed:@"01-startup"];

    // Check for SDL error alerts
    XCUIElement *alert = app.alerts[@"SDL Error"];
    XCTAssertFalse(alert.exists, @"SDL Error dialog should not appear on launch");

    // Test pattern cycling with space key
    for (int i = 1; i <= 5; i++) {
        [app typeKey:@" " modifierFlags:XCUIKeyModifierNone];
        sleep(2);

        // Capture screenshot
        NSString *screenshotName = [NSString stringWithFormat:@"02-pattern-%d", i];
        [self takeScreenshotNamed:screenshotName];
    }

    // Test mouse click in center of window
    XCUICoordinate *centerCoordinate = [app coordinateWithNormalizedOffset:CGVectorMake(0.5, 0.5)];
    [centerCoordinate click];
    sleep(1);
    [self takeScreenshotNamed:@"03-after-click"];

    // Test palette change with 'p' key
    [app typeKey:@"p" modifierFlags:XCUIKeyModifierNone];
    sleep(1);
    [self takeScreenshotNamed:@"04-palette-change"];

    // Capture final state
    sleep(2);
    [self takeScreenshotNamed:@"05-final"];
}

- (void)testLaunchPerformance {
    // This measures how long it takes to launch your application.
    [self measureWithMetrics:@[[[XCTApplicationLaunchMetric alloc] init]] block:^{
        XCUIApplication *app = [[XCUIApplication alloc] init];
        [app launch];
    }];
}

@end
