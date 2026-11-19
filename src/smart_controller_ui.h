#pragma once
#ifndef CATA_SRC_SMART_CONTROLLER_UI_H
#define CATA_SRC_SMART_CONTROLLER_UI_H

#include "cursesdef.h"
#include "input_context.h"





struct smart_controller_settings {
    bool &enabled;
    int &battery_lo;
    int &battery_hi;

    smart_controller_settings( bool &enabled, int &battery_lo, int &battery_hi );
};

enum smart_controller_ui_selection {
    enabled,
    lo_and_hi_slider,
    manual,


    length,

    init = enabled
};


class smart_controller_ui
{
    public:
        static const int WIDTH = 52;
        static const int HEIGHT = 36;

        explicit smart_controller_ui( smart_controller_settings initial_settings );


        void control();

    private:

        static const int LEFT_MARGIN = 6;

        static const int MENU_ITEM_HEIGHT = 5;
        static const int MENU_ITEMS_N = smart_controller_ui_selection::length;

        static const int SLIDER_W = 40;


        catacurses::window win;
        input_context ctxt;

        const smart_controller_settings settings;


        int selection = smart_controller_ui_selection::init;

        int slider = 0;

        void refresh();
};

#endif
