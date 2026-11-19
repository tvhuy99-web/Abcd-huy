#pragma once
#ifndef CATA_SRC_TRANSLATION_H
#define CATA_SRC_TRANSLATION_H

#include <optional>
#include <string>

#include "translation_cache.h"
#include "value_ptr.h"

class JsonObject;
class JsonValue;




class translation
{
    public:
        struct plural_tag {};




        // NOLINTNEXTLINE default constructor
        translation() {}



        explicit translation( plural_tag );




        static translation to_translation( const std::string &raw );
        static translation to_translation( const std::string &ctxt, const std::string &raw );
        static translation pl_translation( const std::string &raw, const std::string &raw_pl );
        static translation pl_translation( const std::string &ctxt, const std::string &raw,
                                           const std::string &raw_pl );



        static translation no_translation( const std::string &str );










        void make_plural();










        void deserialize( const JsonValue &jsin );
        void deserialize( const JsonObject &jsobj );






        const std::string &translated( int num = 1 ) const;











        std::string untranslated() const = delete;






        void serialize( JsonOut &jsout ) const = delete;





        bool empty() const;











        bool translated_lt( const translation &that ) const;
        bool translated_gt( const translation &that ) const;
        bool translated_le( const translation &that ) const;
        bool translated_ge( const translation &that ) const;
        bool translated_eq( const translation &that ) const;
        bool translated_ne( const translation &that ) const;






        bool operator==( const translation &that ) const;
        bool operator!=( const translation &that ) const;




        bool operator<( const translation &that ) const = delete;
        bool operator>( const translation &that ) const = delete;
        bool operator<=( const translation &that ) const = delete;
        bool operator>=( const translation &that ) const = delete;




        std::optional<int> legacy_hash() const;
    private:
        translation( const std::string &ctxt, const std::string &raw );
        explicit translation( const std::string &raw );
        translation( const std::string &raw, const std::string &raw_pl, plural_tag );
        translation( const std::string &ctxt, const std::string &raw, const std::string &raw_pl,
                     plural_tag );
        struct no_translation_tag {};
        translation( const std::string &str, no_translation_tag );

        cata::value_ptr<std::string> ctxt;
        std::string raw;
        cata::value_ptr<std::string> raw_pl;
        bool needs_translation = false;

        mutable int cached_language_version = INVALID_LANGUAGE_VERSION;

        mutable int cached_num = 0;
        mutable cata::value_ptr<std::string> cached_translation;
};











class translated_less
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

class translated_greater
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

class translated_less_equal
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

class translated_greater_equal
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

class translated_equal_to
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

class translated_not_equal_to
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};




translation to_translation( const std::string &raw );
translation to_translation( const std::string &ctxt, const std::string &raw );



translation pl_translation( const std::string &raw, const std::string &raw_pl );
translation pl_translation( const std::string &ctxt, const std::string &raw,
                            const std::string &raw_pl );



translation no_translation( const std::string &str );




std::ostream &operator<<( std::ostream &out, const translation &t );
std::string operator+( const translation &lhs, const std::string &rhs );
std::string operator+( const std::string &lhs, const translation &rhs );
std::string operator+( const translation &lhs, const translation &rhs );

#endif
