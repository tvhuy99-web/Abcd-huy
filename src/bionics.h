#pragma once
#ifndef CATA_SRC_BIONICS_H
#define CATA_SRC_BIONICS_H

#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "calendar.h"
#include "damage.h"
#include "enums.h"
#include "flat_set.h"
#include "item.h"
#include "magic.h"
#include "translation.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

class Character;
class JsonObject;
class JsonOut;

struct bionic_data {
    bionic_data();


    units::energy power_activate = 0_kJ;

    units::energy power_deactivate = 0_kJ;

    units::energy power_over_time = 0_kJ;

    units::energy power_trigger = 0_kJ;

    units::energy capacity = 0_kJ;

    cata::value_ptr<fake_spell> spell_on_activate;

    bionic_id id;


    emit_id power_gen_emission = emit_id::NULL_ID();
    itype_id fake_weapon;



    bionic_id upgraded_bionic;




    bionic_id required_bionic;


    requirement_id installation_requirement;
    std::vector<std::pair<bionic_id, mod_id>> src;

    std::vector<material_id> fuel_opts;

    std::vector<effect_on_condition_id> activated_eocs;

    std::vector<effect_on_condition_id> processed_eocs;

    std::vector<effect_on_condition_id> deactivated_eocs;

    std::vector<enchantment_id> enchantments;

    std::vector<matype_id> ma_styles;



    std::vector<proficiency_id> proficiencies;



    std::vector<itype_id> passive_pseudo_items;
    std::vector<itype_id> toggled_pseudo_items;




    std::vector<trait_id> canceled_mutations;






    std::vector<bionic_id> included_bionics;




    std::vector<bionic_id> autodeactivated_bionics;
    cata::flat_set<json_character_flag> flags;
    cata::flat_set<json_character_flag> active_flags;
    cata::flat_set<json_character_flag> inactive_flags;

    std::map<bodypart_str_id, size_t> env_protec;

    std::map<bodypart_str_id, resistances> protec;



    std::map<bodypart_str_id, size_t> occupied_bodyparts;



    std::map<bodypart_str_id, int> encumbrance;
    std::set<json_character_flag> installable_weapon_flags;




    std::set<bodypart_str_id> replaced_bodyparts;



    std::set<trait_id> mutation_conflicts;



    std::set<trait_id> give_mut_on_removal;



    std::map<spell_id, int> learned_spells;






    std::set<bionic_id> available_upgrades;
    translation name;
    translation description;

    std::optional<translation> cant_remove_reason;

    time_duration charge_time = 0_turns;


    float fuel_efficiency = 0.0f;

    float passive_fuel_efficiency = 0.0f;

    std::optional<float> coverage_power_gen_penalty;

    social_modifiers social_mods;


    bool dupes_allowed = false;

    bool activated = false;

    bool activated_on_install = false;



    bool included = false;

    bool activate_remove_cbm = false;

    bool is_remote_fueled = false;

    bool exothermic_power_gen = false;

    bool activated_close_ui;

    bool deactivated_close_ui;

    bool was_loaded = false;




    bool has_flag( const json_character_flag &flag ) const;
    bool has_active_flag( const json_character_flag &flag ) const;
    bool has_inactive_flag( const json_character_flag &flag ) const;

    itype_id itype() const;

    void load( const JsonObject &obj, std::string_view src );
    void finalize();
    static void load_bionic( const JsonObject &jo, const std::string &src );
    static void finalize_bionic();
    static const std::vector<bionic_data> &get_all();
    static void check_bionic_consistency();

    static std::map<bionic_id, bionic_id> migrations;
    static void load_bionic_migration( const JsonObject &jo, std::string_view );
};

struct bionic {

        using bionic_uid = unsigned int;

        bionic_id id;
        time_duration         charge_timer  = 0_turns;
        char        invlet  = 'a';
        bool        powered = false;
        bool        show_sprite = true;
        bool        auto_shutdown = true;

        time_duration        incapacitated_time;

        bionic() : bionic( bionic_id( "bio_batteries" ), 'a', 0 ) { }
        bionic( bionic_id pid, char pinvlet, bionic_uid pbionic_uid,
                bionic_uid pparent_uid = 0 ) : id( pid ), invlet( pinvlet ),
            incapacitated_time( 0_turns ), uid( pbionic_uid ), parent_uid( pparent_uid ) {
            initialize_pseudo_items( true );
        }

        const bionic_data &info() const {
            return *id;
        }

        void set_flag( const std::string &flag );
        void remove_flag( const std::string &flag );
        bool has_flag( const std::string &flag ) const;

        int get_quality( const quality_id &quality ) const;
        item get_weapon() const;
        void set_weapon( const item &new_weapon );
        bool install_weapon( const item &new_weapon, bool skip_checks = false );
        std::optional<item> uninstall_weapon();
        bool has_weapon() const;
        bool can_install_weapon() const;
        bool can_install_weapon( const item &new_weapon ) const;
        unsigned int get_uid() const;
        void set_uid( bionic_uid new_uid );
        bool is_included() const;
        bionic_uid get_parent_uid() const;
        void set_parent_uid( bionic_uid new_uid );

        std::vector<const item *> get_available_pseudo_items( bool include_weapon = true ) const;

        bool is_this_fuel_powered( const material_id &this_fuel ) const;
        void toggle_safe_fuel_mod();

        void set_safe_fuel_thresh( float val );
        float get_safe_fuel_thresh() const;
        bool is_safe_fuel_on() const;
        bool activate_spell( Character &caster ) const;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );
    private:

        cata::flat_set<std::string> bionic_tags;
        float safe_fuel_threshold = 1.0f;
        item weapon;
        std::vector<item> toggled_pseudo_items; // NOLINT(cata-serialize)
        std::vector<item> passive_pseudo_items; // NOLINT(cata-serialize)
        bionic_uid uid;
        bionic_uid parent_uid;
        void initialize_pseudo_items( bool create_weapon = false );
        void update_weapon_flags();
};



class bionic_collection : public std::vector<bionic>
{
};


std::vector<bodypart_id> get_occupied_bodyparts( const bionic_id &bid );

void reset_bionics();

char get_free_invlet( Character &p );
std::string list_occupied_bps( const bionic_id &bio_id, const std::string &intro,
                               bool each_bp_on_new_line = true );

int bionic_success_chance( bool autodoc, int skill_level, int difficulty, const Character &target );
int bionic_manip_cos( float adjusted_skill, int bionic_difficulty );

std::vector<bionic_id> bionics_cancelling_trait( const std::vector<bionic_id> &bios,
        const trait_id &tid );

#endif
