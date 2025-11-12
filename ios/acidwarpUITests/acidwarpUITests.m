//
//  acidwarpUITests.m
//  acidwarpUITests
//
//  Created by Matthew Zavislak on 9/9/25.
//

#import <XCTest/XCTest.h>

@interface acidwarpUITests : XCTestCase

@end

@implementation acidwarpUITests

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

    // Test pattern cycling with n key (using keyboard commands)
    for (int i = 1; i <= 5; i++) {
        [app typeKey:@"n" modifierFlags:XCUIKeyModifierNone];
        sleep(4); // Wait for fade-out + fade-in to complete

        // Capture screenshot
        NSString *screenshotName = [NSString stringWithFormat:@"02-pattern-%d", i];
        [self takeScreenshotNamed:screenshotName];
    }

    // Test tap in center of screen (iOS uses touch instead of mouse)
    // On iOS, tap the main window/screen element
    XCUIElement *window = app.windows.firstMatch;
    XCTAssertTrue(window.exists, @"Window should exist");

    // Wait for window to be hittable/accessible
    NSPredicate *existsPredicate = [NSPredicate predicateWithFormat:@"exists == YES"];
    [self expectationForPredicate:existsPredicate evaluatedWithObject:window handler:nil];
    [self waitForExpectationsWithTimeout:5.0 handler:nil];

    // Create coordinate relative to the window and tap
    XCUICoordinate *centerCoordinate = [window coordinateWithNormalizedOffset:CGVectorMake(0.5, 0.5)];
    [centerCoordinate tap];
    sleep(1);
    [self takeScreenshotNamed:@"03-after-tap"];

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
