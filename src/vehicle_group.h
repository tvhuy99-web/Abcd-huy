#pragma once
#ifndef CATA_SRC_VEHICLE_GROUP_H
#define CATA_SRC_VEHICLE_GROUP_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "coords_fwd.h"
#include "mapgen_primitives.h"
#include "rng.h"
#include "type_id.h"
#include "units.h"
#include "weighted_list.h"

class JsonObject;
class VehicleGroup;
class VehicleSpawn;
class map;

using vspawn_id = string_id<VehicleSpawn>;

extern std::unordered_map<vgroup_id, VehicleGroup> vgroups;





class VehicleGroup
{
    public:
        VehicleGroup() = default;

        void add_vehicle( const vproto_id &type, const int &probability ) {
            vehicles.add( type, probability );
        }

        const vproto_id &pick() const {
            return *vehicles.pick();
        }

        std::vector<vproto_id> all_possible_results() const;

        static void load( const JsonObject &jo );
        static void reset();

    private:
        weighted_int_list<vproto_id> vehicles;
};




struct VehicleFacings {
    VehicleFacings( const JsonObject &jo, std::string_view key );

    units::angle pick() const {
        return random_entry( values );
    }

    std::vector<units::angle> values;
};

struct VehicleLocation {
    VehicleLocation( const jmapgen_int &x, const jmapgen_int &y, const VehicleFacings &facings )
        : x( x ), y( y ), facings( facings ) {}

    units::angle pick_facing() const {
        return facings.pick();
    }

    point_bub_ms pick_point() const;

    jmapgen_int x;
    jmapgen_int y;
    VehicleFacings facings;
};




struct VehiclePlacement {
    VehiclePlacement() = default;

    void add( const jmapgen_int &x, const jmapgen_int &y, const VehicleFacings &facings ) {
        locations.emplace_back( x, y, facings );
    }

    const VehicleLocation *pick() const;
    static void load( const JsonObject &jo );
    static void reset();

    using LocationMap = std::vector<VehicleLocation>;
    LocationMap locations;
};







class VehicleFunction
{
    public:
        virtual ~VehicleFunction() = default;
        virtual void apply( map &m, const std::string &terrainid ) const = 0;
};

using vehicle_gen_pointer = void ( * )( map &, std::string_view );

class VehicleFunction_builtin : public VehicleFunction
{
    public:
        explicit VehicleFunction_builtin( const vehicle_gen_pointer &func ) : func( func ) {}
        ~VehicleFunction_builtin() override = default;






        void apply( map &m, const std::string &terrainid ) const override {
            func( m, terrainid );
        }

    private:
        vehicle_gen_pointer func;
};

class VehicleFunction_json : public VehicleFunction
{
    public:
        explicit VehicleFunction_json( const JsonObject &jo );
        ~VehicleFunction_json() override = default;






        void apply( map &m, const std::string &terrain_name ) const override;

    private:
        vgroup_id vehicle;
        jmapgen_int number;
        int fuel;
        int status;

        std::string placement;
        std::optional<VehicleLocation> location;
};





class VehicleSpawn
{
    public:
        VehicleSpawn() = default;

        void add( const double &weight, const std::shared_ptr<VehicleFunction> &func ) {
            types.add( func, weight );
        }






        void apply( map &m, const std::string &terrain_name ) const;







        static void apply( const vspawn_id &id, map &m, const std::string &terrain_name );

        static void load( const JsonObject &jo );
        static void reset();

    private:
        weighted_float_list<std::shared_ptr<VehicleFunction>> types;

        using FunctionMap = std::unordered_map<std::string, vehicle_gen_pointer>;
        static FunctionMap builtin_functions;
};

#endif
