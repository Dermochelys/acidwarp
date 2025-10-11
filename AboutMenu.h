/* AboutMenu.h - Cross-platform interface for About window/dialog
 * Part of Acid Warp
 *
 * This header defines a platform-agnostic interface for displaying
 * an About window or dialog. Each platform (macOS, iOS, Android)
 * should provide its own implementation.
 */

/**
 * Set up the platform-specific About menu/dialog when used on an OS with a GUI menubar.
 * Can only be called after SDL_Init.
 *
 * To enable:
 * #define CUSTOM_ABOUT_MENU 1
 */
void setupCustomAboutMenu(void);
