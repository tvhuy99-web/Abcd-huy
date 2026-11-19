#pragma once
#ifndef CATA_SRC_EFFECT_H
#define CATA_SRC_EFFECT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "calendar.h"
#include "color.h"
#include "effect_source.h"
#include "enums.h"
#include "event.h"
#include "flat_set.h"
#include "translation.h"
#include "type_id.h"

class Character;
class JsonObject;
class JsonOut;
class effect_type;


void weed_msg( Character &p );


template<>
const effect_type &string_id<effect_type>::obj() const;

struct vitamin_rate_effect {
    std::vector<std::pair<int, int>> rate;
    std::vector<float> absorb_mult;
    std::vector<time_duration> tick;

    std::vector<std::pair<int, int>> red_rate;
    std::vector<float> red_absorb_mult;
    std::vector<time_duration> red_tick;

    vitamin_id vitamin;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

struct vitamin_applied_effect {
    std::optional<std::pair<int, int>> rate = std::nullopt;
    std::optional<time_duration> tick = std::nullopt;
    std::optional<float> absorb_mult = std::nullopt;
    vitamin_id vitamin;
};

enum class mod_type : uint8_t {
    BASE_MOD = 0,
    SCALING_MOD = 1,

    MAX,
};

using modifier_value_arr = std::array<double, static_cast<size_t>( mod_type::MAX )>;
extern modifier_value_arr default_modifier_values;

enum class mod_action : uint8_t {
    AMOUNT,
    CHANCE_BOT,
    CHANCE_TOP,
    MAX,
    MAX_VAL,
    MIN,
    MIN_VAL,
    TICK,
};


struct limb_score_effect {

    limb_score_id score_id;

    float mod;
    float red_mod;
    float scaling;
    float red_scaling;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

struct effect_dur_mod {
    efftype_id effect_id;
    float modifier;
    bool same_bp;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

class effect_type
{
        friend void load_effect_type( const JsonObject &jo, std::string_view src );
        friend class effect;
        friend struct mod_tracker;
    public:
        enum class memorial_gender : int {
            male,
            female,
        };

        effect_type() = default;

        efftype_id id;


        game_message_type get_rating( int intensity = 1 ) const;



        bool use_name_ints() const;


        bool use_desc_ints( bool reduced ) const;



        game_message_type lose_game_message_type( int intensity = 1 ) const;


        void add_apply_msg( int intensity ) const;


        std::string get_apply_memorial_log( memorial_gender gender ) const;

        std::string get_remove_message() const;

        std::string get_remove_memorial_log( memorial_gender gender ) const;

        std::string get_blood_analysis_description() const;


        bool get_main_parts() const;

        double get_mod_value( const std::string &type, mod_action action, uint8_t reduction_level,
                              int intensity ) const;

        bool is_show_in_info() const;


        void load_mod_data( const JsonObject &jo );
        bool load_miss_msgs( const JsonObject &jo, std::string_view member );
        bool load_decay_msgs( const JsonObject &jo, std::string_view member );
        bool load_apply_msgs( const JsonObject &jo, std::string_view member );


        static void check_consistency();
        void verify() const;


        static void register_ma_buff_effect( const effect_type &eff );


        bool has_flag( const flag_id &flag ) const;

        const time_duration &intensity_duration() const {
            return int_dur_factor;
        }
        std::vector<enchantment_id> enchantments;
        cata::flat_set<json_character_flag> immune_flags;
        cata::flat_set<json_character_flag> immune_bp_flags;
    protected:
        uint32_t get_effect_modifier_key( mod_action action, uint8_t reduction_level ) const {
            return static_cast<uint8_t>( action ) << 0 |
                   reduction_level << 8;
        }

        void extract_effect(
            const std::array<std::optional<JsonObject>, 2> &j,
            const std::string &effect_name,
            const std::vector<std::pair<std::string, mod_action>> &action_keys );

        int max_intensity = 0;
        int max_effective_intensity = 0;
        time_duration max_duration = 365_days;

        int dur_add_perc = 0;
        int int_add_val = 0;

        int int_decay_step = 0;
        int int_decay_tick = 0 ;
        time_duration int_dur_factor = 0_turns;
        bool int_decay_remove = false;

        std::set<flag_id> flags;

        bool main_parts_only = false;


        bool show_in_info = false;


        bool show_intensity = false;

        std::vector<trait_id> resist_traits;
        std::vector<efftype_id> resist_effects;
        std::vector<efftype_id> removes_effects;
        std::vector<efftype_id> blocks_effects;

        std::vector<std::pair<translation, int>> miss_msgs;

        bool pain_sizing = false;
        bool hurt_sizing = false;
        bool harmful_cough = false;


        bool pkill_addict_reduces = false;


        bool impairs_movement = false;

        std::vector<translation> name;
        translation speed_mod_name;
        std::vector<translation> desc;
        std::vector<translation> reduced_desc;
        bool part_descs = false;

        std::vector<std::pair<translation, game_message_type>> decay_msgs;

        std::vector<std::pair<translation, game_message_type>> apply_msgs;

        std::string apply_memorial_log;
        translation remove_message;
        std::string remove_memorial_log;

        translation blood_analysis_description;

        translation death_msg;
        std::optional<event_type> death_event;

        std::unordered_map<std::string, std::unordered_map<uint32_t, modifier_value_arr>> mod_data;
        std::vector<vitamin_rate_effect> vitamin_data;
        std::vector<limb_score_effect> limb_score_data;
        std::vector<effect_dur_mod> effect_dur_scaling;
        std::vector<std::pair<int, int>> kill_chance;
        std::vector<std::pair<int, int>> red_kill_chance;

        std::vector<std::pair<efftype_id, mod_id>> src;
};

class effect;



class effects_map : public
    std::map<efftype_id, std::map<bodypart_id, effect>>
{
};

class effect
{
    public:
        effect() : eff_type( nullptr ), duration( 0_turns ), bp( bodypart_str_id::NULL_ID() ),
            permanent( false ), intensity( 1 ), start_time( calendar::turn_zero ),
            source( effect_source::empty() ) {
        }
        explicit effect( const effect_type *peff_type ) : eff_type( peff_type ), duration( 0_turns ),
            bp( bodypart_str_id::NULL_ID() ),
            permanent( false ), intensity( 1 ), start_time( calendar::turn_zero ),
            source( effect_source::empty() ) {
        }
        effect( const effect_source &source, const effect_type *peff_type, const time_duration &dur,
                bodypart_str_id part, bool perm, int nintensity, const time_point &nstart_time ) :
            eff_type( peff_type ), duration( dur ), bp( part ),
            permanent( perm ), intensity( nintensity ), start_time( nstart_time ),
            source( source ) {
            clamp_duration();
            apply_int_dur_factor();
            clamp_intensity();
        }
        effect( const effect & ) = default;
        effect &operator=( const effect & ) = default;


        bool is_null() const;


        static effect null_effect;


        std::string disp_name() const;

        std::string disp_desc( bool reduced = false ) const;

        std::string disp_short_desc( bool reduced = false ) const;

        std::string disp_mod_source_info() const;


        bool use_part_descs() const;


        const effect_type *get_effect_type() const;




        void decay( std::vector<efftype_id> &rem_ids, std::vector<bodypart_id> &rem_bps,
                    const time_point &time, bool player, const effects_map &eff_map = effects_map() );


        time_duration get_duration() const;

        time_duration get_max_duration() const;

        void set_duration( const time_duration &dur, bool alert = false );

        void mod_duration( const time_duration &dur, bool alert = false );

        void mult_duration( double dur, bool alert = false );

        void clamp_duration();

        std::vector<vitamin_applied_effect> vit_effects( bool reduced ) const;


        time_point get_start_time() const;


        bodypart_id get_bp() const;

        void set_bp( const bodypart_str_id &part );


        bool is_permanent() const;

        void pause_effect();

        void unpause_effect();


        int get_intensity() const;

        int get_max_intensity() const;

        int get_max_effective_intensity() const;

        int get_effective_intensity() const;







        int set_intensity( int val, bool alert = false );





        int clamp_intensity();







        int mod_intensity( int mod, bool alert = false );





        int apply_int_dur_factor( bool alert = false );


        const std::vector<trait_id> &get_resist_traits() const;

        const std::vector<efftype_id> &get_resist_effects() const;

        const std::vector<efftype_id> &get_removes_effects() const;

        std::vector<efftype_id> get_blocks_effects() const;


        int get_mod( const std::string &arg, bool reduced = false ) const;

        int get_avg_mod( const std::string &arg, bool reduced = false ) const;

        int get_amount( const std::string &arg, bool reduced = false ) const;

        int get_min_val( const std::string &arg, bool reduced = false ) const;

        int get_max_val( const std::string &arg, bool reduced = false ) const;

        bool get_sizing( const std::string &arg ) const;

        double get_percentage( const std::string &arg, int val, bool reduced = false ) const;


        bool activated( const time_point &when, const std::string &arg, int val,
                        bool reduced = false, double mod = 1 ) const;


        bool has_flag( const flag_id &flag ) const;


        std::vector<limb_score_effect> get_limb_score_data() const;

        std::vector<effect_dur_mod> get_effect_dur_scaling() const;

        bool kill_roll( bool reduced ) const;
        std::string get_death_message() const;
        event_type death_event() const;


        double get_addict_mod( const std::string &arg, int addict_level ) const;

        bool get_harmful_cough() const;

        int get_dur_add_perc() const;

        time_duration get_int_dur_factor() const;

        int get_int_add_val() const;

        int get_int_decay_step() const;

        int get_int_decay_tick() const;

        bool get_int_decay_remove() const;


        const std::vector<std::pair<translation, int>> &get_miss_msgs() const;


        std::string get_speed_name() const;


        bool impairs_movement() const;

        float get_limb_score_mod( const limb_score_id &score, bool reduced = false ) const;


        const efftype_id &get_id() const {
            return eff_type->id;
        }

        const effect_source &get_source() const;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );

    protected:
        const effect_type *eff_type;
        time_duration duration;
        bodypart_str_id bp;
        bool permanent;
        int intensity;
        time_point start_time;
        effect_source source;

};

class effect_migration
{
    public:
        efftype_id id_old;
        std::optional<efftype_id> id_new;

        static void load( const JsonObject &jo );

        static void reset();

        static void check();


        static const effect_migration *find_migration( const efftype_id &original );
};

void load_effect_type( const JsonObject &jo, std::string_view src );
void reset_effect_types();
const std::map<efftype_id, effect_type> &get_effect_types();

std::string texitify_base_healing_power( float power );
std::string texitify_healing_power( int power );
std::string texitify_bandage_power( int power );
nc_color colorize_bleeding_intensity( int intensity );

#endif
