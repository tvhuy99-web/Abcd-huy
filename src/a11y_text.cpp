#include "a11y_text.h"

#include <utility>
#include <vector>

#include "a11y_semantics.h"
#include "translations.h"

namespace a11y
{
semantic_snapshot text_snapshot( std::string title, std::string description,
                                 std::string value, bool numeric,
                                 double field_step, bool integer )
{
    semantic_snapshot snapshot;
    snapshot.title = std::move( title );
    snapshot.summary = std::move( description );
    snapshot.focus = "text";
    semantic_node field;
    field.id = "text";
    field.role = numeric ? "number" : "text";
    field.label = numeric ? _( "Number" ) : _( "Text" );
    field.value = std::move( value );
    field.actions = { "focus", "set_text" };
    if( numeric ) {





        field.field_type = integer ? "int" : "float";
        field.applied_value = field.value;
        field.field_step = field_step;
    }
    snapshot.nodes.push_back( std::move( field ) );
    semantic_node confirm;
    confirm.id = "confirm";
    confirm.label = _( "Confirm" );
    snapshot.nodes.push_back( std::move( confirm ) );
    semantic_node cancel;
    cancel.id = "cancel";
    cancel.label = _( "Cancel" );
    snapshot.nodes.push_back( std::move( cancel ) );
    return snapshot;
}
}
