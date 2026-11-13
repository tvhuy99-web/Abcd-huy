"""Gesture contracts: single-tap silence must never swallow double-tap activation."""
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

    def test_activity_does_not_treat_every_finger_down_as_pause(self):
        dispatch = self.activity.split(
            "public boolean dispatchTouchEvent(MotionEvent event)", 1)[1].split(
            "public void onBackPressed()", 1)[0]
        self.assertNotIn("silenceSpeechOnTouchDown()", dispatch)
        self.assertIn("!reader.usesTouchExploration()", dispatch)

    def test_silence_only_after_confirmed_single_tap_in_menu(self):
        surface = self.gesture
        single = surface.split("public boolean onSingleTapConfirmed(MotionEvent event)", 1)[1].split(
            "public boolean onDoubleTap(MotionEvent event)", 1)[0]
        self.assertIn("!listener.directWorldGesturesEnabled()", single)
        self.assertIn("previousTapOwner.equals(listener.gestureContext())", single)
        self.assertIn("listener.stopSpeech();", single)
        self.assertNotIn("listener.activate();", single)
        self.assertIn("previousTapOwner = tap ? owner : null;", surface)
        self.assertNotIn("touchSilencedSpeech", surface)

    def test_world_single_double_triple_tap_are_exclusive(self):
        world = self.gesture.split("private void queueWorldTap(", 1)[1].split(
            "private void queueMultiTap(", 1)[0]
        self.assertIn("if (count == 1)", world)
        self.assertIn("listener.stopSpeech();", world)
        self.assertIn("else if (count == 2)", world)
        self.assertIn("listener.activate();", world)
        self.assertIn("worldTapCount >= 3", world)
        self.assertIn("listener.openActionMenu();", world)
        self.assertIn("queueWorldTap(event, owner)", self.gesture)

    def test_swipe_and_hold_remain_separate_gestures(self):
        self.assertIn("listener.openContextActions();", self.gesture)
        self.assertIn("listener.confirm();", self.gesture)
        self.assertIn("listener.goBack();", self.gesture)
        self.assertIn("handleSwipe(event.getX() - firstX", self.gesture)

    def test_speech_stop_clears_queue_and_audio(self):
        stop = self.queue.split("public void stop()", 1)[1].split(
            "public boolean stopForTouch()", 1)[0]
        self.assertIn("clearRequests();", stop)
        self.assertIn("speech.cancel();", stop)
        # STT 175: a single tap toggles speech — it still stops while
        # anything is speaking or queued, and resumes from the cut-off
        # chunk only when the queue is idle.
        speech = self.controller.split("public void stopSpeech()", 1)[1].split(
            "public void onBackPressed()", 1)[0]
        self.assertIn("speech.stop();", speech)
        self.assertIn("speech.resume();", speech)


if __name__ == "__main__":
    unittest.main()
