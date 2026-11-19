#pragma once
#ifndef CATA_SRC_ACTION_H
#define CATA_SRC_ACTION_H

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "coords_fwd.h"

class input_context;
class map;
struct input_event;




enum action_id : int {

    ACTION_NULL = 0,




    ACTION_SELECT,

    ACTION_SEC_SELECT,

    ACTION_CLICK_AND_DRAG,





    ACTION_PAUSE,

    ACTION_TIMEOUT,

    ACTION_MOVE_FORTH,

    ACTION_MOVE_FORTH_RIGHT,

    ACTION_MOVE_RIGHT,

    ACTION_MOVE_BACK_RIGHT,

    ACTION_MOVE_BACK,

    ACTION_MOVE_BACK_LEFT,

    ACTION_MOVE_LEFT,

    ACTION_MOVE_FORTH_LEFT,

    ACTION_MOVE_DOWN,

    ACTION_MOVE_UP,

    ACTION_CYCLE_MOVE,

    ACTION_CYCLE_MOVE_REVERSE,

    ACTION_RESET_MOVE,

    ACTION_TOGGLE_RUN,

    ACTION_TOGGLE_CROUCH,

    ACTION_TOGGLE_PRONE,

    ACTION_OPEN_MOVEMENT,





    ACTION_CENTER,

    ACTION_SHIFT_N,

    ACTION_SHIFT_NE,

    ACTION_SHIFT_E,

    ACTION_SHIFT_SE,

    ACTION_SHIFT_S,

    ACTION_SHIFT_SW,

    ACTION_SHIFT_W,

    ACTION_SHIFT_NW,





    ACTION_OPEN,

    ACTION_CLOSE,

    ACTION_SMASH,

    ACTION_EXAMINE,


    ACTION_EXAMINE_AND_PICKUP,

    ACTION_PICKUP,

    ACTION_PICKUP_ALL,

    ACTION_GRAB,

    ACTION_HAUL,

    ACTION_HAUL_TOGGLE,

    ACTION_BUTCHER,

    ACTION_CHAT,

    ACTION_LOOK,

    ACTION_PEEK,

    ACTION_LIST_ITEMS,

    ACTION_ZONES,

    ACTION_LOOT,





    ACTION_INVENTORY,

    ACTION_ADVANCEDINV,

    ACTION_COMPARE,

    ACTION_ORGANIZE,

    ACTION_USE,

    ACTION_USE_WIELDED,

    ACTION_WEAR,

    ACTION_TAKE_OFF,

    ACTION_EAT,

    ACTION_OPEN_CONSUME,

    ACTION_READ,

    ACTION_WIELD,

    ACTION_PICK_STYLE,

    ACTION_RELOAD_ITEM,

    ACTION_RELOAD_WEAPON,

    ACTION_RELOAD_WIELDED,

    ACTION_UNLOAD,

    ACTION_MEND,

    ACTION_THROW,

    ACTION_THROW_WIELDED,

    ACTION_FIRE,

    ACTION_FIRE_BURST,

    ACTION_SELECT_FIRE_MODE,

    ACTION_SELECT_DEFAULT_AMMO,

    ACTION_CAST_SPELL,

    ACTION_RECAST_SPELL,

    ACTION_INSERT_ITEM,

    ACTION_UNLOAD_CONTAINER,

    ACTION_DROP,

    ACTION_DIR_DROP,

    ACTION_BIONICS,

    ACTION_MUTATIONS,

    ACTION_SORT_ARMOR,

    ACTION_AUTOATTACK,





    ACTION_WAIT,

    ACTION_CRAFT,

    ACTION_RECRAFT,

    ACTION_LONGCRAFT,

    ACTION_CONSTRUCT,

    ACTION_DISASSEMBLE,

    ACTION_SLEEP,

    ACTION_CONTROL_VEHICLE,

    ACTION_TOGGLE_AUTO_TRAVEL_MODE,

    ACTION_TOGGLE_SAFEMODE,

    ACTION_TOGGLE_AUTOSAFE,

    ACTION_TOGGLE_THIEF_MODE,

    ACTION_TOGGLE_LANGUAGE_TO_EN,

    ACTION_IGNORE_ENEMY,

    ACTION_WHITELIST_ENEMY,

    ACTION_WORKOUT,

    ACTION_SAVE,

    ACTION_QUICKSAVE,

    ACTION_QUICKLOAD,

    ACTION_SUICIDE,





    ACTION_PL_INFO,

    ACTION_MAP,

    ACTION_SKY,

    ACTION_MISSIONS,

    ACTION_FACTIONS,

    ACTION_MORALE,

    ACTION_MEDICAL,

    ACTION_MESSAGES,

    ACTION_HELP,

    ACTION_DIARY,

    ACTION_BODYSTATUS,

    ACTION_MAIN_MENU,

    ACTION_KEYBINDINGS,

    ACTION_OPTIONS,

    ACTION_AUTOPICKUP,

    ACTION_AUTONOTES,

    ACTION_SAFEMODE,

    ACTION_COLOR,

    ACTION_WORLD_MODS,

    ACTION_DISTRACTION_MANAGER,



    ACTION_EXPORT_BUG_REPORT_ARCHIVE,




    ACTION_TOGGLE_FULLSCREEN,

    ACTION_DEBUG,

    ACTION_DISPLAY_SCENT,

    ACTION_DISPLAY_SCENT_TYPE,

    ACTION_TOGGLE_DEBUG_MODE,

    ACTION_ZOOM_OUT,

    ACTION_ZOOM_IN,

    ACTION_ACTIONMENU,

    ACTION_ITEMACTION,

    ACTION_TOGGLE_PIXEL_MINIMAP,

    ACTION_TOGGLE_PANEL_ADM,

    ACTION_PANEL_MGMT,

    ACTION_RELOAD_TILESET,

    ACTION_TOGGLE_AUTO_FEATURES,

    ACTION_TOGGLE_AUTO_PULP_BUTCHER,

    ACTION_TOGGLE_AUTO_MINING,

    ACTION_TOGGLE_AUTO_FORAGING,

    ACTION_TOGGLE_AUTO_PICKUP,

    ACTION_DISPLAY_TEMPERATURE,

    ACTION_DISPLAY_SNOW_DEPTH,

    ACTION_DISPLAY_VEHICLE_AI,

    ACTION_DISPLAY_VISIBILITY,

    ACTION_DISPLAY_LIGHTING,

    ACTION_DISPLAY_RADIATION,

    ACTION_DISPLAY_TRANSPARENCY,

    ACTION_TOGGLE_PREVENT_OCCLUSION,
    ACTION_DISPLAY_NPC_ATTACK_POTENTIAL,

    ACTION_TOGGLE_HOUR_TIMER,

    ACTION_INTERACT,

    ACTION_GAME_SETTINGS_MENU,

    NUM_ACTIONS

};


























void load_keyboard_settings( std::map<char, action_id> &keymap,
                             std::string &keymap_file_loaded_from,
                             std::set<action_id> &unbound_keymap );
















std::vector<input_event> keys_bound_to( action_id act,
                                        int maximum_modifier_count = -1,
                                        bool restrict_to_printable = true );












std::optional<input_event> hotkey_for_action( action_id action,
        int maximum_modifier_count = -1, bool restrict_to_printable = true );











action_id look_up_action( const std::string &ident );













std::string action_ident( action_id act );














bool can_action_change_worldstate( action_id act );


















std::optional<tripoint_bub_ms> choose_adjacent( const std::string &message,
        bool allow_vertical = false );
std::optional<tripoint_bub_ms> choose_adjacent( const tripoint_bub_ms &pos,
        const std::string &message, bool allow_vertical = false, int timeout = 50,
        const std::function<std::pair<bool, std::optional<tripoint_bub_ms>>(
            const input_context &ctxt, const std::string &action )> &action_cb = nullptr,
        const std::function<std::string( const tripoint_bub_ms & )> &reader_target_hint = nullptr );






















std::optional<tripoint_rel_ms> choose_direction( const std::string &message,
        bool allow_vertical = false, bool allow_mouse = false, int timeout = 50,
        const std::function<std::pair<bool, std::optional<tripoint_rel_ms>>(
            const input_context &ctxt, const std::string &action )> &action_cb = nullptr,
        const tripoint_bub_ms *reader_origin = nullptr,
        const std::function<std::string( const tripoint_bub_ms & )> &reader_target_hint = nullptr );

















std::optional<tripoint_bub_ms> choose_adjacent_highlight( map &here, const std::string &message,
        const std::string &failure_message, action_id action,
        bool allow_vertical = false, bool allow_autoselect = true );


















std::optional<tripoint_bub_ms> choose_adjacent_highlight( map &here, const std::string &message,
        const std::string &failure_message, const std::function<bool( const tripoint_bub_ms & )> &allowed,
        bool allow_vertical = false, bool allow_autoselect = true );
std::optional<tripoint_bub_ms> choose_adjacent_highlight( map &here, const tripoint_bub_ms &pos,
        const std::string &message,
        const std::string &failure_message, const std::function<bool( const tripoint_bub_ms & )> &allowed,
        bool allow_vertical = false, bool allow_autoselect = true );


std::string press_x( action_id act );
std::string press_x( action_id act, const std::string &key_bound,
                     const std::string &key_unbound );
std::string press_x( action_id act, const std::string &key_bound_pre,
                     const std::string &key_bound_suf, const std::string &key_unbound );

std::string press_x( action_id act, const std::string &act_desc );

std::optional<std::string> press_x_if_bound( action_id act );


enum class iso_rotate : int {
    no,
    yes
};
















action_id get_movement_action_from_delta( const tripoint_rel_ms &d, iso_rotate rot );


point_rel_ms get_delta_from_movement_action( action_id act, iso_rotate rot );









action_id handle_action_menu( map &here );











std::optional<tripoint_bub_ms> choose_accessible_nearby_target(
    const std::string &title,
    const std::function<bool( const tripoint_bub_ms & )> &allowed,
    const std::string &empty_reason );

action_id handle_interact( map &here, const tripoint_bub_ms &pos );









action_id handle_main_menu();














bool can_interact_at( action_id action, map &here, const tripoint_bub_ms &p );













bool can_butcher_at( map &here, const tripoint_bub_ms &p );















bool can_move_vertical_at( const map &here, const tripoint_bub_ms &p, int movez );













bool can_examine_at( map &here,  const tripoint_bub_ms &p, bool with_pickup = false );

#endif
