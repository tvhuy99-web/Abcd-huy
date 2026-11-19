#pragma once
#ifndef CATA_SRC_CRAFT_RESERVATION_SEARCH_H
#define CATA_SRC_CRAFT_RESERVATION_SEARCH_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "character_id.h"
#include "coordinates.h"
#include "craft_reservation.h"
#include "game_constants.h"
#include "item.h"
#include "point.h"
#include "type_id.h"

class Character;
class map;
class vehicle;








struct step_source_context {
    tripoint_bub_ms origin;
    int radius = PICKUP_RANGE;
    Character *present_char = nullptr;
};

struct provider_candidate {
    craft_reservation::provider_kind kind = craft_reservation::provider_kind::last;
    const item *it = nullptr;
    int64_t provider_uid = 0;
    std::optional<tripoint_abs_ms> tile;
    furn_id furn;
    ter_id ter;
    itype_id pseudo_type = itype_id::NULL_ID();


    std::optional<item> synthesized;
    character_id intrinsic_owner;

    bool nested = false;


    int64_t root_uid = 0;


    bool carried = false;
};



struct hidden_units {
    std::set<int64_t> item_roots;
    std::set<int64_t> parts;
    std::set<tripoint_abs_ms> tiles;

    bool covers( const provider_candidate &cand ) const;
};





struct candidate_class {
    std::string key;
    std::vector<const provider_candidate *> members;

    std::vector<std::optional<int>> scores;
    bool abstract = false;


    int held = 0;
};


struct search_state {
    std::vector<int> usage;
    std::vector<craft_reservation::binding> selected;
    int next_abstract_slot = 0;
};


struct selection_context {
    const item *craft = nullptr;
    const step_source_context *src = nullptr;
    const std::vector<provider_candidate> *available = nullptr;


    const std::vector<provider_candidate> *bindable = nullptr;
    const std::vector<candidate_class> *classes = nullptr;
    const std::vector<craft_reservation::binding> *requests = nullptr;

    std::vector<std::pair<int, int>> groups;
    std::set<std::string> failed;
    uint64_t expansions_left = 0;



    std::function<bool( const std::vector<craft_reservation::binding> & )> pool_feasible;
};

enum class selection_result : uint8_t {
    feasible,
    infeasible,
    undetermined,
    last
};

using provider_key = std::tuple<int, int64_t, tripoint_abs_ms>;

bool unreserved_filter( const item &it );
item furniture_pseudo_item( map &m, const tripoint_bub_ms &p, const furn_id &f );
bool furniture_pseudo_available( map &m, const tripoint_bub_ms &p, const furn_id &f );
int intrinsic_exposure( const Character &who, const itype_id &type );
int provider_pseudo_quality( const provider_candidate &cand, const quality_id &qual,
                             const Character *who );
const item *candidate_item( const provider_candidate &cand );
bool candidate_merges_with( const provider_candidate &lhs, const provider_candidate &rhs );
const vehicle *linked_power_source( map &here, const item &it );
hidden_units units_hidden_by( const std::vector<craft_reservation::binding> &selected,
                              const std::vector<provider_candidate> &pool );
std::vector<provider_candidate> collapse_merge_classes(
    const std::vector<provider_candidate> &available,
    const std::vector<craft_reservation::binding> &held,
    const std::vector<craft_reservation::binding> &requests, const Character *who );
std::vector<candidate_class> build_candidate_classes(
    const std::vector<provider_candidate> &available,
    const std::vector<provider_candidate> &pool,
    const std::vector<craft_reservation::binding> &requests,
    const std::vector<craft_reservation::binding> &held, const Character *who );
std::vector<size_t> classes_for_request( const std::vector<candidate_class> &classes,
        size_t request_index );
uint64_t pool_fingerprint( const std::vector<candidate_class> &classes,
                           const std::vector<provider_candidate> &available, Character *who );
selection_result assign_from( selection_context &ctx, size_t depth, int remaining,
                              search_state &state );

#endif
