#pragma once
#ifndef CATA_SRC_A11Y_REQUIREMENTS_H
#define CATA_SRC_A11Y_REQUIREMENTS_H

#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include "a11y_document.h"
#include "temp_crafting_inventory.h"
#include "item.h"
#include "itype.h"
#include "requirements.h"
#include "string_formatter.h"
#include "translations.h"

namespace a11y
{
// State is expressed in words, since removing the UI's colors would otherwise
// make an unavailable alternative sound identical to an available one.
inline void append_requirements( semantic_snapshot &snapshot, const std::string &prefix,
                                 const requirement_data &requirements, const Character &actor,
                                 const temp_crafting_inventory &inventory,
                                 const std::function<bool( const item & )> &component_filter,
                                 int batch,
                                 const deduped_requirement_data *deduped = nullptr )
{
    const auto tools_filter = []( const item & ) { return true; };
    const auto append_groups = [&]( const auto &groups, const std::string &kind,
                                    const std::string &title, const auto &filter ) {
        std::size_t group_index = 0;
        for( const auto &group : groups ) {
            std::string text = _( "Each requirement group must be met. Within this group, choose one alternative:" );
            for( const auto &alternative : group ) {
                const bool present = alternative.has( &actor, inventory, filter, batch );
                text += "\n" + remove_color_tags( alternative.to_string( batch ) ) + ". " +
                        ( present ? _( "Available." ) : _( "Missing." ) );
            }
            append_document( snapshot, prefix + ":" + kind + ":" + std::to_string( group_index ),
                             string_format( _( "%1$s. Group %2$d" ), title, group_index + 1 ), text );
            ++group_index;
        }
    };
    append_groups( requirements.get_qualities(), "quality", _( "Qualities" ), tools_filter );
    append_groups( requirements.get_tools(), "tool", _( "Tools" ), tools_filter );
    append_groups( requirements.get_components(), "component", _( "Components" ), component_filter );

    // Shared total (R12.1). The per-group lines above judge every group
    // against the whole inventory on its own, so one physical item can make
    // two groups both read "Available" while only existing once. The verdict
    // here is the engine's own whole-set check over the same inventory --
    // the deduped form when the caller supplies it, which is the form actual
    // craftability uses -- so a shared item is only counted once overall.
    std::string totals = deduped != nullptr
                         ? ( deduped->can_make_with_inventory( &actor, inventory, component_filter, batch )
                             ? _( "All groups together can be met from the same inventory." )
                             : _( "All groups together cannot be met from the same inventory, even if individual groups look available." ) )
                         : ( requirements.can_make_with_inventory( &actor, inventory, component_filter, batch )
                             ? _( "All groups together can be met from the same inventory." )
                             : _( "All groups together cannot be met from the same inventory, even if individual groups look available." ) );
    // Per-type totals are only stated where they are unambiguous: a group
    // with a single alternative always consumes that type, so summing those
    // groups counts each shared item exactly once. Types inside choice
    // groups depend on the player's selection and are covered by the
    // verdict above, not by an invented sum.
    std::map<itype_id, int> forced;
    for( const auto &group : requirements.get_components() ) {
        if( group.size() == 1 ) {
            forced[group.front().type] += std::abs( group.front().count ) * batch;
        }
    }
    for( const auto &entry : forced ) {
        const int have = item::count_by_charges( entry.first )
                         ? inventory.charges_of( entry.first, INT_MAX, component_filter )
                         : inventory.amount_of( entry.first, false, INT_MAX, component_filter );
        totals += "\n" + string_format( _( "%1$s: need %2$d in total across groups; available %3$d." ),
                                        entry.first.obj().nname( entry.second ), entry.second, have );
    }
    append_document( snapshot, prefix + ":total", _( "Shared total" ), totals );
}
} // namespace a11y
#endif // CATA_SRC_A11Y_REQUIREMENTS_H
