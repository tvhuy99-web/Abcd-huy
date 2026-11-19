#pragma once


#if defined(__clang__) && __clang_major__ == 21 && __clang_minor__ <= 1
#   define LAMBDA_NORETURN_CLANG21x1 __attribute__( ( noreturn ) )
#else
#   define LAMBDA_NORETURN_CLANG21x1
#endif



#ifndef CATA_FORCEINLINE
#   ifdef _MSC_VER
#       define CATA_FORCEINLINE __forceinline
#   else
#       define CATA_FORCEINLINE inline __attribute__( ( always_inline ) )
#   endif
#endif
