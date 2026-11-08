#pragma once
#ifndef CATA_TESTS_A11Y_FIXTURES_H
#define CATA_TESTS_A11Y_FIXTURES_H

#include <string>
#include <vector>

#include "coordinates.h"
#include "point.h"
#include "type_id.h"

class basecamp;
class computer;
class vehicle;

// Deterministic fixtures for the accessibility (R22) test catalogue.
//
// Every builder in namespace a11y_fixture constructs its situation ONLY from
// data already in this repository (core data + the TEST_DATA mod, which the
// test harness always loads) through the same test helpers the existing
// suites use (clear_avatar / clear_map* / build_test_map / spawn_npc /
// spawn_test_monster / map::add_vehicle / map::add_computer / the
// faction_camp_test camp pattern). No personal save is ever loaded.
//
// Determinism: builders use fixed coordinates, fixed item/trap/monster ids,
// direct HP assignment instead of damage rolls, and -- where a game's board
// is derived from the global RNG -- an explicit rng_set_engine_seed() call
// documented per function. A builder REQUIREs the invariants its oracle
// relies on, in the style of spawn_test_monster()/spawn_npc(), so a fixture
// that failed to construct fails the test at the fixture, not downstream.
//
// Written during the coding phase (phase 1): this file has NOT been compiled
// or run. See tools/accessibility/fixtures/FIXTURES.vi.md for the manifest.

namespace a11y_fixture
{

// Shared reset used by every builder: a cleared avatar at (60, 60, 0), an
// empty bubble map, no creatures/NPCs/vehicles/zones, and daylight so the
// visibility cache is in a known state (set_time_to_day() from
// map_helpers_tests.h). Guarantees builders start from the same baseline
// regardless of which test ran before them.
void reset_world_for_fixture();

// ---------------------------------------------------------------- F-01 ----
// Knowledge pair: same known world, different secrets.
//
// build_knowledge_pair() builds ONE variant; a test builds variant::a,
// records what the reader layer reports, then builds variant::b and records
// again. Guaranteed identical between the two variants (the "known" side):
//   * live terrain/furniture west of and including the wall column,
//   * the avatar's whole memorized rectangle (terrain t_grass, wall column
//     t_brick_wall, crate tile decoration f_crate_c), written explicitly
//     through avatar::memorize_terrain()/memorize_decoration() after
//     avatar::clear_map_memory(), so memory cannot drift between variants.
// Guaranteed different between the variants (the "secret" side, all of it
// east of the wall at x = 63, outside the avatar's line of sight):
//   * the monster's tile, the buried beartrap's tile, the buried landmine's
//     tile, and the item inside the closed crate (f_crate_c, CONTAINER +
//     SEALED furniture; a11y::observe_tile() withholds its items).
// Oracle served: a reader snapshot/scan that is a function of knowledge
// only must be byte-identical for the two variants, while a ground-truth
// check on the live map must see the differences above. No seed is used:
// every position and id below is a fixed literal.
enum class knowledge_variant { a, b };

struct knowledge_pair_fixture {
    knowledge_variant variant = knowledge_variant::a;
    tripoint_bub_ms avatar_pos;
    tripoint_bub_ms monster_pos;      // secret: differs per variant
    tripoint_bub_ms hidden_trap_pos;  // secret: tr_beartrap_buried tile
    tripoint_bub_ms mine_pos;         // secret: tr_landmine_buried tile
    tripoint_bub_ms crate_pos;        // known: f_crate_c in both variants
    itype_id crate_content;           // secret: item inside the closed crate
};

knowledge_pair_fixture build_knowledge_pair( knowledge_variant variant );

// ---------------------------------------------------------------- F-02 ----
// Pocket tree: the avatar wears one container holding a second container
// holding (a) a loose item and (b) a sealable can whose pocket is SEALED
// around a water charge. Guaranteed, in construction order:
//   debug_backpack (worn) > backpack > test_can_drink (pocket sealed,
//   contains water) and backpack > knife_combat (loose).
// So the tree has depth 3 below the avatar, one sealed pocket, and items at
// two different depths. Construction uses item::put_in() with
// pocket_type::CONTAINER and item_pocket::seal() exactly as
// item_pocket_test.cpp's "sealed_containers" case does for the same
// test_can_drink type. No RNG is involved.
struct pocket_tree_fixture {
    itype_id outer_container;   // debug_backpack, worn
    itype_id inner_container;   // backpack, inside the outer container
    itype_id sealed_container;  // test_can_drink, inside the inner container
    itype_id sealed_content;    // water, inside the sealed pocket
    itype_id loose_content;     // knife_combat, loose inside the inner container
};

pocket_tree_fixture build_pocket_tree();

// ---------------------------------------------------------------- F-03 ----
// Two-sided inventory: one item in each advanced-inventory source the R09
// reader must keep apart. Guaranteed:
//   * ground item (knife_combat) on the avatar's own tile -- the placement
//     advanced_inventory_test.cpp uses for AIM_CENTER,
//   * cargo item (jeans) inside the cargo part of a test_shopping_cart
//     vehicle on the tile east of the avatar (the same prototype and
//     veh_at().cargo() lookup clzones_test.cpp uses),
//   * on-person items: bandages carried directly, and a test_rock inside
//     the worn debug_backpack's container pocket.
// All ids/positions are fixed literals; no RNG is involved.
struct two_sided_inventory_fixture {
    tripoint_bub_ms ground_pos;
    tripoint_bub_ms vehicle_pos;
    itype_id ground_item;   // knife_combat
    itype_id cargo_item;    // jeans
    itype_id carried_item;  // bandages, directly in the avatar's inventory
    itype_id worn_item;     // test_rock, inside the worn debug_backpack
};

two_sided_inventory_fixture build_two_sided_inventory();

// ---------------------------------------------------------------- F-04 ----
// Bandageable wound: the avatar's right arm is set to exactly 3/5 of its
// maximum HP by direct assignment (Character::set_part_hp_cur(), the same
// mechanism a11y_body_test.cpp uses) -- never by apply_damage(), so no
// wound/damage roll can vary the result. Every other limb stays at full HP.
// The inventory holds bandages and disinfectant (the medical supplies the
// firstaid flow consumes; firstaid_test.cpp bandages a damaged arm with
// itype bandages). Guaranteed: injured_part is arm_r, hp_before < hp_max,
// and both supply types are present in the avatar's inventory.
struct wound_fixture {
    bodypart_id injured_part;  // arm_r
    int hp_before = 0;
    int hp_max = 0;
    itype_id bandage_item;     // bandages
    itype_id disinfectant_item;  // disinfectant
};

wound_fixture build_wound_fixture();

// ---------------------------------------------------------------- F-05 ----
// Combat field on open grass (build_test_map(t_grass)), in daylight.
// Guaranteed:
//   * a visible hostile target: mon_zombie three tiles east of the avatar,
//     spawned with spawn_test_monster() (map_helpers_tests.h),
//   * an empty tile one tile east of the avatar (no creature, no items),
//   * a known ally: an NPC of class test_talker one tile west of the
//     avatar with attitude NPCATT_FOLLOW (spawn_npc() + npc::set_attitude(),
//     the pattern npc_test.cpp uses for followers),
//   * a target on another z-level: a second mon_zombie on a t_floor tile
//     built at z = +1 (the explicit per-z ter_set pattern of
//     monster_stair_test.cpp's build_tower()),
//   * a known AOE spell: test_spell_lava learned with
//     known_magic::learn_spell(..., force = true), as magic_spell_test.cpp
//     learns it; in TEST_DATA magic.json it has min_aoe 4 at level 0, so
//     its area is known and non-trivial.
// Positions are fixed literals; learn_spell with force consumes no RNG for
// the learning itself.
struct combat_field_fixture {
    tripoint_bub_ms avatar_pos;
    tripoint_bub_ms visible_target_pos;
    tripoint_bub_ms empty_pos;
    tripoint_bub_ms ally_pos;
    tripoint_bub_ms other_level_pos;  // z = +1
    spell_id aoe_spell;               // test_spell_lava
};

combat_field_fixture build_combat_field();

// ---------------------------------------------------------------- F-06 ----
// Vehicle: a multi-part "car" prototype spawned with
// map::add_vehicle(..., init_veh_fuel = 50, veh_spawn_status::UNDAMAGED)
// at a fixed origin/facing. Guaranteed:
//   * the engine part (vehicle::engines[0], the lookup vehicle_test.cpp
//     uses for the bicycle pedal) is set to 0 HP with vehicle::set_hp() --
//     exactly one broken part, named in broken_part_name,
//   * the cargo part (found by a fixed-order scan for veh_at().cargo(),
//     the lookup clzones_test.cpp/vehicle_test.cpp use) holds jeans and a
//     test_rock, added with vehicle::add_item(),
//   * fuel is whatever the spawn API leaves for init_veh_fuel = 50 percent
//     of the gasoline tank(s); the oracle must read the live amount through
//     vehicle::fuel_left(map, gasoline), never assume a litre value -- the
//     deterministic guarantee is the spawn parameter, recorded in
//     fuel_percent.
// The returned vehicle pointer is owned by the map; do not delete it.
struct vehicle_fixture {
    vehicle *veh = nullptr;
    tripoint_bub_ms origin;
    tripoint_bub_ms cargo_pos;
    std::string broken_part_name;
    itype_id fuel_type;   // gasoline
    int fuel_percent = 50;  // the init_veh_fuel argument given to add_vehicle
};

vehicle_fixture build_vehicle_fixture();

// ---------------------------------------------------------------- F-07 ----
// Zones and rules. Guaranteed:
//   * the zone manager starts empty (zone_manager::clear()),
//   * a LOOT_FOOD zone named "A11y Loot Food" covering a fixed 2x2 area and
//     a LOOT_UNSORTED zone named "A11y Unsorted" on one fixed tile, added
//     with zone_manager::add() exactly as clzones_test.cpp's
//     create_tile_zone() does (faction your_followers),
//   * the safemode character rule set starts empty
//     (safemode::clear_character_rules()) and then holds one sample rule:
//     "zombie", attitude HOSTILE, proximity 5, BLACKLISTED -- added through
//     safemode::add_rule(), the entry point handle_action.cpp uses,
//   * the auto-pickup character rule set starts empty
//     (player_settings::clear_character_rules()) and then holds one sample
//     rule including a test_rock item, added through
//     auto_pickup::player_settings::add_rule(item, include = true), the
//     entry point item_autopickup_test.cpp uses.
// "In the root rule store" here means these two character rule stores of
// the live game objects get_safemode()/get_auto_pickup(); the fixture does
// not write any file.
struct zones_rules_fixture {
    tripoint_abs_ms food_zone_start;
    tripoint_abs_ms food_zone_end;
    tripoint_abs_ms unsorted_zone_pos;
    std::string safemode_rule;  // "zombie"
    itype_id autopickup_item;   // test_rock
};

zones_rules_fixture build_zones_and_rules();

// ---------------------------------------------------------------- F-08 ----
// Faction camp, built with the faction_camp_test.cpp pattern (place_test_camp
// / camp_calorie_counting): map::add_camp() at mid-map, owner your_fac,
// define_camp("faction_base_bare_bones_NPC_camp_0"), CAMP_FOOD and
// CAMP_STORAGE zones placed with mapgen_place_zone(). Guaranteed:
//   * one resident: an NPC of class test_talker next to the camp centre,
//     attitude NPCATT_FOLLOW,
//   * one assigned job for the avatar: TEST_MISSION_FIND_ITEM reserved and
//     assigned with mission::reserve_new()/mission::assign(), the pattern
//     of mission_test.cpp's assign_test_find_item_mission(); the camp's own
//     companion/upgrade missions remain enumerable through
//     basecamp::get_available_missions_by_dir(),
//   * an exact food supply: the camp faction's supply is emptied, then fed
//     100000 calories of nutrients at calendar::turn_zero, which
//     faction_camp_test.cpp shows reads back as exactly 100 kcal
//     (food_kcal == 100, REQUIREd by the builder).
// Call clear_faction_camp() when done, mirroring faction_camp_test.cpp's
// overmap_buffer.clear_camps() + empty_food_supply() cleanup.
// The returned basecamp pointer is owned by overmap_buffer; do not delete.
struct camp_fixture {
    basecamp *camp = nullptr;
    tripoint_abs_omt camp_omt;
    tripoint_bub_ms resident_pos;
    int food_kcal = 100;
};

camp_fixture build_faction_camp();
void clear_faction_camp( const camp_fixture &fixture );

// ---------------------------------------------------------------- F-09 ----
// Abilities and a real computer on the map. Guaranteed:
//   * an installed bionic: bio_power_storage, added with
//     Character::add_bionic() as bionics_test.cpp does,
//   * an uninstalled CBM item in the inventory: the bio_power_storage CBM
//     item (data/json/items/bionics.json uses the bionic's id for its CBM
//     item type, with the CBM flag),
//   * a known spell: test_spell_pew in the avatar's known_magic spellbook,
//   * a spellbook PROP: see the limitation documented in the .cpp and in
//     FIXTURES.vi.md -- no core/TEST_DATA item teaches a spell, so this is
//     a real BOOK item (test_textbook_fabrication) whose "spell_id" item
//     variable names the spell it stands for (test_spell_lava),
//   * a computer really on the map: map::add_computer() on the tile east
//     of the avatar, security 0, with one option added through
//     computer::add_option() (COMPACT_GEIGER). The returned pointer is
//     owned by the submap; do not delete it.
struct abilities_fixture {
    tripoint_bub_ms computer_pos;
    computer *terminal = nullptr;
    bionic_id installed_bionic;    // bio_power_storage
    itype_id uninstalled_cbm;      // bio_power_storage (item)
    spell_id known_spell;          // test_spell_pew
    itype_id spellbook_item;       // test_textbook_fabrication (prop, see above)
    std::string spellbook_spell;   // "test_spell_lava", in the item's spell_id var
};

abilities_fixture build_abilities();

// --------------------------------------------------------------- F-15 -----
// Lifecycle / two lives. A single test process has exactly one avatar and
// one world, so two real saves/worlds CANNOT be built by a C++ builder;
// that half of F-15 is a device-scenario specification in
// FIXTURES.vi.md (T28/T53). What a builder CAN guarantee in-process is the
// knowledge-isolation half: build_lifecycle_stage() first wipes the avatar
// (clear_avatar()) and the map memory (avatar::clear_map_memory(), the
// call game.cpp makes when a session is replaced), then installs exactly
// one stage's knowledge:
//   stage 1: memorizes a t_floor marker tile east of the avatar and learns
//            test_spell_pew;
//   stage 2: memorizes a DIFFERENT marker tile (further east, t_brick_wall)
//            and learns test_spell_lava instead.
// Oracle served: after building stage 2, nothing of stage 1's knowledge
// (marker tile memory, spell) may be present, and vice versa -- the same
// isolation a load of a second character must provide.
struct lifecycle_stage_fixture {
    int stage = 0;
    tripoint_bub_ms known_marker_pos;
    std::string known_terrain_id;
    spell_id known_spell;
};

lifecycle_stage_fixture build_lifecycle_stage( int stage );

// ------------------------------------------------------------ minigames ---
// F-10..F-14. None of the five minigames can be instantiated headlessly in
// the Catch2 harness: their state lives in function-local variables or in
// private members behind a blocking start_game()/constructor that needs a
// curses window and input (robot_finds_kitten's constructor even runs the
// game loop). So for minigames the fixtures are BOARD SPECIFICATIONS:
// deterministic descriptions derived from the engine source, each stating
// exactly what is and is not derivable. See FIXTURES.vi.md per game.

// F-11 Minesweeper, Beginner board. minesweeper_game::new_level() is
// private and interactive (uilist + string_input_popup), but its bomb
// placement is a pure function of the global RNG: for Beginner it fixes
// level = (8, 8) and iBombs = 10, then repeats
// { x = rng(0, 7); y = rng(0, 7) } until an unmined cell is found.
// minesweeper_beginner_spec() replays exactly that sequence after
// rng_set_engine_seed(seed), so `mines` is the board a game started with
// the same seed and no intervening RNG consumer would deal. This is a
// derivation from src/iuse_software_minesweeper.cpp and must be re-derived
// if that placement code changes; it does not run the game.
struct minesweeper_spec {
    int width = 8;
    int height = 8;
    int mine_count = 10;
    unsigned int seed = 0;
    std::vector<point> mines;  // (x, y), in placement order
};

minesweeper_spec minesweeper_beginner_spec( unsigned int seed = 2201 );

// F-13 Lights On board. lightson_game::new_level() is private and
// start_game() needs a window, but generation consumes only rng()/
// rng_float() in a fixed order (src/iuse_software_lightson.cpp):
// half_perimeter = rng(8, 11); width = rng(4, 6); height = half_perimeter
// - width; steps = int(half_perimeter / 2.0 + rng_float(0.0, 2.0)); then
// generate_change_coords(steps) draws distinct cell indices with
// rng(0, width * height - 1); reset_level() starts from all-on and applies
// toggle_lights_at() (self + orthogonal neighbours) per change coord.
// lightson_spec_for_seed() replays that order after
// rng_set_engine_seed(seed) and returns the change coords plus the cells
// that end up OFF (dark_cells). Same derivation caveat as Minesweeper.
struct lightson_spec {
    point size;  // (width, height)
    unsigned int seed = 0;
    std::vector<point> change_coords;
    std::vector<point> dark_cells;
};

lightson_spec lightson_spec_for_seed( unsigned int seed = 2202 );

// F-12 Sokoban, level 1. sokoban_game::parse_level() is private and reads
// PATH_INFO::sokoban() (data/raw/sokoban.txt), but the level set is fixed
// data in this repo, so level 1 is a fully fixed board: sokoban_level_one_
// spec() returns the rows verbatim from that file plus the player, package
// and goal coordinates read off them ('@', '$', '.'/'*'/' +' per the legend
// in iuse_software_sokoban.cpp). No seed exists in this game.
struct sokoban_spec {
    int level_index = 0;  // first level in data/raw/sokoban.txt ("; #1")
    point size;           // (max row length, row count) = (9, 9)
    point player;         // (1, 7)
    std::vector<point> packages;  // (2, 5), (2, 6), (3, 6)
    std::vector<point> goals;     // (7, 3), (7, 4), (7, 5)
    std::vector<std::string> rows;
};

sokoban_spec sokoban_level_one_spec();

// F-10 Robot Finds Kitten. The board geometry is fixed by
// iuse_software_kitten.h (rfkCOLS = 60, rfkLINES = 20, numbogus = 20, so
// 21 objects of which exactly 1 is the kitten), but the placement sequence
// in the constructor interleaves rng() positions with
// all_colors.get_random() and SNIPPET.random_from_category() draws, and the
// constructor runs the UI loop -- so exact object positions are NOT
// derivable from a seed alone without running the game. positions_derivable
// is therefore false and no positions are returned; the spec fixes only
// the geometry, the object counts, and the seed a device scenario must set
// before launching the game.
struct kitten_spec {
    int width = 60;   // rfkCOLS
    int height = 20;  // rfkLINES
    int object_count = 21;  // 1 kitten + 20 bogus (numbogus)
    int bogus_count = 20;
    unsigned int seed = 0;
    bool positions_derivable = false;
};

kitten_spec kitten_spec_for_seed( unsigned int seed = 2203 );

// F-14 Snake. snake_game::start_game() keeps all state in locals and sizes
// the board from the terminal (FULL_SCREEN_WIDTH/HEIGHT), so there is no
// seed-only board: given a DECLARED board size, the start is fixed by the
// source -- head at (width / 2, height / 2) as (column, row), initial
// direction east, target length 10, and the first fruit is placed by
// repeating { y = rng(1, height - 2); x = rng(1, width - 2) } until it is
// not on the snake's single starting cell. snake_spec_for_board() replays
// that fruit draw after rng_set_engine_seed(seed) for the declared size.
// The terminal-size dependency itself is part of the specification: a
// device scenario must record the board size it actually ran with.
struct snake_spec {
    point board_size;   // (width, height), as declared by the caller
    point head;         // (x, y) = (width / 2, height / 2)
    point fruit;        // (x, y), first fruit for the declared size + seed
    int direction_x = 1;  // east
    int direction_y = 0;
    int starting_length = 10;  // iSnakeBody in start_game()
    unsigned int seed = 0;
};

snake_spec snake_spec_for_board( int width, int height, unsigned int seed = 2204 );

} // namespace a11y_fixture

#endif // CATA_TESTS_A11Y_FIXTURES_H
