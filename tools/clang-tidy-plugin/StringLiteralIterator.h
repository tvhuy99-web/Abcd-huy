#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_STRINGLITERALITERATOR_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_STRINGLITERALITERATOR_H

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/TargetInfo.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>

namespace clang
{
class LangOptions;
class SourceManager;
class StringLiteral;

namespace tidy
{
namespace cata
{



class StringLiteralIterator
{
    public:

        StringLiteralIterator( const StringLiteral &str, size_t ind );







        SourceLocation toSourceLocation( const SourceManager &SrcMgr, const LangOptions &LangOpts,
                                         const TargetInfo &Info ) const;



        uint32_t operator*() const;

        bool operator<( const StringLiteralIterator &rhs ) const;
        bool operator>( const StringLiteralIterator &rhs ) const;
        bool operator<=( const StringLiteralIterator &rhs ) const;
        bool operator>=( const StringLiteralIterator &rhs ) const;
        bool operator==( const StringLiteralIterator &rhs ) const;
        bool operator!=( const StringLiteralIterator &rhs ) const;

        StringLiteralIterator &operator+=( ptrdiff_t inc );
        StringLiteralIterator &operator-=( ptrdiff_t dec );
        StringLiteralIterator operator+( ptrdiff_t inc ) const;
        StringLiteralIterator operator-( ptrdiff_t dec ) const;
        StringLiteralIterator &operator++();
        StringLiteralIterator operator++( int );
        StringLiteralIterator &operator--();
        StringLiteralIterator operator--( int );

        static StringLiteralIterator begin( const StringLiteral &str );
        static StringLiteralIterator end( const StringLiteral &str );

        static ptrdiff_t distance( const StringLiteralIterator &beg,
                                   const StringLiteralIterator &end );

    private:
        std::reference_wrapper<const StringLiteral> str;
        size_t ind;
};

}
}
}

namespace std
{
template<>
struct iterator_traits<clang::tidy::cata::StringLiteralIterator> {
    using difference_type = ptrdiff_t;
    using value_type = uint32_t;
    using pointer = const uint32_t *;
    using reference = const uint32_t &;


    using iterator_category = bidirectional_iterator_tag;
};
}

#endif
