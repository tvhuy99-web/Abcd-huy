#pragma once
#ifndef CATA_SRC_ITEM_WAKEUP_H
#define CATA_SRC_ITEM_WAKEUP_H

#include <cstdint>
#include <map>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "calendar.h"
#include "character_id.h" // IWYU pragma: keep
#include "colony.h"
#include "coordinates.h"
#include "point.h"
#include "type_id.h"

class JsonObject;
class JsonOut;
class item;
class item_location;


enum class item_wakeup_kind : uint8_t {
    alarm = 0,
    env_check,
    ready_check,
    fail_check,
    last
};



struct vehicle_hint {
    tripoint_abs_ms cargo_square;
    int part_index = -1;
    point_rel_ms mount_offset;
};


struct item_locator_hint {
    enum class place : uint8_t { map, vehicle, character, unknown };
    place where = place::unknown;
    std::variant<std::monostate, tripoint_abs_ms, vehicle_hint, character_id> location;

    item_locator_hint() = default;
};


struct desired_wakeup {
    item_wakeup_kind kind;
    time_point when;
};


struct scheduled_wakeup_info {
    int64_t uid;
    item_wakeup_kind kind;
    time_point when;
    item_locator_hint hint;
};



class item_wakeup_manager
{
    public:
        struct stats {
            int total_pending = 0;
            int stale_resolution = 0;
            int duplicate_event = 0;
            int dropped_on_load = 0;
        };


        void schedule_or_update( int64_t uid, time_point when,
                                 item_wakeup_kind kind, item_locator_hint hint );
        void cancel( int64_t uid, item_wakeup_kind kind );
        void cancel_all( int64_t uid );



        void rebuild_for_item( const item_location &loc );

        bool is_scheduled( int64_t uid, item_wakeup_kind kind ) const;
        std::optional<time_point> get( int64_t uid, item_wakeup_kind kind ) const;
        std::optional<scheduled_wakeup_info> peek_next_wakeup() const;

        std::vector<scheduled_wakeup_info> dump() const;
        stats get_stats() const;



        void process( time_point now );


        void clear();

        void serialize( JsonOut &jsout ) const;
        void deserialize( const JsonObject &jo );

    private:
        struct entry {
            int64_t uid = 0;
            item_wakeup_kind kind = item_wakeup_kind::alarm;
            time_point when = calendar::before_time_starts;
            item_locator_hint hint;
        };

        using entry_iter = cata::colony<entry>::iterator;
        using key_t = std::pair<int64_t, item_wakeup_kind>;


        void clean_heap_top();
        bool is_live( const entry_iter &it ) const;
        static bool heap_greater( const entry_iter &a, const entry_iter &b );

        cata::colony<entry> entries_;



        std::map<key_t, entry_iter> by_key_;


        std::vector<entry_iter> heap_; // NOLINT(cata-serialize)
        stats stats_; // NOLINT(cata-serialize)
        int stale_seen_uids_ = 0; // NOLINT(cata-serialize)
        bool stale_msg_emitted_ = false; // NOLINT(cata-serialize)
};

item_wakeup_manager &get_item_wakeups();



using item_wakeup_test_handler =
    void( * )( item &it, item_wakeup_kind kind, time_point now,
               const item_location &loc );
void register_test_wakeup_handler( const itype_id &id, item_wakeup_test_handler fn );
void clear_test_wakeup_handlers();


void actualize_scheduled_dispatch( item &it, item_wakeup_kind kind, time_point now,
                                   const item_location &loc );


using item_wakeup_test_enumerator =
    std::vector<desired_wakeup>( * )( const item &it, const item_location &loc );
void register_test_enumerate_handler( const itype_id &id,
                                      item_wakeup_test_enumerator fn );
void clear_test_enumerate_handlers();


std::vector<desired_wakeup> enumerate_scheduled_dispatch( const item &it,
        const item_location &loc );

#endif
