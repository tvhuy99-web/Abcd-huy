#include "a11y_semantics.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
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

// Compact JSON number for form domains: integers stay integers, other
// values use %g so steps like 0.25 serialize exactly as written.
std::string number( const double value )
{
    if( std::isfinite( value ) && value == std::floor( value ) &&
        value >= -9.0e15 && value <= 9.0e15 ) {
        return std::to_string( static_cast<long long>( value ) );
    }
    char buffer[40];
    std::snprintf( buffer, sizeof( buffer ), "%.6g", value );
    return buffer;
}

bool parse_number( const std::string &text, double &value )
{
    if( text.empty() || text.size() > 64 ) {
        return false;
    }
    const char *begin = text.c_str();
    char *end = nullptr;
    value = std::strtod( begin, &end );
    return end != begin && *end == '\0' && std::isfinite( value );
}

// Shared domain check for one raw form value. Reason codes are stable
// machine codes; presentation layers localize them.
bool check_field_value( const semantic_node &node, const std::string &raw, std::string &reason )
{
    if( raw.size() > max_intent_value_chars ) {
        reason = "value_too_long";
        return false;
    }
    if( node.field_type == "int" || node.field_type == "float" ) {
        double value = 0.0;
        if( !parse_number( raw, value ) ) {
            reason = "not_a_number";
            return false;
        }
        if( node.field_type == "int" && value != std::floor( value ) ) {
            reason = "not_a_number";
            return false;
        }
        if( node.has_field_range ) {
            if( value < node.field_min || value > node.field_max ) {
                reason = "value_out_of_range";
                return false;
            }
            if( node.field_step > 0.0 ) {
                const double steps = ( value - node.field_min ) / node.field_step;
                if( std::fabs( steps - std::round( steps ) ) > 1e-9 ) {
                    reason = "value_step_mismatch";
                    return false;
                }
            }
        }
        return true;
    }
    if( node.field_type == "bool" ) {
        if( raw != "true" && raw != "false" ) {
            reason = "not_a_bool";
            return false;
        }
        return true;
    }
    if( node.field_type == "choice" ) {
        if( std::none_of( node.choices.begin(), node.choices.end(),
        [&raw]( const semantic_choice & choice ) { return choice.value == raw; } ) ) {
            reason = "not_a_choice";
            return false;
        }
        return true;
    }
    return true;
}
} // namespace

bool validate_intent_value( const semantic_node &node, const semantic_intent &intent,
                            std::string &reason )
{
    if( intent.action != "set_value" && intent.action != "select_choice" ) {
        return true;
    }
    if( node.field_type.empty() ) {
        reason = "not_a_field";
        return false;
    }
    if( intent.action == "select_choice" && node.field_type != "choice" && node.choices.empty() ) {
        reason = "not_a_choice";
        return false;
    }
    return check_field_value( node, intent.value, reason );
}

bool validate_snapshot( const semantic_snapshot &snapshot, std::string &reason )
{
    if( snapshot.nodes.size() > max_snapshot_nodes ) {
        reason = "too_many_nodes";
        return false;
    }
    if( snapshot.tables.size() > max_snapshot_tables ) {
        reason = "too_many_tables";
        return false;
    }
    std::map<std::string, const semantic_table *> tables;
    for( const semantic_table &table : snapshot.tables ) {
        if( table.id.empty() || tables.count( table.id ) != 0 ) {
            reason = "duplicate_table_id";
            return false;
        }
        if( table.columns.empty() || table.columns.size() > max_table_columns ) {
            reason = "invalid_table_columns";
            return false;
        }
        std::set<std::string> columns;
        for( const semantic_column &column : table.columns ) {
            if( column.id.empty() || !columns.insert( column.id ).second ) {
                reason = "duplicate_column_id";
                return false;
            }
        }
        tables.emplace( table.id, &table );
    }
    std::map<std::string, const semantic_node *> by_id;
    for( const semantic_node &node : snapshot.nodes ) {
        if( node.id.empty() ) {
            reason = "empty_node_id";
            return false;
        }
        if( !by_id.emplace( node.id, &node ).second ) {
            reason = "duplicate_node_id";
            return false;
        }
    }
    // Tree structure: parents exist, published children only hang under an
    // expanded parent, depths are consistent and acyclic.
    std::set<std::string> parents_with_children;
    for( const semantic_node &node : snapshot.nodes ) {
        if( node.depth < -1 || node.depth > max_tree_depth ) {
            reason = "invalid_tree_depth";
            return false;
        }
        if( node.parent_id.empty() ) {
            if( node.depth > 0 ) {
                reason = "invalid_tree_depth";
                return false;
            }
            continue;
        }
        const auto parent = by_id.find( node.parent_id );
        if( parent == by_id.end() ) {
            reason = "missing_tree_parent";
            return false;
        }
        if( !parent->second->expanded ) {
            reason = "child_of_collapsed_parent";
            return false;
        }
        if( parent->second->depth < 0 || node.depth != parent->second->depth + 1 ) {
            reason = "invalid_tree_depth";
            return false;
        }
        parents_with_children.insert( node.parent_id );
    }
    for( const semantic_node &node : snapshot.nodes ) {
        std::set<std::string> chain;
        const semantic_node *cursor = &node;
        while( !cursor->parent_id.empty() ) {
            if( !chain.insert( cursor->id ).second ) {
                reason = "tree_cycle";
                return false;
            }
            cursor = by_id.at( cursor->parent_id );
        }
        if( node.has_children && node.expanded && parents_with_children.count( node.id ) == 0 ) {
            reason = "expanded_parent_without_children";
            return false;
        }
        if( !node.has_children && parents_with_children.count( node.id ) != 0 ) {
            reason = "undeclared_tree_children";
            return false;
        }
        // Flat position/total of non-tree nodes stay a builder convention
        // (several existing builders publish positions before totals are
        // known); only the tree sibling sets are structurally validated,
        // through the parent/depth checks above.
    }
    // Table rows reference declared tables and columns; row indices are
    // unique per table.
    std::set<std::pair<std::string, int>> seen_rows;
    for( const semantic_node &node : snapshot.nodes ) {
        if( node.table_id.empty() ) {
            continue;
        }
        const auto table = tables.find( node.table_id );
        if( table == tables.end() ) {
            reason = "undeclared_table";
            return false;
        }
        if( node.row_index < 0 || !seen_rows.emplace( node.table_id, node.row_index ).second ) {
            reason = "duplicate_table_row";
            return false;
        }
        if( node.cells.size() > max_node_cells ) {
            reason = "too_many_cells";
            return false;
        }
        for( const semantic_cell &cell : node.cells ) {
            if( std::none_of( table->second->columns.begin(), table->second->columns.end(),
            [&cell]( const semantic_column & column ) { return column.id == cell.column; } ) ) {
                reason = "undeclared_column";
                return false;
            }
        }
    }
    // Form contracts: known types, sane ranges, unique choices, drafts
    // inside their domain and dependencies on published fields.
    for( const semantic_node &node : snapshot.nodes ) {
        if( node.field_type.empty() ) {
            continue;
        }
        if( node.field_type != "bool" && node.field_type != "int" && node.field_type != "float" &&
            node.field_type != "choice" && node.field_type != "text" ) {
            reason = "unknown_field_type";
            return false;
        }
        if( node.has_field_range &&
            ( node.field_min > node.field_max || node.field_step < 0.0 ) ) {
            reason = "invalid_field_range";
            return false;
        }
        if( node.choices.size() > max_form_choices ) {
            reason = "too_many_choices";
            return false;
        }
        std::set<std::string> choice_values;
        for( const semantic_choice &choice : node.choices ) {
            if( !choice_values.insert( choice.value ).second ) {
                reason = "duplicate_choice_value";
                return false;
            }
        }
        if( node.has_draft && !check_field_value( node, node.draft_value, reason ) ) {
            return false;
        }
        for( const std::string &dependency : node.depends_on ) {
            if( dependency == node.id || by_id.count( dependency ) == 0 ) {
                reason = "invalid_field_dependency";
                return false;
            }
        }
    }
    return true;
}

std::string snapshot_content_json( const semantic_snapshot &snapshot )
{
    std::string result = "\"category\":" + quote( snapshot.category ) +
                         ",\"title\":" + quote( snapshot.title ) +
                         ",\"summary\":" + quote( snapshot.summary ) +
                         ",\"focus\":" + quote( snapshot.focus ) +
                         ",\"swipeMovementAllowed\":" + boolean( snapshot.swipe_movement_allowed ) +
                         ",\"tables\":[";
    bool first_table = true;
    for( const semantic_table &table : snapshot.tables ) {
        if( !first_table ) {
            result += ',';
        }
        first_table = false;
        result += "{\"id\":" + quote( table.id ) + ",\"columns\":[";
        bool first_column = true;
        for( const semantic_column &column : table.columns ) {
            if( !first_column ) {
                result += ',';
            }
            first_column = false;
            result += "{\"id\":" + quote( column.id ) + ",\"header\":" + quote( column.header ) +
                      ",\"unit\":" + quote( column.unit ) + '}';
        }
        result += "]}";
    }
    result += "],\"nodes\":[";
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
                  ",\"total\":" + std::to_string( node.total ) +
                  // Tree fields are always serialized; depth -1 and an empty
                  // parent mark a flat node, so old consumers that ignore
                  // them keep working (R02.4 compatibility).
                  ",\"parentId\":" + quote( node.parent_id ) +
                  ",\"depth\":" + std::to_string( node.depth ) +
                  ",\"hasChildren\":" + boolean( node.has_children ) +
                  ",\"expanded\":" + boolean( node.expanded );
        if( !node.table_id.empty() ) {
            result += ",\"tableId\":" + quote( node.table_id ) +
                      ",\"rowIndex\":" + std::to_string( node.row_index ) + ",\"cells\":[";
            bool first_cell = true;
            for( const semantic_cell &cell : node.cells ) {
                if( !first_cell ) {
                    result += ',';
                }
                first_cell = false;
                result += "{\"column\":" + quote( cell.column ) + ",\"value\":" + quote( cell.value ) +
                          ",\"unit\":" + quote( cell.unit ) + ",\"compare\":" + quote( cell.compare ) + '}';
            }
            result += ']';
        }
        if( !node.field_type.empty() ) {
            result += ",\"fieldType\":" + quote( node.field_type ) +
                      ",\"appliedValue\":" + quote( node.applied_value );
            if( node.has_field_range ) {
                result += ",\"fieldMin\":" + number( node.field_min ) +
                          ",\"fieldMax\":" + number( node.field_max ) +
                          ",\"fieldStep\":" + number( node.field_step );
            } else if( node.field_step > 0.0 ) {
                // A step without a range (number popup, R04.2) is a real
                // part of the contract; Java reads fieldStep independently
                // of fieldMin/fieldMax.
                result += ",\"fieldStep\":" + number( node.field_step );
            }
            result += ",\"choices\":[";
            bool first_choice = true;
            for( const semantic_choice &choice : node.choices ) {
                if( !first_choice ) {
                    result += ',';
                }
                first_choice = false;
                result += "{\"value\":" + quote( choice.value ) + ",\"label\":" + quote( choice.label ) + '}';
            }
            result += ']';
            if( node.has_draft ) {
                result += ",\"draftValue\":" + quote( node.draft_value );
            }
            if( !node.field_error.empty() ) {
                result += ",\"fieldError\":" + quote( node.field_error );
            }
            if( !node.depends_on.empty() ) {
                result += ",\"dependsOn\":[";
                bool first_dependency = true;
                for( const std::string &dependency : node.depends_on ) {
                    if( !first_dependency ) {
                        result += ',';
                    }
                    first_dependency = false;
                    result += quote( dependency );
                }
                result += ']';
            }
        }
        result += ",\"actions\":[";
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
