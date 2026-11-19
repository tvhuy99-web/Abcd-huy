#pragma once
#ifndef CATA_SRC_VPART_POSITION_H
#define CATA_SRC_VPART_POSITION_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "coords_fwd.h"
#include "item.h"
#include "type_id.h"

class Character;
class map;
class temp_crafting_inventory;
class vehicle;
class vehicle_stack;
class vpart_info;
class vpart_reference;
enum vpart_bitflags : int;
struct vehicle_part;

















struct vpart_tool_source {
    item tool;
    int hotkey = -1;
    int part_index = -1;
    int64_t part_base_uid = 0;
};

class vpart_position
{
    private:
        ::vehicle *vehicle_;
        size_t part_index_;

    public:
        vpart_position( ::vehicle &v, const size_t part ) : vehicle_( &v ), part_index_( part ) { }

        ::vehicle &vehicle() const {
            return *vehicle_;
        }

        size_t part_index() const {
            return part_index_;
        }

        bool is_inside() const;




        int get_movecost() const;




        void set_label( const std::string &text ) const;



        std::optional<std::string> get_label() const;

        std::optional<vpart_reference> cargo() const;

        std::optional<vpart_reference> part_with_feature( const std::string &f, bool unbroken,
                bool include_fake = false ) const;

        std::optional<vpart_reference> part_with_feature( vpart_bitflags f, bool unbroken,
                bool include_fake = false ) const;

        std::optional<vpart_reference> avail_part_with_feature( const std::string &f ) const;

        std::optional<vpart_reference> avail_part_with_feature( vpart_bitflags f ) const;





        std::optional<vpart_reference> obstacle_at_part() const;



        std::optional<vpart_reference> part_displayed() const;


        std::optional<vpart_reference> part_with_tool( map &here, const itype_id &tool_type ) const;


        std::optional<vpart_reference> part_with_unreserved_tool( map &here,
                const itype_id &tool_type ) const;

        std::map<item, int> get_tools( map &here ) const;
        std::vector<vpart_tool_source> get_tools_with_sources( map &here ) const;

        void form_inventory( map &here, temp_crafting_inventory &inv, std::set<::vehicle *> &veh ) const;

        bool can_load_furniture() const;
        bool has_loaded_furniture() const;

        tripoint_bub_ms pos_bub( const map &here ) const;
        tripoint_abs_ms pos_abs() const;





        point_rel_ms mount_pos() const;


        bool operator<( const vpart_position &other ) const;
};






class optional_vpart_position : public std::optional<vpart_position>
{
    public:
        explicit optional_vpart_position( std::optional<vpart_position> p ) :
            std::optional<vpart_position>( p ) { }

        std::optional<std::string> get_label() const {
            return has_value() ? value().get_label() : std::nullopt;
        }

        std::optional<vpart_reference> cargo() const;
        std::optional<vpart_reference> part_with_feature( const std::string &f, bool unbroken ) const;
        std::optional<vpart_reference> part_with_feature( vpart_bitflags f, bool unbroken ) const;
        std::optional<vpart_reference> avail_part_with_feature( const std::string &f ) const;
        std::optional<vpart_reference> avail_part_with_feature( vpart_bitflags f ) const;
        std::optional<vpart_reference> obstacle_at_part() const;
        std::optional<vpart_reference> part_displayed() const;
        std::optional<vpart_reference> part_with_tool( map &here, const itype_id &tool_type ) const;
        std::optional<vpart_reference> part_with_unreserved_tool( map &here,
                const itype_id &tool_type ) const;
        std::vector<std::string> extended_description() const;
};









class vpart_reference : public vpart_position
{
    public:
        vpart_reference( ::vehicle &v, const size_t part ) : vpart_position( v, part ) { }
        vpart_reference( const vpart_reference & ) = default;
        vpart_reference &operator=( const vpart_reference & ) = default;

        using vpart_position::vehicle;


        vehicle_part &part() const;

        const vpart_info &info() const;

        vehicle_stack items() const;







        bool has_feature( const std::string &f ) const;
        bool has_feature( vpart_bitflags f ) const;



        Character *get_passenger() const;
};



inline vehicle *veh_pointer_or_null( const optional_vpart_position &p )
{
    return p ? &p->vehicle() : nullptr;
}

#endif
