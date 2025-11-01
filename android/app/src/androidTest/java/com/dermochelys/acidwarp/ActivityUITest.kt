package com.dermochelys.acidwarp

import android.content.Context
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
import kotlin.time.Duration.Companion.seconds

private const val ERROR_STRING = "SDL Error"

@RunWith(AndroidJUnit4::class)
@LargeTest
class ActivityUITest {

    @get:Rule
    val activityRule = ActivityScenarioRule(Activity::class.java)

    private lateinit var device: UiDevice

    private lateinit var context: Context

    @Before
    fun setUp() {
        device = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation())
        context = ApplicationProvider.getApplicationContext()
    }

    @Test
    fun when_appIsLaunched_sdlErrorDialogShouldNotAppear() {
        // Wait for the activity to launch
        device.wait(Until.hasObject(By.pkg(context.packageName)), 10.seconds.inWholeMilliseconds)

        // Wait a moment for any potential dialogs to appear
        Thread.sleep(2.seconds.inWholeMilliseconds)

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
    }
}
