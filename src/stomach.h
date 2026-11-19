#pragma once
#ifndef CATA_SRC_STOMACH_H
#define CATA_SRC_STOMACH_H

#include <array>
#include <cstdint>
#include <map>
#include <string_view>
#include <utility>
#include <variant>

#include "calendar.h"
#include "type_id.h"
#include "units.h"

class Character;
class JsonObject;
class JsonOut;
struct needs_rates;

namespace vitamin_units
{
using mass = units::quantity<int, units::mass_in_microgram_tag>;

constexpr mass microgram = units::quantity<int, units::mass_in_microgram_tag>( 1, {} );
constexpr mass milligram = units::quantity<int, units::mass_in_microgram_tag>( 1000, {} );
constexpr mass gram = units::quantity<int, units::mass_in_microgram_tag>( 1000000, {} );
constexpr std::array<std::pair<std::string_view, mass>, 5> mass_units = { {
        { "ug", microgram },
        { "μg", microgram },
        { "mcg", microgram },
        { "mg", milligram },
        { "g", gram }
    }
};
}



struct nutrients {
        friend struct islot_comestible;

        int64_t calories = 0;



        void min_in_place( const nutrients &r );
        void max_in_place( const nutrients &r );


        std::map<vitamin_id, int> vitamins() const;




        void set_vitamin( const vitamin_id &, vitamin_units::mass mass );
        void add_vitamin( const vitamin_id &, vitamin_units::mass mass );

        void set_vitamin( const vitamin_id &, int units );
        void add_vitamin( const vitamin_id &, int units );


        void remove_vitamin( const vitamin_id & );

        int get_vitamin( const vitamin_id & ) const;
        int kcal() const;

        void finalize_vitamins();

        bool operator==( const nutrients &r ) const;
        bool operator!=( const nutrients &r ) const {
            return !( *this == r );
        }

        nutrients operator-();
        nutrients operator-( const nutrients &r );

        nutrients &operator+=( const nutrients &r );
        nutrients &operator-=( const nutrients &r );
        nutrients &operator*=( int r );
        nutrients &operator*=( double r );
        nutrients &operator/=( int r );

        friend nutrients operator*( nutrients l, int r ) {
            l *= r;
            return l;
        }

        friend nutrients operator/( nutrients l, int r ) {
            l /= r;
            return l;
        }




        bool finalized = true; // NOLINT(cata-serialize)

        void serialize( JsonOut & ) const;
        void deserialize( const JsonObject &jo );

        void clear_vitamins();

        void ensure_positive();

    private:

        std::map<vitamin_id, std::variant<int, vitamin_units::mass>>
                vitamins_; // NOLINT(cata-serialize)
};


struct food_summary {
    units::volume water;
    units::volume solids;
    nutrients nutr;
};



struct stomach_digest_rates {
    units::volume solids = 0_ml;
    units::volume water = 0_ml;
    float percent_kcal = 0.0f;

    int min_calories = 0;
    float percent_vitamin = 0.0f;
    int min_vitamin = 0;
};


class stomach_contents
{
    public:
        stomach_contents();







        stomach_contents( units::volume max_volume, bool is_stomach );






        void ingest( const food_summary &ingested );















        food_summary digest( const Character &owner, const needs_rates &metabolic_rates,
                             int five_mins, int half_hours );


        void empty();








        units::volume capacity( const Character &owner ) const;

        bool would_be_engorged_with( const Character &owner, units::volume intake,
                                     bool calorie_deficit ) const;
        bool would_be_full_with( const Character &owner, units::volume intake, bool calorie_deficit ) const;

        units::volume stomach_remaining( const Character &owner ) const;

        units::volume contains() const;

        int get_calories() const;
        units::volume get_water() const;


        void mod_calories( int kcal );


        void mod_nutr( int nutr );


        void mod_water( const units::volume &h2o );


        void mod_quench( int quench );

        void mod_contents( const units::volume &vol );



        time_duration time_since_ate() const;

        void ate();

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );

    private:


        bool stomach = false; // NOLINT(cata-serialize)


        nutrients nutr;

        units::volume water = 0_ml;





        units::volume max_volume = 0_ml;

        units::volume contents = 0_ml;

        time_point last_ate = calendar::turn_zero;


        stomach_digest_rates get_digest_rates( const needs_rates &metabolic_rates,
                                               const Character &owner ) const;

};

#endif
