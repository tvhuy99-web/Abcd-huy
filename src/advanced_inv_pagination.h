#pragma once
#ifndef CATA_SRC_ADVANCED_INV_PAGINATION_H
#define CATA_SRC_ADVANCED_INV_PAGINATION_H

class advanced_inventory_pane;
class item_category;




class advanced_inventory_pagination
{
    private:
        const int linesPerPage;
        const advanced_inventory_pane &pane;
        int lines_needed( int index );
    public:
        int line;
        int page;
        const item_category *last_category;
        advanced_inventory_pagination( int linesPerPage, const advanced_inventory_pane &pane )
            : linesPerPage( linesPerPage ), pane( pane ), line( 0 ), page( 0 ), last_category( nullptr ) { }


        void reset_page();


        bool new_category( const item_category *cat ) const;



        bool step( int index );
};
#endif
