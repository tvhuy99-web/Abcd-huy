#include "imgui_demo.h"

#include <imgui/imgui.h>
#include <string>

#include "cata_imgui.h"
#include "color.h"
#include "input_context.h"
#include "text.h"
#include "translations.h"
#include "ui_manager.h"


imgui_demo_ui::imgui_demo_ui(): cataimgui::window( _( "ImGui Demo Screen" ) )
{
    std::string text =
        "Some long text that will wrap around nicely.  <color_red>Some red text in the middle.</color>  Some long text that will wrap around nicely.";
    stuff = std::make_shared<cataimgui::Paragraph>();
    stuff->append_colored_text( text, c_white );
}

cataimgui::bounds imgui_demo_ui::get_bounds()
{
    return { -1.f, -1.f, ImGui::GetMainViewport()->Size.x, ImGui::GetMainViewport()->Size.y };
}


















































































void imgui_demo_ui::draw_controls()
{
    ImGui::ShowDemoWindow();

}

void imgui_demo_ui::init()
{

    force_to_back = true;
}

void imgui_demo_ui::run()
{
    init();

    input_context ctxt( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    std::string action;

    ui_manager::redraw();

    while( is_open ) {
        ui_manager::redraw();
        action = ctxt.handle_input( 5 );
        if( action == "QUIT" ) {
            break;
        }
    }
}
