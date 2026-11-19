#pragma once
#ifndef CATA_SRC_A11Y_DOCUMENT_H
#define CATA_SRC_A11Y_DOCUMENT_H

#include <string>
#include <string_view>
#include <utility>
#include "a11y_semantics.h"
#include "output.h"

namespace a11y
{


inline void append_document( semantic_snapshot &snapshot, std::string id,
                             const std::string_view title, const std::string_view text )
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
}
#endif
