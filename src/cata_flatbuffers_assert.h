#pragma once
#ifndef CATA_SRC_CATA_FLATBUFFERS_ASSERT_H
#define CATA_SRC_CATA_FLATBUFFERS_ASSERT_H




#ifdef FLATBUFFERS_ASSERT
#undef FLATBUFFERS_ASSERT
#endif

#include "cata_assert.h"

#define FLATBUFFERS_ASSERT(expression) cata_assert(expression)

#endif
