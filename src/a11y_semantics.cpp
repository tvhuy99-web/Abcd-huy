#include "a11y_semantics.h"

#include <string>

namespace a11y
{
namespace
{
std::string quote( const std::string &text )
{
    constexpr char hex[] = "0123456789abcdef";
    std::string result = "\"";
    for( const unsigned char ch : text ) {
        switch( ch ) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if( ch < 0x20 ) {
                    result += "\\u00";
                    result += hex[ch >> 4];
                    result += hex[ch & 15];
                } else {
                    result += static_cast<char>( ch );
                }
        }
    }
    return result + '"';
}

std::string boolean( const bool value )
{
    return value ? "true" : "false";
}
} // namespace

std::string snapshot_content_json( const semantic_snapshot &snapshot )
{
    std::string result = "\"category\":" + quote( snapshot.category ) +
                         ",\"title\":" + quote( snapshot.title ) +
                         ",\"summary\":" + quote( snapshot.summary ) +
                         ",\"focus\":" + quote( snapshot.focus ) + ",\"nodes\":[";
    bool first = true;
    for( const semantic_node &node : snapshot.nodes ) {
        if( !first ) {
            result += ',';
        }
        first = false;
        result += "{\"id\":" + quote( node.id ) + ",\"role\":" + quote( node.role ) +
                  ",\"label\":" + quote( node.label ) + ",\"value\":" + quote( node.value ) +
                  ",\"detail\":" + quote( node.detail ) +
                  ",\"disabledReason\":" + quote( node.disabled_reason ) +
                  ",\"enabled\":" + boolean( node.enabled ) +
                  ",\"selected\":" + boolean( node.selected ) +
                  ",\"checked\":" + boolean( node.checked ) +
                  ",\"position\":" + std::to_string( node.position ) +
                  ",\"total\":" + std::to_string( node.total ) + ",\"actions\":[";
        bool first_action = true;
        for( const std::string &action : node.actions ) {
            if( !first_action ) {
                result += ',';
            }
            first_action = false;
            result += quote( action );
        }
        result += "]}";
    }
    return result + ']';
}

std::string snapshot_json( const semantic_snapshot &snapshot )
{
    return "{\"session\":" + quote( std::to_string( snapshot.token.session ) ) +
           ",\"frame\":" + quote( std::to_string( snapshot.token.frame ) ) +
           ",\"generation\":" + quote( std::to_string( snapshot.token.generation ) ) +
           ",\"revision\":" + quote( std::to_string( snapshot.revision ) ) + ',' +
           snapshot_content_json( snapshot ) + '}';
}
} // namespace a11y
