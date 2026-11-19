#pragma once
#ifndef CATA_SRC_WEATHER_TYPE_H
#define CATA_SRC_WEATHER_TYPE_H

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "calendar.h"
#include "catacharset.h"
#include "color.h"
#include "field_type.h"
#include "lightmap.h"
#include "translation.h"
#include "type_id.h"
#include "units.h"

class JsonObject;
struct const_dialogue;
template <typename E> struct enum_traits;
template<typename T>
class generic_factory;

const weather_type_id WEATHER_NULL( "null" );
const weather_type_id WEATHER_CLEAR( "clear" );

enum class precip_class : int {
    none,
    very_light,
    light,
    heavy,
    last
};
template<>
struct enum_traits<precip_class> {
    static constexpr precip_class last = precip_class::last;
};

enum weather_sound_category : int {
    silent,
    drizzle,
    rainy,
    rainstorm,
    thunder,
    flurries,
    snowstorm,
    snow,
    portal_storm,
    clear,
    sunny,
    cloudy,
    last
};

template<>
struct enum_traits<weather_sound_category> {
    static constexpr weather_sound_category last = weather_sound_category::last;
};




struct weather_animation_t {
    float factor = 0.0f;
    nc_color color = c_white;
    uint32_t symbol = NULL_UNICODE;
    std::string get_symbol() const {
        return utf32_to_utf8( symbol );
    }

    void deserialize( const JsonObject &jo );
};

struct weather_type {
    public:
        friend class generic_factory<weather_type>;
        bool was_loaded = false;
        weather_type_id id;
        std::vector<std::pair<weather_type_id, mod_id>> src;

        translation name;

        nc_color color = c_white;

        nc_color map_color = c_white;

        uint32_t symbol = PERCENT_SIGN_UNICODE;

        uint32_t sun_symbol = NULL_UNICODE;

        int ranged_penalty = 0;

        float sight_penalty = 0.0f;

        int light_modifier = 0;

        units::temperature_delta temperature_modifier = 0_K_delta;

        float light_multiplier = 1.f;

        float sun_multiplier = 1.f;

        int sound_attn = 0;

        bool dangerous = false;

        precip_class precip = precip_class::none;

        bool rains = false;

        std::string tiles_animation;

        weather_animation_t weather_animation;

        weather_sound_category sound_category = weather_sound_category::silent;

        int priority = 0;

        std::function<bool( const_dialogue const & )> condition;
        std::vector<weather_type_id> required_weathers;
        time_duration duration_min = 0_turns;
        time_duration duration_max = 0_turns;
        std::optional<std::string> debug_cause_eoc;
        std::optional<std::string> debug_leave_eoc;


        std::vector<field_effect> passive_effect;


        light_color_rgb tint_color{};
        float tint_strength = 0.0f;
        void load( const JsonObject &jo, std::string_view src );
        void finalize();
        void check() const;
        std::string get_symbol() const {
            return utf32_to_utf8( symbol );
        }
        std::string get_sun_symbol() const {
            return utf32_to_utf8( sun_symbol );
        }
        weather_type() = default;
};
namespace weather_types
{

const std::vector<weather_type> &get_all();

void finalize_all();

void reset();

void load( const JsonObject &jo, const std::string &src );

void check_consistency();
}
#endif
