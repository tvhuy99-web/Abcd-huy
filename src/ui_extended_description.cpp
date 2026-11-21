#include "ui_extended_description.h"

#include "a11y_input.h"
#include "a11y_runtime.h"
#include "a11y_semantics.h"

#include <imgui/imgui.h>

#include "cata_utility.h"
#include "character.h"
#include "color.h"
#include "creature.h"
#include "creature_tracker.h"
#include "map.h"
#include "mapdata.h"
#include "point.h"
#include "string_formatter.h"
#include "text.h"
#include "translations.h"
#include "type_id.h"
#include "ui_manager.h"
#include "viewer.h"
#include "vpart_position.h"

static const trait_id trait_ILLITERATE( "ILLITERATE" );

static description_target &operator++( description_target &c )
{
    c = static_cast<description_target>( static_cast<int>( c ) + 1 );
    if( c == description_target::num_targets ) {
        c = static_cast<description_target>( 0 );
    }
    return c;
}

static description_target &operator--( description_target &c )
{
    if( c == static_cast<description_target>( 0 ) ) {
        c = description_target::num_targets;
    }
    c = static_cast<description_target>( static_cast<int>( c ) - 1 );
    return c;
}

static const Creature *seen_critter( const tripoint_bub_ms &p )
{
    const map &here = get_map();

    const Creature *critter = get_creature_tracker().creature_at( p, true );
    if( critter != nullptr && get_player_view().sees( here, *critter ) ) {
        return critter;
    }

    return nullptr;
}

static void draw_sign_text( const tripoint_bub_ms &p )
{
    std::string signage = get_map().get_signage( p );
    if( !signage.empty() ) {
        ImGui::Separator();
        cataimgui::draw_colored_text( get_player_character().has_trait( trait_ILLITERATE ) ?
                                      // NOLINTNEXTLINE(cata-text-style): the question mark does not end a sentence
                                      _( "Sign: ???" ) : string_format( _( "Sign: %s" ), signage ) );
    }
}

static void draw_graffiti_text( const tripoint_bub_ms &p )
{
    std::string graffiti = get_map().graffiti_at( p );
    if( !graffiti.empty() ) {
        ImGui::Separator();
        cataimgui::draw_colored_text( get_player_character().has_trait( trait_ILLITERATE ) ?
                                      // NOLINTNEXTLINE(cata-text-style): the question mark does not end a sentence
                                      _( "Inscription: ???" ) : string_format( _( "Inscription: %s" ), graffiti ) );
    }
}

void draw_extended_description( const std::vector<std::string> &description, cataimgui::scroll &s )
{
    cataimgui::set_scroll( s );

    for( const std::string &s : description ) {
        if( s == "--" ) {
            ImGui::Separator();
        } else {
            for( const std::string &line : string_split( s, '\n' ) ) {
                cataimgui::TextColoredParagraph( c_light_gray, line );
                ImGui::NewLine();
            }
        }
    }
}

extended_description_window::extended_description_window( tripoint_bub_ms &p ) :
    cataimgui::window( "ext_desc",
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                       ImGuiWindowFlags_NoNavInputs ),
    p( p )
{
    const map &here = get_map();

    ctxt = input_context( "EXTENDED_DESCRIPTION" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "CREATURE" );
    ctxt.register_action( "FURNITURE" );
    ctxt.register_action( "TERRAIN" );
    ctxt.register_action( "VEHICLE" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.set_timeout( 10 );

    const Creature *critter = seen_critter( p );
    if( critter ) {
        creature_description = critter->extended_description();
    }
    bool sees = get_player_character().sees( here, p );
    const furn_id &furn = here.furn( p );
    if( sees && furn ) {
        furniture_description = furn->extended_description();
    }
    const ter_id &ter = here.ter( p );
    if( sees && ter ) {
        terrain_description = ter->extended_description();
    }
    const optional_vpart_position &vp = here.veh_at( p );
    if( sees && vp ) {
        veh_app_description = vp.extended_description();
    }

    if( critter ) {
        switch_target = description_target::creature;
    } else if( here.has_furn( p ) ) {
        switch_target = description_target::furniture;
    } else if( here.veh_at( p ) ) {
        switch_target = description_target::vehicle;
    }


}

void extended_description_window::draw_controls()
{
    if( ImGui::BeginTabBar( "description tabs" ) ) {
        draw_creature_tab();
        draw_furniture_tab();
        draw_terrain_tab();
        draw_vehicle_tab();
        ImGui::EndTabBar();
    }
}

void extended_description_window::draw_creature_tab()
{
    ImGuiTabItemFlags_ flags = ImGuiTabItemFlags_None;
    if( switch_target == description_target::creature ) {
        flags = ImGuiTabItemFlags_SetSelected;
        switch_target = description_target::num_targets;
    }
    std::string title = string_format( _( "[%s] Creature" ), ctxt.get_desc( "CREATURE" ) );
    if( ImGui::BeginTabItem( title.c_str(), nullptr, flags ) ) {
        cur_target = description_target::creature;
        if( !creature_description.empty() ) {
            draw_extended_description( creature_description, info_scroll );
        } else {
            cataimgui::draw_colored_text( _( "You do not see any creature here." ), c_light_gray );
        }
        ImGui::EndTabItem();
    }
}

void extended_description_window::draw_furniture_tab()
{
    ImGuiTabItemFlags_ flags = ImGuiTabItemFlags_None;
    if( switch_target == description_target::furniture ) {
        flags = ImGuiTabItemFlags_SetSelected;
        switch_target = description_target::num_targets;
    }
    std::string title = string_format( _( "[%s] Furniture" ), ctxt.get_desc( "FURNITURE" ) );
    if( ImGui::BeginTabItem( title.c_str(), nullptr, flags ) ) {
        cur_target = description_target::furniture;
        if( !furniture_description.empty() ) {
            draw_extended_description( furniture_description, info_scroll );
        } else {
            cataimgui::draw_colored_text( _( "You do not see any furniture here." ), c_light_gray );
        }
        draw_sign_text( p );
        ImGui::EndTabItem();
    }
}

void extended_description_window::draw_terrain_tab()
{
    ImGuiTabItemFlags_ flags = ImGuiTabItemFlags_None;
    if( switch_target == description_target::terrain ) {
        flags = ImGuiTabItemFlags_SetSelected;
        switch_target = description_target::num_targets;
    }
    std::string title = string_format( _( "[%s] Terrain" ), ctxt.get_desc( "TERRAIN" ) );
    if( ImGui::BeginTabItem( title.c_str(), nullptr, flags ) ) {
        cur_target = description_target::terrain;
        if( !terrain_description.empty() ) {
            draw_extended_description( terrain_description, info_scroll );
        } else {
            cataimgui::draw_colored_text( _( "You can't see the terrain here." ), c_light_gray );
        }
        draw_graffiti_text( p );
        ImGui::EndTabItem();
    }
}

void extended_description_window::draw_vehicle_tab()
{
    ImGuiTabItemFlags_ flags = ImGuiTabItemFlags_None;
    if( switch_target == description_target::vehicle ) {
        flags = ImGuiTabItemFlags_SetSelected;
        switch_target = description_target::num_targets;
    }
    std::string title = string_format( _( "[%s] Vehicle/Appliance" ), ctxt.get_desc( "VEHICLE" ) );
    if( ImGui::BeginTabItem( title.c_str(), nullptr, flags ) ) {
        cur_target = description_target::vehicle;
        if( !veh_app_description.empty() ) {
            draw_extended_description( veh_app_description, info_scroll );
        } else {
            cataimgui::draw_colored_text( _( "You can't see vehicles or appliances here." ), c_light_gray );
        }
        ImGui::EndTabItem();
    }
}

cataimgui::bounds extended_description_window::get_bounds()
{
    return bounds;
}

void extended_description_window::show()
{
    a11y::screen_scope reader( &ctxt, "extended-description",
    [this]( const a11y::semantic_intent & intent ) -> std::optional<std::string> {
        static constexpr description_target targets[4] = {
            description_target::creature, description_target::furniture,
            description_target::terrain, description_target::vehicle
        };
        for( int i = 0; i < 4; ++i ) {
            if( intent.node == "ext:tab:" + std::to_string( i ) ) {
                if( intent.action == "focus" || intent.action == "activate" ) {
                    switch_target = targets[i];
                    cur_target = targets[i];
                    return std::string( "TIMEOUT" );
                }
                return std::nullopt;
            }
        }
        return a11y::registered_intent( ctxt, intent );
    } );
    while( true ) {
        if( a11y::enabled() ) {
            a11y::semantic_snapshot snapshot = a11y::command_snapshot(
                                                   ctxt, _( "Extended description" ) );
            auto join_lines = []( const std::vector<std::string> &lines, const char *fallback ) {
                if( lines.empty() ) {
                    return std::string( fallback );
                }
                std::string out;
                for( const std::string &line : lines ) {
                    if( line != "--" ) {
                        out += remove_color_tags( line ) + "\n";
                    }
                }
                return out;
            };
            struct ext_tab {
                description_target target;
                const char *title;
                std::string body;
            };
            std::string terrain_body = join_lines( terrain_description,
                                                   _( "You can't see the terrain here." ) );
            const std::string signage = get_map().get_signage( p );
            if( !signage.empty() ) {
                terrain_body += "\n" + ( get_player_character().has_trait( trait_ILLITERATE ) ?
                                          std::string( _( "Sign: ???" ) ) :
                                          string_format( _( "Sign: %s" ), signage ) );
            }
            const std::string graffiti = get_map().graffiti_at( p );
            if( !graffiti.empty() ) {
                terrain_body += "\n" + ( get_player_character().has_trait( trait_ILLITERATE ) ?
                                          std::string( _( "Inscription: ???" ) ) :
                                          string_format( _( "Inscription: %s" ), graffiti ) );
            }
            const std::vector<ext_tab> tabs = {
                {
                    description_target::creature, _( "Creature" ),
                    join_lines( creature_description, _( "You can't see any creatures here." ) )
                },
                {
                    description_target::furniture, _( "Furniture" ),
                    join_lines( furniture_description, _( "You can't see any furniture here." ) )
                },
                { description_target::terrain, _( "Terrain" ), std::move( terrain_body ) },
                {
                    description_target::vehicle, _( "Vehicle or appliance" ),
                    join_lines( veh_app_description, _( "You can't see vehicles or appliances here." ) )
                },
            };
            for( std::size_t i = 0; i < tabs.size(); ++i ) {
                a11y::semantic_node node;
                node.id = "ext:tab:" + std::to_string( i );
                node.role = "listitem";
                node.label = tabs[i].title;
                node.detail = tabs[i].body;
                node.actions = { "focus", "activate" };
                node.selected = tabs[i].target == cur_target;
                node.position = static_cast<int>( i ) + 1;
                node.total = static_cast<int>( tabs.size() );
                if( node.selected ) {
                    snapshot.focus = node.id;
                }
                snapshot.nodes.push_back( std::move( node ) );
            }
            reader.update( std::move( snapshot ) );
        }
        ui_manager::redraw_invalidated();
        std::string action = ctxt.handle_input();
        if( action == "NEXT_TAB" ) {
            switch_target = cur_target;
            ++switch_target;
            cur_target = switch_target;
        } else if( action == "PREV_TAB" ) {
            switch_target = cur_target;
            --switch_target;
            cur_target = switch_target;
        }  else if( action == "UP" ) {
            info_scroll = cataimgui::scroll::line_up;
        }  else if( action == "DOWN" ) {
            info_scroll = cataimgui::scroll::line_down;
        }  else if( action == "PAGE_UP" ) {
            info_scroll = cataimgui::scroll::page_up;
        }  else if( action == "PAGE_DOWN" ) {
            info_scroll = cataimgui::scroll::page_down;
        } else if( action == "CREATURE" ) {
            switch_target = description_target::creature;
            cur_target = switch_target;
        } else if( action == "FURNITURE" ) {
            switch_target = description_target::furniture;
            cur_target = switch_target;
        } else if( action == "TERRAIN" ) {
            switch_target = description_target::terrain;
            cur_target = switch_target;
        } else if( action == "VEHICLE" ) {
            switch_target = description_target::vehicle;
            cur_target = switch_target;
        } else if( action == "CONFIRM" || action == "QUIT" ) {
            return;
        }
    }
}
