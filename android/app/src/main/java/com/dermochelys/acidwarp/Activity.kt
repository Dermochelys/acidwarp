/* ACID WARP (c)Copyright 1992, 1993 by Noah Spurrier
 * All Rights reserved. Private Proprietary Source Code by Noah Spurrier
 * Ported to Linux by Steven Wills
 * Ported to SDL by Boris Gjenero
 * Ported to Android / iOS / iPadOS / macOS / Windows by Matthew Zavislak
 */

package com.dermochelys.acidwarp
import android.view.KeyEvent
import org.libsdl.app.SDLActivity

class Activity(): SDLActivity() {
    /** See [org.libsdl.app.SDLActivity.handleKeyEvent] */
    override fun dispatchKeyEvent(event: KeyEvent?): Boolean {
        // Handle back press while connected to a TV (not the TV remote back button) through ADB
        // as that reports SOURCE_MOUSE and triggers the eating of the event in SDLActivity.
        if (event?.keyCode == 4) {
            finish()
            sendCommand(3, 1) // CMD_QUIT
            return true
        }

        return super.dispatchKeyEvent(event)
    }
}
