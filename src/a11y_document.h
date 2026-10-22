#pragma once
#ifndef CATA_SRC_A11Y_DOCUMENT_H
#define CATA_SRC_A11Y_DOCUMENT_H

#include <string>
#include <utility>
#include "a11y_semantics.h"
#include "output.h"

namespace a11y
{
// Use the text model supplied to the game's UI, never pixels or terminal cells.
// The Android presenter reads documents explicitly, with stoppable speech chunks.
inline void append_document( semantic_snapshot &snapshot, std::string id,
                             std::string title, const std::string &text )
{
    if( text.empty() ) {
        return;
    }
    semantic_node node;
    node.id = std::move( id );
    node.role = "document";
    node.label = remove_color_tags( title );
    node.detail = remove_color_tags( text );
    snapshot.nodes.push_back( std::move( node ) );
}
} // namespace a11y
#endif // CATA_SRC_A11Y_DOCUMENT_H
