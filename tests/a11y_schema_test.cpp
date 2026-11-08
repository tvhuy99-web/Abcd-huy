#include <string>

#include "a11y_semantics.h"
#include "cata_catch.h"

// Written during the coding phase (R22.2). These exercise the pure R02
// snapshot/intent contracts in src/a11y_semantics.cpp; they need no game
// world. Execute them with the full suite only after the coding phase is
// complete. Oracle: the validator's verdict and its stable machine reason
// code; a rejected snapshot must be rejected whole (the node list is never
// silently truncated to fit a limit).

namespace
{

a11y::semantic_node plain_node( const std::string &id )
{
    a11y::semantic_node node;
    node.id = id;
    node.label = id;
    return node;
}

a11y::semantic_node int_field( const std::string &id, double min, double max, double step )
{
    a11y::semantic_node node = plain_node( id );
    node.role = "field";
    node.field_type = "int";
    node.has_field_range = true;
    node.field_min = min;
    node.field_max = max;
    node.field_step = step;
    return node;
}

a11y::semantic_intent value_intent( const std::string &node, const std::string &value,
                                    const std::string &action = "set_value" )
{
    a11y::semantic_intent intent;
    intent.node = node;
    intent.action = action;
    intent.value = value;
    return intent;
}

} // namespace

TEST_CASE( "a11y_schema_accepts_a_minimal_valid_snapshot", "[a11y][schema]" )
{
    a11y::semantic_snapshot snapshot;
    snapshot.title = "World";
    snapshot.nodes.push_back( plain_node( "world:here" ) );
    snapshot.nodes.push_back( plain_node( "world:north" ) );
    std::string reason;
    CHECK( a11y::validate_snapshot( snapshot, reason ) );
    CHECK( snapshot.nodes.size() == 2 );
}

TEST_CASE( "a11y_schema_rejects_duplicate_node_ids", "[a11y][schema]" )
{
    a11y::semantic_snapshot snapshot;
    snapshot.nodes.push_back( plain_node( "dup" ) );
    snapshot.nodes.push_back( plain_node( "dup" ) );
    std::string reason;
    CHECK_FALSE( a11y::validate_snapshot( snapshot, reason ) );
    CHECK( reason == "duplicate_node_id" );
    // Rejection is whole: nothing was dropped to make the snapshot fit.
    CHECK( snapshot.nodes.size() == 2 );
}

TEST_CASE( "a11y_schema_rejects_too_many_nodes_instead_of_truncating", "[a11y][schema]" )
{
    a11y::semantic_snapshot snapshot;
    for( std::size_t i = 0; i <= a11y::max_snapshot_nodes; i++ ) {
        snapshot.nodes.push_back( plain_node( "n" + std::to_string( i ) ) );
    }
    std::string reason;
    CHECK_FALSE( a11y::validate_snapshot( snapshot, reason ) );
    CHECK( reason == "too_many_nodes" );
    CHECK( snapshot.nodes.size() == a11y::max_snapshot_nodes + 1 );
}

TEST_CASE( "a11y_schema_tree_parentage_rules", "[a11y][schema]" )
{
    std::string reason;
    SECTION( "missing parent is rejected" ) {
        a11y::semantic_snapshot snapshot;
        a11y::semantic_node child = plain_node( "child" );
        child.parent_id = "ghost";
        child.depth = 1;
        snapshot.nodes.push_back( child );
        CHECK_FALSE( a11y::validate_snapshot( snapshot, reason ) );
        CHECK( reason == "missing_tree_parent" );
    }
    SECTION( "a parent cycle is rejected" ) {
        a11y::semantic_snapshot snapshot;
        a11y::semantic_node a = plain_node( "a" );
        a.parent_id = "b";
        a.depth = 1;
        a.has_children = true;
        a.expanded = true;
        a11y::semantic_node b = plain_node( "b" );
        b.parent_id = "a";
        b.depth = 1;
        b.has_children = true;
        b.expanded = true;
        snapshot.nodes.push_back( a );
        snapshot.nodes.push_back( b );
        // The exact code is not pinned: the validator checks depth
        // consistency before its cycle walk, and a cycle cannot have
        // consistent depths, so an earlier structural code may fire.
        // The oracle is rejection with some stable code, never acceptance.
        CHECK_FALSE( a11y::validate_snapshot( snapshot, reason ) );
        CHECK_FALSE( reason.empty() );
    }
    SECTION( "children of a collapsed node are never published" ) {
        a11y::semantic_snapshot snapshot;
        a11y::semantic_node parent = plain_node( "parent" );
        parent.has_children = true;
        parent.expanded = false;
        parent.depth = 0;
        a11y::semantic_node child = plain_node( "child" );
        child.parent_id = "parent";
        child.depth = 1;
        snapshot.nodes.push_back( parent );
        snapshot.nodes.push_back( child );
        CHECK_FALSE( a11y::validate_snapshot( snapshot, reason ) );
        CHECK( reason == "child_of_collapsed_parent" );
    }
    SECTION( "an expanded tree with declared children is accepted" ) {
        a11y::semantic_snapshot snapshot;
        a11y::semantic_node parent = plain_node( "parent" );
        parent.has_children = true;
        parent.expanded = true;
        parent.depth = 0;
        a11y::semantic_node child = plain_node( "child" );
        child.parent_id = "parent";
        child.depth = 1;
        snapshot.nodes.push_back( parent );
        snapshot.nodes.push_back( child );
        CHECK( a11y::validate_snapshot( snapshot, reason ) );
    }
}

TEST_CASE( "a11y_schema_table_rows_must_reference_declared_tables_and_columns", "[a11y][schema]" )
{
    std::string reason;
    SECTION( "undeclared table" ) {
        a11y::semantic_snapshot snapshot;
        a11y::semantic_node row = plain_node( "row:0" );
        row.table_id = "ghost:table";
        row.row_index = 0;
        snapshot.nodes.push_back( row );
        CHECK_FALSE( a11y::validate_snapshot( snapshot, reason ) );
        CHECK( reason == "undeclared_table" );
    }
    SECTION( "undeclared column" ) {
        a11y::semantic_snapshot snapshot;
        a11y::semantic_table table;
        table.id = "t";
        table.columns.push_back( a11y::semantic_column{ "name", "Name", "" } );
        snapshot.tables.push_back( table );
        a11y::semantic_node row = plain_node( "row:0" );
        row.table_id = "t";
        row.row_index = 0;
        row.cells.push_back( a11y::semantic_cell{ "ghost", "x", "", "" } );
        snapshot.nodes.push_back( row );
        CHECK_FALSE( a11y::validate_snapshot( snapshot, reason ) );
        CHECK( reason == "undeclared_column" );
    }
    SECTION( "a declared table with matching cells is accepted" ) {
        a11y::semantic_snapshot snapshot;
        a11y::semantic_table table;
        table.id = "t";
        table.columns.push_back( a11y::semantic_column{ "name", "Name", "" } );
        snapshot.tables.push_back( table );
        a11y::semantic_node row = plain_node( "row:0" );
        row.table_id = "t";
        row.row_index = 0;
        row.cells.push_back( a11y::semantic_cell{ "name", "x", "", "" } );
        snapshot.nodes.push_back( row );
        CHECK( a11y::validate_snapshot( snapshot, reason ) );
    }
}

TEST_CASE( "a11y_intent_value_domain_checks", "[a11y][schema]" )
{
    std::string reason;
    const a11y::semantic_node field = int_field( "f", 0.0, 90.0, 5.0 );
    SECTION( "in-domain values pass" ) {
        CHECK( a11y::validate_intent_value( field, value_intent( "f", "0" ), reason ) );
        CHECK( a11y::validate_intent_value( field, value_intent( "f", "45" ), reason ) );
        CHECK( a11y::validate_intent_value( field, value_intent( "f", "90" ), reason ) );
    }
    SECTION( "out of range is rejected with its code" ) {
        CHECK_FALSE( a11y::validate_intent_value( field, value_intent( "f", "95" ), reason ) );
        CHECK( reason == "value_out_of_range" );
    }
    SECTION( "off-step is rejected with its code" ) {
        CHECK_FALSE( a11y::validate_intent_value( field, value_intent( "f", "47" ), reason ) );
        CHECK( reason == "value_step_mismatch" );
    }
    SECTION( "non-numeric and fractional values are not numbers for an int field" ) {
        CHECK_FALSE( a11y::validate_intent_value( field, value_intent( "f", "abc" ), reason ) );
        CHECK( reason == "not_a_number" );
        CHECK_FALSE( a11y::validate_intent_value( field, value_intent( "f", "4.5" ), reason ) );
        CHECK( reason == "not_a_number" );
    }
    SECTION( "bool and choice domains" ) {
        a11y::semantic_node flag = plain_node( "flag" );
        flag.field_type = "bool";
        CHECK( a11y::validate_intent_value( flag, value_intent( "flag", "true" ), reason ) );
        CHECK_FALSE( a11y::validate_intent_value( flag, value_intent( "flag", "yes" ), reason ) );
        CHECK( reason == "not_a_bool" );

        a11y::semantic_node pick = plain_node( "pick" );
        pick.field_type = "choice";
        pick.choices.push_back( a11y::semantic_choice{ "a", "A" } );
        pick.choices.push_back( a11y::semantic_choice{ "b", "B" } );
        CHECK( a11y::validate_intent_value( pick, value_intent( "pick", "b", "select_choice" ),
                                           reason ) );
        CHECK_FALSE( a11y::validate_intent_value( pick, value_intent( "pick", "c", "select_choice" ),
                       reason ) );
        CHECK( reason == "not_a_choice" );
    }
    SECTION( "a value aimed at a non-field node is refused" ) {
        const a11y::semantic_node button = plain_node( "button" );
        CHECK_FALSE( a11y::validate_intent_value( button, value_intent( "button", "1" ), reason ) );
        CHECK( reason == "not_a_field" );
    }
    SECTION( "non-value actions carry no domain check" ) {
        // The R03 gate relies on this: only set_value/select_choice may
        // carry a value; other actions pass validation untouched.
        a11y::semantic_intent activate;
        activate.node = "f";
        activate.action = "activate";
        activate.value = "not a number";
        CHECK( a11y::validate_intent_value( field, activate, reason ) );
    }
}
