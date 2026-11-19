#include "craft_reservation_search.h"

#include <algorithm>
#include <climits>
#include <map>
#include <memory>
#include <string_view>

#include "calendar.h"
#include "character.h"
#include "enums.h"
#include "flag.h"
#include "inventory.h"
#include "itype.h"
#include "map.h"
#include "mapdata.h"
#include "pocket_type.h"
#include "requirements.h"
#include "safe_reference.h"
#include "value_ptr.h"
#include "vpart_position.h"


item furniture_pseudo_item( map &m, const tripoint_bub_ms &p, const furn_id &f )
{
    item pseudo( f->crafting_pseudo_item, calendar::turn );
    if( !pseudo.has_pocket_type( pocket_type::MAGAZINE ) ) {
        return pseudo;
    }
    const furn_t &fo = f.obj();
    for( const itype *ammo : fo.crafting_ammo_item_types() ) {
        itype_id ammo_id = ammo->get_id();
        const int amount = fo.has_flag( ter_furn_flag::TFLAG_AMMOTYPE_RELOAD )
                           ? count_charges_in_list( &ammo->ammo->type, m.i_at( p ), ammo_id )
                           : count_charges_in_list( ammo, m.i_at( p ) );
        if( amount > 0 ) {
            pseudo.force_insert_item( item( ammo_id, calendar::turn, amount ),
                                      pocket_type::MAGAZINE );
        }
    }
    return pseudo;
}


int intrinsic_exposure( const Character &who, const itype_id &type )
{
    int exposed = 0;
    for( const item &pseudo : who.crafting_pseudo_items() ) {
        if( pseudo.typeId() == type ) {
            ++exposed;
        }
    }
    return exposed;
}


int provider_pseudo_quality( const provider_candidate &cand, const quality_id &qual,
                             const Character *who )
{
    const item pseudo = cand.synthesized ? *cand.synthesized
                        : item( cand.pseudo_type, calendar::turn );
    return provider_quality_level( pseudo, qual, who, true );
}


bool furniture_pseudo_available( map &m, const tripoint_bub_ms &p, const furn_id &f )
{
    const itype_id &pseudo_id = f->crafting_pseudo_item;
    return !pseudo_id.is_valid() || !pseudo_id->has_flag( flag_NEEDS_SUNLIGHT ) ||
           tile_has_sufficient_sunlight( m, p );
}


const item *candidate_item( const provider_candidate &cand )
{
    if( cand.it != nullptr ) {
        return cand.it;
    }
    return cand.synthesized ? &*cand.synthesized : nullptr;
}



bool candidate_merges_with( const provider_candidate &lhs, const provider_candidate &rhs )
{
    const item *a = candidate_item( lhs );
    const item *b = candidate_item( rhs );
    return a != nullptr && b != nullptr && a->made_of( phase_id::LIQUID ) &&
           b->made_of( phase_id::LIQUID ) && craft_reservation::merge_equivalent( *a, *b );
}

static provider_key provider_identity( const provider_candidate &cand )
{
    switch( cand.kind ) {
        case craft_reservation::provider_kind::item:
        case craft_reservation::provider_kind::vehicle_part:
            return provider_key{ 1, cand.provider_uid, tripoint_abs_ms() };
        case craft_reservation::provider_kind::furniture:
        case craft_reservation::provider_kind::terrain:
            return provider_key{ 2, 0, cand.tile.value_or( tripoint_abs_ms() ) };
        default:
            return provider_key{ 0, 0, tripoint_abs_ms() };
    }
}


static bool provider_is_exclusive( const provider_candidate &cand )
{
    return std::get<0>( provider_identity( cand ) ) != 0;
}




const vehicle *linked_power_source( map &here, const item &it )
{
    if( !it.has_link_data() ) {
        return nullptr;
    }
    if( it.link().t_veh ) {
        return it.link().t_veh.get();
    }
    const optional_vpart_position vp = here.veh_at( it.link().t_abs_pos );
    return vp ? &vp->vehicle() : nullptr;
}

hidden_units units_hidden_by(
    const std::vector<craft_reservation::binding> &selected,
    const std::vector<provider_candidate> &pool )
{
    hidden_units hidden;
    for( const craft_reservation::binding &b : selected ) {
        switch( b.kind ) {
            case craft_reservation::provider_kind::item: {


                const auto found = std::find_if( pool.begin(), pool.end(),
                [&b]( const provider_candidate & c ) {
                    return c.kind == craft_reservation::provider_kind::item &&
                           c.provider_uid == b.provider_uid;
                } );
                hidden.item_roots.insert( found == pool.end() ? b.provider_uid : found->root_uid );
                break;
            }
            case craft_reservation::provider_kind::vehicle_part:
                hidden.parts.insert( b.provider_uid );
                break;
            case craft_reservation::provider_kind::furniture:
            case craft_reservation::provider_kind::terrain:
                if( b.tile ) {
                    hidden.tiles.insert( *b.tile );
                }
                break;
            default:
                break;
        }
    }
    return hidden;
}


static std::optional<int> score_reservation_candidate(
    const provider_candidate &cand, const craft_reservation::binding &req,
    const Character *who )
{

    switch( cand.kind ) {
        case craft_reservation::provider_kind::furniture:
        case craft_reservation::provider_kind::terrain: {
            if( req.req != craft_reservation::requirement_kind::quality ) {
                return cand.pseudo_type == req.tool_type ? std::optional<int>( 0 )
                       : std::nullopt;
            }
            const int level = provider_pseudo_quality( cand, req.qual, who );
            return level >= req.level ? std::optional<int>( level ) : std::nullopt;
        }
        case craft_reservation::provider_kind::vehicle_part: {
            if( req.req != craft_reservation::requirement_kind::quality ) {
                return cand.pseudo_type == req.tool_type ? std::optional<int>( 1 )
                       : std::nullopt;
            }
            const int level = provider_pseudo_quality( cand, req.qual, who );
            return level >= req.level ? std::optional<int>( 1 + level ) : std::nullopt;
        }
        case craft_reservation::provider_kind::environment: {

            if( req.req != craft_reservation::requirement_kind::quality ) {
                return cand.pseudo_type == req.tool_type ? std::optional<int>( 0 )
                       : std::nullopt;
            }
            return provider_pseudo_quality( cand, req.qual, who ) >= req.level
                   ? std::optional<int>( 0 )
                   : std::nullopt;
        }
        case craft_reservation::provider_kind::intrinsic: {
            if( who == nullptr || who->getID() != cand.intrinsic_owner ) {
                return std::nullopt;
            }
            if( req.req != craft_reservation::requirement_kind::presence_tool ) {
                if( !cand.pseudo_type.is_null() ) {
                    return std::nullopt;
                }



                if( !who->has_intrinsic_quality( req.qual, req.level, req.group_count ) ) {
                    return std::nullopt;
                }
                return std::optional<int>( 100 );
            }
            if( cand.pseudo_type != req.tool_type ) {
                return std::nullopt;
            }
            return std::optional<int>( 100 );
        }
        case craft_reservation::provider_kind::item:
            break;
        default:
            return std::nullopt;
    }

    if( cand.it == nullptr || cand.it->has_flag( flag_ITEM_BROKEN ) ) {
        return std::nullopt;
    }
    if( req.req == craft_reservation::requirement_kind::quality ) {
        const int level = provider_quality_level( *cand.it, req.qual, who,
                          true );
        if( level < req.level ) {
            return std::nullopt;
        }
        return 10000 + ( cand.nested ? 1000 : 0 ) + level * 10 +
               ( cand.it->is_favorite ? 1 : 0 );
    }
    if( cand.it->typeId() != req.tool_type ) {
        return std::nullopt;
    }
    return 10000 + ( cand.nested ? 1000 : 0 ) + ( cand.it->is_favorite ? 1 : 0 );
}




std::vector<provider_candidate> collapse_merge_classes(
    const std::vector<provider_candidate> &available,
    const std::vector<craft_reservation::binding> &held,
    const std::vector<craft_reservation::binding> &requests, const Character *who )
{
    std::vector<const provider_candidate *> order;
    order.reserve( available.size() );
    for( const provider_candidate &cand : available ) {
        order.push_back( &cand );
    }
    const auto best_score = [&requests, who]( const provider_candidate & cand ) {
        std::optional<int> best;
        for( const craft_reservation::binding &req : requests ) {
            const std::optional<int> score = score_reservation_candidate( cand, req, who );
            if( score && ( !best || *score < *best ) ) {
                best = score;
            }
        }
        return best;
    };
    const auto already_held = [&held]( const provider_candidate & cand ) {
        for( const craft_reservation::binding &b : held ) {
            if( b.kind == cand.kind && b.provider_uid == cand.provider_uid &&
                b.pseudo_type == cand.pseudo_type ) {
                return true;
            }
        }
        return false;
    };


    std::stable_sort( order.begin(), order.end(),
    [&]( const provider_candidate * a, const provider_candidate * b ) {
        if( already_held( *a ) != already_held( *b ) ) {
            return already_held( *a );
        }
        const std::optional<int> lhs = best_score( *a );
        const std::optional<int> rhs = best_score( *b );
        if( lhs.has_value() != rhs.has_value() ) {
            return lhs.has_value();
        }
        if( lhs && *lhs != *rhs ) {
            return *lhs < *rhs;
        }
        return provider_identity( *a ) < provider_identity( *b );
    } );

    std::vector<const provider_candidate *> kept;
    for( const provider_candidate *cand : order ) {
        const bool merged = std::any_of( kept.begin(), kept.end(),
        [cand]( const provider_candidate * other ) {
            return candidate_merges_with( *cand, *other );
        } );
        if( !merged ) {
            kept.push_back( cand );
        }
    }

    std::vector<provider_candidate> out;
    out.reserve( kept.size() );

    for( const provider_candidate &cand : available ) {
        if( std::find( kept.begin(), kept.end(), &cand ) != kept.end() ) {
            out.push_back( cand );
        }
    }
    return out;
}




static bool abstract_capacity_allows( const provider_candidate &cand,
                                      const craft_reservation::binding &req,
                                      const step_source_context &src,
                                      const std::vector<provider_candidate> &pool, int wanted )
{
    if( cand.kind == craft_reservation::provider_kind::intrinsic ) {
        if( src.present_char == nullptr ) {
            return false;
        }
        if( req.req == craft_reservation::requirement_kind::presence_tool ) {
            return intrinsic_exposure( *src.present_char, req.tool_type ) >= wanted;
        }
        return src.present_char->has_intrinsic_quality( req.qual, req.level, wanted );
    }
    if( cand.kind == craft_reservation::provider_kind::environment ) {
        const int sources = std::count_if( pool.begin(), pool.end(),
        [&cand]( const provider_candidate & other ) {
            return other.kind == craft_reservation::provider_kind::environment &&
                   other.pseudo_type == cand.pseudo_type;
        } );
        return sources >= wanted;
    }
    return true;
}



static std::string exclusion_signature( const provider_candidate &cand,
                                        const std::vector<provider_candidate> &pool )
{
    std::map<itype_id, int> charges;
    std::set<itype_id> tools;
    for( const provider_candidate &other : pool ) {
        const bool same_unit =
            cand.kind == craft_reservation::provider_kind::item
            ? other.kind == craft_reservation::provider_kind::item &&
            other.root_uid == cand.root_uid
            : cand.kind == craft_reservation::provider_kind::vehicle_part
            ? other.kind == craft_reservation::provider_kind::vehicle_part &&
            other.provider_uid == cand.provider_uid


            : cand.tile && other.tile == cand.tile &&
            ( other.kind == craft_reservation::provider_kind::furniture ||
              other.kind == craft_reservation::provider_kind::terrain );
        if( !same_unit ) {
            continue;
        }
        if( other.it != nullptr ) {
            charges[other.it->typeId()] += other.it->ammo_remaining();
        } else {
            tools.insert( other.pseudo_type );
        }
    }
    std::string sig;
    for( const std::pair<const itype_id, int> &c : charges ) {
        sig += c.first.str() + ':' + std::to_string( c.second ) + ',';
    }
    sig += '|';
    for( const itype_id &t : tools ) {
        sig += t.str() + ',';
    }
    return sig;
}



static std::string candidate_class_key( const provider_candidate &cand,
                                        const std::vector<std::optional<int>> &scores,
                                        const std::string &signature )
{
    std::string key = std::to_string( static_cast<int>( cand.kind ) ) + '/' +
                      cand.pseudo_type.str() + '/' +
                      ( cand.it != nullptr ? cand.it->typeId().str() : std::string() ) + '/' +
                      ( cand.carried ? '1' : '0' ) + '/';
    for( const std::optional<int> &s : scores ) {
        key += s ? std::to_string( *s ) : std::string( "x" );
        key += ',';
    }
    return key + signature;
}

static std::string exclusion_unit_key( const provider_candidate &cand )
{
    switch( cand.kind ) {
        case craft_reservation::provider_kind::item:
            return "i" + std::to_string( cand.root_uid );
        case craft_reservation::provider_kind::vehicle_part:
            return "p" + std::to_string( cand.provider_uid );
        case craft_reservation::provider_kind::furniture:
        case craft_reservation::provider_kind::terrain:
            return cand.tile ? "t" + cand.tile->to_string() : std::string( "t" );
        default:
            return "a";
    }
}





std::vector<candidate_class> build_candidate_classes(
    const std::vector<provider_candidate> &available,
    const std::vector<provider_candidate> &pool,
    const std::vector<craft_reservation::binding> &requests,
    const std::vector<craft_reservation::binding> &held, const Character *who )
{
    std::set<provider_key> held_keys;
    for( const craft_reservation::binding &b : held ) {
        if( b.kind == craft_reservation::provider_kind::item ||
            b.kind == craft_reservation::provider_kind::vehicle_part ) {
            held_keys.insert( provider_key{ 1, b.provider_uid, tripoint_abs_ms() } );
        } else if( b.tile ) {
            held_keys.insert( provider_key{ 2, 0, *b.tile } );
        }
    }
    std::set<const provider_candidate *> held_members;
    std::map<std::string, candidate_class> by_key;

    std::map<std::string, size_t> unit_ids;
    std::map<const provider_candidate *, size_t> unit_of;
    for( const provider_candidate &cand : available ) {
        std::vector<std::optional<int>> scores;
        scores.reserve( requests.size() );
        bool usable = false;
        for( const craft_reservation::binding &req : requests ) {
            scores.push_back( score_reservation_candidate( cand, req, who ) );
            usable = usable || scores.back().has_value();
        }
        if( !usable ) {
            continue;
        }
        const std::string signature =
            provider_is_exclusive( cand ) ? exclusion_signature( cand, pool ) : std::string();
        const std::string key = candidate_class_key( cand, scores, signature );
        candidate_class &cls = by_key[key];
        cls.key = key;
        cls.scores = scores;
        cls.abstract = !provider_is_exclusive( cand );
        cls.members.push_back( &cand );
        if( provider_is_exclusive( cand ) && held_keys.count( provider_identity( cand ) ) > 0 ) {
            held_members.insert( &cand );
        }
        const std::string unit = exclusion_unit_key( cand );
        const auto inserted = unit_ids.emplace( unit, unit_ids.size() );
        unit_of[&cand] = inserted.first->second;
    }

    std::vector<candidate_class> classes;
    classes.reserve( by_key.size() );
    for( std::pair<const std::string, candidate_class> &entry : by_key ) {
        candidate_class &cls = entry.second;



        std::stable_sort( cls.members.begin(), cls.members.end(),
        [&unit_of, &held_members]( const provider_candidate * a, const provider_candidate * b ) {
            const bool a_held = held_members.count( a ) > 0;
            const bool b_held = held_members.count( b ) > 0;
            if( a_held != b_held ) {
                return a_held;
            }
            if( unit_of[a] != unit_of[b] ) {
                return unit_of[a] < unit_of[b];
            }
            return provider_identity( *a ) < provider_identity( *b );
        } );
        cls.held = 0;
        for( const provider_candidate *member : cls.members ) {
            if( held_members.count( member ) > 0 ) {
                ++cls.held;
            }
        }
        classes.push_back( std::move( cls ) );
    }
    return classes;
}



static uint64_t fold_fingerprint( uint64_t hash, std::string_view bytes )
{
    for( const char c : bytes ) {
        hash ^= static_cast<uint64_t>( static_cast<unsigned char>( c ) );
        hash *= 1099511628211ULL;
    }
    return hash;
}




uint64_t pool_fingerprint( const std::vector<candidate_class> &classes,
                           const std::vector<provider_candidate> &available,
                           Character *who )
{
    uint64_t hash = 14695981039346656037ULL;
    hash = fold_fingerprint( hash, std::to_string( craft_reservation::pool_fingerprint_version ) );
    std::vector<std::string> rows;
    rows.reserve( classes.size() );
    for( const candidate_class &cls : classes ) {


        std::map<std::string, int> units;
        for( const provider_candidate *member : cls.members ) {
            ++units[exclusion_unit_key( *member )];
        }
        std::vector<int> spread;
        spread.reserve( units.size() );
        for( const std::pair<const std::string, int> &unit : units ) {
            spread.push_back( unit.second );
        }
        std::sort( spread.begin(), spread.end() );
        std::string row = cls.key + '#' + std::to_string( cls.members.size() ) + '#';
        for( const int n : spread ) {
            row += std::to_string( n ) + ',';
        }
        rows.push_back( row );
    }


    std::map<std::pair<bool, itype_id>, int> pool;
    std::set<itype_id> carried_types;


    std::map<tripoint_abs_ms, int> linked_pools;
    map &here = get_map();
    for( const provider_candidate &cand : available ) {
        if( cand.it != nullptr ) {
            pool[ { cand.carried, cand.it->typeId() }] += cand.it->ammo_remaining();
            if( linked_power_source( here, *cand.it ) != nullptr ) {
                linked_pools[cand.it->link().t_abs_pos] =
                    cand.it->ammo_remaining_linked( here ) - cand.it->ammo_remaining();
            }
            if( cand.carried ) {
                carried_types.insert( cand.it->typeId() );
            }
        } else {
            pool[ { cand.carried, cand.pseudo_type }] += 1;
        }
    }



    if( who != nullptr ) {
        for( const itype_id &type : carried_types ) {
            pool[ { true, type }] = who->charges_of( type, INT_MAX, unreserved_filter );
        }
    }
    for( const std::pair<const std::pair<bool, itype_id>, int> &entry : pool ) {
        rows.push_back( std::string( "pool/" ) + ( entry.first.first ? 'p' : 'm' ) + '/' +
                        entry.first.second.str() + '/' + std::to_string( entry.second ) );
    }
    for( const std::pair<const tripoint_abs_ms, int> &entry : linked_pools ) {
        rows.push_back( "link/" + entry.first.to_string() + '/' + std::to_string( entry.second ) );
    }
    std::sort( rows.begin(), rows.end() );
    for( const std::string &row : rows ) {
        hash = fold_fingerprint( hash, row );
    }
    return hash == 0 ? 1 : hash;
}


std::vector<size_t> classes_for_request( const std::vector<candidate_class> &classes,
        size_t request_index )
{
    std::vector<size_t> order;
    for( size_t c = 0; c < classes.size(); ++c ) {
        if( classes[c].scores[request_index] ) {
            order.push_back( c );
        }
    }
    std::stable_sort( order.begin(), order.end(), [&]( size_t a, size_t b ) {
        return *classes[a].scores[request_index] < *classes[b].scores[request_index];
    } );
    return order;
}





static std::string memo_key( size_t depth, const search_state &state )
{
    std::string key = std::to_string( depth ) + ':';
    for( int used : state.usage ) {
        key += std::to_string( used ) + ',';
    }



    std::vector<std::string> slots;
    for( const craft_reservation::binding &b : state.selected ) {
        if( b.occurrence_slot < 0 ) {
            continue;
        }
        slots.push_back( std::to_string( static_cast<int>( b.kind ) ) + '/' +
                         std::to_string( b.intrinsic_owner.get_value() ) + '/' +
                         b.qual.str() + '/' + std::to_string( b.level ) + '/' +
                         b.pseudo_type.str() + '/' +
                         std::to_string( b.group_index ) + '/' +
                         std::to_string( b.occurrence_slot ) );
    }
    std::sort( slots.begin(), slots.end() );
    key += '|';
    for( const std::string &slot : slots ) {
        key += slot + ';';
    }
    return key;
}



static std::vector<std::pair<int, int>> abstract_slots_in_multiset(
        const search_state &state, const provider_candidate &cand,
        const craft_reservation::binding &req )
{
    std::vector<std::pair<int, int>> slots;
    for( const craft_reservation::binding &b : state.selected ) {
        if( b.occurrence_slot < 0 || b.kind != cand.kind ) {
            continue;
        }
        const bool same_source =
            cand.kind == craft_reservation::provider_kind::intrinsic
            ? b.intrinsic_owner == cand.intrinsic_owner &&
            ( req.req == craft_reservation::requirement_kind::presence_tool
              ? b.pseudo_type == cand.pseudo_type
              : b.qual == req.qual && b.level == req.level )
            : b.pseudo_type == cand.pseudo_type;
        if( same_source ) {
            slots.emplace_back( b.group_index, b.occurrence_slot );
        }
    }
    return slots;
}



static std::set<provider_key> group_taken( const search_state &state, int group )
{
    std::set<provider_key> taken;
    for( const craft_reservation::binding &b : state.selected ) {
        if( b.group_index != group ) {
            continue;
        }
        if( b.kind == craft_reservation::provider_kind::item ||
            b.kind == craft_reservation::provider_kind::vehicle_part ) {
            taken.insert( provider_key{ 1, b.provider_uid, tripoint_abs_ms() } );
        } else if( b.tile ) {
            taken.insert( provider_key{ 2, 0, *b.tile } );
        }
    }
    return taken;
}


static selection_result take_one( selection_context &ctx, size_t depth, search_state &state )
{
    const int group = ctx.groups[depth].first;
    const std::set<provider_key> taken = group_taken( state, group );


    int chosen_alternative = -1;
    for( const craft_reservation::binding &b : state.selected ) {
        if( b.group_index == group ) {
            chosen_alternative = b.alternative_index;
            break;
        }
    }
    bool budget_ran_out = false;
    for( size_t r = 0; r < ctx.requests->size(); ++r ) {
        const craft_reservation::binding &req = ( *ctx.requests )[r];
        if( req.group_index != group ||
            ( chosen_alternative >= 0 && req.alternative_index != chosen_alternative ) ) {
            continue;
        }


        std::set<size_t> tried;
        for( size_t c : classes_for_request( *ctx.classes, r ) ) {
            if( !tried.insert( c ).second ) {
                continue;
            }
            const candidate_class &cls = ( *ctx.classes )[c];
            const int used = state.usage[c];


            std::vector<int> choices;
            choices.reserve( used + 1 );
            for( int m = 0; m < used; ++m ) {
                choices.push_back( m );
            }
            if( cls.abstract ) {
                choices.push_back( 0 );
            } else if( used < static_cast<int>( cls.members.size() ) ) {
                choices.push_back( used );
            }



            std::vector<int> shared_slots;
            int multiset_held = 0;
            if( cls.abstract && !cls.members.empty() ) {
                const std::vector<std::pair<int, int>> slots =
                                                        abstract_slots_in_multiset( state, *cls.members[0], req );
                std::set<int> own_slots;
                std::set<int> distinct;
                for( const std::pair<int, int> &held : slots ) {
                    distinct.insert( held.second );
                    if( held.first == group ) {
                        own_slots.insert( held.second );
                    }
                }

                multiset_held = static_cast<int>( distinct.size() );
                for( const std::pair<int, int> &held : slots ) {


                    if( held.first != group && own_slots.count( held.second ) == 0 ) {
                        shared_slots.push_back( held.second );
                    }
                }
            }

            for( const int member : choices ) {
                const bool fresh = !cls.abstract && member == used;
                const provider_candidate &cand = *cls.members[cls.abstract ? 0 : member];
                if( provider_is_exclusive( cand ) &&
                    taken.count( provider_identity( cand ) ) > 0 ) {
                    continue;
                }
                const bool shares_slot = cls.abstract && !shared_slots.empty();
                if( cls.abstract && !shares_slot &&
                    !abstract_capacity_allows( cand, req, *ctx.src, *ctx.bindable,
                                               multiset_held + 1 ) ) {
                    continue;
                }

                craft_reservation::binding bound = req;
                bound.kind = cand.kind;
                bound.provider_uid = cand.provider_uid;
                bound.tile = cand.tile;
                bound.furn = cand.furn;
                bound.ter = cand.ter;
                bound.intrinsic_owner = cand.intrinsic_owner;
                if( cand.kind == craft_reservation::provider_kind::intrinsic ) {

                    bound.pseudo_type = req.req == craft_reservation::requirement_kind::quality
                                        ? itype_id::NULL_ID()
                                        : cand.pseudo_type;
                    bound.occurrence_slot = shares_slot ? shared_slots.front()
                                            : state.next_abstract_slot++;
                } else if( cand.kind == craft_reservation::provider_kind::environment ) {
                    bound.pseudo_type = cand.pseudo_type;
                    bound.occurrence_slot = shares_slot ? shared_slots.front()
                                            : state.next_abstract_slot++;
                } else if( cand.kind != craft_reservation::provider_kind::item ) {
                    bound.pseudo_type = cand.pseudo_type;
                }

                const bool claims_capacity = fresh || ( cls.abstract && !shares_slot );
                state.selected.push_back( bound );
                if( claims_capacity ) {
                    ++state.usage[c];
                }
                if( ctx.expansions_left == 0 ) {
                    state.selected.pop_back();
                    if( claims_capacity ) {
                        --state.usage[c];
                    }
                    if( bound.occurrence_slot >= 0 && !shares_slot ) {
                        --state.next_abstract_slot;
                    }
                    return selection_result::undetermined;
                }
                --ctx.expansions_left;
                const bool pool_ok = ctx.pool_feasible( state.selected );
                if( pool_ok ) {
                    int held = 0;
                    for( const craft_reservation::binding &b : state.selected ) {
                        if( b.group_index == group ) {
                            ++held;
                        }
                    }
                    const int left = std::max( 0, std::max( 1, req.group_count ) - held );
                    const selection_result rest = assign_from( ctx, depth, left, state );
                    if( rest == selection_result::feasible ) {
                        return rest;
                    }
                    budget_ran_out = budget_ran_out || rest == selection_result::undetermined;
                }
                state.selected.pop_back();
                if( claims_capacity ) {
                    --state.usage[c];
                }
                if( bound.occurrence_slot >= 0 && !shares_slot ) {
                    --state.next_abstract_slot;
                }
            }
        }
    }
    return budget_ran_out ? selection_result::undetermined : selection_result::infeasible;
}

selection_result assign_from( selection_context &ctx, size_t depth, int remaining,
                              search_state &state )
{
    if( remaining > 0 ) {


        const bool memoizable = remaining == ctx.groups[depth].second;
        const std::string key = memoizable ? memo_key( depth, state ) : std::string();
        if( memoizable && ctx.failed.count( key ) > 0 ) {
            return selection_result::infeasible;
        }
        const selection_result res = take_one( ctx, depth, state );
        if( memoizable && res == selection_result::infeasible ) {
            ctx.failed.insert( key );
        }
        return res;
    }
    if( depth + 1 >= ctx.groups.size() ) {
        return selection_result::feasible;
    }
    return assign_from( ctx, depth + 1, ctx.groups[depth + 1].second, state );
}

bool hidden_units::covers( const provider_candidate &cand ) const
{
    switch( cand.kind ) {
        case craft_reservation::provider_kind::item:
            return item_roots.count( cand.root_uid ) > 0;
        case craft_reservation::provider_kind::vehicle_part:
            return parts.count( cand.provider_uid ) > 0;
        case craft_reservation::provider_kind::furniture:
        case craft_reservation::provider_kind::terrain:
            return cand.tile && tiles.count( *cand.tile ) > 0;
        default:
            return false;
    }
}


bool unreserved_filter( const item &it )
{
    return craft_reservation::usable_by_automation( it );
}
