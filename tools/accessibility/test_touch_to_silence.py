"""Regression contract: one finger immediately silences game speech everywhere."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class TouchToSilenceTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.activity = (JAVA / "CataclysmDDA.java").read_text(encoding="utf-8")
        cls.controller = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        cls.gesture = (JAVA / "ReaderGestureSurface.java").read_text(encoding="utf-8")
        cls.queue = (JAVA / "ReaderSpeechQueue.java").read_text(encoding="utf-8")

    def test_activity_stops_on_initial_down_before_dispatch(self):
        dispatch = self.activity.split(
            "public boolean dispatchTouchEvent(MotionEvent event)", 1)[1].split(
            "public void onBackPressed()", 1)[0]
        self.assertIn("action == MotionEvent.ACTION_DOWN", dispatch)
        self.assertIn("reader.silenceSpeechOnTouchDown();", dispatch)
        self.assertLess(dispatch.index("reader.silenceSpeechOnTouchDown();"),
                        dispatch.index("return super.dispatchTouchEvent(event);"))
        self.assertNotIn("ACTION_UP) {\n                reader.silenceSpeechOnTouchDown", dispatch)
        self.assertIn("!reader.usesTouchExploration()", dispatch)

    def test_speech_queue_interrupts_active_and_queued_audio(self):
        touch = self.queue.split("public boolean stopForTouch()", 1)[1].split(
            "public void clearForLifecycle()", 1)[0]
        self.assertIn("active != null || !pending.isEmpty()", touch)
        self.assertIn("stop();", touch)
        self.assertIn("return interrupted;", touch)
        stop = self.queue.split("public void stop()", 1)[1].split(
            "public boolean stopForTouch()", 1)[0]
        self.assertIn("clearRequests();", stop)
        self.assertIn("speech.cancel();", stop)

    def test_controller_does_not_interrupt_talkback(self):
        touch = self.controller.split("public void silenceSpeechOnTouchDown()", 1)[1].split(
            "public void onBackPressed()", 1)[0]
        self.assertIn("!preferences.isEnabled()", touch)
        self.assertIn("usesTouchExploration()", touch)
        self.assertIn("root.setTouchSilencedSpeech(speech.stopForTouch());", touch)

    def test_stopping_touch_does_not_trigger_world_tap(self):
        self.assertIn("void setTouchSilencedSpeech(boolean interrupted)", self.gesture)
        self.assertIn("previousTapOwner = tap && !touchSilencedSpeech ? owner : null;",
                      self.gesture)
        self.assertIn("listener.directWorldGesturesEnabled() && !touchSilencedSpeech",
                      self.gesture)
        self.assertIn("queueWorldTap(event, owner)", self.gesture)
        self.assertIn("touchSilencedSpeech = false;", self.gesture)
        # Gestures remain intact for swipes and future taps.
        self.assertIn("listener.confirm();", self.gesture)
        self.assertIn("listener.goBack();", self.gesture)
        self.assertIn("handleSwipe(event.getX() - firstX", self.gesture)


if __name__ == "__main__":
    unittest.main()
