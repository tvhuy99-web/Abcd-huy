#pragma once
#ifndef CATA_SRC_SDL_GAMEPAD_H
#define CATA_SRC_SDL_GAMEPAD_H
#if defined(TILES)
#include "input_enums.h"
#include "sdl_wrappers.h"

#define SDL_GAMEPAD_SCHEDULER (SDL_EVENT_USER+1)

struct tripoint;

// IWYU pragma: no_forward_declare input_event  // Is valid, but looks silly
extern input_event last_input;

namespace gamepad
{


enum class direction : int {
    NONE = 0,
    N,
    NE,
    E,
    SE,
    S,
    SW,
    W,
    NW
};

void init();
void quit();

bool is_gamepad_event( const SDL_Event &event );


bool handle_event( SDL_Event &event );


direction get_left_stick_direction();
direction get_right_stick_direction();
direction get_radial_left_direction();
direction get_radial_right_direction();
bool is_radial_left_open();
bool is_radial_right_open();
bool is_alt_held();
bool is_in_menu();
bool is_active();


tripoint direction_to_offset( direction dir );


int direction_to_radial_joy( direction dir, int stick_idx );

}

#endif
#endif
