#include "help.h"

#include <charconv>
#include <cmath>
#include <iterator>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "a11y_document.h"
#include "a11y_input.h"
#include "a11y_runtime.h"
#include "cata_assert.h"
#include "cata_imgui.h"
#include "cata_path.h"
#include "cata_utility.h"
#include "character.h"
#include "color.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "input_context.h"
#include "input_enums.h"
#include "npc.h"
#include "output.h"
#include "path_info.h"
#include "string_formatter.h"
#include "text.h"
#include "translations.h"
#include "ui_manager.h"

help_data &get_help()
{
    static help_data single_instance;
    return single_instance;
}

void help_data::reset()
{
    current_order_start = 0;
    current_src = "";
    _help_categories.clear();
    _read_categories.clear();
}

void help_data::load( const JsonObject &jo, const std::string &src )
{
    if( src == "dda" ) {
        jo.throw_error( string_format( "Vanilla help must be located in %s",
                                       PATH_INFO::jsondir().generic_u8string() ) );
    }
    if( src != current_src ) {
        current_order_start = _help_categories.empty() ? 0 : _help_categories.crbegin()->first + 1;
        current_src = src;
    }

    help_category category;

    translation name;
    jo.read( "name", category.name );
    if( jo.has_string( "color" ) ) {
        const std::string color_string = jo.get_string( "color" );
        category.color = color_from_string( color_string, report_color_error::no );
        if( category.color == c_unset ) {
            jo.throw_error_at( "color", "Can't parse color" );
            category.color = c_light_blue;
        }
    }

    for( JsonValue jv : jo.get_array( "messages" ) ) {
        if( jv.test_string() ) {
            category.paragraphs.emplace_back( to_translation( jv.get_string() ), MM_NORMAL );
        } else {
            JsonObject jobj = jv.get_object();
            // Subobject handling for translator/style comments
            if( jobj.has_string( "subtitle" ) || jobj.has_object( "subtitle" ) ) {
                translation subtitle;
                mandatory( jobj, false, "subtitle", subtitle );
                category.paragraphs.emplace_back( subtitle, MM_SUBTITLE );
            } else if( jobj.has_string( "force_monospaced" ) || jobj.has_object( "force_monospaced" ) ) {
                translation force_monospaced;
                mandatory( jobj, false, "force_monospaced", force_monospaced );
                category.paragraphs.emplace_back( force_monospaced, MM_MONOFONT );
            } else if( jobj.has_string( "separator" ) ) {
                auto it = category.paragraphs.emplace_back( no_translation( jobj.get_string( "separator" ) ),
                          MM_SEPARATOR );
                // Verify color now bc later the debugmsg would cause an ImGui CTD, don't use color_from_string's debugmsg though bc that will be useless context wise
                if( color_from_string( it.first.translated(), report_color_error::no ) == c_unset ) {
                    jobj.throw_error_at( "separator", "Can't parse color" );
                }
            } else if( jobj.has_object( "paragraph" ) ) {
                translation paragraph;
                mandatory( jobj, false, "paragraph", paragraph );
                category.paragraphs.emplace_back( paragraph, MM_NORMAL );
            }
        }
    }


    const int modified_order = jo.get_int( "order" ) + current_order_start;
    if( !_help_categories.try_emplace( modified_order, category ).second ) {
        jo.throw_error_at( "order", "\"order\" must be unique per source" );
    }
}

bool help_window::is_space_for_two_cat()
{
    static std::optional<float> two_column_min_width;
    if( !two_column_min_width ) {
        const std::map<const int, const help_category> &cats = data.get_help_categories();
        float longest = 0;
        for( const auto &cat : cats ) {
            float cat_x = ImGui::CalcTextSize( cat.second.name.translated().c_str() ).x;
            if( longest < cat_x ) {
                longest = cat_x;
            }
        }
        two_column_min_width = ( 2 * longest ) + 20.0f; // Guarentee some padding
    }
    return ImGui::GetContentRegionAvail().x > *two_column_min_width;
}

help_window::help_window() : cataimgui::window( "help",
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavFocus )
{
    ctxt = input_context( "DISPLAY_HELP", keyboard_mode::keychar );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    // Mouse selection for ImTui ( ImGui mouse selection is handled in draw_controls() )
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );
    // Generated shortcuts
    ctxt.register_action( "ANY_INPUT" );
    // Switching between category options and scrolling open category
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "HOME" );
    ctxt.register_action( "END" );
    ctxt.register_updown();
    // Switching between category options and open categories
    ctxt.register_leftright();
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    const hotkey_queue &hkq = hotkey_queue::alphabets();
    input_event next_hotkey = ctxt.first_unassigned_hotkey( hkq );
    for( const std::pair<const int, const help_category> &option_category :
         data.get_help_categories() ) {
        hotkeys.emplace( option_category.first, next_hotkey );
        next_hotkey = ctxt.next_unassigned_hotkey( hkq, next_hotkey );
    }
}

void help_window::draw_controls()
{

#if defined(TILES)
    // Retrieve captured mouse clicks
    if( ImGui::IsMouseReleased( ImGuiMouseButton_Left ) ) {
        imgui_action = "SELECT";
    } else if( ImGui::IsMouseReleased( ImGuiMouseButton_Right ) ) {
        imgui_action = "QUIT";
    }
#endif // TILES

    if( !has_selected_category ) {
        draw_category_selection();
    } else {
        draw_category();
    }
}

void help_window::draw_category_selection()
{
    selected_option = -1;
    const std::map<const int, const help_category> &cats = data.get_help_categories();
    if( !is_space_for_two_cat() ) {
        format_title();
        if( ImGui::BeginChild( "Category Options" ) ) {
            for( const auto &cat : cats ) {
                draw_category_option( cat.first, cat.second );
            }
            ImGui::EndChild();
        }
    } else {
        format_title();
        // Split the categories in half
        if( ImGui::BeginTable( "Category Options", 2, ImGuiTableFlags_None ) ) {
            ImGui::TableSetupColumn( "Left Column" );
            ImGui::TableSetupColumn( "Right Column" );
            int half_size = cats.size() / 2;
            if( cats.size() % 2 != 0 ) {
                half_size++;
            }
            auto half_it = cats.begin();
            std::advance( half_it, half_size );
            auto jt = cats.begin();
            std::advance( jt, half_size - 1 );
            for( auto it = cats.begin(); it != half_it; it++ ) {
                ImGui::TableNextColumn();
                draw_category_option( it->first, it->second );
                ImGui::TableNextColumn();
                if( jt != std::prev( cats.end() ) ) {
                    jt++;
                    draw_category_option( jt->first, jt->second );
                }
            }
            ImGui::EndTable();
        }
    }
}

void help_window::draw_category_option( const int &option, const help_category &category )
{
    std::string cat_name;
    auto hotkey_it = hotkeys.find( option );
    if( hotkey_it != hotkeys.end() ) {
        cat_name = hotkey_it->second.short_description();
        cat_name += ": ";
    }
    cat_name += category.name.translated();
    if( data.is_read( option ) ) {
        if( screen_reader ) {
            //~ Prefix for options that has already been viewed when using a screen reader
            cat_name = _( "(read) " ) + cat_name;
        }
        ImGui::PushStyleColor( ImGuiCol_Text, c_light_gray );
        ImGui::Selectable( remove_color_tags( cat_name ).c_str() );
        ImGui::PopStyleColor();
    } else {
        ImGui::Selectable( remove_color_tags( cat_name ).c_str() );
    }
    if( ImGui::IsItemHovered() ) {
        selected_option = option;
    }
}

void help_window::draw_category()
{
    const help_category &cat = data.get_help_category( loaded_option );
    const std::string prev_cat_name =
        data.get_help_category( previous_option( loaded_option ) ).name.translated();
    const std::string next_cat_name =
        data.get_help_category( next_option( loaded_option ) ).name.translated();
    const bool scroll_title = ImGui::GetContentRegionAvail().y < 500.0f;
    const bool show_footer = is_space_for_two_cat();

    int child_rows = TERMY;
    child_rows -= scroll_title ? 0 : 5;
    child_rows -= show_footer ? 5 : 0;

    if( !scroll_title ) {
        format_title( cat );
    }

    ImGui::Indent( one_em );
    if( ImGui::BeginChild( "HELP_PARAGRAPHS",
                           ImVec2( -1, static_cast<float>( str_height_to_pixels( child_rows ) ) ) ) ) {

        cataimgui::set_scroll( s );
        if( scroll_title ) {
            cataimgui::TextColoredParagraph( c_white, cat.name.translated() );
            ImGui::NewLine();
            ImGui::NewLine();
        }
        for( const std::pair<std::string, message_modifier> &translated_paragraph :
             translated_paragraphs ) {
            switch( translated_paragraph.second ) {
                case MM_NORMAL:
                    cataimgui::draw_colored_text( translated_paragraph.first, c_white, get_wrap_width() );
                    break;
                case MM_SUBTITLE:
                    format_subtitle( translated_paragraph.first, cat.color );
                    break;
                case MM_MONOFONT:
                    cataimgui::PushMonoFont();
                    //TODO: Try messing with ImGuiStyleVar_ItemSpacing etc again to remove the y spacing
                    cataimgui::draw_colored_text( translated_paragraph.first, c_white, get_wrap_width() );
                    ImGui::PopFont();
                    break;
                case MM_SEPARATOR: {
                    // Can't verify here bc the debugmsg would cause an ImGui CTD
                    nc_color separator_color = color_from_string( translated_paragraph.first, report_color_error::no );
                    if( separator_color == c_unset ) {
                        separator_color = cat.color;
                    }
                    ImGui::PushStyleColor( ImGuiCol_Separator, separator_color );
                    ImGui::Separator();
                    ImGui::PopStyleColor();
                    break;
                }
                default:
                    debugmsg( "Unexpected help message modifier" );
                    continue;
            }
            ImGui::NewLine();
            ImGui::NewLine();
        }
        ImGui::EndChild();
    }
    if( show_footer ) {
        format_footer( prev_cat_name, next_cat_name, cat.color );
    }
}

cataimgui::bounds help_window::get_bounds()
{
    return {0, 0, 1.0, 1.0};
}

float help_window::get_wrap_width()
{
    return static_cast<float>( str_width_to_pixels( TERMX - 6 ) );
}

void help_window::show()
{
    const std::map<const int, const help_category> &cats = data.get_help_categories();

    // Reader adapter (R19.2). Structure of the original screen, verified
    // against this file and data/core/help.json (+ mod help files):
    // - Topics ARE the "categories": help_data loads a flat, order-keyed
    //   map of help_category (name + paragraphs) from JSON type "help".
    //   There is no grouping level above them, no search field and no
    //   links inside a topic: paragraphs carry only color tags and
    //   monospaced diagrams. Navigation is exactly the two loops below:
    //   the selection loop (per-topic alphabet hotkeys assigned in the
    //   constructor, SELECT/CONFIRM opens, QUIT leaves the window) and the
    //   topic loop (UP/DOWN/HOME/END/PAGE scroll, LEFT/PREV_TAB and
    //   RIGHT/NEXT_TAB jump to the wrap-around neighbour topic,
    //   CONFIRM/QUIT returns to the selection loop, and a footer
    //   SELECT/CONFIRM jumps to the hovered neighbour). Opening a topic
    //   only marks it read in help_data (per-session set) and swaps the
    //   translated paragraphs; it never costs a turn or touches the game.
    //
    // Audit of the pre-existing screen_reader mode (the SCREEN_READER_MODE
    // option, read once into the screen_reader member): it changes ONLY
    // how this window draws, for an EXTERNAL screen reader scraping the
    // curses/ImTui text (the option text names orca on curses). Its four
    // effects here are: read topics get a "(read) " prefix in
    // draw_category_option() (otherwise read state is color-only), and
    // format_title()/format_subtitle()/format_footer() drop their
    // box-drawing art, the footer becoming the sentence "Previous
    // category: %s. Next category: %s". It publishes nothing, has no
    // navigation model and cannot expose topic text as a document, so it
    // is NOT equivalent to this adapter and the adapter is not gated on
    // it. It is left fully in place: the two do not conflict (the adapter
    // builds nodes from the same help_data/translated_paragraphs the draw
    // code uses, never from drawn text), and where they overlap the
    // adapter carries the same facts structurally -- read state as the
    // topic row's status cell instead of the "(read) " prefix, and the
    // prev/next names in the topic summary in the footer's own words.
    //
    // Because the original UI has no search, the topic filter below is a
    // reader-only presentation filter (the same lcmatch the keybinding
    // menu's filter uses): it only changes which rows are published, it
    // never alters help_data, and its node detail says so. Focus on any
    // node is observation-only; "activate" on a topic row only sets
    // selected_option and returns CONFIRM so the selection loop itself
    // opens the topic (swap_translated_paragraphs + mark_read included),
    // the neighbour/back nodes return the topic loop's own LEFT/RIGHT/
    // QUIT actions, and returning lands on the topic row last opened.
    std::string reader_filter;
    std::optional<std::string> reader_filter_draft;
    int reader_last_topic = -1;
    const auto reader_handle_intent = [&]( const a11y::semantic_intent &intent )
    -> std::optional<std::string> {
        if( intent.node == "help:filter" ) {
            if( intent.action == "focus" ) {
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "set_value" ) {
                reader_filter_draft = intent.value;
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "apply" ) {
                if( reader_filter_draft ) {
                    reader_filter = *reader_filter_draft;
                    reader_filter_draft.reset();
                }
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "cancel_draft" ) {
                reader_filter_draft.reset();
                return std::string( "TIMEOUT" );
            }
            return std::nullopt;
        }
        if( intent.node == "help:no_results" || intent.node == "help:document" ) {
            // Reading a document is Java-side (ReaderDocumentReader);
            // neither node carries a game-side effect.
            if( intent.action == "focus" || intent.action == "activate" ) {
                return std::string( "TIMEOUT" );
            }
            return std::nullopt;
        }
        if( intent.node == "help:nav:prev" || intent.node == "help:nav:next" ||
            intent.node == "help:nav:back" ) {
            if( intent.action == "focus" ) {
                return std::string( "TIMEOUT" );
            }
            if( intent.action == "activate" ) {
                // The topic loop's own actions: LEFT/RIGHT move between
                // neighbour topics (marking them read), QUIT returns to
                // the topic list, exactly like the footer controls.
                if( intent.node == "help:nav:prev" ) {
                    return std::string( "LEFT" );
                }
                if( intent.node == "help:nav:next" ) {
                    return std::string( "RIGHT" );
                }
                return std::string( "QUIT" );
            }
            return std::nullopt;
        }
        static const std::string topic_prefix = "help:topic:";
        if( intent.node.rfind( topic_prefix, 0 ) == 0 &&
            ( intent.action == "focus" || intent.action == "activate" ) ) {
            const std::string suffix = intent.node.substr( topic_prefix.size() );
            int option = -1;
            const char *const begin = suffix.data();
            const char *const end = begin + suffix.size();
            const std::from_chars_result parsed = std::from_chars( begin, end, option );
            if( parsed.ec != std::errc() || parsed.ptr != end ||
                cats.find( option ) == cats.end() ) {
                return std::nullopt;
            }
            if( intent.action == "focus" ) {
                // Browsing must not arm the selection loop's CONFIRM
                // branch, so selected_option is left untouched.
                return std::string( "TIMEOUT" );
            }
            selected_option = option;
            return std::string( "CONFIRM" );
        }
        return a11y::registered_intent( ctxt, intent );
    };
    const auto make_reader_snapshot = [&]() {
        a11y::semantic_snapshot snapshot;
        a11y::semantic_snapshot commands = a11y::command_snapshot( ctxt, _( "Help" ) );
        std::vector<a11y::semantic_node> command_nodes = std::move( commands.nodes );
        if( !has_selected_category ) {
            snapshot.title = _( "Help" );
            a11y::semantic_node filter_node;
            filter_node.id = "help:filter";
            filter_node.role = "text";
            filter_node.label = _( "Filter topics by name" );
            filter_node.detail = _( "Reader-only filter: the help screen itself has no "
                                    "search. Filtering only changes which topic rows are "
                                    "listed here; the help topics themselves never change." );
            filter_node.field_type = "text";
            filter_node.applied_value = reader_filter;
            filter_node.value = reader_filter.empty() ? _( "All topics" ) : reader_filter;
            filter_node.actions = { "focus", "set_value", "apply", "cancel_draft" };
            if( reader_filter_draft ) {
                filter_node.has_draft = true;
                filter_node.draft_value = *reader_filter_draft;
            }
            snapshot.nodes.push_back( filter_node );

            a11y::semantic_table topics_table;
            topics_table.id = "help:topics";
            topics_table.columns = { { "topic", _( "Topic" ), "" },
                { "status", _( "Status" ), "" },
                { "hotkey", _( "Hotkey" ), "" }
            };
            snapshot.tables.push_back( topics_table );
            int shown = 0;
            for( const auto &cat : cats ) {
                const std::string name = remove_color_tags( cat.second.name.translated() );
                if( !reader_filter.empty() && !lcmatch( name, reader_filter ) ) {
                    continue;
                }
                const auto hotkey_it = hotkeys.find( cat.first );
                const std::string hotkey = hotkey_it == hotkeys.end() ? std::string()
                                           : hotkey_it->second.short_description();
                const std::string status = data.is_read( cat.first ) ? _( "Read" ) : _( "Unread" );
                a11y::semantic_node node;
                node.id = "help:topic:" + std::to_string( cat.first );
                node.role = "listitem";
                node.label = name;
                node.value = status;
                if( !hotkey.empty() ) {
                    node.detail = string_format( _( "Hotkey: %s" ), hotkey );
                }
                node.table_id = "help:topics";
                node.row_index = shown;
                node.cells = { { "topic", name, "", "" },
                    { "status", status, "", "" },
                    { "hotkey", hotkey, "", "" }
                };
                node.selected = cat.first == reader_last_topic;
                snapshot.nodes.push_back( std::move( node ) );
                ++shown;
            }
            if( shown == 0 ) {
                a11y::semantic_node node;
                node.id = "help:no_results";
                node.role = "status";
                node.label = reader_filter.empty()
                             ? _( "No help topics are available." )
                             : _( "No help topics match the filter." );
                node.actions = { "focus", "activate" };
                snapshot.nodes.push_back( std::move( node ) );
            }
            snapshot.summary = string_format( _( "%1$d of %2$d help topics shown." ),
                                              shown, static_cast<int>( cats.size() ) );
            if( !reader_filter.empty() ) {
                snapshot.summary += " " + string_format( _( "Filter: %s" ), reader_filter );
            }
            snapshot.focus = "help:filter";
            if( reader_last_topic != -1 && cats.find( reader_last_topic ) != cats.end() &&
                ( reader_filter.empty() ||
                  lcmatch( remove_color_tags( cats.at( reader_last_topic ).name.translated() ),
                           reader_filter ) ) ) {
                // Back from a topic returns to that topic's row.
                snapshot.focus = "help:topic:" + std::to_string( reader_last_topic );
            } else if( shown > 0 ) {
                // The first published row is the first category in map
                // order that passed the filter.
                for( const auto &cat : cats ) {
                    if( reader_filter.empty() ||
                        lcmatch( remove_color_tags( cat.second.name.translated() ),
                                 reader_filter ) ) {
                        snapshot.focus = "help:topic:" + std::to_string( cat.first );
                        break;
                    }
                }
            } else {
                snapshot.focus = "help:no_results";
            }
        } else {
            const help_category &cat = data.get_help_category( loaded_option );
            reader_last_topic = loaded_option;
            snapshot.title = remove_color_tags( cat.name.translated() );
            const std::string prev_cat_name =
                data.get_help_category( previous_option( loaded_option ) ).name.translated();
            const std::string next_cat_name =
                data.get_help_category( next_option( loaded_option ) ).name.translated();
            // The same sentence the screen_reader footer draws.
            snapshot.summary = string_format(
                                   _( "Previous category: %s. Next category: %s" ),
                                   remove_color_tags( prev_cat_name ),
                                   remove_color_tags( next_cat_name ) );
            // The document is the topic's own translated paragraphs --
            // the exact strings draw_category() renders (after
            // swap_translated_paragraphs() ran parse_tags on them).
            // MM_SEPARATOR paragraphs are skipped: their stored string
            // is a color name for the drawn rule, not text. Subtitles
            // and monospaced diagrams stay as paragraphs of the text.
            std::string document_text;
            for( const std::pair<std::string, message_modifier> &paragraph :
                 translated_paragraphs ) {
                if( paragraph.second == MM_SEPARATOR || paragraph.first.empty() ) {
                    continue;
                }
                if( !document_text.empty() ) {
                    document_text += "\n\n";
                }
                document_text += paragraph.first;
            }
            const bool has_document = !document_text.empty();
            if( has_document ) {
                a11y::append_document( snapshot, "help:document", cat.name.translated(),
                                       document_text );
            }
            const auto append_nav = [&]( const char *id, const std::string &label,
                                         const std::string &detail ) {
                a11y::semantic_node node;
                node.id = id;
                node.role = "command";
                node.label = label;
                node.detail = detail;
                node.actions = { "focus", "activate" };
                snapshot.nodes.push_back( std::move( node ) );
            };
            append_nav( "help:nav:prev", _( "Previous category" ),
                        remove_color_tags( prev_cat_name ) );
            append_nav( "help:nav:back", _( "Back to the topic list" ), "" );
            append_nav( "help:nav:next", _( "Next category" ),
                        remove_color_tags( next_cat_name ) );
            snapshot.focus = has_document ? "help:document" : "help:nav:back";
        }
        snapshot.nodes.insert( snapshot.nodes.end(),
                               std::make_move_iterator( command_nodes.begin() ),
                               std::make_move_iterator( command_nodes.end() ) );
        for( std::size_t i = 0; i < snapshot.nodes.size(); ++i ) {
            snapshot.nodes[i].position = static_cast<int>( i ) + 1;
            snapshot.nodes[i].total = static_cast<int>( snapshot.nodes.size() );
        }
        return snapshot;
    };
    a11y::screen_scope reader_scope( &ctxt, "help", reader_handle_intent );

    while( true ) {
        while( !has_selected_category ) {
            ui_manager::redraw_invalidated();
            if( a11y::enabled() ) {
                reader_scope.update( make_reader_snapshot() );
            }
            std::string action = imgui_action == "" ? ctxt.handle_input( 50 ) : imgui_action;
            imgui_action = "";
            input_event input = ctxt.get_raw_input();

            for( const auto &hotkey_entry : hotkeys ) {
                if( input == hotkey_entry.second ) {
                    action = "CONFIRM";
                    selected_option = hotkey_entry.first;
                    break;
                }
            }

            if( selected_option != -1 && ( action == "SELECT" || action == "CONFIRM" ) ) {
                has_selected_category = cats.find( selected_option ) != cats.end();
                if( has_selected_category ) {
                    loaded_option = selected_option;
                    swap_translated_paragraphs();
                    data.mark_read( loaded_option );
                    selected_option = -1;
                    window_flags |= ImGuiWindowFlags_NoNavInputs;
                } else {
                    debugmsg( "Category not found: option %s", selected_option );
                }
            } else if( action == "QUIT" ) {
                return;
            }
        }
        while( has_selected_category ) {
            ui_manager::redraw_invalidated();
            if( a11y::enabled() ) {
                reader_scope.update( make_reader_snapshot() );
            }
            std::string action = imgui_action == "" ? ctxt.handle_input( 50 ) : imgui_action;
            imgui_action = "";
            if( selected_option != -1 ) {
                if( action == "SELECT" || action == "CONFIRM" ) {
                    s = cataimgui::scroll::begin;
                    loaded_option = selected_option;
                    swap_translated_paragraphs();
                    data.mark_read( loaded_option );
                    continue;
                }
                selected_option = -1;
            }

            if( action == "UP" ) {
                s = cataimgui::scroll::line_up;
            } else if( action == "DOWN" ) {
                s = cataimgui::scroll::line_down;
            } else if( action == "HOME" ) {
                s = cataimgui::scroll::begin;
            } else if( action == "END" ) {
                s = cataimgui::scroll::end;
            } else if( action == "PAGE_UP" ) {
                s = cataimgui::scroll::page_up;
            } else if( action == "PAGE_DOWN" ) {
                s = cataimgui::scroll::page_down;
            } else if( action == "CONFIRM" || action == "QUIT" ) {
                s = cataimgui::scroll::begin;
                has_selected_category = false;
                window_flags = window_flags & ~ImGuiWindowFlags_NoNavInputs;
            } else if( action == "LEFT" || action == "PREV_TAB" ) {
                s = cataimgui::scroll::begin;
                loaded_option = previous_option( loaded_option );
                swap_translated_paragraphs();
                data.mark_read( loaded_option );
            } else if( action == "RIGHT" || action == "NEXT_TAB" ) {
                s = cataimgui::scroll::begin;
                loaded_option = next_option( loaded_option );
                swap_translated_paragraphs();
                data.mark_read( loaded_option );
            }
        }
    }
}

void help_window::swap_translated_paragraphs()
{
    Character &u = get_player_character();
    translated_paragraphs.clear();
    const help_category &cat = data.get_help_category( loaded_option );

    for( const auto &[untranslated_paragraph, modifier] : cat.paragraphs ) {
        std::string &translated_paragraph = translated_paragraphs.emplace_back(
                                                untranslated_paragraph.translated(), modifier ).first;
        parse_tags( translated_paragraph, u, u );
    }
}

int help_window::previous_option( int option )
{
    const std::map<const int, const help_category> &cats = data.get_help_categories();
    auto it = cats.find( option );
    cata_assert( it != cats.end() );
    if( --it == --cats.begin() ) {
        it = --cats.end();
    }
    return it->first;
}

int help_window::next_option( int option )
{
    const std::map<const int, const help_category> &cats = data.get_help_categories();
    auto it = cats.find( option );
    cata_assert( it != cats.end() );
    if( ++it == cats.end() ) {
        it = cats.begin();
    }
    return it->first;
}

void help_window::format_title( std::optional<help_category> category ) const
{
    //~ Help menu header
    const std::string translated_category_name = !category ? _( "Help" ) : category->name.translated();

    if( screen_reader ) {
        cataimgui::TextColoredParagraph( c_white, translated_category_name );
        ImGui::NewLine();
        return;
    }

    nc_color category_color = !category ? c_light_blue : category->color;

    const float title_length = ImGui::CalcTextSize( remove_color_tags(
                                   translated_category_name ).c_str() ).x;
    ImGui::PushStyleVarX( ImGuiStyleVar_ItemSpacing, 0 );
    ImGui::PushStyleVarY( ImGuiStyleVar_ItemSpacing, 0 );
    ImGui::PushStyleColor( ImGuiCol_Separator, category_color );
    cataimgui::PushMonoFont();
    const int sep_len = std::ceil( ( title_length / ImGui::CalcTextSize( "═" ).x )  + 2 );
    ImGui::TextColored( category_color, "╔" );
    ImGui::SameLine();
    for( int i = sep_len; i > 0; i-- ) {
        ImGui::TextColored( category_color, "═" );
        ImGui::SameLine();
    }
    const int x = ImGui::GetCursorPosX();
    ImGui::TextColored( category_color,  "╗" );
    ImGui::TextColored( category_color,  "║ " );
    ImGui::SameLine();
    const ImVec2 text_pos = ImGui::GetCursorPos();
    ImGui::SetCursorPosX( x );
    ImGui::TextColored( category_color,  "║" );
    ImGui::TextColored( category_color,  "╚" );
    ImGui::SameLine();
    for( int i = sep_len; i > 0; i-- ) {
        ImGui::TextColored( category_color,  "═" );
        ImGui::SameLine();
    }
    ImGui::TextColored( category_color,  "╝" );

    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();

    const ImVec2 end_pos = ImGui::GetCursorPos();
    ImGui::PopFont();
    ImGui::PopStyleColor( 1 );
    ImGui::PopStyleVar( 2 );

    ImGui::SetCursorPos( text_pos );
    cataimgui::TextColoredParagraph( c_white, translated_category_name );
    ImGui::SetCursorPos( end_pos );
}

void help_window::format_subtitle( const std::string_view &translated_category_name,
                                   const nc_color &category_color ) const
{

    if( screen_reader ) {
        cataimgui::TextColoredParagraph( c_white, translated_category_name );
        ImGui::NewLine();
        return;
    }

    const float title_length = ImGui::CalcTextSize( remove_color_tags(
                                   translated_category_name ).c_str() ).x;

    ImGui::PushStyleColor( ImGuiCol_Separator, category_color );
    cataimgui::PushMonoFont();
    const int sep_len = std::ceil( ( title_length / ImGui::CalcTextSize( "═" ).x )  + 4 );
    for( int i = sep_len; i > 0; i-- ) {
        ImGui::SameLine( 0.f, 0.f );
    }
    ImGui::NewLine();
    ImGui::TextColored( category_color, "▏ " );
    ImGui::SameLine( 0.f, 0.f );
    ImGui::PopFont();

    cataimgui::TextColoredParagraph( c_white, translated_category_name );

    cataimgui::PushMonoFont();
    ImGui::SameLine( 0.f, 0.f );
    ImGui::TextColored( category_color, " ▕" );
    for( int i = sep_len; i > 0; i-- ) {
        ImGui::TextColored( category_color, "▔" );
        ImGui::SameLine( 0.f, 0.f );
    }
    ImGui::NewLine();
    ImGui::Separator();

    ImGui::PopFont();
    ImGui::PopStyleColor( 1 );
}

void help_window::format_footer( const std::string &prev, const std::string &next,
                                 const nc_color &category_color )
{
    if( screen_reader ) {
        const std::string screen_reader_footer = string_format(
                    //~ Help menu footer when using screen reader
                    _( "Previous category: %s. Next category: %s" ), prev, next );
        cataimgui::TextColoredParagraph( c_white, screen_reader_footer );
        return;
    }

    const float next_size = ImGui::CalcTextSize( next.c_str() ).x;
    const float brackets_size = ImGui::CalcTextSize( "<]" ).x;
    const float next_x = ImGui::GetCursorPos().x + ImGui::GetContentRegionAvail().x - next_size -
                         brackets_size - one_em;

    ImGui::PushStyleColor( ImGuiCol_Separator, category_color );
    ImGui::Separator();
    ImGui::NewLine();

    ImGui::TextColored( c_yellow, "<" );
    ImGui::SameLine( 0.f, 0.f );

    ImGui::Text( "%s", prev.c_str() );
    if( ImGui::IsItemHovered() ) {
        selected_option = previous_option( loaded_option );
    }

    ImGui::SameLine( 0.f, 0.f );
    ImGui::TextColored( c_yellow, "]" );
    ImGui::SameLine( next_x );
    ImGui::TextColored( c_yellow, "[" );
    ImGui::SameLine( 0.f, 0.f );

    ImGui::Text( "%s", next.c_str() );
    if( ImGui::IsItemHovered() ) {
        selected_option = next_option( loaded_option );
    }

    ImGui::SameLine( 0.f, 0.f );
    ImGui::TextColored( c_yellow, ">" );
    ImGui::PopStyleColor( 1 );
}
