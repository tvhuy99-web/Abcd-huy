#include <string>

#include "a11y_semantics.h"
#include "cata_catch.h"












namespace
{

a11y::semantic_snapshot snapshot_with_label( const std::string &label )
{
    a11y::semantic_snapshot snapshot;
    snapshot.title = label;
    a11y::semantic_node node;
    node.id = "n";
    node.label = label;
    snapshot.nodes.push_back( node );
    return snapshot;
}

}

TEST_CASE( "a11y_snapshot_json_preserves_vietnamese_utf8", "[a11y][unicode]" )
{



    const std::string precomposed = "Nguyễn Đặng";


    const std::string decomposed = "Nguye\xcc\x82\xcc\x83n";
    const std::string outside_bmp = "Mục tiêu \xf0\x9f\x98\x80";

    for( const std::string &text : {
             precomposed, decomposed, outside_bmp
         } ) {
        const std::string json = a11y::snapshot_json( snapshot_with_label( text ) );
        CHECK( json.find( text ) != std::string::npos );
    }
}

TEST_CASE( "a11y_snapshot_json_escapes_only_control_characters", "[a11y][unicode]" )
{
    const std::string label = std::string( "dòng 1\ndòng 2\t\"ngoặc\"" );
    const std::string json = a11y::snapshot_json( snapshot_with_label( label ) );


    CHECK( json.find( "\\n" ) != std::string::npos );
    CHECK( json.find( "\\t" ) != std::string::npos );
    CHECK( json.find( "dòng 1" ) != std::string::npos );
    CHECK( json.find( "dòng 2" ) != std::string::npos );
}
