#pragma once
#ifndef CATA_SRC_A11Y_REQUIREMENTS_H
#define CATA_SRC_A11Y_REQUIREMENTS_H

#include <functional>
#include <string>
#include "a11y_document.h"
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
                                 int batch )
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
}
} // namespace a11y
#endif // CATA_SRC_A11Y_REQUIREMENTS_H
