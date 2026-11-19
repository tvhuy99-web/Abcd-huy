#pragma once
#ifndef CATA_SRC_INPUT_WAIT_H
#define CATA_SRC_INPUT_WAIT_H

#include <cstdint>
#include <optional>



uint32_t ms_until_elapsed_exceeds( uint32_t start, uint32_t now, uint32_t period );

uint32_t ms_until_elapsed_reaches( uint32_t start, uint32_t now, uint32_t period );


struct input_wait_state {

    std::optional<uint32_t> input_ms;

    std::optional<uint32_t> present_ms;



    std::optional<uint32_t> platform_ms;
};


int input_wait_timeout_ms( const input_wait_state &state );


struct touch_timers {
    uint32_t now = 0;
    uint32_t initial_delay = 0;
    uint32_t finger_down_time = 0;
    uint32_t finger_repeat_time = 0;
    uint32_t finger_repeat_delay = 0;
    uint32_t last_tap_time = 0;
    uint32_t back_down_time = 0;
    bool back_toggle_handled = false;
    bool quick_shortcut_touch = false;
    bool multi_finger_touch = false;
    uint32_t last_present = 0;
    uint32_t present_interval = 0;
};



std::optional<uint32_t> touch_wait_ms( const touch_timers &t );

#endif
