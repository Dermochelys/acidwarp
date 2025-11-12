package com.dermochelys.acidwarp

import android.content.Context
import android.view.KeyEvent
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.rules.ActivityScenarioRule
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.LargeTest
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.uiautomator.By
import androidx.test.uiautomator.UiDevice
import androidx.test.uiautomator.Until
import junit.framework.TestCase.assertTrue
import junit.framework.TestCase.fail
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File
import kotlin.time.Duration.Companion.seconds

private const val ERROR_STRING = "SDL Error"

@RunWith(AndroidJUnit4::class)
@LargeTest
class ActivityUITest {

    @get:Rule
    val activityRule = ActivityScenarioRule(Activity::class.java)

    private lateinit var device: UiDevice

    private lateinit var context: Context

    private lateinit var screenshotDir: File

    @Before
    fun setUp() {
        device = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation())
        context = ApplicationProvider.getApplicationContext()

        // Create screenshot directory
        screenshotDir = File(context.getExternalFilesDir(null), "screenshots")
        screenshotDir.mkdirs()
    }

    private fun takeScreenshot(name: String) {
        val screenshotFile = File(screenshotDir, "$name.png")
        device.takeScreenshot(screenshotFile)
    }

    @Test
    fun when_appIsLaunched_sdlErrorDialogShouldNotAppear() {
        // Wait for the activity to launch
        device.wait(Until.hasObject(By.pkg(context.packageName)), 10.seconds.inWholeMilliseconds)

        // Wait for app initialization
        Thread.sleep(5.seconds.inWholeMilliseconds)

        // Capture startup screenshot
        takeScreenshot("01-startup")

        // Check that the "SDL Error" dialog is not showing
        val dllDialog = device.findObject(By.textContains(ERROR_STRING))
        assertTrue("Dialog containing '$ERROR_STRING' text should not appear on launch", dllDialog == null)

        // Also check for any dialogs in general that might contain DLL
        val allDialogs = device.findObjects(By.clazz("android.app.AlertDialog"))

        for (dialog in allDialogs) {
            val dialogText = dialog.text

            if (dialogText != null && dialogText.contains(ERROR_STRING, ignoreCase = true)) {
                fail("Found dialog containing '$ERROR_STRING': $dialogText")
            }
        }

        // Test pattern cycling with n key
        for (i in 1..5) {
            device.pressKeyCode(KeyEvent.KEYCODE_N)
            Thread.sleep(4.seconds.inWholeMilliseconds)  // Wait for fade-out + fade-in to complete
            takeScreenshot("02-pattern-$i")
        }

        // Test touch input - tap center of screen
        val displayWidth = device.displayWidth
        val displayHeight = device.displayHeight
        device.click(displayWidth / 2, displayHeight / 2)
        Thread.sleep(1.seconds.inWholeMilliseconds)
        takeScreenshot("03-after-click")

        // Test palette change
        device.pressKeyCode(KeyEvent.KEYCODE_P)
        Thread.sleep(1.seconds.inWholeMilliseconds)
        takeScreenshot("04-palette-change")

        // Capture final state
        Thread.sleep(2.seconds.inWholeMilliseconds)
        takeScreenshot("05-final")
    }
}
