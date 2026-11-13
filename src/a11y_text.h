#pragma once
#ifndef CATA_SRC_A11Y_TEXT_H
#define CATA_SRC_A11Y_TEXT_H

#include <string>

#include "a11y_semantics.h"

namespace a11y
{
semantic_snapshot text_snapshot( std::string title, std::string description,
                                 std::string value, bool numeric,
                                 double field_step = 0.0, bool integer = false );
} // namespace a11y

#endif // CATA_SRC_A11Y_TEXT_H
