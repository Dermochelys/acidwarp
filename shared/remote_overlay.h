/* Remote Control Overlay for Acid Warp
 * Ported to Android, iOS / iPadOS, macOS, Linux, Windows by Matthew Zavislak
 */

#ifndef REMOTE_OVERLAY_H
#define REMOTE_OVERLAY_H

#include "handy.h"

/* Initialize the remote overlay system */
void remote_overlay_init(void);

/* Clean up remote overlay resources */
void remote_overlay_cleanup(void);

/* Render the overlay if it's visible */
void remote_overlay_render_if_visible(int window_width, int window_height);

/* Render dim feedback if active (call this after rendering overlay) */
void remote_overlay_render_dim(int window_width, int window_height);

/* Handle mouse/touch click events
 * Returns: -1 if click was outside overlay bounds
 *           0 if click was on overlay but not on a button
 *           1 if a button was clicked and key event was generated
 */
int remote_overlay_handle_click(int x, int y, int window_width, int window_height);

/* Check if coordinates are within the remote overlay bounds */
int remote_overlay_is_point_inside(int x, int y, int window_width, int window_height);

/* Show or hide the overlay */
void remote_overlay_show(void);
void remote_overlay_hide(void);
int remote_overlay_is_visible(void);

#endif /* REMOTE_OVERLAY_H */
