#pragma once
#ifndef CATA_SRC_A11Y_QUANTITY_SELECTION_H
#define CATA_SRC_A11Y_QUANTITY_SELECTION_H

#include <cstddef>
#include <optional>
#include <string_view>

namespace a11y
{


std::optional<std::size_t> selected_quantity( std::string_view text, std::size_t available );
}

#endif
