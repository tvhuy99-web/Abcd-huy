#include "study_zone_ui.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "a11y_input.h"
#include "a11y_runtime.h"
#include "cata_imgui.h"
#include "character_id.h"
#include "game.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "localized_comparator.h"
#include "memory_fast.h"
#include "npc.h"
#include "overmapbuffer.h"
#include "skill.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_manager.h"


static int filter_skill_input_callback( ImGuiInputTextCallbackData *data )
{
    if( data->EventChar < 256 ) {
        char c = static_cast<char>( data->EventChar );
        if( !std::islower( c ) && c != ' ' ) {
            return 1;
        }
    }
    return 0;
}

namespace
{
class study_zone_window : public cataimgui::window
{
    public:
        explicit study_zone_window( std::map<std::string, std::set<skill_id>> &npc_skill_preferences ) :
            cataimgui::window( _( "Study Zone Skill Preferences" ), ImGuiWindowFlags_NoNavInputs ),
            npc_skill_preferences( npc_skill_preferences ) {
            ctxt = input_context( "STUDY_ZONE_UI" );
            ctxt.register_action( "CONFIRM" );
            ctxt.register_action( "FILTER" );
            ctxt.register_action( "QUIT" );
            ctxt.set_timeout( 10 );

            // Get all skills
            for( const Skill &s : Skill::skills ) {
                all_skills.push_back( s.ident() );
            }
            std::sort( all_skills.begin(), all_skills.end(),
            []( const skill_id & a, const skill_id & b ) {
                return localized_compare( a->name(), b->name() );
            } );

            // get followers
            std::set<character_id> follower_ids = g->get_follower_list();
            for( const character_id &id : follower_ids ) {
                shared_ptr_fast<npc> npc_ptr = overmap_buffer.find_npc( id );
                if( npc_ptr && !npc_ptr->is_hallucination() && npc_ptr->is_player_ally() ) {
                    npc_names.push_back( npc_ptr->name );
                }
            }
            // include NPCs that are already in preferences but not in follower list
            for( const auto &pair : npc_skill_preferences ) {
                if( std::find( npc_names.begin(), npc_names.end(), pair.first ) == npc_names.end() ) {
                    npc_names.push_back( pair.first );
                }
            }
            std::sort( npc_names.begin(), npc_names.end(), localized_compare );

            // for new NPCs set all checkboxes
            for( const std::string &npc_name : npc_names ) {
                if( npc_skill_preferences.find( npc_name ) == npc_skill_preferences.end() ) {
                    std::set<skill_id> &npc_skills = npc_skill_preferences[npc_name];
                    for( const skill_id &skill : all_skills ) {
                        npc_skills.insert( skill );
                    }
                    preferences_changed = true;
                }
            }

            max_skill_name_width = 0.0f;
            for( const skill_id &skill : all_skills ) {
                ImVec2 text_size = ImGui::CalcTextSize( skill->name().c_str() );
                float width = text_size.x + ImGui::GetStyle().FramePadding.x * 2.0f;
                max_skill_name_width = std::max( max_skill_name_width, width );
            }

            max_npc_name_width = 0.0f;
            for( const std::string &npc_name : npc_names ) {
                ImVec2 text_size = ImGui::CalcTextSize( npc_name.c_str() );
                float width = text_size.x + ImGui::GetStyle().FramePadding.x * 2.0f;
                if( width > max_npc_name_width ) {
                    max_npc_name_width = width;
                }
            }
        }

        study_zone_ui_result execute() {
            // Reader adapter (R16): this ImGui window would otherwise only
            // get the generic command fallback, which never publishes the
            // per-companion skill checkboxes. The preferences are published
            // as an R02 table (one row per skill, one column per companion)
            // whose cells are presentation only; each (skill, companion)
            // pair is also a bool form field in the tree under its skill
            // row. Reader edits follow the form contract --
            // select_choice only records a draft in reader_drafts, apply
            // writes it through the same insert/erase logic as the ImGui
            // checkbox in draw_skill_row, and activate toggles immediately
            // like that checkbox. Every write goes to
            // npc_skill_preferences, the single real store owned by
            // study_zone_options; the drafts are never a parallel store.
            a11y::screen_scope reader( &ctxt, "study-zone",
            [this]( const a11y::semantic_intent & intent ) -> std::optional<std::string> {
                return reader_intent( intent );
            } );
            bool canceled_result = false;
            bool confirmed = false;
            while( get_is_open() ) {
                ui_manager::redraw();
                if( a11y::enabled() ) {
                    reader.update( build_snapshot() );
                }
                std::string action = ctxt.handle_input();

                if( action == "CONFIRM" ) {
                    confirmed = true;
                    break;
                }

                if( action == "QUIT" ) {
                    if( !skill_filter.empty() ) {
                        skill_filter.clear();
                    } else {
                        canceled_result = true;
                        break;
                    }
                }
                if( action == "FILTER" ) {
                    filter_just_focused = true;
                }
            }

            if( done_clicked ) {
                confirmed = true;
            }

            if( !get_is_open() && !confirmed && !canceled_result ) {
                canceled_result = true;
            }

            if( canceled_result ) {
                return study_zone_ui_result::canceled;
            }
            return preferences_changed ? study_zone_ui_result::changed : study_zone_ui_result::successful;
        }

    protected:
        void draw_skill_row( const skill_id &skill ) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text( "%s", skill->name().c_str() );

            for( const std::string &npc_name : npc_names ) {
                ImGui::TableNextColumn();

                std::set<skill_id> &npc_skills = npc_skill_preferences[npc_name];

                bool is_selected = npc_skills.count( skill ) > 0;
                bool was_selected = is_selected;

                // center the checkbox
                float offset = ( ImGui::GetColumnWidth() - ImGui::GetFrameHeight() ) * 0.5f;
                ImGui::SetCursorPosX( ImGui::GetCursorPosX() + offset );

                ImGui::PushID( checkbox_id_counter++ );
                if( ImGui::Checkbox( "##checkbox", &is_selected ) ) {
                    preferences_changed = true;
                }
                ImGui::PopID();
                if( is_selected != was_selected ) {
                    if( is_selected ) {
                        npc_skills.insert( skill );
                    } else {
                        npc_skills.erase( skill );
                    }
                    if( npc_skills.empty() ) {
                        npc_skill_preferences.erase( npc_name );
                    }
                }
            }
        }

        void draw_footer( const std::vector<skill_id> &filtered_skills ) {
            // filter input, buttons, and Done button
            std::string filter_label = _( "Filter skills: " );
            ImGui::TextUnformatted( filter_label.c_str() );
            ImGui::SameLine();

            if( filter_just_focused ) {
                ImGui::SetKeyboardFocusHere();
                filter_just_focused = false;
            }

            ImGui::SetNextItemWidth( 300.0f );
            std::array<char, 256> filter_buffer = {0};
            strncpy( filter_buffer.data(), skill_filter.c_str(), filter_buffer.size() - 1 );
            ImGui::InputText( "##skill_filter", filter_buffer.data(), filter_buffer.size(),
                              ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCharFilter,
                              filter_skill_input_callback );
            skill_filter = filter_buffer.data();

            ImGui::SameLine();
            if( ImGui::Button( _( "Check All" ) ) ) {
                for( const std::string &npc_name : npc_names ) {
                    std::set<skill_id> &npc_skills = npc_skill_preferences[npc_name];
                    for( const skill_id &skill : filtered_skills ) {
                        npc_skills.insert( skill );
                    }
                }
                preferences_changed = true;
            }

            ImGui::SameLine();
            if( ImGui::Button( _( "Clear All" ) ) ) {
                for( const std::string &npc_name : npc_names ) {
                    std::set<skill_id> &npc_skills = npc_skill_preferences[npc_name];
                    for( const skill_id &skill : filtered_skills ) {
                        npc_skills.erase( skill );
                    }
                    if( npc_skills.empty() ) {
                        npc_skill_preferences.erase( npc_name );
                    }
                }
                preferences_changed = true;
            }

            ImGui::SameLine();
            if( ImGui::Button( _( "Done" ) ) ) {
                done_clicked = true;
                is_open = false;
            }
        }

        cataimgui::bounds get_bounds() override {
            ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
            float width = std::min( 1200.0f, viewport_size.x * 0.9f );
            float height = viewport_size.y;
            return { -1.f, -1.f, width, height };
        }

        void draw_controls() override {
            checkbox_id_counter = 0;
            std::vector<skill_id> filtered_skills;
            if( skill_filter.empty() ) {
                filtered_skills = all_skills;
            } else {
                for( const skill_id &skill : all_skills ) {
                    if( skill->name().find( skill_filter ) != std::string::npos ) {
                        filtered_skills.push_back( skill );
                    }
                }
            }

            const float footer_height_to_reserve = ImGui::GetFrameHeightWithSpacing() * 3;

            // inner scroll
            if( ImGui::BeginChild( "table_scroll_region", ImVec2( 0, -footer_height_to_reserve ), false,
                                   ImGuiWindowFlags_HorizontalScrollbar ) ) {
                // Create a table with Skills as rows and npc names as columns
                if( ImGui::BeginTable( "skill_npc_table", static_cast<int>( npc_names.size() ) + 1,
                                       ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
                                       ImGuiTableFlags_RowBg ) ) {
                    // table header row
                    ImGui::TableSetupColumn( "Skill", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide,
                                             max_skill_name_width );
                    for( const std::string &npc_name : npc_names ) {
                        ImGui::TableSetupColumn( npc_name.c_str(), ImGuiTableColumnFlags_WidthFixed,
                                                 max_npc_name_width );
                    }
                    // freeze skill column horizontally and header vertically
                    ImGui::TableSetupScrollFreeze( 1, 1 );
                    ImGui::TableHeadersRow();

                    // skill rows
                    ImGuiListClipper clipper;
                    clipper.Begin( filtered_skills.size() );
                    while( clipper.Step() ) {
                        for( int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++ ) {
                            draw_skill_row( filtered_skills[row] );
                        }
                    }

                    ImGui::EndTable();
                }
            }
            ImGui::EndChild();

            draw_footer( filtered_skills );
        }

    private:
        // The state the ImGui checkbox in draw_skill_row shows for one
        // (companion, skill) pair: an entry must exist and contain the
        // skill. A missing entry (erased when its set became empty) reads
        // as unchecked for every skill, exactly as drawn -- even though
        // study_zone_options::get_skill_preferences treats a missing entry
        // as "reads everything" when the zone is later applied.
        bool skill_allowed( const std::string &npc_name, const skill_id &skill ) const {
            const auto found = npc_skill_preferences.find( npc_name );
            return found != npc_skill_preferences.end() && found->second.count( skill ) > 0;
        }

        // The state change of draw_skill_row, shared by the reader's
        // apply and activate paths: insert or erase the skill, and erase
        // the companion's entry altogether when its set becomes empty.
        void set_skill_allowed( const std::string &npc_name, const skill_id &skill,
                                bool allowed ) {
            if( allowed ) {
                npc_skill_preferences[npc_name].insert( skill );
            } else {
                const auto found = npc_skill_preferences.find( npc_name );
                if( found != npc_skill_preferences.end() ) {
                    found->second.erase( skill );
                    if( found->second.empty() ) {
                        npc_skill_preferences.erase( found );
                    }
                }
            }
            preferences_changed = true;
        }

        // The Check All / Clear All loops of draw_footer. The reader has
        // no filter, so they run over every skill in all_skills, not over
        // the footer's filtered_skills.
        void set_all_skills( bool allowed ) {
            for( const std::string &npc_name : npc_names ) {
                std::set<skill_id> &npc_skills = npc_skill_preferences[npc_name];
                for( const skill_id &skill : all_skills ) {
                    if( allowed ) {
                        npc_skills.insert( skill );
                    } else {
                        npc_skills.erase( skill );
                    }
                }
                if( !allowed && npc_skills.empty() ) {
                    npc_skill_preferences.erase( npc_name );
                }
            }
            preferences_changed = true;
        }

        std::optional<std::string> reader_intent( const a11y::semantic_intent &intent ) {
            if( intent.node == "study:npc_prev" || intent.node == "study:npc_next" ||
                intent.node == "study:skill_prev" || intent.node == "study:skill_next" ) {
                if( intent.action == "activate" ) {
                    if( intent.node == "study:npc_prev" && reader_npc_page > 0 ) { --reader_npc_page; }
                    if( intent.node == "study:npc_next" ) { ++reader_npc_page; }
                    if( intent.node == "study:skill_prev" && reader_skill_page > 0 ) { --reader_skill_page; }
                    if( intent.node == "study:skill_next" ) { ++reader_skill_page; }
                }
                return std::string( "TIMEOUT" );
            }
            if( intent.node == "study:check_all" || intent.node == "study:clear_all" ) {
                if( intent.action == "focus" ) {
                    return std::string( "TIMEOUT" );
                }
                if( intent.action == "activate" ) {
                    set_all_skills( intent.node == "study:check_all" );
                    return std::string( "TIMEOUT" );
                }
                return std::nullopt;
            }
            if( intent.node == "study:done" ) {
                if( intent.action == "focus" ) {
                    return std::string( "TIMEOUT" );
                }
                if( intent.action == "activate" ) {
                    return std::string( "CONFIRM" );
                }
                return std::nullopt;
            }
            if( intent.node == "study:empty" ) {
                if( intent.action == "focus" ) {
                    return std::string( "TIMEOUT" );
                }
                return std::nullopt;
            }
            for( const skill_id &skill : all_skills ) {
                if( intent.node == "study:skill:" + skill.str() ) {
                    if( intent.action == "focus" ) {
                        return std::string( "TIMEOUT" );
                    }
                    return std::nullopt;
                }
                for( std::size_t i = 0; i < npc_names.size(); ++i ) {
                    if( intent.node != "study:allow:" + std::to_string( i ) + ":" + skill.str() ) {
                        continue;
                    }
                    const std::string &npc_name = npc_names[i];
                    const std::pair<std::string, std::string> draft_key( npc_name, skill.str() );
                    if( intent.action == "focus" ) {
                        return std::string( "TIMEOUT" );
                    }
                    if( intent.action == "select_choice" ) {
                        reader_drafts[draft_key] = intent.value == "true";
                        return std::string( "TIMEOUT" );
                    }
                    if( intent.action == "apply" ) {
                        const auto draft = reader_drafts.find( draft_key );
                        if( draft != reader_drafts.end() ) {
                            set_skill_allowed( npc_name, skill, draft->second );
                            reader_drafts.erase( draft );
                        }
                        return std::string( "TIMEOUT" );
                    }
                    if( intent.action == "cancel_draft" ) {
                        reader_drafts.erase( draft_key );
                        return std::string( "TIMEOUT" );
                    }
                    if( intent.action == "activate" ) {
                        set_skill_allowed( npc_name, skill, !skill_allowed( npc_name, skill ) );
                        reader_drafts.erase( draft_key );
                        return std::string( "TIMEOUT" );
                    }
                    return std::nullopt;
                }
            }
            return a11y::registered_intent( ctxt, intent );
        }

        a11y::semantic_snapshot build_snapshot() {
            constexpr std::size_t npc_page_size = 8;
            constexpr std::size_t skill_page_size = 32;
            reader_npc_page = std::min( reader_npc_page,
                                       npc_names.empty() ? std::size_t( 0 ) : ( npc_names.size() - 1 ) / npc_page_size );
            reader_skill_page = std::min( reader_skill_page,
                                         all_skills.empty() ? std::size_t( 0 ) : ( all_skills.size() - 1 ) / skill_page_size );
            const std::size_t npc_begin = reader_npc_page * npc_page_size;
            const std::size_t npc_end = std::min( npc_names.size(), npc_begin + npc_page_size );
            const std::size_t skill_begin = reader_skill_page * skill_page_size;
            const std::size_t skill_end = std::min( all_skills.size(), skill_begin + skill_page_size );
            a11y::semantic_snapshot snapshot = a11y::command_snapshot( ctxt,
                                               _( "Study Zone Skill Preferences" ) );
            std::vector<a11y::semantic_node> commands = std::move( snapshot.nodes );
            snapshot.nodes.clear();
            // The semantics study_zone_options applies later: a companion
            // with a preference entry studies only the skills in it, and a
            // companion with no entry reads every skill.
            snapshot.summary = string_format(
                                   _( "Each companion studies only books for the skills marked "
                                      "for them in this study zone; a companion with no "
                                      "preference entry reads every skill. "
                                      "%1$d companions, %2$d skills." ),
                                   static_cast<int>( npc_names.size() ),
                                   static_cast<int>( all_skills.size() ) );
            if( npc_names.empty() ) {
                a11y::semantic_node empty;
                empty.id = "study:empty";
                empty.role = "document";
                empty.label = _( "No companions." );
                empty.detail = _( "No companions." );
                empty.actions = { "focus" };
                snapshot.nodes.push_back( std::move( empty ) );
            } else {
                // Table contract (R02.2): the skills table is declared
                // once; skill rows below only carry cells referencing
                // these columns, and cells never carry actions.
                a11y::semantic_table skills_table;
                skills_table.id = "study:skills";
                skills_table.columns.push_back( { "skill", _( "Skill" ), "" } );
                for( std::size_t i = npc_begin; i < npc_end; ++i ) {
                    skills_table.columns.push_back( { "npc:" + std::to_string( i ), npc_names[i],
                                                      "" } );
                }
                snapshot.tables.push_back( skills_table );
                for( std::size_t row = skill_begin; row < skill_end; ++row ) {
                    const skill_id &skill = all_skills[row];
                    const std::string row_id = "study:skill:" + skill.str();
                    int allowed_count = 0;
                    for( const std::string &npc_name : npc_names ) {
                        if( skill_allowed( npc_name, skill ) ) {
                            ++allowed_count;
                        }
                    }
                    // Row role: medical and trade rows leave the default
                    // role, so this row does the same.
                    a11y::semantic_node node;
                    node.id = row_id;
                    node.label = skill->name();
                    // The row itself only carries the cells; editing a
                    // (skill, companion) pair goes through the child
                    // checkbox nodes below, so the row offers no activate.
                    node.actions = { "focus" };
                    node.detail = string_format( _( "Allowed for %1$d of %2$d companions." ),
                                                 allowed_count,
                                                 static_cast<int>( npc_names.size() ) );
                    node.depth = 0;
                    node.has_children = true;
                    node.expanded = true;
                    node.table_id = "study:skills";
                    node.row_index = static_cast<int>( row );
                    node.cells.push_back( { "skill", skill->name(), "", "" } );
                    for( std::size_t i = npc_begin; i < npc_end; ++i ) {
                        const bool allowed = skill_allowed( npc_names[i], skill );
                        node.cells.push_back( { "npc:" + std::to_string( i ),
                                                allowed ? _( "Allowed" ) : _( "Not allowed" ), "",
                                                allowed ? "1" : "0" } );
                    }
                    snapshot.nodes.push_back( node );
                    for( std::size_t i = npc_begin; i < npc_end; ++i ) {
                        const std::string &npc_name = npc_names[i];
                        const bool allowed = skill_allowed( npc_name, skill );
                        a11y::semantic_node child;
                        child.id = "study:allow:" + std::to_string( i ) + ":" + skill.str();
                        child.role = "checkbox";
                        child.label = npc_name;
                        child.detail = string_format(
                                           _( "Allows %1$s to study %2$s. Currently %3$s." ),
                                           npc_name, skill->name(),
                                           allowed ? _( "allowed" ) : _( "not allowed" ) );
                        child.parent_id = row_id;
                        child.depth = 1;
                        child.field_type = "bool";
                        child.applied_value = allowed ? "true" : "false";
                        child.value = allowed ? _( "Allowed" ) : _( "Not allowed" );
                        child.checked = allowed;
                        child.choices = { { "true", _( "Enabled" ) },
                                          { "false", _( "Disabled" ) } };
                        child.actions = { "focus", "activate", "select_choice", "apply",
                                          "cancel_draft" };
                        const auto draft = reader_drafts.find(
                                               std::make_pair( npc_name, skill.str() ) );
                        if( draft != reader_drafts.end() ) {
                            child.has_draft = true;
                            child.draft_value = draft->second ? "true" : "false";
                        }
                        snapshot.nodes.push_back( std::move( child ) );
                    }
                }
                a11y::semantic_node check_all;
                check_all.id = "study:check_all";
                check_all.label = _( "Check All" );
                check_all.detail = _( "Check every skill for every companion." );
                snapshot.nodes.push_back( std::move( check_all ) );
                a11y::semantic_node clear_all;
                clear_all.id = "study:clear_all";
                clear_all.label = _( "Clear All" );
                clear_all.detail = _( "Clear every skill for every companion." );
                snapshot.nodes.push_back( std::move( clear_all ) );
            }
            a11y::semantic_node done;
            done.id = "study:done";
            done.label = _( "Done" );
            done.detail = _( "Finish editing the study zone skill preferences." );
            snapshot.nodes.push_back( std::move( done ) );
            const auto page_button = [&]( const std::string &id, const std::string &label,
                                          const bool available ) {
                a11y::semantic_node node;
                node.id = id;
                node.label = label;
                node.enabled = available;
                if( !available ) { node.disabled_reason = _( "No more entries in this direction." ); }
                snapshot.nodes.push_back( std::move( node ) );
            };
            if( npc_names.size() > npc_page_size ) {
                page_button( "study:npc_prev", _( "Previous companions" ), reader_npc_page > 0 );
                page_button( "study:npc_next", _( "Next companions" ), npc_end < npc_names.size() );
            }
            if( all_skills.size() > skill_page_size ) {
                page_button( "study:skill_prev", _( "Previous skills" ), reader_skill_page > 0 );
                page_button( "study:skill_next", _( "Next skills" ), skill_end < all_skills.size() );
            }
            snapshot.nodes.insert( snapshot.nodes.end(),
                                   std::make_move_iterator( commands.begin() ),
                                   std::make_move_iterator( commands.end() ) );
            snapshot.focus = !npc_names.empty() && !all_skills.empty()
                             ? "study:skill:" + all_skills[skill_begin].str()
                             : "study:done";
            // Flat positions for ordinary nodes; tree nodes count their
            // sibling set under the same parent (R02.1).
            for( std::size_t i = 0; i < snapshot.nodes.size(); ++i ) {
                snapshot.nodes[i].position = static_cast<int>( i ) + 1;
                snapshot.nodes[i].total = static_cast<int>( snapshot.nodes.size() );
            }
            std::map<std::string, int> sibling_seen;
            std::map<std::string, int> sibling_total;
            for( const a11y::semantic_node &node : snapshot.nodes ) {
                if( node.depth >= 0 ) {
                    sibling_total[node.parent_id]++;
                }
            }
            for( a11y::semantic_node &node : snapshot.nodes ) {
                if( node.depth >= 0 ) {
                    node.position = ++sibling_seen[node.parent_id];
                    node.total = sibling_total[node.parent_id];
                }
            }
            return snapshot;
        }

        std::map<std::string, std::set<skill_id>> &npc_skill_preferences;
        // Unapplied reader edits, keyed by (companion name, skill id).
        // A draft changes nothing until apply writes it through
        // set_skill_allowed into npc_skill_preferences above.
        std::map<std::pair<std::string, std::string>, bool> reader_drafts;
        std::size_t reader_npc_page = 0;
        std::size_t reader_skill_page = 0;
        std::vector<skill_id> all_skills;
        std::vector<std::string> npc_names;
        std::string skill_filter;
        bool filter_just_focused = false;
        bool preferences_changed = false;
        bool done_clicked = false;
        int checkbox_id_counter = 0;
        float max_skill_name_width = 0.0f;
        float max_npc_name_width = 0.0f;
        input_context ctxt;
};
} // namespace

study_zone_ui_result query_study_zone_skills( std::map<std::string, std::set<skill_id>>
        &npc_skill_preferences )
{
    study_zone_window window( npc_skill_preferences );
    return window.execute();
}

