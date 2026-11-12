#pragma once
#ifndef CATA_SRC_A11Y_ANDROID_H
#define CATA_SRC_A11Y_ANDROID_H

#include <array>
#include <cstddef>
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
// Android reader-only display option. Skip world graphics, never gameplay or
// the map/visibility caches that the spoken observer needs.
bool suppress_world_visuals();
bool performance_profiling_enabled();
void configure_performance( bool skip_world_visuals, bool profile );
// Native game-thread observations. Once enough consecutive actual walking
// steps meet the latency budget without rejected input or shifts, profiling
// automatically disables itself and persists that choice through Java.
void performance_draw_sample( long long ms );
void performance_sim_sample( long long ms );
void performance_vision_sample( long long ms );
void performance_reader_sample( long long ms );
void performance_step_sample( bool moved );
void performance_world_shift();

bool enqueue( semantic_intent intent );
std::optional<semantic_intent> take();
void clear();
// Native movement readiness is set only by the game input wait. Unlike
// menu intents, an accepted movement must never sit behind a game turn.
void set_world_step_ready( std::optional<context_token> token, bool ready );
// Small thread-safe atomic/gate probe for bounded Java retry. No game state.
bool world_step_can_accept();
// Called by Android on entering its UI-only command hub; the native
// modal/lifecycle boundaries already invalidate older context tokens.
std::size_t cancel_pending_world_steps();
// Automatic travel is controlled from an Android-only reader panel. JNI
// queues bounded requests; the game thread alone changes the character's
// route. States: 0=inactive, 1=moving, 2=paused, 3=safety-blocked.
// Requests: 1=pause, 2=resume, 3=cancel, 4=ignore current warning. No direct game/Character access from JNI.
int travel_state();
void set_travel_state( int state );
bool request_travel_control( int control );
int take_travel_control();
// One short-lived request to close an observation cursor still active after
// an interrupted trip was explicitly canceled. Never affects zone pickers.
void request_travel_map_return();
bool take_travel_map_return();
void reset_travel_control();
void publish( const std::string &json );
void announce( const std::string &text, int priority );
// Open app-owned speech controls from the Android SDL game thread.
void open_tts_settings();
#if defined(__ANDROID__)
// Main menu controls for the bounded, per-launch Android diagnostic journal.
void open_debug_log();
void open_accessibility_settings();
void open_backup_restore();
void normal_exit();
void diagnostic_event( const std::string &category, const std::string &detail );
// Sound settings are queued by the Java UI, then applied on the C++ game
// thread from the main-menu loop, preserving SDL/option thread ownership.
void open_sound_settings( int music, int effects, bool enabled );
void queue_sound_settings( int music, int effects, bool enabled );
void apply_pending_sound_settings();
#endif
// Accessible recovery actions for SDL debug messages, even without an
// active semantic snapshot. The game thread retains authority to continue.
void present_debug_prompt( const std::string &report );
int take_debug_prompt_response();
void dismiss_debug_prompt();
// Sound/haptic cue channel (R21.2). The name is one of a small set of
// internal ASCII literals ("blocked", "danger", "pickup",
// "activity_done"); cues are only ever fired at engine points that
// already speak the same event to the player, so a cue can never carry
// information the player is not entitled to hear. Playback is opt-in on
// the Java side and every cue has a spoken equivalent.
void cue( const std::string &name );
// Only scan while the user has opted into directional sonar.
void configure_directional_sonar( bool active );
bool directional_sonar_enabled();
// Independent nearest visible creature and item distances on eight rays.
void directional_sonar_targets( const std::array<int, 8> &creatures,
                                const std::array<int, 8> &items );
void status( const std::string &code, const semantic_intent *intent = nullptr );
} // namespace a11y::android
#endif // CATA_SRC_A11Y_ANDROID_H
