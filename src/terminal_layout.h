#pragma once
#ifndef CATA_SRC_TERMINAL_LAYOUT_H
#define CATA_SRC_TERMINAL_LAYOUT_H

#include <optional>
#include <string>
#include <vector>

#include "point.h"


namespace terminal_layout
{


bool is_portrait( const point &window );



int portrait_reserved_bottom( int window_height, int reserve_percent, int shortcut_strip );



const std::vector<point> &text_size_presets();

std::string text_size_id( const point &cell );

std::optional<point> parse_text_size( const std::string &id );

point custom_text_size( int height );

int narrow_side( const point &area );


int sizing_reference( const point &window, const point &outer );

point auto_text_size( int narrow );

point resolve_text_size( const std::string &choice, int custom_height, int narrow );



point fit_grid( const point &bounds, const point &cell, const point &caps );

int bitmap_scale( int narrow, int cell_width );


point present_size( const point &buffer, const point &bounds, int scale );

}

#endif
