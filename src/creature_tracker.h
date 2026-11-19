#pragma once
#ifndef CATA_SRC_CREATURE_TRACKER_H
#define CATA_SRC_CREATURE_TRACKER_H

#include <cstddef>
#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "coordinates.h"
#include "creature.h"
#include "memory_fast.h"
#include "type_id.h"

class JsonArray;
class JsonOut;
class game;
class monster;
class npc;

class creature_tracker
{
    public:
        creature_tracker();
        ~creature_tracker();





        shared_ptr_fast<monster> find( const tripoint_abs_ms &pos ) const;







        template <typename PredicateFn>
        Creature *find_reachable( const Creature &origin, PredicateFn &&predicate_fn );








        template <typename FactionPredicateFn, typename CreaturePredicateFn>
        Creature *find_reachable( const Creature &origin, FactionPredicateFn &&faction_fn,
                                  CreaturePredicateFn &&creature_fn );





        template <typename VisitFn>
        void for_each_reachable( const Creature &origin, VisitFn &&visit_fn );







        template <typename FactionPredicateFn, typename CreatureVisitFn>
        void for_each_reachable( const Creature &origin, FactionPredicateFn &&faction_fn,
                                 CreatureVisitFn &&creature_fn );








        int temporary_id( const monster &critter ) const;
        shared_ptr_fast<monster> from_temporary_id( int id );






        bool add( const shared_ptr_fast<monster> &critter );
        size_t size() const;


        bool update_pos( const monster &critter, const tripoint_abs_ms &old_pos,
                         const tripoint_abs_ms &new_pos );

        void remove( const monster &critter );
        void clear();
        void clear_npcs() {
            active_npc.clear();
        }

        void swap_positions( monster &first, monster &second );

        bool kill_marked_for_death();

        void remove_dead();







        template<typename T = Creature>
        T * creature_at( const tripoint_bub_ms &p, bool allow_hallucination = false );
        template<typename T = Creature>
        T * creature_at( const tripoint_abs_ms &p, bool allow_hallucination = false );
        template<typename T = Creature>
        const T * creature_at( const tripoint_bub_ms &p, bool allow_hallucination = false ) const;
        template<typename T = Creature>
        const T * creature_at( const tripoint_abs_ms &p, bool allow_hallucination = false ) const;

        const std::vector<shared_ptr_fast<monster>> &get_monsters_list() const {
            return monsters_list;
        }

        void serialize( JsonOut &jsout ) const;
        void deserialize( const JsonArray &ja );




        void invalidate_reachability_cache() {
            dirty_ = true;
        }

    private:

        void remove_from_location_map( const monster &critter );

        void flood_fill_zone( const Creature &origin );

        void rebuild_cache();


        bool is_present( Creature *creature ) const;

        std::list<shared_ptr_fast<npc>> active_npc; // NOLINT(cata-serialize)
        std::vector<shared_ptr_fast<monster>> monsters_list;
        // NOLINTNEXTLINE(cata-serialize)
        std::unordered_map<tripoint_abs_ms, shared_ptr_fast<monster>> monsters_by_location;





        std::unordered_set<shared_ptr_fast<monster>> removed_this_turn_;  // NOLINT(cata-serialize)




        bool dirty_ = true;  // NOLINT(cata-serialize)
        int zone_tick_ = 1;  // NOLINT(cata-serialize)
        int zone_number_ = 0;  // NOLINT(cata-serialize)
        std::unordered_map<int, std::unordered_map<mfaction_id, std::vector<shared_ptr_fast<Creature>>>>
        creatures_by_zone_and_faction_;  // NOLINT(cata-serialize)

        friend game;
};

creature_tracker &get_creature_tracker();



template <typename PredicateFn>
Creature *creature_tracker::find_reachable( const Creature &origin, PredicateFn &&predicate_fn )
{
    return find_reachable( origin, []( const mfaction_id & ) {
        return true;
    }, std::forward<PredicateFn>( predicate_fn ) );
}

template <typename FactionPredicateFn, typename CreaturePredicateFn>
Creature *creature_tracker::find_reachable( const Creature &origin, FactionPredicateFn &&faction_fn,
        CreaturePredicateFn &&creature_fn )
{
    flood_fill_zone( origin );

    const auto map_iter = creatures_by_zone_and_faction_.find( origin.get_reachable_zone() );
    if( map_iter != creatures_by_zone_and_faction_.end() ) {
        for( auto& [faction, creatures] : map_iter->second ) {
            if( !faction_fn( faction ) ) {
                continue;
            }
            for( std::size_t i = 0; i < creatures.size(); ) {
                if( Creature *other = creatures[i].get(); is_present( other ) ) {
                    if( creature_fn( other ) ) {
                        return other;
                    }
                    ++i;
                } else {
                    using std::swap;
                    swap( creatures[i], creatures.back() );
                    creatures.pop_back();
                }
            }
        }
    }
    return nullptr;
}

template <typename VisitFn>
void creature_tracker::for_each_reachable( const Creature &origin, VisitFn &&visit_fn )
{
    find_reachable( origin, [&visit_fn]( Creature * other ) {
        visit_fn( other );
        return false;
    } );
}

template <typename FactionPredicateFn, typename CreatureVisitFn>
void creature_tracker::for_each_reachable( const Creature &origin, FactionPredicateFn &&faction_fn,
        CreatureVisitFn &&creature_fn )
{
    find_reachable( origin, std::forward<FactionPredicateFn>( faction_fn ), [&creature_fn](
    Creature * other ) {
        creature_fn( other );
        return false;
    } );
}

#endif
