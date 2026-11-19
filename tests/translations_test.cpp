#include <iosfwd>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "translations.h"


#ifndef TRANSLATE_MACRO
#define TRANSLATE_MACRO(msg) _( msg )
#endif


#ifndef TRANSLATE_TRANSLATION
#define TRANSLATE_TRANSLATION(msg) to_translation( msg ).translated()
#endif

TEST_CASE( "translations_sanity_test", "[translations]" )
{
    const std::string test_string = "__untranslated_test_string__";

    CHECK( TRANSLATE_MACRO( test_string ) == test_string );
    CHECK( TRANSLATE_TRANSLATION( test_string ) == test_string );
}



TEST_CASE( "translations_macro_string_stability", "[translations]" )
{
    std::vector<std::string> test_strings;

    SECTION( "untranslated strings" ) {
        test_strings = { "__untranslated_string1__", "__untranslated_string2__" };
    }

    SECTION( "translated strings" ) {
        test_strings = { "thread", "yarn" };
    }

    std::string test_arg;


    for( const std::string &test_string : test_strings ) {
        test_arg.assign( test_string );

        const std::string &res1 = TRANSLATE_MACRO( test_arg );
        CHECK( res1 == test_string );
    }
}



TEST_CASE( "translations_macro_char_address", "[translations]" )
{
    SECTION( "address should be same when translation is absent" ) {
        const char *test_string = "__untranslated_string1__";
        CHECK( TRANSLATE_MACRO( test_string ) == test_string );
    }

    SECTION( "current address of the arg should be returned when translation is absent" ) {

        const auto translate = []( const char *msg ) {
            return TRANSLATE_MACRO( msg );
        };


        const std::string test_string1 = "__untranslated_string1__";
        const std::string test_string2 = "__untranslated_string1__";

        REQUIRE( test_string1.c_str() != test_string2.c_str() );

        CHECK( translate( test_string1.c_str() ) == test_string1.c_str() );
        CHECK( translate( test_string2.c_str() ) == test_string2.c_str() );
        CHECK( translate( test_string1.c_str() ) == test_string1.c_str() );
    }
}

#ifdef LOCALIZE



TEST_CASE( "translations_macro_char_address_translated", "[.][translations]" )
{
    set_language_from_options();

    const char *test_string = "thread";


    CHECK( TRANSLATE_MACRO( test_string ) != test_string );
}
#endif
