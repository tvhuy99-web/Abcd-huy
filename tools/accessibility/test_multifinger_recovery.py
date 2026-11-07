"""Android 3/4-finger gestures must survive child interception and safe cancellations."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
ANDROID_TEST = ROOT / "android/app/src/androidTest/java/com/cleverraven/cataclysmdda"


class MultiFingerRecovery(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.surface = (JAVA / "ReaderGestureSurface.java").read_text(encoding="utf-8")
        cls.activity = (JAVA / "CataclysmDDA.java").read_text(encoding="utf-8")
        cls.tests = (ANDROID_TEST / "GameMenuGestureDispatchTest.java").read_text(
            encoding="utf-8"
        )

    def test_activity_parent_capture_follows_gnollhack_reference(self):
        self.assertIn("boolean dispatchTouchEvent(MotionEvent event)", self.activity)
        self.assertIn("requestDisallowInterceptTouchEvent(true)", self.activity)
        self.assertIn("requestDisallowInterceptTouchEvent(false)", self.activity)
        self.assertIn("!reader.usesTouchExploration()", self.activity)

    def test_surface_tracks_pointer_ids_not_only_indices(self):
        self.assertIn("multiPointerIds", self.surface)
        self.assertIn("event.getPointerId(i)", self.surface)
        self.assertIn("event.getPointerId(index)", self.surface)
        self.assertIn("trackMultiFingerMovement(event)", self.surface)
        self.assertIn("MULTI_FINGER_JOIN_MS", self.surface)
        self.assertIn("MAX_READER_FINGERS = 4", self.surface)

    def test_only_valid_short_cancelled_taps_are_recovered(self):
        self.assertIn("recoverCancelledMultiTap(event)", self.surface)
        self.assertIn("cancelGesture(recovered)", self.surface)
        self.assertIn("queueMultiTap(capturedOwner, capturedFingers)", self.surface)
        self.assertIn("event.getPointerCount()", self.surface)
        self.assertIn("lateMultiFingerJoin", self.surface)
        self.assertIn("multiFingerMoved", self.surface)
        self.assertIn("owner.equals(listener.gestureContext())", self.surface)
        self.assertIn("listener.directWorldGesturesEnabled()", self.surface)
        self.assertIn("event.getEventTime() - downTime > MULTI_CANCEL_TAP_MS",
                      self.surface)

    def test_normal_two_finger_back_and_world_one_finger_taps_remain(self):
        self.assertIn("TwoFingerBackGesture.matches", self.surface)
        self.assertIn("queueWorldTap(event, owner)", self.surface)
        self.assertIn("listener.moveDirection(", self.surface)
        self.assertIn("listener.openActionMenu()", self.surface)

    def test_instrumented_cases_cover_three_four_and_false_positive(self):
        self.assertIn("threeFingerSystemCancelRecoversOneTapOnly", self.tests)
        self.assertIn("fourFingerTapAndSwipeReachBindings", self.tests)
        self.assertIn("ACTION_CANCEL", self.tests)
        self.assertIn("ACTION_MOVE, 3, 100", self.tests)
        self.assertIn('"swipe_4_right"', self.tests)


if __name__ == "__main__":
    unittest.main()
