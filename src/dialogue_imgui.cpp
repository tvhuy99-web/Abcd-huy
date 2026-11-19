#include "dialogue_imgui.h"

#include <algorithm>
#include <cmath>
#include <imgui/imgui_internal.h>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "a11y_runtime.h"
#include "cata_imgui.h"
#include "debug.h"
#include "dialogue.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "output.h"
#include "string_formatter.h"
#include "talker.h"
#include "text.h"
#include "translation.h"
#include "ui_manager.h"

#if defined(TILES)
#include "cata_tiles.h"
#include "imgui_texture.h"
#endif

struct character_portrait; // IWYU pragma: keep





























static int topic_category( const talk_topic &the_topic )
{
    const std::string &topic = the_topic.id;



    static const std::unordered_set<std::string> topic_1 = { {
            "TALK_MISSION_START", "TALK_MISSION_DESCRIBE", "TALK_MISSION_OFFER",
            "TALK_MISSION_ACCEPTED", "TALK_MISSION_REJECTED", "TALK_MISSION_ADVICE",
            "TALK_MISSION_INQUIRE", "TALK_MISSION_SUCCESS", "TALK_MISSION_SUCCESS_LIE",
            "TALK_MISSION_FAILURE", "TALK_MISSION_REWARD", "TALK_MISSION_END",
            "TALK_MISSION_DESCRIBE_URGENT"
        }
    };
    if( topic_1.count( topic ) > 0 ) {
        return 1;
    }
    static const std::unordered_set<std::string> topic_2 = { {
            "TALK_SHARE_EQUIPMENT", "TALK_GIVE_EQUIPMENT", "TALK_DENY_EQUIPMENT"
        }
    };
    if( topic_2.count( topic ) > 0 ) {
        return 2;
    }
    static const std::unordered_set<std::string> topic_3 = { {
            "TALK_SUGGEST_FOLLOW", "TALK_AGREE_FOLLOW", "TALK_DENY_FOLLOW",
        }
    };
    if( topic_3.count( topic ) > 0 ) {
        return 3;
    }
    static const std::unordered_set<std::string> topic_4 = { {
            "TALK_COMBAT_ENGAGEMENT",
        }
    };
    if( topic_4.count( topic ) > 0 ) {
        return 4;
    }
    static const std::unordered_set<std::string> topic_5 = { {
            "TALK_COMBAT_COMMANDS",
        }
    };
    if( topic_5.count( topic ) > 0 ) {
        return 5;
    }
    static const std::unordered_set<std::string> topic_6 = { {
            "TALK_TRAIN", "TALK_TRAIN_START", "TALK_TRAIN_FORCE",
            "TALK_TRAIN_NPC_START", "TALK_TRAIN_NPC_FORCE"
        }
    };
    if( topic_6.count( topic ) > 0 ) {
        return 6;
    }
    static const std::unordered_set<std::string> topic_7 = { {
            "TALK_MISC_RULES",
        }
    };
    if( topic_7.count( topic ) > 0 ) {
        return 7;
    }
    static const std::unordered_set<std::string> topic_8 = { {
            "TALK_AIM_RULES",
        }
    };
    if( topic_8.count( topic ) > 0 ) {
        return 8;
    }
    static const std::unordered_set<std::string> topic_9 = { {
            "TALK_FRIEND", "TALK_GIVE_ITEM", "TALK_USE_ITEM",
        }
    };
    if( topic_9.count( topic ) > 0 ) {
        return 9;
    }
    return -1;
}

static float border_size()
{
    return 4.0f;
}


static float horizontal_separator_pos_y( const float window_height )
{
    return window_height * 0.6;
}


static void set_cursor_for_center_draw( float width_of_thing_to_draw )
{
    const float avail_width = ImGui::GetContentRegionAvail().x;
    if( width_of_thing_to_draw >= avail_width ) {

        ImGui::SetCursorPosX( 0 );
        return;
    }
    ImGui::SetCursorPosX( ( avail_width / 2.0f ) - ( width_of_thing_to_draw / 2.0f ) );
}

static void print_ASCII_portrait()
{

    set_cursor_for_center_draw( ImGui::CalcTextSize( "|--------------------------|" ).x );

    ImGui::BeginGroup();


    std::string my_beautiful_NPC_ASCII_portait = string_format(
                "|--------------------------|\n"
                "|           _____   -Renech|\n"
                "|          /     \\         |\n"
                "|         | 0  0 |         |\n"
                "|          \\ -  /          |\n"
                "|          |\\__/|          |\n"
                "|      ___/------\\___      |\n"
                "|     / _/        \\_ \\     |\n"
                "|    / / |  NPC   | \\ \\    |\n"
                "|   / /  |        |  \\ \\   |\n"
                "|                          |\n"
                "|--------------------------|"
            );
    cataimgui::draw_colored_text( my_beautiful_NPC_ASCII_portait );
    ImGui::EndGroup();
}

float dialogue_imgui_impl::sidebar_width() const
{
    const int num_characters_line_in_ASCII_portrait = 28;
    const int num_characters_width = num_characters_line_in_ASCII_portrait + 4;

#ifdef TILES
    const float min_width = std::max( portrait_tilecontext->get_tile_width() + border_size() * 4,
                                      num_characters_width * ImGui::CalcTextSize( "0" ).x );
#else
    const float min_width = num_characters_width * ImGui::CalcTextSize( "0" ).x;
#endif
    const float desired_width = window_width * 0.3;
    const float actual_width = std::max( min_width, desired_width );
    return actual_width;
}



float dialogue_imgui_impl::dialogue_window_width() const
{

    const float total_horizontal_padding = border_size() * 5;
    return ImGui::GetWindowSize().x - sidebar_width() - total_horizontal_padding;
}

void dialogue_imgui_impl::draw_controls()
{


    if( hide_ui ) {
        ImGuiWindow *w = ImGui::GetCurrentWindowRead();
        ImGui::SetWindowHiddenAndSkipItemsForCurrentFrame( w );
    }

    ImGui::SetWindowSize( ImVec2( window_width, window_height ), ImGuiCond_Once );
    draw_dialogue_sidebar();
    ImGui::SetNextWindowPos( ImVec2( sidebar_width() + border_size() * 3,
                                     border_size() * 2 ) );
    draw_dialogue_history();
    ImGui::SetNextWindowPos( ImVec2( sidebar_width() + border_size() * 3,
                                     horizontal_separator_pos_y( window_height ) + border_size() * 2 ) );
    draw_dialogue_responses();
}

cataimgui::bounds dialogue_imgui_impl::get_bounds()
{

    ImVec2 viewport = ImGui::GetMainViewport()->WorkSize;
    return { 0,
             0,
             window_width,
             viewport.y
           };
}

void dialogue_imgui::draw_dialogue_imgui( bool is_computer, bool is_not_conversation,
        const std::string &remote_name )
{
    input_context ctxt( "DIALOGUE" );
    dialogue_imgui_impl p_impl( conversation, is_computer, is_not_conversation, remote_name );


    ctxt.register_updown();
    ctxt.register_action( "CONFIRM", to_translation( "Select dialogue option" ) );
    ctxt.register_action( "HOME", to_translation( "Scroll dialogue history down (entire page)" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Scroll dialogue history down (one line)" ) );
    ctxt.register_action( "END", to_translation( "Scroll dialogue history up (entire page)" ) );
    ctxt.register_action( "PAGE_UP", to_translation( "Scroll dialogue history up (one line)" ) );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "DEBUG_DIALOGUE_DL_CONDITIONAL" );
    ctxt.register_action( "DEBUG_DIALOGUE_RESP_CONDITIONAL" );
    ctxt.register_action( "DEBUG_DIALOGUE_DL_EFFECT" );
    ctxt.register_action( "DEBUG_DIALOGUE_RESP_EFFECT" );
    ctxt.register_action( "DEBUG_DIALOGUE_SHOW_ALL_RESPONSE" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "A11Y_INTERNAL_DIALOGUE_FOCUS" );

    ctxt.set_timeout( 10 );


    conversation->actor( true )->ensure_portrait_valid();

    while( !conversation->done ) {
        ui_manager::redraw_invalidated();

        conversation->actor( true )->update_missions( conversation->missions_assigned );
        const talk_topic next = conversation->opt_imgui( p_impl, conversation->topic_stack.back(), ctxt );
        if( next.id == "TALK_NONE" ) {
            int cat = topic_category( conversation->topic_stack.back() );
            do {
                conversation->topic_stack.pop_back();
            } while( cat != -1 && topic_category( conversation->topic_stack.back() ) == cat );
        }
        if( next.id == "TALK_DONE" || conversation->topic_stack.empty() ) {
            const std::string bye =
                dialog_helper::bye_message( conversation->actor( true )->get_npc() );
            conversation->actor( true )->say( bye );







            if( !bye.empty() ) {
                a11y::announce_game_message( bye, false );
            }
            conversation->done = true;
        } else if( next.id != "TALK_NONE" ) {
            conversation->add_topic( next );
        }
    }
}

void dialogue_imgui_impl::draw_dialogue_sidebar() const
{
    ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, border_size() );
    ImGuiChildFlags child_flags = ImGuiChildFlags_Borders;
    ImVec2 child_size = {sidebar_width(), ImGui::GetWindowHeight() - ( border_size() * 2 )};
    if( ImGui::BeginChild( "##DIALOGUE_SIDEBAR", child_size, child_flags ) ) {
#ifdef TILES
        std::optional<character_portrait_id> portrait = conversation->portrait_or_nullopt();
        if( get_option<bool>( "USE_TILES" ) && portrait.has_value() && portrait.value().is_valid() ) {
            if( debug_mode ) {
                cataimgui::draw_colored_text( "Portrait filename: " + portrait.value().str() );
            }
            set_cursor_for_center_draw( portrait_tilecontext->get_tile_width() );

            cataimgui::draw_texture( portrait.value(), tripoint_bub_ms() );
        } else {
            print_ASCII_portrait();
        }
#else
        print_ASCII_portrait();
#endif


        cataimgui::PushGuiFont1_5x();
        set_cursor_for_center_draw( ImGui::CalcTextSize( conversation->speaker_name( *this ).c_str() ).x );
        cataimgui::draw_colored_text( conversation->speaker_name( *this ) );
        cataimgui::PopGuiFont1_5x();


        ImGui::NewLine();
        ImGui::NewLine();

        if( conversation->actor( false )->can_see() ) {
            cataimgui::TextColoredParagraph( c_blue, conversation->actor( true )->short_description() );
        } else {
            cataimgui::TextColoredParagraph( c_blue, string_format( _( "You're blind and can't look at %s." ),
                                             conversation->actor( true )->disp_name() ) );
        }


        ImGui::NewLine();
        ImGui::NewLine();

        cataimgui::TextColoredParagraph( c_red,
                                         conversation->actor( true )->evaluation_by( *conversation->actor( false ) ) );

        ImGui::NewLine();
        ImGui::NewLine();

        cataimgui::TextColoredParagraph( c_pink, conversation->actor( true )->view_personality_traits() );


        ImGui::NewLine();
        ImGui::NewLine();

        cataimgui::TextColoredParagraph( c_yellow, conversation->actor( true )->opinion_text() );

    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void dialogue_imgui_impl::draw_dialogue_history()
{

    ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, border_size() );
    ImGuiChildFlags child_flags = ImGuiChildFlags_Borders;
    ImVec2 child_size = {dialogue_window_width(), ( horizontal_separator_pos_y( window_height ) - border_size() * 2 )};
    if( ImGui::BeginChild( "##DIALOGUE_HISTORY", child_size, child_flags ) ) {
        draw_history();
        set_scroll( scroll_to );
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void dialogue_imgui_impl::draw_dialogue_responses()
{
    ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, border_size() );
    ImGuiChildFlags child_flags = ImGuiChildFlags_Borders;
    ImVec2 child_size = {dialogue_window_width(), ( window_height - horizontal_separator_pos_y( window_height ) - border_size() * 2 )};
    if( ImGui::BeginChild( "##DIALOGUE_RESPONSES", child_size, child_flags ) ) {
        draw_responses();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void dialogue_imgui_impl::add_to_history( const std::string &text, const std::string &speaker_name,
        nc_color speaker_color )
{
    add_to_history( speaker_name, speaker_color );
    add_to_history( text );
}

void dialogue_imgui_impl::add_to_history( const std::string &text )
{
    add_to_history( text, default_color() );
}

void dialogue_imgui_impl::add_to_history( const std::string &text, nc_color color )
{
    history.emplace_back( color, text );
}

std::string dialogue_imgui_impl::get_accessibility_history() const
{
    std::string result;









    bool pending_break = false;
    for( const history_message &msg : history ) {
        const std::string text = remove_color_tags( msg.text );
        if( text.empty() ) {
            if( !result.empty() ) {
                pending_break = true;
            }
            continue;
        }
        if( !result.empty() ) {
            result += pending_break ? "\n\n" : "\n";
        }
        result += text;
        pending_break = false;
    }
    return result;
}

void dialogue_imgui_impl::draw_history() const
{
    for( const history_message &msg : history ) {
        cataimgui::TextColoredParagraph( msg.color, msg.text );
        ImGui::NewLine();
    }
}

nc_color dialogue_imgui_impl::default_color() const
{
    return is_computer ? c_green : c_white;
}

void dialogue_imgui_impl::set_responses( const std::vector<talk_data> &responses )
{
    response_list = responses;
}

void dialogue_imgui_impl::draw_responses()
{

    if( debug_mode ) {
        cataimgui::draw_colored_text( "talk_topic: " + debug_topic_name );
    }


    static int prev_response = 0;


    ImGui::SetNavCursorVisible( false );



    for( int i = 0; i < static_cast<int>( response_list.size() ); i++ ) {
        const talk_data &talk = response_list[i];


        if( sel_response == i ) {
            cataimgui::draw_colored_text( ">>>>>", c_yellow );
            if( prev_response != sel_response ) {

                ImGui::SetKeyboardFocusHere();
                prev_response = sel_response;
            }
        } else {

            cataimgui::draw_colored_text( "     " );
        }

        ImGui::SameLine();


        cataimgui::draw_colored_text( formatted_hotkey( talk.hotkey_desc, talk.color ) );

        ImGui::SameLine();


        const bool should_color_button = talk.color != c_white;
        if( should_color_button ) {
            ImGui::PushStyleColor( ImGuiCol_Button, talk.color );
        }


        float total_allowed_width = ImGui::GetWindowWidth() - ImGui::CalcTextSize( ">>>>>   a:   " ).x;
        const int num_expected_lines = std::ceil( ImGui::CalcTextSize( talk.text.c_str() ).x /
                                       total_allowed_width );

        ImVec2 button_size = { std::min( total_allowed_width, ImGui::CalcTextSize( talk.text.c_str() ).x ),
                               ( ImGui::GetTextLineHeight() * num_expected_lines )
                             };



        button_size += ( ImGui::GetStyle().FramePadding * 2 );

        const std::string button_name = "##" + talk.text;
        ImVec2 stored_cursor = ImGui::GetCursorPos();
        if( ImGui::Button( button_name.c_str(), button_size ) ) {
            sel_response = i;

            user_clicked_response_button = true;
        }

        ImGui::SetCursorPos( stored_cursor + ImGui::GetStyle().FramePadding );

        ImGui::BeginGroup();

        cataimgui::TextColoredParagraph( c_white, talk.text, std::nullopt,
                                         ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x );
        ImGui::EndGroup();

        if( should_color_button ) {
            ImGui::PopStyleColor();
        }
        ImGui::NewLine();
    }


    if( debug_mode ) {
        for( const std::string &dbg_info : responses_debug ) {
            cataimgui::TextColoredParagraph( c_yellow, dbg_info );
            ImGui::NewLine();
        }
    }
}

void dialogue_imgui_impl::set_responses_debug( const std::vector<std::string> &responses )
{
    responses_debug = responses;
}
