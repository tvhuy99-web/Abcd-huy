#pragma once
#ifndef CATA_SRC_HELP_H
#define CATA_SRC_HELP_H

#include <imgui/imgui.h>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cata_imgui.h"
#include "color.h"
#include "input_context.h"
#include "input_enums.h"
#include "options.h"
#include "translation.h"

class JsonObject;

enum message_modifier {
    MM_NORMAL = 0,
    MM_SUBTITLE,
    MM_MONOFONT,
    MM_SEPARATOR
};

struct help_category {
    translation name;
    nc_color color = c_light_blue;
    std::vector<std::pair<translation, message_modifier>> paragraphs;
};

class help_data
{
    public:
        void load( const JsonObject &jo, const std::string &src );
        void reset();
        bool is_read( const int &option ) {
            return _read_categories.find( option ) != _read_categories.end();
        }
        void mark_read( const int &option ) {
            _read_categories.insert( option );
        }
        const help_category &get_help_category( const int &option ) {
            return _help_categories[option];
        }
        const std::map<const int, const help_category> &get_help_categories() {
            return _help_categories;
        }
    private:
        void load_object( const JsonObject &jo, const std::string &src );
        std::map<const int, const help_category> _help_categories;

        std::unordered_set<int> _read_categories;

        int current_order_start = 0;
        std::string current_src;
};

help_data &get_help();

class help_window : public cataimgui::window
{
    public:
        explicit help_window();
        void show();
    protected:
        void draw_controls() override;
        cataimgui::bounds get_bounds() override;
    private:
        help_data &data = get_help();

        input_context ctxt;
        std::map<int, input_event> hotkeys;

        const bool screen_reader = get_option<bool>( "SCREEN_READER_MODE" );


        bool is_space_for_two_cat();
        void draw_category_selection();
        void draw_category_option( const int &option, const help_category &category );

        int previous_option( int option );
        int next_option( int option );
        int selected_option;
        bool has_selected_category = false;
        int loaded_option;
        std::string imgui_action;


        void swap_translated_paragraphs();
        std::vector<std::pair<std::string, message_modifier>> translated_paragraphs;

        void draw_category();
        cataimgui::scroll s;

        void format_title( std::optional<help_category> category = std::nullopt ) const;
        void format_subtitle( const std::string_view &translated_category_name,
                              const nc_color &category_color ) const;
        void format_footer( const std::string &prev, const std::string &next,
                            const nc_color &category_color );
        const float one_em = ImGui::CalcTextSize( "M" ).x;


        float get_wrap_width();
};

#endif
