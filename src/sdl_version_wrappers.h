#pragma once
#ifndef CATA_SRC_SDL_VERSION_WRAPPERS_H
#define CATA_SRC_SDL_VERSION_WRAPPERS_H





#include <SDL3/SDL_version.h>

struct SDLVersionInfo {
    int major;
    int minor;
    int patch;
};



inline SDLVersionInfo GetCompiledSDLVersion()
{
    const int v = SDL_VERSION;
    return { SDL_VERSIONNUM_MAJOR( v ), SDL_VERSIONNUM_MINOR( v ), SDL_VERSIONNUM_MICRO( v ) };
}



inline SDLVersionInfo GetLinkedSDLVersion()
{
    const int v = SDL_GetVersion();
    return { SDL_VERSIONNUM_MAJOR( v ), SDL_VERSIONNUM_MINOR( v ), SDL_VERSIONNUM_MICRO( v ) };
}

#endif
