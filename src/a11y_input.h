#pragma once
#ifndef CATA_SRC_A11Y_INPUT_H
#define CATA_SRC_A11Y_INPUT_H
#include <optional>
#include <string>
#include "a11y_semantics.h"
class input_context;
namespace a11y
{
semantic_snapshot command_snapshot( const input_context &context, const std::string &title );
std::optional<std::string> registered_intent( const input_context &context,
        const semantic_intent &intent );
} // namespace a11y
#endif // CATA_SRC_A11Y_INPUT_H
