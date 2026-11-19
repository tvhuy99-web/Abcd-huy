#pragma once
#ifndef CATA_SRC_TRANSLATIONS_H
#define CATA_SRC_TRANSLATIONS_H



#include <locale> // IWYU pragma: keep

// pragma export needed because IWYU gets confused about FMT_STRING
#include <fmt/format.h> // IWYU pragma: export

#include "string_formatter.h"
#include "translation.h"

#if !defined(translate_marker)






#define translate_marker(x) x
#endif
#if !defined(translate_marker_context)





#define translate_marker_context(c, x) x
#endif

#if defined(LOCALIZE)

#include "translation_cache.h"

#if defined(__GNUC__)
#  define ATTRIBUTE_FORMAT_ARG(a) __attribute__((format_arg(a)))
#else
#  define ATTRIBUTE_FORMAT_ARG(a)
#endif

std::string select_language();





template<typename T>
inline const T &translation_argument_identity( const T &t )
{
    // NOLINTNEXTLINE(bugprone-return-const-ref-from-parameter)
    return t;
}





// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#define _( msg ) \
    ( ( []( const auto & arg ) { \
        static auto cache = detail::get_local_translation_cache( arg ); \
        return cache( arg ); \
    } )( translation_argument_identity( msg ) ) )

inline const char *n_gettext( const char *msgid, const char *msgid_plural,
                              std::size_t n ) ATTRIBUTE_FORMAT_ARG( 1 );

inline const char *n_gettext( const char *msgid, const char *msgid_plural,
                              std::size_t n )
{
    return TranslationManager::GetInstance().TranslatePlural( msgid, msgid_plural, n );
}

inline const char *pgettext( const char *context, const char *msgid ) ATTRIBUTE_FORMAT_ARG( 2 );

inline const char *pgettext( const char *context, const char *msgid )
{
    return TranslationManager::GetInstance().TranslateWithContext( context, msgid );
}

inline const char *npgettext( const char *context, const char *msgid,
                              const char *msgid_plural, unsigned long long n ) ATTRIBUTE_FORMAT_ARG( 2 );

inline const char *npgettext( const char *const context, const char *const msgid,
                              const char *const msgid_plural, const unsigned long long n )
{
    return TranslationManager::GetInstance().TranslatePluralWithContext( context, msgid, msgid_plural,
            n );
}

#else

#define _(STRING) (STRING)

#define n_gettext(STRING1, STRING2, COUNT) ((COUNT) == 1 ? _(STRING1) : _(STRING2))
#define pgettext(STRING1, STRING2) _(STRING2)
#define npgettext(STRING0, STRING1, STRING2, COUNT) n_gettext(STRING1, STRING2, COUNT)

#endif




template<typename ...Ts>
struct type_pack {
    template<template<typename ...> typename T> using apply = T<Ts...>;
};

template<typename ...Ts>
constexpr static auto make_type_pack( Ts &&... )
{
    return type_pack<Ts...> {};
}

template<typename ...Args>
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
inline std::string _fmt_inner( std::string_view translation, Args &&...args )
{
    return string_vfmt( translation, fmt::make_format_args( args... ) );
}

#define _fmt( msg, ... ) \
    (decltype(make_type_pack(__VA_ARGS__))::apply<fmt::format_string>{ FMT_STRING(msg) }, _fmt_inner( _(msg), __VA_ARGS__ ))



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wredundant-decls"
[[deprecated( "Use _( ... ) instead" )]]
//NOLINTNEXTLINE(readability-redundant-declaration,readability-inconsistent-declaration-parameter-name,cata-no-long)
char *gettext( const char *msgid );
[[deprecated( "Use n_gettext( ... ) instead" )]]
//NOLINTNEXTLINE(readability-redundant-declaration,readability-inconsistent-declaration-parameter-name,cata-no-long)
char *ngettext( const char *msgid, const char *msgid2, unsigned long int n );
#pragma GCC diagnostic pop

std::string locale_dir();

void set_language_from_options();
void set_language( const std::string &lang );

#endif
