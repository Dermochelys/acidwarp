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

    // First tap shows the remote control overlay
    XCUICoordinate *centerCoordinate = [app coordinateWithNormalizedOffset:CGVectorMake(0.5, 0.5)];
    [centerCoordinate tap];
    sleep(1);

    // Test pattern cycling by tapping "Next Pattern" button on overlay
    // The "Next Pattern" button is at the bottom of the centered overlay (y ~0.6)
    XCUICoordinate *nextPatternButton = [app coordinateWithNormalizedOffset:CGVectorMake(0.5, 0.6)];
    for (int i = 1; i <= 5; i++) {
        [nextPatternButton tap];
        sleep(2);

        // Capture screenshot
        NSString *screenshotName = [NSString stringWithFormat:@"02-pattern-%d", i];
        [self takeScreenshotNamed:screenshotName];
    }

    // Tap anywhere to re-show overlay (it may have auto-hidden)
    [centerCoordinate tap];
    sleep(1);
    [self takeScreenshotNamed:@"03-after-click"];

    // Test palette change by tapping "Next Palette" button on overlay
    // The "Next Palette" button is in the middle of the centered overlay (y ~0.5)
    XCUICoordinate *nextPaletteButton = [app coordinateWithNormalizedOffset:CGVectorMake(0.5, 0.5)];
    [nextPaletteButton tap];
    sleep(1);
    [self takeScreenshotNamed:@"04-palette-change"];

    // Capture final state
    sleep(2);
    [self takeScreenshotNamed:@"05-final"];
}

- (void)testLaunchPerformance {
    // This measures how long it takes to launch your application.
    [self measureWithMetrics:@[[[XCTApplicationLaunchMetric alloc] init]] block:^{
        [[[XCUIApplication alloc] init] launch];
    }];
}

@end
