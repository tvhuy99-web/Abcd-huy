#include <string>

#include "a11y_semantics.h"
#include "cata_catch.h"

// Written during the coding phase (R22.2). Oracle: Vietnamese text --
// precomposed, combining-mark sequences, and characters outside the BMP --
// must survive snapshot serialization byte-for-byte, because the JSON is
// what the Java side speaks. src/a11y_semantics.cpp's quote() passes every
// byte >= 0x20 through unchanged and escapes only control characters, so
// the serialized form must contain the original UTF-8 byte sequences
// verbatim and still be one well-formed JSON string. Grapheme-caret
// behaviour itself lives in the Java editor (ICU BreakIterator) and is
// covered by the device catalog (T17), not by this host test.
// Execute with the full suite only after the coding phase is complete.

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

} // namespace

TEST_CASE( "a11y_snapshot_json_preserves_vietnamese_utf8", "[a11y][unicode]" )
{
    // "Nguyễn Đặng" precomposed, "Nguyễn" written with combining marks
    // (e + circumflex + tilde as separate code points), and an
    // outside-BMP character (U+1F600) that needs a UTF-8 4-byte sequence.
    const std::string precomposed = "Nguyễn Đặng";
    // "Nguyễn" decomposed: e + combining circumflex (U+0302) + combining
    // tilde (U+0303), as a Vietnamese IME in combining mode produces it.
    const std::string decomposed = "Nguye\xcc\x82\xcc\x83n";
    const std::string outside_bmp = "Mục tiêu \xf0\x9f\x98\x80";

    for( const std::string &text : { precomposed, decomposed, outside_bmp } ) {
        const std::string json = a11y::snapshot_json( snapshot_with_label( text ) );
        CHECK( json.find( text ) != std::string::npos );
    }
}

TEST_CASE( "a11y_snapshot_json_escapes_only_control_characters", "[a11y][unicode]" )
{
    const std::string label = std::string( "dòng 1\ndòng 2\t\"ngoặc\"" );
    const std::string json = a11y::snapshot_json( snapshot_with_label( label ) );
    // The raw newline/tab must not appear inside the JSON string; the
    // Vietnamese bytes around them must be untouched.
    CHECK( json.find( "\\n" ) != std::string::npos );
    CHECK( json.find( "\\t" ) != std::string::npos );
    CHECK( json.find( "dòng 1" ) != std::string::npos );
    CHECK( json.find( "dòng 2" ) != std::string::npos );
}
