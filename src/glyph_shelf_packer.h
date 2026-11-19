#pragma once
#ifndef CATA_SRC_GLYPH_SHELF_PACKER_H
#define CATA_SRC_GLYPH_SHELF_PACKER_H

#include <optional>
#include <vector>

#include "point.h"


int next_power_of_two( int value );


class glyph_shelf_packer
{
    public:
        struct slot {
            int page = 0;
            point pos;
        };

        struct placement {
            slot at;

            int page_w = 0;
            int page_h = 0;
            bool opens_page = false;
            int next_current = -1;
            point next;
        };


        glyph_shelf_packer( int page_size, int cell_height, int gap );


        std::optional<placement> plan( int width ) const;

        void commit( const placement &p );

        std::optional<slot> place( int width );
        int page_count() const;
        int page_width( int page ) const;
        int page_height( int page ) const;
        void clear();

    private:
        struct page_dims {
            int w;
            int h;
        };
        int page_size_;
        int cell_height_;
        int gap_;
        std::vector<page_dims> pages_;

        int current_ = -1;

        point cursor_;
};

#endif
