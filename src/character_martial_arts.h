#pragma once
#ifndef CATA_SRC_CHARACTER_MARTIAL_ARTS_H
#define CATA_SRC_CHARACTER_MARTIAL_ARTS_H

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "damage.h"
#include "martialarts.h"
#include "type_id.h"

class Character;
class JsonObject;
class JsonOut;
class item;
class item_location;

class character_martial_arts
{
    private:
        std::vector<matype_id> ma_styles;
        matype_id style_selected = matype_id( "style_none" );
    public:
        character_martial_arts();
        character_martial_arts( const std::vector<matype_id> &styles,
                                const matype_id &style_selected, bool keep_hands_free )
            : ma_styles( styles ), style_selected( style_selected ), keep_hands_free( keep_hands_free ) {}

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );

        void reset_style();
        void clear_style( const matype_id &id );

        void selected_style_check();
        bool keep_hands_free = false;

        bool pick_style( const Character &you );

        bool knows_selected_style() const;
        bool selected_strictly_melee() const;
        bool selected_allow_all_weapons() const;
        bool selected_has_weapon( const itype_id &weap ) const;
        bool selected_force_unarmed() const;
        bool selected_prevent_weapon_blocking() const;
        bool selected_is_none() const;


        bool has_martialart( const matype_id &ma_id ) const;

        void add_martialart( const matype_id &ma_id );
        void learn_current_style_CQB( bool is_avatar );
        void learn_style( const matype_id &mastyle, bool is_avatar );
        void set_style( const matype_id &mastyle, bool force = false );
        void clear_styles();

        void martialart_use_message( const Character &owner ) const;


        void clear_all_effects( Character &owner );

        void ma_static_effects( Character &owner );

        void ma_onmove_effects( Character &owner );

        void ma_onpause_effects( Character &owner );

        void ma_onhit_effects( Character &owner );

        void ma_onattack_effects( Character &owner );

        void ma_ondodge_effects( Character &owner );

        void ma_onblock_effects( Character &owner );

        void ma_ongethit_effects( Character &owner );

        void ma_onmiss_effects( Character &owner );

        void ma_oncrit_effects( Character &owner );

        void ma_onkill_effects( Character &owner );


        std::optional<std::pair<attack_vector_id, sub_bodypart_str_id>> choose_attack_vector(
                    const Character &user, const matec_id &tech ) const;

        damage_instance calculate_vector_damage( const Character &user, const attack_vector_id &vec,
                const sub_bodypart_str_id &contact_area ) const;

        bool can_leg_block( const Character &owner ) const;

        bool can_arm_block( const Character &owner ) const;

        bool can_nonstandard_block( const Character &owner ) const;

        bool is_force_unarmed() const;

        bool can_weapon_block() const;

        std::vector<matec_id> get_all_techniques( const item_location &weap, const Character &u ) const;
        std::vector<matype_id> get_unknown_styles( const character_martial_arts &from,
                bool teachable_only ) const;
        std::vector<matype_id> get_known_styles( bool teachable_only ) const;

        bool has_technique( const Character &guy, const matec_id &id, const item &weap ) const;

        ma_technique get_grab_break( const Character &owner ) const;

        ma_technique get_miss_recovery( const Character &owner ) const;

        std::string enumerate_known_styles( const itype_id &weap ) const;
        std::string selected_style_name( const Character &owner ) const;
        const matype_id &selected_style() const {
            return style_selected;
        }
};

#endif
