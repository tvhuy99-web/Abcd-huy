"""Guard opt-in native spoken radar and independent cached contact tracking."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"


class MasterNativeRadar(unittest.TestCase):
    def test_disabled_by_default_and_only_in_native_world_context(self):
        native = (SRC / "a11y_android.cpp").read_text(encoding="utf-8")
        world = (SRC / "a11y_world.cpp").read_text(encoding="utf-8")
        self.assertIn("spoken_radar_mask{ 0 }", native)
        self.assertIn("spoken_radar_revision{ 0 }", native)
        self.assertIn("mask & 0xffu", native)
        self.assertIn("enabled() && !suspended()", native)
        self.assertIn("automatic_radar_mask() != 0 && !explore_active && !camera_offset", world)
        self.assertIn('context.category_id() == "DEFAULTMODE"', world)
        self.assertIn("a11y::update_automatic_radar( avatar_origin );", world)

    def test_jni_and_event_deduplication_exist(self):
        bridge = (SRC / "a11y_android.cpp").read_text(encoding="utf-8")
        controller = (JAVA / "CataclysmDDA.java").read_text(encoding="utf-8")
        radar = (SRC / "a11y_radar.cpp").read_text(encoding="utf-8")
        self.assertIn("nativeConfigureAutomaticRadar(", controller)
        self.assertIn("nativeConfigureAutomaticRadar(", bridge)
        self.assertIn("automatic_radar_revision()", radar)
        self.assertIn("A11Y_RADAR_", radar)
        header = (SRC / "a11y_radar.h").read_text(encoding="utf-8")
        self.assertIn("class entry_tracker", header)
        self.assertIn("class change_tracker", header)


    def test_radar_events_are_saved_to_existing_game_messages_without_duplicate_tts(self):
        radar = (SRC / "a11y_radar.cpp").read_text(encoding="utf-8")
        messages = (SRC / "messages.cpp").read_text(encoding="utf-8")
        self.assertIn('#include "messages.h"', radar)
        self.assertIn("Messages::add_msg_silent( urgent ? m_warning : m_info, spoken );", radar)
        self.assertIn("if( !get_avatar().in_vehicle )", radar)
        self.assertIn('A11Y_RADAR_DANGER:', radar)
        self.assertIn('A11Y_RADAR_ALERT:', radar)
        self.assertIn("void Messages::add_msg_silent(", messages)

    def test_door_pass_alert_is_independent_from_eight_tile_entry(self):
        radar = (SRC / "a11y_radar.cpp").read_text(encoding="utf-8")
        header = (SRC / "a11y_radar.h").read_text(encoding="utf-8")
        self.assertIn("cardinally_adjacent( dx, dy )", radar)
        self.assertIn("nearby_doors.entering( adjacent_door_keys )", radar)
        self.assertIn("newly_adjacent.count( key ) == 0", radar)
        self.assertIn("inline bool cardinally_adjacent(", header)
        self.assertIn("return std::abs( dx ) + std::abs( dy ) == 1;", header)

    def test_multi_tile_landmarks_are_groups_not_fabricated_object_instances(self):
        radar = (SRC / "a11y_radar.cpp").read_text(encoding="utf-8")
        header = (SRC / "a11y_radar.h").read_text(encoding="utf-8")
        self.assertIn("summarize_tile_groups( tile_samples )", radar)
        self.assertIn("std::vector<radar_detail::tile_sample> tile_samples;", radar)
        self.assertIn("tiles[j].type != current.type", header)
        self.assertIn("if( adjacent == 1 )", header)
        self.assertIn("occupied_tiles", header)
        self.assertIn("connected group(s) across", radar)

    def test_speech_reports_total_objects_not_only_new_entries(self):
        radar = (SRC / "a11y_radar.cpp").read_text(encoding="utf-8")
        self.assertIn("for( const auto &entry : contacts ) {", radar)
        self.assertIn("const auto changed_totals = contact_counts.changed( visible_counts );", radar)
        self.assertIn("speak_new( fresh, contacts, changes );", radar)
        self.assertIn("%1$d %2$s nearby, closest to the %3$s", radar)
        self.assertNotIn("%1$d new %2$s nearby, closest to the %3$s", radar)
        # Two physical items on the same tile may have the same type and no UID.
        self.assertIn('"ptr:" + pointer_id( &obj )', radar)
        self.assertNotIn('position_id + ":" + obj.typeId().str()', radar)


if __name__ == "__main__":
    unittest.main()
