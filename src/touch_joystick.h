#pragma once
#ifndef CATA_SRC_TOUCH_JOYSTICK_H
#define CATA_SRC_TOUCH_JOYSTICK_H

#include <string>
#include <utility>
#include <vector>


namespace touch_joystick
{


enum class direction : int {
    none,
    right,
    down_right,
    down,
    down_left,
    left,
    up_left,
    up,
    up_right,
};




direction classify( float dx, float dy, float deadzone, float straight_width );


std::pair<float, float> wedge( direction dir, float straight_width );


float speed_fraction( float dist, float deadzone, float range );



float octagon_radius( float degrees, float apothem );



std::vector<float> band_angles( float start, float end );



bool is_bare_context( const std::vector<std::string> &actions );

}

#endif
