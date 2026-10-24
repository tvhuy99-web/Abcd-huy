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
// Sound/haptic cue channel (R21.2). The name is one of a small set of
// internal ASCII literals ("blocked", "danger", "pickup",
// "activity_done"); cues are only ever fired at engine points that
// already speak the same event to the player, so a cue can never carry
// information the player is not entitled to hear. Playback is opt-in on
// the Java side and every cue has a spoken equivalent.
void cue( const std::string &name );
void status( const std::string &code, const semantic_intent *intent = nullptr );
} // namespace a11y::android
#endif // CATA_SRC_A11Y_ANDROID_H
