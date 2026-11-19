#pragma once
#ifndef CATA_SRC_CRAFT_RESERVATION_H
#define CATA_SRC_CRAFT_RESERVATION_H

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

#include "calendar.h"
#include "character_id.h"
#include "coordinates.h"
#include "type_id.h"

class Creature;
class JsonObject;
class JsonOut;
class item;
class item_location;
class map;

namespace craft_reservation
{

enum class provider_kind : uint8_t {
    item,
    furniture,
    terrain,
    vehicle_part,
    intrinsic,
    environment,
    last
};

enum class requirement_kind : uint8_t { quality, presence_tool, last };


struct binding {
    bool root = false;
    int group_index = -1;
    int alternative_index = -1;


    requirement_kind req = requirement_kind::last;
    quality_id qual;
    int level = 0;
    itype_id tool_type = itype_id::NULL_ID();
    int group_count = 0;

    provider_kind kind = provider_kind::last;
    int64_t provider_uid = 0;
    std::optional<tripoint_abs_ms> tile;
    furn_id furn;
    ter_id ter;

    itype_id pseudo_type = itype_id::NULL_ID();
    character_id intrinsic_owner;



    int occurrence_slot = -1;

    bool established() const {
        return kind != provider_kind::last;
    }

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonObject &data );
};



bool merge_equivalent( const item &lhs, const item &rhs );









bool contains_reserved( const item &it );


bool usable_by_automation( const item &it );




bool contains_live_craft( const item &it );


bool contains_reserved_or_live_craft( const item &it );



bool bashing_would_break_reservation( map &here, const Creature &who,
                                      const tripoint_bub_ms &p );



uint64_t search_expansions_total();
void note_search_expansion();
void reset_search_expansions();



constexpr uint64_t search_budget_initial = 4096;
constexpr uint8_t search_escalation_cap = 4;




constexpr uint64_t search_budget_for_attempt( uint8_t attempts )
{
    return search_budget_initial << std::min( attempts, search_escalation_cap );
}


constexpr uint8_t pool_fingerprint_version = 7;




class scoped_own_claims
{
    public:
        explicit scoped_own_claims( const item &craft );
        ~scoped_own_claims();
        scoped_own_claims( const scoped_own_claims & ) = delete;
        scoped_own_claims &operator=( const scoped_own_claims & ) = delete;
};

}

class craft_reservation_index
{
    public:
        struct record {
            int64_t craft_uid = 0;
            character_id crafter;
            std::optional<tripoint_abs_ms> craft_tile;
            std::vector<tripoint_abs_ms> provider_tiles;
            std::vector<int64_t> provider_item_uids;
            std::vector<int64_t> provider_part_uids;

            time_point expires_at = calendar::before_time_starts;

            bool live( time_point now ) const {
                return expires_at > now;
            }

            bool empty() const {
                return !craft_tile && provider_tiles.empty() && provider_item_uids.empty() &&
                       provider_part_uids.empty();
            }
        };

        void set( const record &rec );
        void erase( int64_t owner_token );
        const record *find( int64_t owner_token ) const;



        bool is_reserved_uid( int64_t item_uid ) const;



        bool any_item_claims() const;


        bool any_claims() const;

        const record *record_for_item_uid( int64_t item_uid ) const;



        bool craft_site_reserved( const tripoint_abs_ms &tile ) const;
        bool provider_tile_reserved( const tripoint_abs_ms &tile ) const;
        bool vehicle_part_reserved( int64_t part_base_uid ) const;



        bool item_claimed_by_other( int64_t item_uid, int64_t owner_token ) const;
        bool part_claimed_by_other( int64_t part_base_uid, int64_t owner_token ) const;
        bool provider_tile_claimed_by_other( const tripoint_abs_ms &tile,
                                             int64_t owner_token ) const;

        void rebuild_for_craft( const item_location &loc );
        void clear();

        void sweep_expired_records();


        uint64_t generation() const;




        void set_overlay( std::set<int64_t> items, std::set<int64_t> parts,
                          std::set<tripoint_abs_ms> tiles );
        void clear_overlay();

    private:

        bool incumbent_is_live( int64_t owner_token ) const;
        void unindex( const record &rec );
        void index( const record &rec );

        std::unordered_map<int64_t, record> by_uid_;
        std::unordered_map<int64_t, int64_t> item_uid_to_owner_;
        std::unordered_map<int64_t, int64_t> part_uid_to_owner_;

        std::map<tripoint_abs_ms, std::set<int64_t>> craft_sites_;
        std::map<tripoint_abs_ms, std::set<int64_t>> provider_tiles_;
        std::set<int64_t> overlay_items_;
        std::set<int64_t> overlay_parts_;
        std::set<tripoint_abs_ms> overlay_tiles_;
        uint64_t generation_ = 0;
};

craft_reservation_index &get_craft_reservations();

#endif
