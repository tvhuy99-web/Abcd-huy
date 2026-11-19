#pragma once
#ifndef CATA_SRC_ACTIVITY_TRACKER_H
#define CATA_SRC_ACTIVITY_TRACKER_H

#include <string>

#include "calendar.h"

class JsonObject;
class JsonOut;

class activity_tracker
{
    private:
        float current_activity = 0.0;
        float accumulated_activity = 0.0;
        float previous_activity = 0.0;
        float previous_turn_activity = 0.0;
        time_point current_turn = calendar::turn_zero;
        bool activity_reset = true;
        int num_events = 1;



        float low_activity_ticks = 0.0f;
    public:
        int tracker = 0;
        int intake = 0;

        void log_activity( float new_level );

        void new_turn( bool sleeping = false );

        void reset_activity_level();

        std::string activity_level_str() const;

        float activity( bool sleeping = false ) const;

        float average_activity() const;

        float instantaneous_activity_level() const;

        int weariness() const;
        void try_reduce_weariness( int bmr, float sleepiness_mod, float sleepiness_regen_mod );
        void calorie_adjust( int ncal );
        void weary_clear();
        int debug_get_tracker() const;
        void debug_set_tracker( int new_tracker );
        void set_intake( int ncal );
        std::string debug_weary_info() const;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );
};

#endif
