// Created by Anthropic Claude in a vibe-coding session.
// There are likely cleaner and/or more modern ways to accomplish this
// functionality, but it considered "good enough" as-is.

#import "AboutWindowController.h"

@interface AboutWindowController ()
@property (nonatomic, strong) NSWindow *aboutWindow;
@property (nonatomic, strong) NSTextView *creditsTextView;
@end

@implementation AboutWindowController

+ (instancetype)sharedController {
    static AboutWindowController *sharedController = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedController = [[self alloc] init];
    });
    return sharedController;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        [self createAboutWindow];
    }
    return self;
}

- (void)createAboutWindow {
    // Create resizable window
    NSRect frame = NSMakeRect(0, 0, 600, 840);
    NSWindowStyleMask styleMask = NSWindowStyleMaskTitled |
                                   NSWindowStyleMaskClosable |
                                   NSWindowStyleMaskResizable;

    self.aboutWindow = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:styleMask
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    [self.aboutWindow setTitle:@"About Acid Warp"];
    [self.aboutWindow setMinSize:NSMakeSize(300, 350)];
    [self.aboutWindow setReleasedWhenClosed:NO];
    [self.aboutWindow center];

    // Create content view
    NSView *contentView = [[NSView alloc] initWithFrame:frame];

    // App icon
    NSImageView *iconView = [[NSImageView alloc] initWithFrame:NSMakeRect(0, 0, 320, 240)];
    iconView.image = [NSImage imageNamed:@"about"];
    iconView.imageScaling = NSImageScaleProportionallyUpOrDown;
    iconView.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:iconView];

    // App name
    NSTextField *nameLabel = [[NSTextField alloc] init];
    nameLabel.stringValue = @"Acid Warp";
    nameLabel.font = [NSFont boldSystemFontOfSize:20];
    nameLabel.alignment = NSTextAlignmentCenter;
    nameLabel.bordered = NO;
    nameLabel.editable = NO;
    nameLabel.drawsBackground = NO;
    nameLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:nameLabel];

    // Version
    NSTextField *versionLabel = [[NSTextField alloc] init];
    NSString *version = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
    NSString *build = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];
    versionLabel.stringValue = [NSString stringWithFormat:@"Version %@ (%@)", version, build];
    versionLabel.alignment = NSTextAlignmentCenter;
    versionLabel.bordered = NO;
    versionLabel.editable = NO;
    versionLabel.drawsBackground = NO;
    versionLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:versionLabel];

    // Copyright
    NSTextField *copyrightLabel = [[NSTextField alloc] init];
    NSString *copyright = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"NSHumanReadableCopyright"];
    copyrightLabel.stringValue = copyright ?: @"";
    copyrightLabel.alignment = NSTextAlignmentCenter;
    copyrightLabel.bordered = NO;
    copyrightLabel.editable = NO;
    copyrightLabel.drawsBackground = NO;
    copyrightLabel.font = [NSFont systemFontOfSize:11];
    copyrightLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:copyrightLabel];

    // Credits scroll view
    NSScrollView *scrollView = [[NSScrollView alloc] init];
    scrollView.hasVerticalScroller = YES;
    scrollView.borderType = NSBezelBorder;
    scrollView.translatesAutoresizingMaskIntoConstraints = NO;

    self.creditsTextView = [[NSTextView alloc] init];
    self.creditsTextView.editable = NO;
    self.creditsTextView.drawsBackground = YES;

    // Use system colors that adapt to Light/Dark mode
    self.creditsTextView.textColor = [NSColor labelColor];
    self.creditsTextView.backgroundColor = [NSColor textBackgroundColor];

    [self loadCredits];

    scrollView.documentView = self.creditsTextView;
    [contentView addSubview:scrollView];

    // Set up constraints
    [NSLayoutConstraint activateConstraints:@[
        // Icon
        [iconView.topAnchor constraintEqualToAnchor:contentView.topAnchor constant:20],
        [iconView.centerXAnchor constraintEqualToAnchor:contentView.centerXAnchor],
        [iconView.widthAnchor constraintEqualToConstant:128],
        [iconView.heightAnchor constraintEqualToConstant:128],

        // Name
        [nameLabel.topAnchor constraintEqualToAnchor:iconView.bottomAnchor constant:12],
        [nameLabel.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:20],
        [nameLabel.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-20],

        // Version
        [versionLabel.topAnchor constraintEqualToAnchor:nameLabel.bottomAnchor constant:4],
        [versionLabel.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:20],
        [versionLabel.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-20],

        // Copyright
        [copyrightLabel.topAnchor constraintEqualToAnchor:versionLabel.bottomAnchor constant:8],
        [copyrightLabel.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:20],
        [copyrightLabel.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-20],

        // Credits
        [scrollView.topAnchor constraintEqualToAnchor:copyrightLabel.bottomAnchor constant:16],
        [scrollView.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:20],
        [scrollView.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-20],
        [scrollView.bottomAnchor constraintEqualToAnchor:contentView.bottomAnchor constant:-20]
    ]];

    [self.aboutWindow setContentView:contentView];
}

- (void)loadCredits {
    // Try to load Credits.rtf
    NSString *creditsPath = [[NSBundle mainBundle] pathForResource:@"Credits" ofType:@"rtfd"];
    if (creditsPath) {
        NSURL *creditsURL = [NSURL fileURLWithPath:creditsPath];
        NSAttributedString *credits = [[NSAttributedString alloc] initWithURL:creditsURL
                                                                       options:@{}
                                                            documentAttributes:nil
                                                                         error:nil];
        if (credits) {
            // Create a mutable copy and update colors to adapt to Dark mode
            NSMutableAttributedString *mutableCredits = [credits mutableCopy];
            NSRange fullRange = NSMakeRange(0, [mutableCredits length]);

            // Remove any hardcoded colors and use system label color
            [mutableCredits removeAttribute:NSForegroundColorAttributeName range:fullRange];
            [mutableCredits addAttribute:NSForegroundColorAttributeName
                                   value:[NSColor labelColor]
                                   range:fullRange];

            [self.creditsTextView.textStorage setAttributedString:mutableCredits];
            return;
        }
    }

    // Fallback to plain text
    self.creditsTextView.string = @"Credits:\nDOS (original): Noah Spurrier & Mark Bilk\nLinux: Steven Wills\nAmiga: megacz\nSDL: Boris Gjenero\nAndroid, iOS, iPadOS, macOS: Matthew Zavislak ";
}

- (void)showWindow {
    [self.aboutWindow makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

@end
