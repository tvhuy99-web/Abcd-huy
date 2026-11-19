#pragma once
#ifndef CATA_SRC_MAP_SCALE_CONSTANTS_H
#define CATA_SRC_MAP_SCALE_CONSTANTS_H







constexpr int MAPSIZE = 11;
constexpr int HALF_MAPSIZE = static_cast<int>( MAPSIZE / 2 );



constexpr int SEEX = 12;
constexpr int SEEY = SEEX;

constexpr int MAPSIZE_X = SEEX * MAPSIZE;
constexpr int MAPSIZE_Y = SEEY * MAPSIZE;

constexpr int HALF_MAPSIZE_X = SEEX * HALF_MAPSIZE;
constexpr int HALF_MAPSIZE_Y = SEEY * HALF_MAPSIZE;



constexpr int MAX_VIEW_DISTANCE = SEEX * HALF_MAPSIZE;


constexpr int RANGE_HARD_CAP = MAX_VIEW_DISTANCE;






constexpr int OMAPX = 180;
constexpr int OMAPY = OMAPX;


constexpr int SEG_SIZE = 32;


constexpr int MM_REG_SIZE = 8;


constexpr int OVERMAP_DEPTH = 10;

constexpr int OVERMAP_HEIGHT = 10;

constexpr int OVERMAP_LAYERS = 1 + OVERMAP_DEPTH + OVERMAP_HEIGHT;

#endif
