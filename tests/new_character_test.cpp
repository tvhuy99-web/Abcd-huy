#include <cstddef>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "item.h"
#include "item_location.h"
#include "mutation.h"
#include "player_helpers.h"
#include "profession.h"
#include "scenario.h"
#include "string_formatter.h"
#include "type_id.h"
#include "visitable.h"

static const trait_id trait_ALBINO( "ALBINO" );
static const trait_id trait_ANTIFRUIT( "ANTIFRUIT" );
static const trait_id trait_ANTIJUNK( "ANTIJUNK" );
static const trait_id trait_ANTIWHEAT( "ANTIWHEAT" );
static const trait_id trait_ASTHMA( "ASTHMA" );
static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_LACTOSE( "LACTOSE" );
static const trait_id trait_MEATARIAN( "MEATARIAN" );
static const trait_id trait_TAIL_FLUFFY( "TAIL_FLUFFY" );
static const trait_id trait_VEGETARIAN( "VEGETARIAN" );
static const trait_id trait_WOOLALLERGY( "WOOLALLERGY" );

static std::ostream &operator<<( std::ostream &s, const std::vector<trait_id> &v )
{
    for( const auto &e : v ) {
        s << e.c_str() << " ";
    }
    return s;
}

static std::vector<trait_id> next_subset( const std::vector<trait_id> &set )
{


    static unsigned bitset = 0;
    std::vector<trait_id> ret;

    ++bitset;

    for( size_t idx = 0; idx < set.size(); idx++ ) {
        if( bitset & ( 1 << idx ) ) {
            ret.push_back( set[idx] );
        }
    }
    return ret;
}

static bool try_set_traits( const std::vector<trait_id> &traits )
{
    avatar &player_character = get_avatar();
    player_character.clear_mutations();
    player_character.add_traits();
    std::vector<trait_id> oked_traits;
    for( const trait_id &tr : traits ) {
        if( player_character.has_conflicting_trait( tr ) ||
            !get_scenario()->traitquery( tr ) ) {
            return false;
        } else if( !player_character.has_trait( tr ) ) {
            oked_traits.push_back( tr );
        }
    }
    player_character.set_mutations( oked_traits );
    return true;
}

static int get_item_count( const std::set<const item *> &items )
{
    int sum = 0;
    for( const item *it : items ) {
        sum += it->count();
    }
    return sum;
}

namespace
{

struct failure {
    string_id<profession> prof;
    std::vector<trait_id> mut;
    itype_id item_name;
    std::string reason;
};

}

namespace std
{
template<>
struct less<failure> {
    bool operator()( const failure &lhs, const failure &rhs ) const {
        return lhs.prof < rhs.prof;
    }
};
}








TEST_CASE( "starting_items", "[slow]" )
{

    const std::vector<trait_id> mutations = {
        trait_ALBINO,
        trait_ANTIFRUIT,
        trait_ANTIJUNK,
        trait_ANTIWHEAT,





        trait_LACTOSE,

        trait_MEATARIAN,
        trait_ASTHMA,



        trait_VEGETARIAN,
        trait_WOOLALLERGY
    };

    std::vector<std::pair<const scenario *, std::vector<string_id<profession>>>> scen_prof_combos;
    scen_prof_combos.emplace_back( scenario::generic(), std::vector<string_id<profession>> {} );
    for( const auto &id : scenario::generic()->permitted_professions() ) {
        scen_prof_combos.back().second.push_back( id );
    }

    std::set<failure> failures;

    avatar &player_character = get_avatar();

    std::vector<trait_id> traits = next_subset( mutations );
    for( ; !traits.empty(); traits = next_subset( mutations ) ) {
        CAPTURE( traits );
        for( const auto &pair : scen_prof_combos ) {
            set_scenario( pair.first );
            INFO( "Scenario = " + pair.first->ident().str() );
            for( const string_id<profession> &prof : pair.second ) {
                CAPTURE( prof );
                player_character.prof = &prof.obj();
                if( !try_set_traits( traits ) ) {
                    continue;
                }
                for( int i = 0; i < 2; i++ ) {
                    player_character.clear_worn();
                    player_character.remove_weapon();
                    player_character.calc_encumbrance();
                    player_character.male = i == 0;

                    player_character.add_profession_items();
                    std::set<const item *> items_visited;
                    const auto visitable_counter = [&items_visited]( const item_location & it ) {
                        items_visited.emplace( it.get_item() );
                        return VisitResponse::NEXT;
                    };
                    player_character.visit_items( visitable_counter );
                    const int num_items_pre_migration = get_item_count( items_visited );
                    items_visited.clear();

                    player_character.visit_items( visitable_counter );
                    const int num_items_post_migration = get_item_count( items_visited );
                    items_visited.clear();

                    if( num_items_pre_migration != num_items_post_migration ) {
                        failure cur_fail;
                        cur_fail.prof = player_character.prof->ident();
                        cur_fail.mut = player_character.get_mutations();
                        cur_fail.reason = string_format( "does not have enough space to store all items." );

                        failures.insert( cur_fail );
                    }
                    CAPTURE( player_character.prof->ident().c_str() );
                    CHECK( num_items_pre_migration == num_items_post_migration );
                }
            }
        }
    }
    std::stringstream failure_messages;
    for( const failure &f : failures ) {
        failure_messages << f.prof.c_str() << " " << f.mut <<
                         " " << f.item_name.str() << ": " << f.reason << "\n";
    }
    INFO( failure_messages.str() );
    REQUIRE( failures.empty() );
}

TEST_CASE( "Generated_character_with_category_mutations", "[mutation]" )
{
    REQUIRE( !trait_TAIL_FLUFFY.obj().category.empty() );
    avatar &u = get_avatar();
    clear_avatar();
    REQUIRE( u.get_mutations().empty() );
    REQUIRE( u.get_base_traits().empty() );
    REQUIRE( u.mutation_category_level.empty() );

    SECTION( "Mutations have category levels" ) {
        u.toggle_trait_deps( trait_TAIL_FLUFFY );
        CHECK( u.has_trait( trait_TAIL_FLUFFY ) );
        CHECK( !u.get_mutations().empty() );
        CHECK( u.get_base_traits().empty() );
        CHECK( !u.mutation_category_level.empty() );
        u.toggle_trait_deps( trait_TAIL_FLUFFY );
        CHECK( !u.has_trait( trait_TAIL_FLUFFY ) );
        CHECK( u.get_mutations().empty() );
        CHECK( u.get_base_traits().empty() );
        CHECK( u.mutation_category_level.empty() );
    }

    SECTION( "Category mutations can be removed" ) {
        u.toggle_trait_deps( trait_TAIL_FLUFFY );
        CHECK( u.has_trait( trait_TAIL_FLUFFY ) );
        u.remove_mutation( trait_TAIL_FLUFFY );
        CHECK( !u.has_trait( trait_TAIL_FLUFFY ) );
    }
}

TEST_CASE( "cannibal_not_randomly_selected", " [character] [traits] [random]" )
{
    for( int i = 0; i < 1000; ++i ) {
        trait_id random_trait = get_avatar().random_bad_trait();
        REQUIRE( random_trait != trait_CANNIBAL );
    }
}
