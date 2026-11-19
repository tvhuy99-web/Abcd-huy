#pragma once
#ifndef CATA_SRC_SDLSOUND_H
#define CATA_SRC_SDLSOUND_H

#include <string>
#include <string_view>
#if defined(SDL_SOUND)




bool init_sound();
void initSDLAudioOnly();
void shutdown_sound();
void play_music( std::string_view playlist );
void stop_music();
void update_music_volume();
void load_soundset();
extern bool sound_init_success;

#else

inline bool init_sound()
{
    return false;
}
inline void shutdown_sound() { }
inline void play_music( std::string_view )
{
}
inline void update_music_volume() { }
inline void load_soundset() { }

#endif

#endif
