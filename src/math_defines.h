#pragma once
#ifndef CATA_SRC_MATH_DEFINES_H
#define CATA_SRC_MATH_DEFINES_H



// We centralize that requirement in this header so that IWYU can force this


// one.  If more are needed, add them to tools/iwyu/vs.stdlib.imp.




// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#define _USE_MATH_DEFINES
// NOLINTNEXTLINE(modernize-deprecated-headers)
#include <math.h>



#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

#ifndef M_LOG2E
#define M_LOG2E 1.44269504088896340736
#endif

#endif
