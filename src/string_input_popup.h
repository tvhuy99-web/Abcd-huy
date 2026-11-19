#pragma once
#ifndef CATA_SRC_STRING_INPUT_POPUP_H
#define CATA_SRC_STRING_INPUT_POPUP_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "color.h"
#include "coordinates.h"
#include "cursesdef.h"
#include "dialogue_helpers.h"

class JsonObject;
class input_context;
class scrolling_text_view;
class translation;
class ui_adaptor;
class utf8_wrapper;
struct point;



























class string_input_popup // NOLINT(cata-xy)
{
    private:
        std::string _title;
        std::string _text;
        std::string _description;
        std::string _identifier;
        std::string _session_str_entered;
        nc_color _title_color = c_light_red;
        nc_color _desc_color = c_green;
        nc_color _string_color = c_magenta;
        nc_color _cursor_color = h_light_gray;
        nc_color _underscore_color = c_light_gray;
        int _width = 0;
        int _max_length = -1;
        bool _only_digits = false;
        bool _hist_use_uilist = true;
        int _startx = 0;
        int _starty = 0;
        int _endx = 0;
        int _position = -1;

        int shift = 0;
        int _hist_str_ind = 0;

        const size_t _hist_max_size = 100;


        int w_width = 0;
        int w_height = 0;
        int title_height = 0;
        int description_height = 0;
        std::vector<std::string> title_split;
        int titlesize = 0;

        bool custom_window = false;
        catacurses::window w_full;
        catacurses::window w_description;
        catacurses::window w_title_and_entry;
        std::unique_ptr<scrolling_text_view> desc_view_ptr;

        std::unique_ptr<input_context> ctxt_ptr;
        input_context *ctxt = nullptr;
        std::vector<std::tuple<std::string, int, std::function<bool()>>> callbacks;

        bool _canceled = false;
        bool _confirmed = false;
        bool _handled = false;
        bool _text_changed = false;

        void create_window();
        void create_context();

        void show_history( utf8_wrapper &ret );
        void add_to_history( const std::string &value ) const;
        void update_input_history( utf8_wrapper &ret, bool up );
        void draw( ui_adaptor *ui, const utf8_wrapper &ret, const utf8_wrapper &edit ) const;

    public:
        string_input_popup();
        ~string_input_popup();




        string_input_popup &title( const std::string &value ) {
            _title = value;
            return *this;
        }






        string_input_popup &text( const std::string &value );
        const std::string &text() const {
            return _text;
        }





        string_input_popup &description( const std::string &value ) {
            _description = value;
            return *this;
        }








        string_input_popup &identifier( const std::string &value ) {
            _identifier = value;
            return *this;
        }



        string_input_popup &width( int value ) {
            _width = value;
            return *this;
        }




        string_input_popup &max_length( int value ) {
            _max_length = value;
            return *this;
        }



        string_input_popup &only_digits( bool value ) {
            _only_digits = value;
            return *this;
        }





        string_input_popup &hist_use_uilist( bool value ) {
            _hist_use_uilist = value;
            return *this;
        }










        string_input_popup &window( const catacurses::window &w, const point &start, int endx );





        string_input_popup &context( input_context &ctxt );
        input_context &context() const {
            return *ctxt;
        }




        string_input_popup &title_color( const nc_color &color ) {
            _title_color = color;
            return *this;
        }




        string_input_popup &desc_color( const nc_color &color ) {
            _desc_color = color;
            return *this;
        }




        string_input_popup &string_color( const nc_color &color ) {
            _string_color = color;
            return *this;
        }




        string_input_popup &cursor_color( const nc_color &color ) {
            _cursor_color = color;
            return *this;
        }




        string_input_popup &underscore_color( const nc_color &color ) {
            _underscore_color = color;
            return *this;
        }






        void query( bool loop = true, bool draw_only = false );
        std::optional<int> query_int( bool loop = true, bool draw_only = false );
        std::optional<int64_t> query_int64_t( bool loop = true, bool draw_only = false );
        const std::string &query_string( bool loop = true, bool draw_only = false );
        std::optional<tripoint_abs_omt> query_coordinate_abs_impl( bool loop = true,
                bool draw_only = false );






        bool canceled() const {
            return _canceled;
        }



        bool confirmed() const {
            return _confirmed;
        }
        void confirm() {
            _confirmed = true;
        }




        bool handled() const {
            return _handled;
        }







        void edit( std::string &value );

        // NOLINTNEXTLINE(cata-no-long)
        void edit( long &value );
        void edit( int &value );


        void add_callback( const std::string &action, const std::function<bool()> &callback_func );
        void add_callback( int input, const std::function<bool()> &callback_func );


        std::vector<std::pair<std::string, translation>> custom_actions;
};

struct string_input_params {
    std::optional<translation_or_var> title;
    std::optional<translation_or_var> description;
    std::optional<translation_or_var> default_text;
    std::optional<str_or_var> identifier;

    int width = 40;

    static string_input_params parse_string_input_params( const JsonObject &jo );
};
#endif
