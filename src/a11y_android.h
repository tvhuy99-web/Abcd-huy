#pragma once
#ifndef CATA_SRC_A11Y_ANDROID_H
#define CATA_SRC_A11Y_ANDROID_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "a11y_semantics.h"

namespace a11y { struct context_token; }

namespace a11y::android
{

bool enabled();
bool suspended();
std::uint64_t lifecycle_epoch();
void configure( bool enabled, bool suspended );


bool suppress_world_visuals();
bool performance_profiling_enabled();
void configure_performance( bool skip_world_visuals, bool profile );



void performance_draw_sample( long long ms );
void performance_sim_sample( long long ms );
void performance_vision_sample( long long ms );
void performance_reader_sample( long long ms );
void performance_step_sample( bool moved );
void performance_world_shift();

bool enqueue( semantic_intent intent );
std::optional<semantic_intent> take();
void clear();


void set_world_step_ready( std::optional<context_token> token, bool ready );

bool world_step_can_accept();


std::size_t cancel_pending_world_steps();




int travel_state();
void set_travel_state( int state );
bool request_travel_control( int control );
int take_travel_control();


bool travel_control_pending();


void request_travel_map_return();
bool take_travel_map_return();
void reset_travel_control();
void publish( std::string_view json );
void announce( std::string_view text, int priority );

void open_tts_settings();
#if defined(__ANDROID__)

void open_debug_log();
void open_accessibility_settings();
void open_backup_restore();
void normal_exit();
void diagnostic_event( std::string_view category, std::string_view detail );


void open_sound_settings( int music, int effects, bool enabled );
void queue_sound_settings( int music, int effects, bool enabled );
void apply_pending_sound_settings();
#else


inline void diagnostic_event( std::string_view, std::string_view ) {}
#endif


void present_debug_prompt( std::string_view report );
int take_debug_prompt_response();
void dismiss_debug_prompt();






void cue( const std::string &name );

void configure_directional_sonar( bool active );
bool directional_sonar_enabled();

void directional_sonar_targets( const std::array<int, 8> &creatures,
                                const std::array<int, 8> &items );
void status( const std::string &code, const semantic_intent *intent = nullptr );
}
#endif
