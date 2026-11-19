#pragma once
#ifndef CATA_SRC_FIELD_H
#define CATA_SRC_FIELD_H

#include <map>
#include <string>
#include <vector>

#include "calendar.h"
#include "cata_lazy.h"
#include "color.h"
#include "effect_source.h"
#include "enums.h"
#include "field_type.h"
#include "type_id.h"

class Creature;





class field_entry
{
    public:
        field_entry() : type( fd_null.id_or( INVALID_FIELD_TYPE_ID ) ), intensity( 1 ), age( 0_turns ),
            is_alive( false ) { }
        field_entry( const field_type_id &t, const int i, const time_duration &a,
                     effect_source source = effect_source() ) : type( t ),
            intensity( i ), age( a ), causer( source ), is_alive( true ) { }

        nc_color color() const;

        std::string symbol() const;


        const field_intensity_level &get_intensity_level() const;


        field_type_id get_field_type() const;




        field_type_id set_field_type( const field_type_id &new_type );


        int get_max_field_intensity() const;

        int get_field_intensity() const;

        int set_field_intensity( int new_intensity );
        void mod_field_intensity( int mod );


        Creature *get_causer() const;

        void set_causer( effect_source source );


        effect_source get_effect_source() const;


        time_duration get_field_age() const;


        time_duration set_field_age( const time_duration &new_age );


        time_duration mod_field_age( const time_duration &mod_age ) {
            return set_field_age( get_field_age() + mod_age );
        }

        bool is_dangerous() const;
        bool is_mopsafe() const;



        std::string name() const {
            return type.obj().get_name( intensity - 1 );
        }


        bool is_field_alive() const {
            return intensity > 0 && is_alive;
        }

        bool gas_can_spread() const {
            return is_field_alive() && type.obj().phase == phase_id::GAS && type.obj().percent_spread > 0;
        }

        void initialize_decay();
        void do_decay();

        std::vector<field_effect> field_effects() const;

    private:

        field_type_id type;

        int intensity;

        time_duration age;

        effect_source causer;

        time_point decay_time;

        bool is_alive;
};









class field
{
    public:
        field();






        field_entry *find_field( const field_type_id &field_type_to_find, bool alive_only = true );





        const field_entry *find_field( const field_type_id &field_type_to_find,
                                       bool alive_only = true ) const;








        bool add_field( const field_type_id &field_type_to_add, int new_intensity = 1,
                        const time_duration &new_age = 0_turns, effect_source source = effect_source() );







        bool remove_field( const field_type_id &field_to_remove );




        void remove_field( std::map<field_type_id, field_entry>::iterator );





        void clear();


        unsigned int field_count() const;




        field_type_id displayed_field_type() const;




        int displayed_intensity() const;

        description_affix displayed_description_affix() const;


        std::map<field_type_id, field_entry>::iterator begin();
        std::map<field_type_id, field_entry>::const_iterator begin() const;


        std::map<field_type_id, field_entry>::iterator end();
        std::map<field_type_id, field_entry>::const_iterator end() const;




        int total_move_cost() const;


        bool any_negative_move_cost() const;

    private:

        lazy<std::map<field_type_id, field_entry>> _field_type_list;

        field_type_id _displayed_field_type;
};

#endif
