#pragma once
#ifndef CATA_SRC_CURRENT_MAP_H
#define CATA_SRC_CURRENT_MAP_H

class map;




class current_map
{
        friend class game;
        friend class swap_map;

    protected:
        void set( map *new_map ) {
            current = new_map;
        }

    public:
        map &get_map();
    protected:
        map *current;
};




class swap_map
{
    public:
        explicit swap_map( map &new_map );
        ~swap_map();

    private:
        map *previous;
};

#endif
