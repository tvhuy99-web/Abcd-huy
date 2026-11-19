#pragma once
#ifndef CATA_SRC_CATA_UNREACHABLE_H
#define CATA_SRC_CATA_UNREACHABLE_H


#ifdef __GNUC__
[[noreturn]] inline __attribute__( ( always_inline ) ) void cata_unreachable()
{
    __builtin_unreachable();
}
#elif defined(_MSC_VER)
[[noreturn]] __forceinline void cata_unreachable()
{
    __assume( false );
}
#else
inline void cata_unreachable() {}
#endif

#endif
