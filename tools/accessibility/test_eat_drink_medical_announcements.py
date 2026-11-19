"""Guard eating/drinking/medical outcome announcements.

After eating, drinking or using a medical item the game must create a
plain game message stating the resulting status (full / still hungry,
no longer thirsty / still thirsty, bandage or disinfectant applied).
A plain add_msg is the only channel that is BOTH spoken live by the
internal reader (Messages::add_msg -> announce_message ->
a11y::announce_game_message -> Java speech queue) AND kept in the
message log (Messages::recent_messages -> world:messages) so the
two-finger double-tap recap can re-read it. The reader-side status
monitor (announce_status_changes) only speaks threshold crossings
live and never writes to the log, so it cannot carry these outcomes.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
JAVA = (ROOT / "android/app/src/main/java"
        / "com/cleverraven/cataclysmdda")


def eat_body():
    """The static eat() function in consumption.cpp, act message onward."""
    text = (SRC / "consumption.cpp").read_text("utf-8")
    body = text.split("static bool eat( item &food, Character &you, bool force )", 1)[1]
    return body.split("void Character::modify_health", 1)[0]


def finish_using_body():
    """heal_actor::finish_using in iuse_actor.cpp."""
    text = (SRC / "iuse_actor.cpp").read_text("utf-8")
    body = text.split("int heal_actor::finish_using(", 1)[1]
    return body.split("static bodypart_id pick_part_to_heal(", 1)[0]


class EatDrinkOutcomeMessages(unittest.TestCase):
    def test_eating_states_full_or_still_hungry(self):
        body = eat_body()
        # The act alone never states the outcome.
        self.assertIn('_( "You eat your %s." )', body)
        # Outcome follows the act message, in the same function.
        act = body.index('_( "You eat your %s." )')
        full = body.index('_( "You feel full." )')
        hungry = body.index('_( "You are still hungry." )')
        self.assertGreater(full, act)
        self.assertGreater(hungry, act)
        # Player-only, like the other sensation lines in eat().
        self.assertIn("you.add_msg_if_player( m_good, _( \"You feel full.\" ) )", body)
        self.assertIn("you.add_msg_if_player( m_info, _( \"You are still hungry.\" ) )", body)
        # The hunger effect is only recomputed later by update_needs and
        # get_hunger() does not move when eating, so the outcome must be
        # read from the stomach that already holds this meal.
        self.assertIn("you.stomach.would_be_full_with( you, 0_ml", body)
        self.assertIn("you.stomach.would_be_engorged_with( you, 0_ml", body)
        # "Still hungry" mirrors update_needs' just-ate row: with a
        # calorie deficit and under half a stomach, the game still calls
        # the eater hungry.
        self.assertIn("you.has_calorie_deficit()", body)
        self.assertIn("you.stomach.contains() <= you.stomach.capacity( you ) / 2", body)

    def test_drinking_states_no_longer_thirsty_or_still_thirsty(self):
        body = eat_body()
        self.assertIn('_( "You drink your %s." )', body)
        act = body.index('_( "You drink your %s." )')
        slaked = body.index('_( "You are no longer thirsty." )')
        thirsty = body.index('_( "You are still thirsty." )')
        self.assertGreater(slaked, act)
        self.assertGreater(thirsty, act)
        self.assertIn(
            "you.add_msg_if_player( m_good, _( \"You are no longer thirsty.\" ) )", body)
        self.assertIn(
            "you.add_msg_if_player( m_info, _( \"You are still thirsty.\" ) )", body)
        # Instant thirst already counts the water just swallowed, and
        # its bands are the sidebar's: above 40 is "Thirsty", below 0
        # is "Slaked" or better.
        self.assertIn("you.get_instant_thirst() < 0", body)
        self.assertIn("you.get_instant_thirst() > 40", body)

    def test_outcome_lines_only_cover_food_and_drink(self):
        # Medication is neither chew nor drinkable: it keeps its own
        # iuse / consume_drug messages and must not gain a food line.
        body = eat_body()
        block = body[body.index("// The reader speaks every game message"):
                     body.index("if( item::find_type( food.get_comestible()->tool )")]
        self.assertIn("if( chew ) {", block)
        self.assertIn("} else if( drinkable ) {", block)
        self.assertNotIn("is_medication()", block)
        self.assertNotIn("else {", block)


class MedicalOutcomeMessages(unittest.TestCase):
    def test_bleed_bite_infection_outcomes_unchanged(self):
        body = finish_using_body()
        self.assertIn('_( "You stop the bleeding." )', body)
        self.assertIn('_( "You reduce the bleeding, but it\'s not stopped yet." )', body)
        self.assertIn('_( "Your dressing is too ineffective for a bleeding of this extent,'
                      ' and you fail to stop it." )', body)
        self.assertIn('_( "You clean the wound." )', body)
        self.assertIn('_( "Your wound still aches." )', body)
        self.assertIn('_( "You disinfect the wound." )', body)
        self.assertIn('_( "Your wound still hurts." )', body)
        self.assertIn('_( "You finish using the %s." )', body)

    def test_plain_bandage_and_disinfectant_are_named(self):
        body = finish_using_body()
        # A bandage or disinfectant on a clean wound used to apply its
        # effect silently, leaving only "You finish using the %s."
        self.assertIn('_( "You bandage the wound." )', body)
        self.assertIn('_( "The wound is bandaged." )', body)
        # The disinfectant wording reuses the existing infection-cleared
        # strings, so no new msgid is needed for that half.
        finish = body.index('_( "You finish using the %s." )')
        bandage = body.index('_( "You bandage the wound." )')
        self.assertLess(bandage, finish)
        # The fallback only fires when no bleed/bite/infection outcome
        # was reported for this limb, so a treated wound is not
        # announced twice.
        self.assertIn("bool treatment_reported = false;", body)
        self.assertIn("treatment_reported = true;", body)
        self.assertIn("if( !treatment_reported ) {", body)

    def test_oral_medicine_already_announces_act_and_outcome(self):
        # No change was needed here: antibiotics name the act and the
        # treatable outcomes, so pin them instead of adding duplicates.
        iuse = (SRC / "iuse.cpp").read_text("utf-8")
        self.assertIn('_( "You take some antibiotics." )', iuse)
        self.assertIn('_( "The muscle spasms start to go away." )', iuse)
        self.assertIn('_( "The medication does nothing to help the spasms." )', iuse)
        self.assertIn('_( "Maybe this is just the placebo effect, but you feel a little'
                      ' better as the dose settles in." )', iuse)


class OutcomeMessagesReachSpeechAndHistory(unittest.TestCase):
    def test_plain_add_msg_is_spoken_live(self):
        messages = (SRC / "messages.cpp").read_text("utf-8")
        runtime = (SRC / "a11y_runtime.cpp").read_text("utf-8")
        activity = (JAVA / "CataclysmDDA.java").read_text("utf-8")
        # Every game message (except debug) is forwarded to the reader.
        self.assertIn("void announce_message(", messages)
        self.assertIn("message->type == m_debug", messages)
        self.assertIn("a11y::announce_game_message(", messages)
        self.assertIn("android::announce( text, urgent ? 3 : 1 );", runtime)
        self.assertIn("public void onAccessibilityAnnouncement(String text, int priority)",
                      activity)
        self.assertIn("reader.announceGameMessage(text, priority);", activity)
        # The new outcome lines are plain add_msg calls, not the
        # reader-only announce path that bypasses the message log.
        world = (SRC / "a11y_world.cpp").read_text("utf-8")
        for line in ("You feel full.", "You are still hungry.",
                     "You are no longer thirsty.", "You are still thirsty.",
                     "You bandage the wound."):
            self.assertNotIn(line, world)

    def test_plain_add_msg_lands_in_message_history(self):
        world = (SRC / "a11y_world.cpp").read_text("utf-8")
        messages = (SRC / "messages.cpp").read_text("utf-8")
        self.assertIn("Messages::recent_messages( 12 )", world)
        self.assertIn("std::vector<std::pair<std::string, std::string>>"
                      " Messages::recent_messages( const size_t count )", messages)


class UpstreamMealAndWoundCompatibility(unittest.TestCase):
    """Keep the upstream API fixes and NPC meal behavior during merges."""

    def test_medical_skill_lists_use_wound_skill_in_both_places(self):
        source = (SRC / "medical_ui.cpp").read_text("utf-8")
        self.assertIn("for( const wound_skill &req : fix_id->skills )",
                      source)
        self.assertIn("fix.skills.end(), [&]( const wound_skill &sk )",
                      source)
        self.assertIn("sk.is_mandatory ? c_red : c_light_gray",
                      source)
        self.assertNotIn("const std::pair<skill_id, int> &sk", source)
        self.assertIn("a11y::append_requirements(", source)

    def test_consumption_uses_actual_eater_not_avatar(self):
        source = (SRC / "activity_actor.cpp").read_text("utf-8")
        start = source.split("void consume_activity_actor::start(", 1)[1]
        start = start.split("void consume_activity_actor::finish(", 1)[0]
        finish = source.split("void consume_activity_actor::finish(", 1)[1]
        finish = finish.split("void consume_activity_actor::canceled(", 1)[0]
        self.assertIn("guy.will_eat( it, guy.is_avatar() )", start)
        self.assertIn("who.consume( consume_loc, /*force=*/true )", finish)
        self.assertIn("who.consume( consume_item, /*force=*/true )", finish)
        self.assertIn("who.is_avatar() && reprompt_consume_menu", finish)

    def test_camp_meal_is_serializable_and_charged_only_when_eaten(self):
        actor = (SRC / "activity_actor.cpp").read_text("utf-8")
        header = (SRC / "activity_actor_definitions.h").read_text("utf-8")
        npc = (SRC / "npcmove.cpp").read_text("utf-8")
        self.assertIn("class camp_meal_activity_actor", header)
        self.assertIn("void camp_meal_activity_actor::finish(", actor)
        self.assertIn("camp_food_supply( -portion )", actor)
        self.assertIn("camp_meal::personal", actor)
        self.assertIn('data.read( "meal_kcals", actor.meal_kcals )', actor)
        self.assertIn(
            "{ ACT_CAMP_MEAL, &camp_meal_activity_actor::deserialize }", actor)
        self.assertIn("return id == ACT_CONSUME || id == ACT_CAMP_MEAL;", npc)


class UpstreamNativeLinkageContracts(unittest.TestCase):
    """Catch missing out-of-line definitions before the full native linker."""

    def test_followers_can_query_avatar_relocation(self):
        source = (SRC / "avatar.cpp").read_text("utf-8")
        self.assertIn("void avatar::on_move( const tripoint_abs_ms &old_pos )", source)
        self.assertIn("time_duration avatar::time_since_relocated() const", source)
        self.assertIn("relocation_anchor = now_pos;", source)

    def test_character_creator_virtual_input_handler_is_defined(self):
        source = (SRC / "newcharacter.cpp").read_text("utf-8")
        self.assertIn(
            "void character_creator_ui::setup_input_context( input_context &cc_ictxt )",
            source)
        self.assertIn("setup_input_context( cc_ictxt, false );", source)

    def test_npc_food_helpers_have_out_of_line_definitions(self):
        source = (SRC / "consumption.cpp").read_text("utf-8")
        self.assertIn("bool Character::can_stomach(", source)
        self.assertIn("bool Character::wants_to_consume_now(", source)
        self.assertIn("stomach.would_be_engorged_with(", source)
        self.assertIn("will_eat( food ).success()", source)

    def test_treatment_and_fault_helpers_are_linkable(self):
        wounds = (SRC / "wound.cpp").read_text("utf-8")
        faults = (SRC / "fault.cpp").read_text("utf-8")
        self.assertIn("std::pair<bool, float> wound_fix::doable_by(", wounds)
        self.assertIn("req.is_mandatory && !met", wounds)
        self.assertIn("double fault::attack_cost_modifier() const", faults)
        self.assertIn("return attack_cost_modifier_;", faults)

    def test_activity_reason_and_bodygraph_helpers_exist(self):
        actors = (SRC / "activity_actor.cpp").read_text("utf-8")
        bodygraph = (SRC / "bodygraph.cpp").read_text("utf-8")
        self.assertIn("enum_to_string<do_activity_reason>(", actors)
        self.assertIn("do_activity_reason::REFUSES_THIS_WORK", actors)
        self.assertIn("bool bodygraph_part::has_bodypart(", bodygraph)
        self.assertIn("sub_bodyparts.begin(), sub_bodyparts.end()", bodygraph)


if __name__ == "__main__":
    unittest.main()
