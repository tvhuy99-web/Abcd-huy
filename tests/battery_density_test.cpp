#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "item_factory.h"
#include "itype.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

static const ammotype ammo_battery( "battery" );
static const flag_id json_flag_DEBUG_ONLY( "DEBUG_ONLY" );

namespace
{
struct battery_chemistry_family {
    float max_energy_density_kj_kg;
    float max_energy_density_kj_l;
};
}

// NOLINTBEGIN(cata-static-string_id-constants)
static const std::map<itype_id, std::string> battery_to_chemistry = {
    {itype_id( "light_minus_battery_cell" ), "LiON"},
    {itype_id( "medium_battery_cell" ), "LiON"},
    {itype_id( "heavy_battery_cell" ), "LiON"},
    {itype_id( "heavy_plus_battery_cell" ), "LiON"},
    {itype_id( "folding_solar_panel_deployed" ), "LiON"},
    {itype_id( "folding_solar_panel_v2_deployed" ), "LiON"},
    {itype_id( "power_station_portable" ), "LiON"},
    {itype_id( "rm13_battery" ), "LiON"},
    {itype_id( "light_cell_rechargeable" ), "Nickel-Metal Hydride"},

    {itype_id( "light_battery_cell" ), "Lithium/Iron Disulfide"},

    {itype_id( "heavy_atomic_battery_cell" ), "atomic"},
    {itype_id( "huge_atomic_battery_cell" ), "atomic"},

    {itype_id( "light_minus_disposable_cell" ), "Lithium-Manganese"},

    {itype_id( "battery_car" ), "Lead-acid"},
    {itype_id( "battery_motorbike" ), "Lead-acid"},
    {itype_id( "battery_motorbike_small" ), "Lead-acid"},



    {itype_id( "large_storage_battery" ), "LiON"},
    {itype_id( "medium_storage_battery" ), "LiON"},
    {itype_id( "small_storage_battery" ), "LiON"},
    {itype_id( "storage_battery" ), "LiON"}
};
// NOLINTEND(cata-static-string_id-constants)











































static const std::map<std::string, battery_chemistry_family> chemistries = {
    {std::string( "LiON" ), { 740.0, 3330.0 }},
    {std::string( "atomic" ), { 3000.0, 5000.0 }},
    {std::string( "Lithium-Manganese" ), { 1010.0, 2090.0}},
    {std::string( "Alkaline" ), { 600.0, 1430.0}},
    {std::string( "Zinc-Carbon" ), { 130.0, 331.0}},
    {std::string( "Nickel-Metal Hydride" ), { 400.0, 1550.0}},
    {std::string( "Zinc-Manganese Dioxide" ), { 590.0, 1430.0}},
    {std::string( "Lithium/Iron Disulfide" ), { 1069.0, 2160.0}},
    {std::string( "Lead-acid" ), { 180, 360 }}
};

static bool is_battery( const itype &type )
{


    if( type.has_flag( json_flag_DEBUG_ONLY ) ||
        type.src.size() > 1 ||
        ( type.src.size() == 1 && type.src.back().second.str() != std::string( "dda" ) ) ) {
        return false;
    }
    if( !!type.magazine && type.magazine->type.count( ammo_battery ) > 0 ) {
        return true;
    }
    return false;
}

TEST_CASE( "battery_density_check" )
{
    for( const itype *id : Item_factory::find( is_battery ) ) {
        auto chemistry_name = battery_to_chemistry.find( id->get_id() );
        battery_chemistry_family chemistry;
        if( chemistry_name == battery_to_chemistry.end() ) {
            WARN( "Didn't find a battery chemistry for " << id->get_id().str().c_str() <<
                  ", assuming Alkaline." );
            chemistry = chemistries.at( "Alkaline" );
        } else {
            chemistry = chemistries.at( chemistry_name->second );
        }
        float capacity_in_kj = id->magazine ? id->magazine->capacity : id->tool->max_charges;
        float weight_in_kg = units::to_kilogram( id->weight );
        float volume_in_L = units::to_liter( id->volume );
        INFO( id->get_id().str().c_str() );
        CAPTURE( capacity_in_kj );
        CAPTURE( weight_in_kg );
        CAPTURE( volume_in_L );
        INFO( "Capacity should be: " << std::min( chemistry.max_energy_density_kj_kg * weight_in_kg,
                chemistry.max_energy_density_kj_l * volume_in_L ) << " kJ" );
        CHECK( capacity_in_kj / weight_in_kg <= chemistry.max_energy_density_kj_kg );
        CHECK( capacity_in_kj / volume_in_L <= chemistry.max_energy_density_kj_l );
    }
}
