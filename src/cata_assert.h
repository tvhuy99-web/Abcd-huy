#pragma once
// NOLINTNEXTLINE(cata-header-guard)





#undef cata_assert


#ifdef NDEBUG


#if defined(_MSC_VER)
#define cata_assert(exp) decltype((exp) ? void() : __assume(0))()
#elif defined(__GNUC__) || defined(__clang__)
#define cata_assert(exp) decltype((exp) ? void() : __builtin_unreachable())()
#else
#include <cstdlib>
#define cata_assert(exp) decltype((exp) ? void() : std::abort())()
#endif
#else
#ifdef _WIN32
#include <cstdlib>
#include <cstdio>
#define cata_assert(expression) \
    do { \
        if( expression ) { \
            break; \
        } \
        (void) fprintf( stderr, "%s at %s:%d: Assertion `%s` failed.\n", __func__, __FILE__, __LINE__, #expression ); \
        std::abort(); \
    } while( false )
#else
#include <cassert>
#define cata_assert(expression) assert(expression)
#endif
#endif
