#pragma once
#ifndef CATA_SRC_TRANSLATION_CACHE_H
#define CATA_SRC_TRANSLATION_CACHE_H

#include <string>

#if defined(LOCALIZE)
#include "translation_manager.h"
#endif

constexpr int INVALID_LANGUAGE_VERSION = 0;

namespace detail
{

#if defined(LOCALIZE)

inline const char *_translate_internal( const char *msg )
{
    return TranslationManager::GetInstance().Translate( msg );
}

inline std::string _translate_internal( const std::string &msg )
{
    return TranslationManager::GetInstance().Translate( msg );
}

#else

inline const char *_translate_internal( const char *msg )
{
    return msg;
}

inline std::string _translate_internal( const std::string &msg )
{
    return msg;
}

#endif


int get_current_language_version();

template<typename T>
class local_translation_cache;

template<>
class local_translation_cache<std::string>
{
    private:
#ifndef CATA_IN_TOOL
        int cached_lang_version = INVALID_LANGUAGE_VERSION;
#endif
        std::string cached_arg;
        std::string cached_translation;
    public:
        const std::string &operator()( const std::string &arg ) {
#ifndef CATA_IN_TOOL
            if( cached_lang_version != get_current_language_version() || cached_arg != arg ) {
                cached_lang_version = get_current_language_version();
                cached_arg = arg;
                cached_translation = _translate_internal( arg );
            }
            return cached_translation;
#else
            return arg;
#endif
        }
};

template<>
class local_translation_cache<const char *>
{
    private:
        std::string cached_arg;
#ifndef CATA_IN_TOOL
        int cached_lang_version = INVALID_LANGUAGE_VERSION;
        bool same_as_arg = false;
        const char *cached_translation = nullptr;
#endif
    public:
        const char *operator()( const char *arg ) {
#ifndef CATA_IN_TOOL
            if( cached_lang_version != get_current_language_version() || cached_arg != arg ) {
                cached_lang_version = get_current_language_version();
                cached_translation = _translate_internal( arg );
                same_as_arg = cached_translation == arg;
                cached_arg = arg;
            }


            return same_as_arg ? arg : cached_translation;
#else
            return arg;
#endif
        }
};



inline local_translation_cache<const char *> get_local_translation_cache( const char * )
{
    return local_translation_cache<const char *>();
}

inline local_translation_cache<std::string> get_local_translation_cache(
    const std::string_view )
{
    return local_translation_cache<std::string>();
}

}

#endif
