#pragma once
#ifndef CATA_SRC_TRAP_H
#define CATA_SRC_TRAP_H

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "color.h"
#include "coords_fwd.h"
#include "flat_set.h"
#include "magic.h"
#include "translation.h"
#include "type_id.h"
#include "units.h"

class Character;
class Creature;
class JsonObject;
class JsonValue;
class item;
class map;

namespace trapfunc
{




bool none( const tripoint_bub_ms &, Creature *, item * );
bool bubble( const tripoint_bub_ms &p, Creature *c, item *i );
bool glass( const tripoint_bub_ms &p, Creature *c, item *i );
bool cot( const tripoint_bub_ms &p, Creature *c, item *i );
bool beartrap( const tripoint_bub_ms &p, Creature *c, item *i );
bool snare_light( const tripoint_bub_ms &p, Creature *c, item *i );
bool snare_heavy( const tripoint_bub_ms &p, Creature *c, item *i );
bool snare_species( const tripoint_bub_ms &p, Creature *critter, item *trap_item );
bool board( const tripoint_bub_ms &p, Creature *c, item *i );
bool caltrops( const tripoint_bub_ms &p, Creature *c, item *i );
bool caltrops_glass( const tripoint_bub_ms &p, Creature *c, item *i );
bool eocs( const tripoint_bub_ms &p, Creature *critter, item * );
bool tripwire( const tripoint_bub_ms &p, Creature *c, item *i );
bool crossbow( const tripoint_bub_ms &p, Creature *c, item *i );
bool shotgun( const tripoint_bub_ms &p, Creature *c, item *i );
bool blade( const tripoint_bub_ms &p, Creature *c, item *i );
bool landmine( const tripoint_bub_ms &p, Creature *c, item *i );
bool goo( const tripoint_bub_ms &p, Creature *c, item *i );
bool dissector( const tripoint_bub_ms &p, Creature *c, item *i );
bool sinkhole( const tripoint_bub_ms &p, Creature *c, item *i );
bool pit( const tripoint_bub_ms &p, Creature *c, item *i );
bool pit_spikes( const tripoint_bub_ms &p, Creature *c, item *i );
bool pit_glass( const tripoint_bub_ms &p, Creature *c, item *i );
bool lava( const tripoint_bub_ms &p, Creature *c, item *i );
bool boobytrap( const tripoint_bub_ms &p, Creature *c, item *i );
bool temple_flood( const tripoint_bub_ms &p, Creature *c, item *i );
bool temple_toggle( const tripoint_bub_ms &p, Creature *c, item *i );
bool glow( const tripoint_bub_ms &p, Creature *c, item *i );
bool hum( const tripoint_bub_ms &p, Creature *c, item *i );
bool shadow( const tripoint_bub_ms &p, Creature *c, item *i );
bool map_regen( const tripoint_bub_ms &p, Creature *c, item *i );
bool drain( const tripoint_bub_ms &p, Creature *c, item *i );
bool snake( const tripoint_bub_ms &p, Creature *c, item *i );
bool thick_ice( const tripoint_bub_ms &p, Creature *c, item *i );
bool thin_ice( const tripoint_bub_ms &p, Creature *c, item *i );
bool cast_spell( const tripoint_bub_ms &p, Creature *critter, item * );
bool dummy_trap( const tripoint_bub_ms &p, Creature *critter, item * );
}

struct vehicle_handle_trap_data {
    bool remove_trap = false;
    bool do_explosion = false;
    bool is_falling = false;
    int chance = 100;
    int damage = 0;
    int shrapnel = 0;
    int sound_volume = 0;
    translation sound;
    std::string sound_type;
    std::string sound_variant;

    std::vector<std::pair<itype_id, double>> spawn_items;
    trap_str_id set_trap = trap_str_id::NULL_ID();

    void deserialize( const JsonObject &jo );
};

using trap_function = std::function<bool( const tripoint_bub_ms &, Creature *, item * )>;





















struct trap {
        trap_str_id id;
        std::vector<std::pair<trap_str_id, mod_id>> src;
        trap_id loadid;

        bool was_loaded = false;

        int sym = 0;
        nc_color color;
    private:







        int visibility = 1;

        int avoidance = 0;





        int difficulty = 0;

        int trap_radius = 0;
        bool benign = false;
        bool always_invisible = false;
        update_mapgen_id map_regen;
        trap_function act;
        translation name_;

        std::optional<translation> memorial_male;
        std::optional<translation> memorial_female;

        std::optional<translation> trigger_message_u;
        std::optional<translation> trigger_message_npc;

        cata::flat_set<flag_id> _flags;




        units::mass trigger_weight = 500_gram;



        std::pair<int, int> sound_threshold = {0, 0};
        int funnel_radius_mm = 0;

        struct comp {
            itype_id item_type;
            int quantity;
            int charges;
            void deserialize( const JsonValue &jv );
        };

        std::vector<comp> components;
    public:
        std::optional<itype_id> trap_item_type;

        fake_spell spell_data;
        std::vector<effect_on_condition_id> eocs;
        int comfort = 0;
        units::temperature_delta floor_bedding_warmth = 0_C_delta;
        vehicle_handle_trap_data vehicle_data;
        std::string name() const;



        bool is_always_invisible() const {
            return always_invisible;
        }

        bool operator==( const trap_id &id ) const {
            return loadid == id;
        }
        bool operator!=( const trap_id &id ) const {
            return loadid != id;
        }

        bool has_flag( const flag_id &flag ) const {
            return _flags.count( flag );
        }

        bool has_memorial_msg() const {
            return memorial_male && memorial_female;
        }

        std::string memorial_msg( bool male ) const {
            if( male ) {
                return memorial_male->translated();
            }
            return memorial_female->translated();
        }








        void examine( const tripoint_bub_ms &examp ) const;

        update_mapgen_id map_regen_target() const;






        int get_avoidance() const {
            return avoidance;
        }




        bool is_benign() const {
            return benign;
        }




        bool easy_take_down() const;

        bool is_trivial_to_spot() const;

        void set_trap_data( itype_id trap_item_type_id );




        bool can_not_be_disarmed() const;




        bool detected_by_ground_sonar() const;



        bool detected_by_echolocation() const;


        bool detect_trap( const tripoint_bub_ms &pos, const Character &p ) const;




        bool can_see( const tripoint_bub_ms &pos, const Character &p ) const;

        bool has_trigger_msg() const {
            return trigger_message_u && trigger_message_npc;
        }



        std::string get_trigger_message_u() const {
            return trigger_message_u->translated();
        }



        std::string get_trigger_message_npc() const {
            return trigger_message_npc->translated();
        }
    private:










        void trigger( const tripoint_bub_ms &pos, Creature *creature, item *item ) const;
    public:









        void trigger( const tripoint_bub_ms &pos, Creature &creature ) const;
        void trigger( const tripoint_bub_ms &pos, item &item ) const;


        void trigger( const tripoint_bub_ms &pos ) const;




        bool triggered_by_item( const item &itm ) const;






        void on_disarmed( map &m, const tripoint_bub_ms &p ) const;



        int get_trap_radius() const {
            return trap_radius;
        }



        bool is_null() const;



        void load( const JsonObject &jo, std::string_view src );

        std::string debug_describe() const;











        bool is_funnel() const;
        double funnel_turns_per_charge( double rain_depth_mm_per_hour ) const;




        static const std::vector<const trap *> &get_funnels();





        bool has_sound_trigger() const;
        static const std::vector<const trap *> &get_sound_triggered_traps();
        bool triggered_by_sound( int vol, int dist ) const;











        static void load_trap( const JsonObject &jo, const std::string &src );



        static void reset();





        void finalize();
        static void finalize_all();



        static void check_consistency();

        static size_t count();
};

const trap_function &trap_function_from_string( const std::string &function_name );

extern trap_id tr_null;

#endif
