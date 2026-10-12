#include <string>

#include "cata_utility.h"
#include "debug.h"
#include "get_version.h"
#include "path_info.h"
#include "text_snippets.h"
#include "translations.h"
#include "translation_gendered.h"

// int version/generation that is incremented each time language is changed
// used to invalidate translation cache
static int current_language_version = INVALID_LANGUAGE_VERSION + 1;

int detail::get_current_language_version()
{
    return current_language_version;
}

#if defined(LOCALIZE)
std::string select_language()
{
    return "en";
}
#endif // LOCALIZE

std::string locale_dir()
{
    std::string loc_dir = PATH_INFO::langdir();
#if defined(LOCALIZE)

#if (defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)) && !defined(CATA_IS_ON_BSD)
#define CATA_IS_ON_BSD
#endif

#if !defined(__ANDROID__) && ((defined(__linux__) || defined(CATA_IS_ON_BSD) || (defined(MACOSX) && !defined(TILES))))
    if( !PATH_INFO::base_path().get_logical_root_path().empty() ) {
        loc_dir = ( PATH_INFO::base_path() / "share" / "locale" ).generic_u8string();
    } else {
        loc_dir = PATH_INFO::langdir();
    }
#endif
#endif // LOCALIZE
    return loc_dir;
}

void set_language_from_options()
{
    // Keep existing saves and non-English device locales on the source language.
    set_language( "en" );
}

void set_language( const std::string &lang )
{
    // Retain the translation API for future Vietnamese work, while this stage is English-only.
    static_cast<void>( lang );
#if defined(LOCALIZE)
    DebugLog( D_INFO, D_MAIN ) << "Setting language to: 'en'";
    TranslationManager::GetInstance().SetLanguage( "en" );
    TranslationManager::GetInstance().LoadDocuments( {} );
#if defined(_WIN32)
    // Use the ANSI code page 1252 to work around some language output bugs. (#8665)
    if( setlocale( LC_ALL, ".1252" ) == nullptr ) {
        DebugLog( D_WARNING, D_MAIN ) << "Error while setlocale(LC_ALL, '.1252').";
    }
#endif

    reset_sanity_check_genders();

    // increment version to invalidate translation cache
    do {
        current_language_version++;
    } while( current_language_version == INVALID_LANGUAGE_VERSION );

#endif // LOCALIZE

    // Names depend on the language settings. They are loaded from different files
    // based on the currently used language. If that changes, we have to reload the
    // names.
    SNIPPET.reload_names( PATH_INFO::names() );

    set_title( string_format( _( "Cataclysm: Dark Days Ahead - %s" ), getVersionString() ) );
}
