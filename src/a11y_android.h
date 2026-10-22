#pragma once
#ifndef CATA_SRC_A11Y_ANDROID_H
#define CATA_SRC_A11Y_ANDROID_H

#include <cstdint>
#include <optional>
#include <string>

#include "a11y_semantics.h"

namespace a11y::android
{
// This mailbox is the only UI-thread entry. No gameplay access in JNI callbacks.
bool enabled();
bool suspended();
std::uint64_t lifecycle_epoch();
void configure( bool enabled, bool suspended );
bool enqueue( semantic_intent intent );
std::optional<semantic_intent> take();
void clear();
void publish( const std::string &json );
void announce( const std::string &text, int priority );
void status( const std::string &code, const semantic_intent *intent = nullptr );
} // namespace a11y::android
#endif // CATA_SRC_A11Y_ANDROID_H
