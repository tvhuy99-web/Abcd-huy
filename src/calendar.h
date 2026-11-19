#pragma once
#ifndef CATA_SRC_CALENDAR_H
#define CATA_SRC_CALENDAR_H

#include <array>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <climits>

#include "units_fwd.h"

class JsonOut;
class JsonValue;
struct rl_vec2d;
class time_duration;
class time_point;
template<typename T> struct enum_traits;


enum season_type {
    SPRING = 0,
    SUMMER = 1,
    AUTUMN = 2,
    WINTER = 3,
    NUM_SEASONS
};

template<>
struct enum_traits<season_type> {
    static constexpr season_type last = season_type::NUM_SEASONS;
};


enum moon_phase {

    MOON_NEW = 0,

    MOON_WAXING_CRESCENT,

    MOON_HALF_MOON_WAXING,

    MOON_WAXING_GIBBOUS,

    MOON_FULL,

    MOON_WANING_GIBBOUS,

    MOON_HALF_MOON_WANING,

    MOON_WANING_CRESCENT,

    MOON_PHASE_MAX
};

inline auto format_as( moon_phase mp )
{
    return static_cast<std::underlying_type_t<moon_phase>>( mp );
}

enum class time_accuracy {

    NONE = 0,

    PARTIAL = 1,

    FULL = 2,

    NUM_TIME_ACCURACY
};

template<>
struct enum_traits<time_accuracy> {
    static constexpr time_accuracy last = time_accuracy::NUM_TIME_ACCURACY;
};







namespace calendar
{



bool once_every( const time_duration &event_frequency );








extern const int INDEFINITELY_LONG;


bool eternal_season();
void set_eternal_season( bool is_eternal_season );


bool eternal_night();
void set_eternal_night( bool is_eternal_night );


bool eternal_day();
void set_eternal_day( bool is_eternal_day );


time_duration year_length();


time_duration season_length();
void set_season_length( int dur );

void set_location( float latitude, float longitude );


time_duration turn_zero_offset();
int years_since_cataclysm( time_point );


float season_ratio();






float season_from_default_ratio();


std::string name_season( season_type s );

extern time_point start_of_cataclysm;
extern time_point start_of_game;
extern time_point turn;
extern season_type initial_season;

}

template<typename T>
constexpr T to_turns( const time_duration &duration );
template<typename T>
constexpr T to_seconds( const time_duration &duration );
template<typename T>
constexpr T to_minutes( const time_duration &duration );
template<typename T>
constexpr T to_hours( const time_duration &duration );
template<typename T>
constexpr T to_days( const time_duration &duration );
template<typename T>
constexpr T to_weeks( const time_duration &duration );
template<typename T>
constexpr T to_moves( const time_duration &duration );

template<typename T>
constexpr T to_turn( const time_point &point );

template<typename T>
constexpr time_duration operator/( const time_duration &lhs, T rhs );
template<typename T>
inline time_duration &operator/=( time_duration &lhs, T rhs );
template<typename T>
constexpr time_duration operator*( const time_duration &lhs, T rhs );
template<typename T>
constexpr time_duration operator*( T lhs, const time_duration &rhs );
template<typename T>
inline time_duration &operator*=( time_duration &lhs, T rhs );























class time_duration
{
    private:
        friend class time_point;
        int turns_;

        explicit constexpr time_duration( const int t ) : turns_( t ) { }

    public:
        time_duration() : turns_( 0 ) {}

        void serialize( JsonOut &jsout ) const;
        void deserialize( const JsonValue &jsin );











        template<typename T>
        static constexpr time_duration from_turns( const T t ) {
            return time_duration( t );
        }
        template<typename T>
        static constexpr time_duration from_moves( const T t ) {
            return time_duration( t / 100 );
        }
        template<typename T>
        static constexpr time_duration from_seconds( const T t ) {
            return time_duration( t );
        }
        template<typename T>
        static constexpr time_duration from_minutes( const T m ) {
            return from_turns( m * 60 );
        }
        template<typename T>
        static constexpr time_duration from_hours( const T h ) {
            return from_minutes( h * 60 );
        }
        template<typename T>
        static constexpr time_duration from_days( const T d ) {
            return from_hours( d * 24 );
        }
        template<typename T>
        static constexpr time_duration from_weeks( const T d ) {
            return from_days( d * 7 );
        }













        template<typename T>
        friend constexpr T to_turns( const time_duration &duration ) {
            return duration.turns_;
        }
        template<typename T>
        friend constexpr T to_seconds( const time_duration &duration ) {
            return duration.turns_;
        }
        template<typename T>
        friend constexpr T to_minutes( const time_duration &duration ) {
            return static_cast<T>( duration.turns_ ) / static_cast<T>( 60 );
        }
        template<typename T>
        friend constexpr T to_hours( const time_duration &duration ) {
            return static_cast<T>( duration.turns_ ) / static_cast<T>( 60 * 60 );
        }
        template<typename T>
        friend constexpr T to_days( const time_duration &duration ) {
            return static_cast<T>( duration.turns_ ) / static_cast<T>( 60 * 60 * 24 );
        }
        template<typename T>
        friend constexpr T to_weeks( const time_duration &duration ) {
            return static_cast<T>( duration.turns_ ) / static_cast<T>( 60 * 60 * 24 * 7 );
        }
        template<typename T>
        friend constexpr T to_moves( const time_duration &duration ) {
            return to_turns<int>( duration ) * 100;
        }


        constexpr bool operator<( const time_duration &rhs ) const {
            return turns_ < rhs.turns_;
        }
        constexpr bool operator<=( const time_duration &rhs ) const {
            return turns_ <= rhs.turns_;
        }
        constexpr bool operator>( const time_duration &rhs ) const {
            return turns_ > rhs.turns_;
        }
        constexpr bool operator>=( const time_duration &rhs ) const {
            return turns_ >= rhs.turns_;
        }
        constexpr bool operator==( const time_duration &rhs ) const {
            return turns_ == rhs.turns_;
        }
        constexpr bool operator!=( const time_duration &rhs ) const {
            return turns_ != rhs.turns_;
        }

        friend constexpr time_duration operator-( const time_duration &duration ) {
            return time_duration( -duration.turns_ );
        }
        friend constexpr time_duration operator+( const time_duration &lhs, const time_duration &rhs ) {
            return time_duration( lhs.turns_ + rhs.turns_ );
        }
        friend time_duration &operator+=( time_duration &lhs, const time_duration &rhs ) {
            return lhs = time_duration( lhs.turns_ + rhs.turns_ );
        }
        friend constexpr time_duration operator-( const time_duration &lhs, const time_duration &rhs ) {
            return time_duration( lhs.turns_ - rhs.turns_ );
        }
        friend time_duration &operator-=( time_duration &lhs, const time_duration &rhs ) {
            return lhs = time_duration( lhs.turns_ - rhs.turns_ );
        }

        friend double operator/( const time_duration &lhs, const time_duration &rhs ) {
            return static_cast<double>( lhs.turns_ ) / static_cast<double>( rhs.turns_ );
        }
        template<typename T>
        friend constexpr time_duration operator/( const time_duration &lhs, const T rhs ) {
            return time_duration( lhs.turns_ / rhs );
        }
        template<typename T>
        friend time_duration &operator/=( time_duration &lhs, const T rhs ) {
            return lhs = time_duration( lhs.turns_ / rhs );
        }
        template<typename T>
        friend constexpr time_duration operator*( const time_duration &lhs, const T rhs ) {
            return time_duration( lhs.turns_ * rhs );
        }
        template<typename T>
        friend constexpr time_duration operator*( const T lhs, const time_duration &rhs ) {
            return time_duration( lhs * rhs.turns_ );
        }
        template<typename T>
        friend time_duration &operator*=( time_duration &lhs, const T rhs ) {
            return lhs = time_duration( lhs.turns_ * rhs );
        }
        friend time_duration operator%( const time_duration &lhs, const time_duration &rhs ) {
            return time_duration( lhs.turns_ % rhs.turns_ );
        }


        friend time_duration rng( time_duration lo, time_duration hi );

        static const std::array<std::pair<std::string_view, time_duration>, 15> units;
};


bool x_in_y( const time_duration &a, const time_duration &b );






constexpr time_duration operator""_turns( const unsigned long long int v )
{
    return time_duration::from_turns( v );
}
constexpr time_duration operator""_seconds( const unsigned long long int v )
{
    return time_duration::from_seconds( v );
}
constexpr time_duration operator""_minutes( const unsigned long long int v )
{
    return time_duration::from_minutes( v );
}
constexpr time_duration operator""_hours( const unsigned long long int v )
{
    return time_duration::from_hours( v );
}
constexpr time_duration operator""_days( const unsigned long long int v )
{
    return time_duration::from_days( v );
}
constexpr time_duration operator""_weeks( const unsigned long long int v )
{
    return time_duration::from_weeks( v );
}










std::string to_string( const time_duration &d, bool compact = false );

enum class clipped_align : int {
    none,
    right,
    compact
};

enum class clipped_unit : int {
    forever,
    second,
    minute,
    hour,
    day,
    week,
    season,
    year,
};









std::pair<int, clipped_unit> clipped_time( const time_duration &d );









std::string to_string_clipped( const time_duration &d,
                               clipped_align align = clipped_align::none );




std::string to_string_approx( const time_duration &dur, bool verbose = true );



std::string to_string_writable( const time_duration &dur );










class time_point
{
    private:
        friend class time_duration;
        int turn_;

    public:
        time_point();

        explicit constexpr time_point( const int t ) : turn_( t ) { }



        static constexpr time_point from_turn( const int t ) {
            return time_point( t );
        }

        void serialize( JsonOut &jsout ) const;
        void deserialize( int );


        template<typename T>
        friend constexpr T to_turn( const time_point &point ) {
            return point.turn_;
        }

        friend constexpr inline bool operator<( const time_point &lhs, const time_point &rhs ) {
            return to_turn<int>( lhs ) < to_turn<int>( rhs );
        }
        friend constexpr inline bool operator<=( const time_point &lhs, const time_point &rhs ) {
            return to_turn<int>( lhs ) <= to_turn<int>( rhs );
        }
        friend constexpr inline bool operator>( const time_point &lhs, const time_point &rhs ) {
            return to_turn<int>( lhs ) > to_turn<int>( rhs );
        }
        friend constexpr inline bool operator>=( const time_point &lhs, const time_point &rhs ) {
            return to_turn<int>( lhs ) >= to_turn<int>( rhs );
        }
        friend constexpr inline bool operator==( const time_point &lhs, const time_point &rhs ) {
            return to_turn<int>( lhs ) == to_turn<int>( rhs );
        }
        friend constexpr inline bool operator!=( const time_point &lhs, const time_point &rhs ) {
            return to_turn<int>( lhs ) != to_turn<int>( rhs );
        }

        friend constexpr inline time_duration operator-(
            const time_point &lhs, const time_point &rhs ) {
            return time_duration::from_turns( to_turn<int>( lhs ) - to_turn<int>( rhs ) );
        }
        friend constexpr inline time_point operator+(
            const time_point &lhs, const time_duration &rhs ) {
            return time_point::from_turn( to_turn<int>( lhs ) + to_turns<int>( rhs ) );
        }
        friend time_point inline &operator+=( time_point &lhs, const time_duration &rhs ) {
            return lhs = time_point::from_turn( to_turn<int>( lhs ) + to_turns<int>( rhs ) );
        }
        friend constexpr inline time_point operator-(
            const time_point &lhs, const time_duration &rhs ) {
            return time_point::from_turn( to_turn<int>( lhs ) - to_turns<int>( rhs ) );
        }
        friend time_point inline &operator-=( time_point &lhs, const time_duration &rhs ) {
            return lhs = time_point::from_turn( to_turn<int>( lhs ) - to_turns<int>( rhs ) );
        }


        friend constexpr inline time_point operator/( const time_point &lhs, const time_duration &rhs ) {
            return time_point::from_turn( to_turn<int>( lhs ) / to_turns<int>( rhs ) );
        }
        friend constexpr inline time_point operator*( const time_point &lhs, const time_duration &rhs ) {
            return time_point::from_turn( to_turn<int>( lhs ) * to_turns<int>( rhs ) );
        }
        friend time_point inline &operator/=( time_point &lhs, const time_duration &rhs ) {
            return lhs = time_point::from_turn( to_turn<int>( lhs ) / to_turns<int>( rhs ) );
        }
        friend time_point inline &operator*=( time_point &lhs, const time_duration &rhs ) {
            return lhs = time_point::from_turn( to_turn<int>( lhs ) * to_turns<int>( rhs ) );
        }


};

namespace calendar
{









inline constexpr time_duration INDEFINITELY_LONG_DURATION(
    time_duration::from_turns( std::numeric_limits<int>::max() ) );







constexpr time_point before_time_starts = time_point::from_turn( -1 );





constexpr time_point turn_zero = time_point::from_turn( 0 );




constexpr time_point turn_max = time_point::from_turn( INT_MAX );

}

inline time_duration time_past_midnight( const time_point &p )
{
    return ( p - calendar::turn_zero ) % 1_days;
}

inline time_duration time_past_new_year( const time_point &p )
{
    return ( p - calendar::turn_zero + calendar::turn_zero_offset() ) % calendar::year_length();
}

template<typename T>
inline T minute_of_hour( const time_point &p )
{
    return to_minutes<T>( ( p - calendar::turn_zero ) % 1_hours );
}

template<typename T>
inline T hour_of_day( const time_point &p )
{
    return to_hours<T>( ( p - calendar::turn_zero ) % 1_days );
}


template<typename T>
inline T day_of_season( const time_point &p )
{
    return to_days<T>( ( p - calendar::turn_zero ) % calendar::season_length() );
}



season_type season_of_year( const time_point &p );

std::string to_string( const time_point &p );

std::string to_string_time_of_day( const time_point &p );

std::string get_diary_time_str( const time_point &turn, time_accuracy acc );

std::string get_diary_time_since_str( const time_duration &turn_diff, time_accuracy acc,
                                      bool include_postfix = true );

time_duration lunar_month();

moon_phase get_moon_phase( const time_point &p );

time_point sunrise( const time_point &p );

time_point sunset( const time_point &p );

time_point daylight_time( const time_point &p );

time_point night_time( const time_point &p );

time_point noon( const time_point &p );

bool is_night( const time_point &p );

bool is_day( const time_point &p );

bool is_dusk( const time_point &p );

bool is_dawn( const time_point &p );

bool is_twilight( const time_point &p );

float default_daylight_level();

float max_sun_irradiance();





float sun_light_at( const time_point &p );
float moon_light_at( const time_point &p );


float sun_irradiance( const time_point &p );




float sun_moon_light_at( const time_point &p );

float sun_moon_light_at_noon_near( const time_point &p );

std::pair<units::angle, units::angle> sun_azimuth_altitude( time_point );




std::optional<rl_vec2d> sunlight_angle( const time_point & );

enum class weekdays : int {
    SUNDAY = 0,
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
};

weekdays day_of_week( const time_point &p );
std::string to_string( const weekdays &d );

enum class month : int {
    JANUARY = 0,
    FEBRUARY,
    MARCH,
    APRIL,
    MAY,
    JUNE,
    JULY,
    AUGUST,
    SEPTEMBER,
    OCTOBER,
    NOVEMBER,
    DECEMBER,
    UNKNOWN
};

std::pair<month, int> month_and_day( time_point );
std::string to_string( month m );








struct season_effective_time {
    season_effective_time() = default;
    explicit season_effective_time( const time_point & );
    time_point t;
};

#endif
