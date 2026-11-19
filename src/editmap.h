#pragma once
#ifndef CATA_SRC_EDITMAP_H
#define CATA_SRC_EDITMAP_H

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "cata_imgui.h"
#include "color.h"
#include "coordinates.h"
#include "cursesdef.h"
#include "game.h"
#include "input_context.h"
#include "memory_fast.h"
#include "point.h"
#include "translations.h"
#include "type_id.h"

class Creature;
class field;
class input_context;
class map;
class smallmap;
class tinymap;
class ui_adaptor;
class uilist;
class vehicle;

constexpr int SELECTABLE_ACTIONS = 8;

enum editmap_shapetype {
    editmap_point, editmap_rect, editmap_rect_filled, editmap_line, editmap_circle, editmap_shapetype_LAST
};
template<>
struct enum_traits<editmap_shapetype> {
    static constexpr editmap_shapetype last = editmap_shapetype::editmap_shapetype_LAST;
};

enum editmap_action : int {
    EDIT_ITEMS = 0, EDIT_MAPGEN, SELECT_BRUSH, SELECT_TERRAIN, SELECT_FURNITURE, SELECT_TRAP, SELECT_FIELD, SELECT_RADIATION, editmap_action_LAST
};
template<>
struct enum_traits<editmap_action> {
    static constexpr editmap_action last = editmap_action::editmap_action_LAST;
};

enum editmap_mode : int {
    EMM_DRAWING = 0,
    EMM_SELECTING,
    EMM_LAST
};
template<>
struct enum_traits<editmap_mode> {
    static constexpr editmap_mode last = editmap_mode::EMM_LAST;
};

enum omt_mapgen_action : int {
    OMTM_REGENERATE = 0,
    OMTM_ROTATE,
    OMTM_APPLY,
    OMTM_CHANGE_OTER_ID,
    OMTM_TOGGLE_POSTPROCESS
};

class editmap_ui : public cataimgui::window
{
        input_context ictxt;
        class game_draw_callback_t_container
        {
            public:
                explicit game_draw_callback_t_container( editmap_ui *em ) : em( em ) {}
                shared_ptr_fast<game::draw_callback_t> create_or_get();
            private:
                editmap_ui *em;
                weak_ptr_fast<game::draw_callback_t> cbw;
        };

        std::unique_ptr<game_draw_callback_t_container> draw_cb_container_;
        game_draw_callback_t_container &draw_cb_container();

    public:
        editmap_ui();

        void draw_controls() override;
        void handle_action();
        input_context setup_input_context();
        bool get_direction( tripoint_rel_ms &p, const std::string &action,
                            const input_context &ctxt ) const;

        void do_ui_invalidation();
        void draw_main_ui_overlay();

        void draw_current_point_info();
        void draw_select_menu();

        void draw_symbol_and_info( nc_color symbol_color, int symbol,
                                   nc_color draw_color, const std::string_view &info );




        bool move_target( const std::string &action, const input_context &ctxt, bool moveorigin ) const;




        void edit_mapgen() const;
        void edit_items();



        void mapgen_preview( const real_coords &tc, oter_id previewed_omt ) const;




        void mapgen_retarget();



        void cleartmpmap( smallmap &tmpmap ) const;


        void brush_apply();

    protected:
        cataimgui::bounds get_bounds() override;
};




struct editmap_brush {

    tripoint_bub_ms origin = tripoint_bub_ms::invalid;

    tripoint_bub_ms target = tripoint_bub_ms::invalid;

    ter_id selected_terrain;
    furn_id selected_furniture;
    trap_id selected_trap;
    field_type_id selected_field;
    int selected_radiation = 0;

    bool drawing_terrain = false;
    bool drawing_furniture = false;
    bool drawing_trap = false;
    bool drawing_field = false;
    bool drawing_radiation = false;

    int selected_field_intensity = 1;

    editmap_shapetype shape_basic_brush = editmap_shapetype::editmap_point;

    std::vector<tripoint_bub_ms> brush_points; // NOLINT(cata-serialize)


    template <typename T_t>
    std::optional<int_id<T_t>> select_feature();
    std::optional<field_type_id> select_field();
    int select_radiation() const;




    void select_shape();



    void update_brush_points();

    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &jo );
};

#endif
