#pragma once
#ifndef CATA_SRC_DEBUG_H
#define CATA_SRC_DEBUG_H

#include <cstdlib>
#include <string>
#include <unordered_set>
#include <utility>

#include "string_formatter.h"









































#include <functional>


#include <iostream>
#include <type_traits>

#define STRING2(x) #x
#define STRING(x) STRING2(x)

#if defined(__GNUC__)
#define CATA_FUNCTION_NAME __PRETTY_FUNCTION__
#else
#define CATA_FUNCTION_NAME __func__
#endif







#define debugmsg(...) realDebugmsg(__FILE__, STRING(__LINE__), CATA_FUNCTION_NAME, __VA_ARGS__) // NOLINT(bugprone-lambda-function-name)


void realDebugmsg( const char *filename, const char *line, const char *funcname,
                   const std::string &text );
template<typename ...Args>
inline void realDebugmsg( const char *const filename, const char *const line,
                          const char *const funcname, const char *const mes, Args &&... args )
{
    return realDebugmsg( filename, line, funcname, string_format( mes,
                         std::forward<Args>( args )... ) );
}


#define cata_fatal(...) \
    do { \
        debugmsg(__VA_ARGS__); \
        std::abort(); \
    } while( false )








#if defined(__GNUC__) && __GNUC__ < 9
#define constexpr_fatal(ret, ...) \
    do { return false ? ( ret ) : ( abort(), ( ret ) ); } while(false)
#else
#define constexpr_fatal(ret, ...) \
    do { debugmsg(__VA_ARGS__); abort(); } while(false)
#endif




namespace game_info
{


std::string operating_system();


std::string operating_system_version();


std::string bitness();


std::string game_version();


std::string graphics_version();


std::string mods_loaded();


std::string game_report();
}








enum DebugLevel {
    D_INFO          = 1,
    D_WARNING       = 1 << 2,
    D_ERROR         = 1 << 3,
    D_PEDANTIC_INFO = 1 << 4,

    DL_ALL = ( 1 << 5 ) - 1
};

inline DebugLevel operator|( DebugLevel l, DebugLevel r )
{
    return static_cast<DebugLevel>(
               static_cast<std::underlying_type_t<DebugLevel>>( l ) | r );
}






enum DebugClass {

    D_MAIN    = 1,

    D_MAP     = 1 << 2,

    D_MAP_GEN = 1 << 3,

    D_GAME    = 1 << 4,

    D_NPC     = 1 << 5,

    D_SDL     = 1 << 6,

    D_MMAP    = 1 << 7,

    DC_ALL    = ( 1 << 30 ) - 1
};

enum class DebugOutput : int {
    std_err,
    file,
};


void setupDebug( DebugOutput );

void deinitDebug();








void limitDebugLevel( int );





void limitDebugClass( int );




bool debug_has_error_been_observed();




void debug_reset_error_observed();






std::string capture_debugmsg_during( const std::function<void()> &func );





void replay_buffered_debugmsg_prompts();





std::ostream &DebugLog( DebugLevel, DebugClass );






extern bool debug_mode;

namespace debugmode
{

enum debug_filter : int {
    DF_ACT_BUTCHER = 0,
    DF_ACT_EBOOK,
    DF_ACT_HARVEST,
    DF_ACT_LOCKPICK,
    DF_ACT_READ,
    DF_ACT_SAFECRACKING,
    DF_ACT_SHEARING,
    DF_ACT_WORKOUT,
    DF_ACTIVITY,
    DF_ANATOMY_BP,
    DF_AVATAR,
    DF_BALLISTIC,
    DF_CAMPS,
    DF_CHARACTER,
    DF_CHAR_CALORIES,
    DF_CHAR_HEALTH,
    DF_CRAFTING,
    DF_CREATURE,
    DF_EFFECT,
    DF_EXPLOSION,
    DF_FOOD,
    DF_GAME,
    DF_HIGHWAY,
    DF_IEXAMINE,
    DF_IUSE,
    DF_MAP,
    DF_MATTACK,
    DF_MELEE,
    DF_MONMOVE,
    DF_MONSTER,
    DF_MUTATION,
    DF_NPC,
    DF_NPC_COMBATAI,
    DF_NPC_ITEMAI,
    DF_NPC_MOVEAI,
    DF_NPC_NEEDS,
    DF_OVERMAP,
    DF_RADIO,
    DF_RANGED,
    DF_REQUIREMENTS_MAP,
    DF_SOUND,
    DF_TALKER,
    DF_VEHICLE,
    DF_VEHICLE_DRAG,
    DF_VEHICLE_MOVE,
    DF_WEAKPOINTS,
    DF_WOUNDS,
    DF_MONITOR,
    DF_LAST
};

inline auto format_as( debug_filter df )
{
    return static_cast<std::underlying_type_t<debug_filter>>( df );
}

extern std::unordered_set<debug_filter> enabled_filters;
std::string filter_name( debug_filter value );
}





bool isDebuggerActive();

#if defined(BACKTRACE)



void debug_write_backtrace( std::ostream &out );
#endif


#endif
