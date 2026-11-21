#include <algorithm>
#include "a11y_document.h"
#include "a11y_semantics.h"
#include <initializer_list>
#include <cstdlib>
#include <functional>
#include <imgui/imgui.h>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "a11y_body.h"
#include "a11y_input.h"
#include "a11y_requirements.h"
#include "a11y_runtime.h"
#include "avatar_action.h"
#include "body_status_ui.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_imgui.h"
#include "catacharset.h"
#include "character.h"
#include "character_attire.h"
#include "color.h"
#include "coordinates.h"
#include "creature.h"
#include "debug.h"
#include "display.h"
#include "effect.h"
#include "enum_conversions.h"
#include "enum_traits.h"
#include "enums.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "input_context.h"
#include "item.h"
#include "item_location.h"
#include "math_parser_diag_value.h"
#include "messages.h"
#include "output.h"
#include "proficiency.h"
#include "requirements.h"
#include "skill.h"
#include "string_formatter.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "ui_manager.h"
#include "uilist.h"
#include "units.h"
#include "units_utility.h"
#include "weighted_list.h"
#include "wound.h"

static const efftype_id effect_bite( "bite" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_mending( "mending" );

static const json_character_flag json_flag_BIONIC_LIMB( "BIONIC_LIMB" );
static const json_character_flag json_flag_PAIN_IMMUNE( "PAIN_IMMUNE" );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );
static const trait_id trait_SUNLIGHT_DEPENDENT( "SUNLIGHT_DEPENDENT" );

template <typename T> struct enum_traits;

class temp_crafting_inventory;

namespace
{
enum class medical_tab_mode {
    TAB_LIMBS,
    TAB_SUMMARY,
    last
};
} // namespace

template<>
struct enum_traits<medical_tab_mode> {
    static constexpr medical_tab_mode last = medical_tab_mode::last;
    static constexpr medical_tab_mode first = medical_tab_mode::TAB_LIMBS;
};

static std::string coloured_stat_display( int statCur, int statMax )
{
    nc_color cstatus;
    if( statCur <= 0 ) {
        cstatus = c_dark_gray;
    } else if( statCur < statMax / 2 ) {
        cstatus = c_red;
    } else if( statCur < statMax ) {
        cstatus = c_light_red;
    } else if( statCur == statMax ) {
        cstatus = c_white;
    } else if( statCur < statMax * 1.5 ) {
        cstatus = c_light_green;
    } else {
        cstatus = c_green;
    }
    std::string cur = colorize( string_format( _( "%2d" ), statCur ), cstatus );
    return string_format( _( "%s (%d)" ), cur, statMax );
}

std::string medical_ui::get_limb_effects( const Character &you, const bodypart_id &part )
{
    std::string description;

    const int bleed_intensity = you.get_effect_int( effect_bleed, part );
    const bool bleeding = bleed_intensity > 0;
    const bool bitten = you.has_effect( effect_bite, part.id() );
    const bool infected = you.has_effect( effect_infected, part.id() );

    // BLEEDING block
    if( bleeding ) {
        const effect bleed_effect = you.get_effect( effect_bleed, part );
        const nc_color bleeding_color = colorize_bleeding_intensity( bleed_intensity );
        description += string_format( "[ %s ] - %s\n",
                                      colorize( bleed_effect.get_speed_name(), bleeding_color ),
                                      string_format( bleed_effect.disp_short_desc(), body_part_name( part.id() ) ) );
    }

    // BITTEN block
    if( bitten ) {
        const effect bite_effect = you.get_effect( effect_bite, part );
        description += string_format( "[ %s ] - %s\n",
                                      colorize( bite_effect.get_speed_name(), c_yellow ),
                                      string_format( bite_effect.disp_short_desc(), body_part_name( part.id() ) ) );
    }

    // INFECTED block
    if( infected ) {
        const effect infected_effect = you.get_effect( effect_infected, part );
        description += string_format( "[ %s ] - %s\n",
                                      colorize( infected_effect.get_speed_name(), c_pink ),
                                      string_format( infected_effect.disp_short_desc(), body_part_name( part.id() ) ) );
    }

    // rest of effects
    for( const effect &eff : you.get_effects_from_bp( part ) ) {
        if( eff.get_id() != effect_bleed &&
            eff.get_id() != effect_bite &&
            eff.get_id() != effect_infected ) {
            description += string_format( "[ %s ] - %s\n", colorize( eff.get_speed_name(), c_bold ),
                                          string_format( eff.disp_short_desc(), body_part_name( part.id() ) ) );
        }
    }

    return description.empty() ? _( "None" ) : description;
}

std::string medical_ui::get_limb_wounds( const Character &you_ref, const bodypart_id &part )
{
    const Character *you = &you_ref;

    std::string detail_str;
    const bodypart *bp = you->get_part( part );

    // for existing wounds
    // Keep the body's wound storage untouched. The visual medical view and
    // screen reader should encounter the most severe injuries first.
    std::vector<wound> wds = bp->get_wounds();
    std::stable_sort( wds.begin(), wds.end(), []( const wound &a, const wound &b ) {
        return a.type->severity > b.type->severity;
    } );
    if( !wds.empty() ) {
        detail_str += _( "Current wounds" );
        detail_str += ":\n";
        for( const wound &wd : wds ) {
            // Mirror upstream's visible severity colors while retaining the
            // existing native / accessible medical UI and treatment actions.
            nc_color severity_color = c_cyan;
            if( wd.type->severity == wound_severity::CRITICAL ) {
                severity_color = c_red;
            } else if( wd.type->severity == wound_severity::MAJOR ) {
                severity_color = c_light_red;
            }
            if( !debug_mode ) {
                detail_str +=
                    string_format( "%s - %s\n", colorize( wd.type->get_name(), severity_color ), wd.type->get_description() );
            } else {

                std::string limb_score_output = "limb scores applied:\n";
                for( const wound_limb_score &s : wd.type->get_limb_scores() ) {
                    limb_score_output += string_format( "%s: %.3f\n", s.score.str(), s.value );
                }

                detail_str += string_format( "%s (%s) - %s\n",
                                             colorize( wd.type->get_name(), severity_color ), wd.type.str(), wd.type->get_description() );
                detail_str += string_format( "healing time: %s\n", to_string( wd.get_healing_time() ) );
                detail_str += string_format( "lifetime: %s\n", to_string( time_duration::from_seconds( wd.get_healing_progress() ) ) );
                detail_str += string_format( "healing percentage: %.3f%%\n", wd.healing_percentage() );
                detail_str += string_format( "healing speed multiplier: %.3fx\n",
                                             wd.get_wound_healing_modifier( you ) );
                detail_str += string_format( "gives pain: %d (raw %d)\n", wd.get_pain( you ),
                                             wd.get_pain_raw() );
                // Report the remaining wound HP damage via its public accessor.
                // The original unhealed damage is intentionally private.
                detail_str += string_format( "hp damage: %d\n", wd.get_hp_damage() );
                if( wd.wound_progress_timer.has_value() ) {
                    detail_str += string_format( "progresses into: %s, in %s\n", wd.wound_progress_timer->first.str(),
                                                 to_string( wd.wound_progress_timer->second -
                                                            time_duration::from_seconds( wd.get_healing_progress() ) ) );
                }
                for( const wound_mod_values &mv : wd.type->get_modify_values() ) {
                    detail_str += string_format( "mod_value: %s%s, %d every %s\n",
                                                 io::enum_to_string( mv.type ),
                                                 mv.vitamin.has_value() ? string_format( " (%s)", mv.vitamin->str() ) : "",
                                                 mv.amount, to_string( mv.rate ) );
                }
                detail_str += string_format( "%s\n", limb_score_output );
            }
        }
    }
    if( debug_mode ) {
        detail_str += "All potential wounds:\n";
        for( const std::pair<bp_wounds, int> &wd : bp->get_id()->potential_wounds ) {
            std::string damage_types;

            for( const damage_type_id dt : wd.first.damage_type ) {
                damage_types += dt.c_str();
                damage_types += ", ";
            }
            damage_types.pop_back();
            damage_types.pop_back();
            detail_str +=
                string_format( "%s:\nweight: %d\ndamage types required: %s\ndamage required: [ %d - %d ]\n",
                               colorize( wd.first.id.c_str(), c_yellow ), wd.second, damage_types, wd.first.damage_required.first,
                               wd.first.damage_required.second );
        }
    }

    return detail_str;
}

std::string medical_ui::get_limb_scores_and_modifiers( const Character &you,
        const bodypart_id &part )
{
    const bodypart *bp = you.get_part( part );
    std::string detail_str;
    for( const limb_score &sc : limb_score::get_all() ) {
        if( !part->has_limb_score( sc.getId() ) ) {
            continue;
        }

        float injury_score = bp->get_limb_score( you, sc.getId() );
        float max_score = part->get_limb_score( sc.getId() );

        if( !debug_mode ) {
            std::pair<std::string, nc_color> score_c;
            if( injury_score < max_score * 0.4f ) {
                score_c.first = _( "Crippled" );
                score_c.second = c_red;
            } else if( injury_score < max_score * 0.6f ) {
                score_c.first = _( "Impaired" );
                score_c.second = c_light_red;
            } else if( injury_score < max_score * 0.75f ) {
                score_c.first = _( "Weakened" );
                score_c.second = c_yellow;
            } else if( injury_score < max_score * 0.9f ) {
                score_c.first = _( "Weakened" );
                score_c.second = c_dark_gray;
            } else {
                score_c.first = _( "OK" );
                score_c.second = c_dark_gray;
            }
            detail_str += string_format( _( "%s: %s\n" ), sc.name().translated(), colorize( score_c.first,
                                         score_c.second ) );
        } else {
            detail_str += string_format( _( "%s: %.2f/%.2f\n" ), sc.getId().str(), injury_score, max_score );
        }
    }

    return detail_str;
}

void medical_ui::print_debug_wound_info( const Character &you, const bodypart_id &part )
{
    const bodypart *bp = you.get_part( part );

    if( !bp->get_wounds().empty() ) {

        ImGui::BeginTable( "Debug wound info", 8,
                           ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_HighlightHoveredColumn |
                           ImGuiTableFlags_Hideable );
        ImGui::TableSetupColumn( "id" );
        ImGui::TableSetupColumn( "heal time" );
        ImGui::TableSetupColumn( "heal progr" );
        ImGui::TableSetupColumn( "heal %" );
        ImGui::TableSetupColumn( "heal spd mult" );
        ImGui::TableSetupColumn( "pain" );
        ImGui::TableSetupColumn( "damage" );
        ImGui::TableSetupColumn( "misc" );
        ImGui::TableHeadersRow();
        ImGui::TableNextColumn();

        for( const wound &wd : bp->get_wounds() ) {

            // id
            cataimgui::draw_colored_text( wd.type.str() );
            ImGui::TableNextColumn();
            // healing_time
            cataimgui::draw_colored_text( to_string( wd.get_healing_time(), true ) );
            ImGui::TableNextColumn();
            // healing progress
            cataimgui::draw_colored_text( to_string( time_duration::from_seconds(
                                              wd.get_healing_progress() ), true ) );
            ImGui::TableNextColumn();
            // healing percentage
            cataimgui::draw_colored_text( string_format( "%.3f%%", wd.healing_percentage() ) );
            ImGui::TableNextColumn();
            // healing speed multiplier
            cataimgui::draw_colored_text( string_format( "%.3fx", wd.get_wound_healing_modifier( &you ) ) );
            ImGui::TableNextColumn();
            // pain
            cataimgui::draw_colored_text( string_format( "%d (raw %d)", wd.get_pain( &you ),
                                          wd.get_pain_raw() ) );
            ImGui::TableNextColumn();
            // damage
            cataimgui::draw_colored_text( string_format( "%d (raw %d)", wd.get_hp_damage(), wd.hp_damage ) );
            ImGui::TableNextColumn();
            // mics
            cataimgui::draw_colored_text( "[hover]" );
            if( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) ) {
                std::string tooltip;

                if( wd.wound_progress_timer.has_value() ) {
                    tooltip += string_format( "progresses into: %s, in %s\n", wd.wound_progress_timer->first.str(),
                                              to_string( wd.wound_progress_timer->second -
                                                         time_duration::from_seconds( wd.get_healing_progress() ), true ) );
                    tooltip += "-----\n";
                }

                if( !wd.type->get_limb_scores().empty() ) {
                    tooltip += "limb scores applied:\n";
                    for( const wound_limb_score &s : wd.type->get_limb_scores() ) {
                        tooltip += string_format( "%s: %s%s\n", s.score.str(),
                                                  s.value != 0.f ? string_format( "%.2f", s.value ) : "",
                                                  s.mod != 1.f ? string_format( "%.2f", s.mod ) : "" );

                    }
                    tooltip += "-----\n";
                }


                if( !wd.type->get_modify_values().empty() ) {
                    tooltip += "mod_values:\n";
                    for( const wound_mod_values &mv : wd.type->get_modify_values() ) {
                        tooltip += string_format( "%s%s, %d every %s\n",
                                                  io::enum_to_string( mv.type ),
                                                  mv.vitamin.has_value() ? string_format( " (%s)", mv.vitamin->str() ) : "",
                                                  mv.amount,
                                                  to_string( mv.rate ) );
                    }
                    tooltip += "-----\n";
                }


                ImGui::SetTooltip( "%s", tooltip.c_str() );
            }

            ImGui::TableNextColumn();
        }
        ImGui::EndTable();

        ImGui::Separator();
    }

    if( !bp->get_id()->potential_wounds.empty() ) {

        ImGui::BeginTable( "Possible wounds", 4,
                           ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_HighlightHoveredColumn |
                           ImGuiTableFlags_Hideable );
        ImGui::TableSetupColumn( "id" );
        ImGui::TableSetupColumn( "weight" );
        ImGui::TableSetupColumn( "damage type" );
        ImGui::TableSetupColumn( "damage amt" );
        ImGui::TableHeadersRow();
        ImGui::TableNextColumn();

        for( const std::pair<bp_wounds, int> &wd : bp->get_id()->potential_wounds ) {
            std::string damage_types;

            // id
            cataimgui::draw_colored_text( wd.first.id.str() );
            ImGui::TableNextColumn();
            // weight
            cataimgui::draw_colored_text( std::to_string( wd.second ) );
            ImGui::TableNextColumn();
            // damage type
            cataimgui::draw_colored_text( enumerate_as_string( wd.first.damage_type, [](
            const damage_type_id & dt ) {
                return dt.str();
            },
            enumeration_conjunction::none ) );
            ImGui::TableNextColumn();
            // damage amt
            cataimgui::draw_colored_text(
                string_format( "%d - %d", wd.first.damage_required.first, wd.first.damage_required.second ) );
            ImGui::TableNextColumn();
        }
        ImGui::EndTable();
    }
}

static std::string hp_feeling( const float cur_hp_pcnt )
{
    if( cur_hp_pcnt < 0.125f ) {
        return colorize( _( "Very Bad" ), c_red );
    } else if( cur_hp_pcnt < 0.375f ) {
        return colorize( _( "Bad" ), c_light_red );
    } else if( cur_hp_pcnt < 0.625f ) {
        return colorize( _( "So-so" ), c_yellow );
    } else if( cur_hp_pcnt < 0.875f ) {
        return colorize( _( "Okay" ), c_light_green );
    } else {
        return colorize( _( "Good" ), c_green );
    }
}

std::string a11y::limb_health_description( const Character &who, const bodypart_id &part )
{
    const bool no_feeling = who.has_flag( json_flag_PAIN_IMMUNE );
    if( who.worn_with_flag( flag_SPLINT, part ) ) {
        if( no_feeling ) {
            return _( "Splinted.  Mending progress is unknown." );
        }
        const effect &mending = who.get_effect( effect_mending, part );
        const int percent = mending.is_null() || mending.get_max_duration() <= 0_turns ? 0 :
                            100 * mending.get_duration() / mending.get_max_duration();
        return string_format( _( "Splinted.  Mending indicator %d of 5." ), std::clamp( percent / 20, 0, 5 ) );
    }
    if( who.is_limb_broken( part ) ) {
        return _( "Broken." );
    }
    const int maximum = who.get_part_hp_max( part );
    const int current = who.get_part_hp_cur( part );
    if( no_feeling ) {
        return remove_color_tags( hp_feeling( maximum > 0 ? current / static_cast<float>( maximum ) : 0 ) );
    }
    const std::string bar = get_hp_bar( current, maximum, false ).first;
    const int full = static_cast<int>( std::count( bar.begin(), bar.end(), '|' ) );
    std::string result = string_format( _( "Health indicator: %d full segments of 5." ), full );
    if( bar.find( '\\' ) != std::string::npos ) {
        result += " " + std::string( _( "One additional half segment." ) );
    } else if( bar.find( ':' ) != std::string::npos ) {
        result += " " + std::string( _( "Less than half a segment remains." ) );
    }
    return result;
}

std::string medical_ui::get_modified_limb_name( const Character &you,
        const bodypart_id &part )
{
    std::string header; // Bodypart Title
    std::string hp_str; // Bodypart HP, later merged into the header
    std::string detail; // additional info, like [ BLEEDING ]

    const int bleed_intensity = you.get_effect_int( effect_bleed, part );
    const bool bleeding = bleed_intensity > 0;
    const bool bitten = you.has_effect( effect_bite, part.id() );
    const bool infected = you.has_effect( effect_infected, part.id() );
    const bool wounded = you.has_wounds( part.id() );
    const bool no_feeling = you.has_flag( json_flag_PAIN_IMMUNE );
    const int maximal_hp = you.get_part_hp_max( part );
    const int current_hp = you.get_part_hp_cur( part );
    const bool limb_is_broken = you.is_limb_broken( part );
    const bool limb_is_mending = you.worn_with_flag( flag_SPLINT, part );

    if( limb_is_mending ) {
        detail += string_format( _( "[ %s ]" ), colorize( _( "SPLINTED" ), c_yellow ) );
        if( no_feeling ) {
            hp_str = colorize( "==%==", c_blue );
        } else {
            const effect &eff = you.get_effect( effect_mending, part );
            const int mend_perc = eff.is_null() ? 0.0 : 100 * eff.get_duration() / eff.get_max_duration();

            const int num = mend_perc / 20;
            hp_str = colorize( std::string( num, '#' ) + std::string( 5 - num, '=' ), c_blue );
        }
    } else if( limb_is_broken ) {
        detail += string_format( _( "[ %s ]" ), colorize( _( "BROKEN" ), c_red ) );
        hp_str = "==%==";
    } else if( no_feeling ) {
        const float cur_hp_pcnt = current_hp / static_cast<float>( maximal_hp );
        hp_str = hp_feeling( cur_hp_pcnt );
    } else {
        std::pair<std::string, nc_color> h_bar = get_hp_bar( current_hp, maximal_hp, false );
        hp_str = colorize( h_bar.first, h_bar.second ) +
                 colorize( std::string( 5 - utf8_width( h_bar.first ), '.' ), c_white );
    }
    const std::string bp_name = uppercase_first_letter( body_part_name( part, 1 ) );
    header += colorize( bp_name,
                        display::limb_color( you,
                                part, true, true, true ) ) + " " + hp_str;

    // BLEEDING block
    if( bleeding ) {
        const nc_color bleeding_color = colorize_bleeding_intensity( bleed_intensity );
        detail += string_format( _( "[ %s ]" ), colorize( _( "BLEEDING" ), bleeding_color ) );
    }

    // BITTEN block
    if( bitten ) {
        detail += string_format( _( "[ %s ]" ), colorize( _( "BITTEN" ), c_yellow ) );
    }

    // INFECTED block
    if( infected ) {
        detail += string_format( _( "[ %s ]" ), colorize( _( "SEPTIC" ), c_pink ) );
    }

    if( wounded ) {
        detail += string_format( _( "[ %s ]" ), colorize( _( "WOUNDED" ), c_cyan ) );
    }

    return string_format( "%s %s", header, detail );

}

static std::string draw_speed_summary( const Character &you )
{
    std::string desc;

    const int runcost = you.run_cost( 100 );
    const int newmoves = you.get_speed();

    const std::string runcost_colored = colorize( string_format( _( "%d" ), runcost ),
                                        ( runcost <= 100 ? c_green : c_red ) );
    desc += string_format( _( "Base Move Cost: %s" ), runcost_colored );
    desc += "\n";

    const std::string speed_colored = colorize( string_format( _( "%d" ), newmoves ),
                                      ( newmoves >= 100 ? c_green : c_red ) );
    desc += string_format( _( "Current Speed: %s" ), speed_colored );
    desc += "\n";

    const int speed_modifier = you.get_enchantment_speed_bonus();

    std::string pge_str;
    if( speed_modifier != 0 ) {
        pge_str = pgettext( "speed bonus", "Bio/Mut/Effects" );
        desc += colorize( string_format( _( "%s: -%2d%%\n" ), pge_str, speed_modifier ),
                          c_green );
    }

    int pen = 0;
    if( you.weight_carried() > you.weight_capacity() ) {
        pen = 25 * ( you.weight_carried() - you.weight_capacity() ) / you.weight_capacity();
        pge_str = pgettext( "speed penalty", "Overburdened" );
        desc += colorize( string_format( _( "%s: -%2d%%" ), pge_str, pen ), c_red );
        desc += "\n";
    }

    pen = you.ppen_spd;
    if( pen >= 1 ) {
        pge_str = pgettext( "speed penalty", "Pain" );
        desc += colorize( string_format( _( "%s: -%2d%%" ), pge_str, pen ), c_red );
        desc += "\n";
    }
    if( you.get_thirst() > 40 ) {
        pen = std::abs( Character::thirst_speed_penalty( you.get_thirst() ) );
        pge_str = pgettext( "speed penalty", "Thirst" );
        desc += colorize( string_format( _( "%s: -%2d%%" ), pge_str, pen ), c_red );
        desc += "\n";
    }
    if( you.kcal_speed_penalty() < 0 ) {
        pen = std::abs( you.kcal_speed_penalty() );
        pge_str = pgettext( "speed penalty", you.get_bmi() < character_weight_category::underweight ?
                            "Starving" : "Underfed" );
        desc += colorize( string_format( _( "%s: -%2d%%" ), pge_str, pen ), c_red );
        desc += "\n";
    }
    if( you.has_trait( trait_SUNLIGHT_DEPENDENT ) && !g->is_in_sunlight( you.pos_bub() ) ) {
        pen = ( g->light_level( you.posz() ) >= 12 ? 5 : 10 );
        pge_str = pgettext( "speed penalty", "Out of Sunlight" );
        desc += colorize( string_format( _( "%s: -%2d%%" ), pge_str, pen ), c_red );
        desc += "\n";
    }

    int speed_effects = 0;
    for( const effect &elem : you.get_effects() ) {
        bool reduced = you.resists_effect( elem );
        int move_adjust = elem.get_mod( "SPEED", reduced );
        if( move_adjust != 0 ) {
            speed_effects += move_adjust;
        }
    }

    if( speed_effects > 0 ) {
        nc_color col = speed_effects > 0 ? c_green : c_red;
        desc += colorize( string_format( _( "Effects: %s%d%%" ), ( speed_effects > 0 ? "+" : "-" ),
                                         std::abs( speed_effects ) ), col );
    }

    return desc;
}

static std::string draw_stats_summary( const Character &you )
{

    std::string desc;

    std::string strength_str = coloured_stat_display( you.get_str(), you.get_str_base() );
    desc += string_format( _( "Strength: %s" ), strength_str );
    desc += "\n";

    std::string dexterity_str = coloured_stat_display( you.get_dex(), you.get_dex_base() );
    desc += string_format( _( "Dexterity: %s" ), dexterity_str );
    desc += "\n";

    std::string intelligence_str = coloured_stat_display( you.get_int(), you.get_int_base() );
    desc += string_format( _( "Intelligence: %s" ), intelligence_str );
    desc += "\n";

    std::string perception_str = coloured_stat_display( you.get_per(), you.get_per_base() );
    desc += string_format( _( "Perception: %s" ), perception_str );

    return desc;
}
void medical_ui::summary_tab( const Character &you )
{
    const float col_width = ImGui::GetContentRegionAvail().x;

    cataimgui::draw_colored_text( draw_speed_summary( you ), col_width );

    ImGui::Separator();

    cataimgui::draw_colored_text( draw_stats_summary( you ), col_width );

    const diag_value *last_weighting_time = you.maybe_get_value( "last_weighting_time" );
    if( last_weighting_time != nullptr ) {
        const std::string last_weighting_weight = string_format( "%.0f %s",
                convert_weight( units::from_kilogram( you.get_value( "last_weighting_weight_kg" ).dbl() ) ),
                weight_units() );
        const std::string last_weighted_time = string_format( _( "last weighed %s ago." ),
                                               to_string_approx( calendar::turn - time_point( last_weighting_time->dbl() ) ) );
        const std::string weight_desc = string_format(
                                            "%s: %s (%s)",
                                            _( "Weight" ),
                                            colorize( last_weighting_weight, c_yellow ),
                                            colorize( last_weighted_time, c_dark_gray ) );
        cataimgui::draw_colored_text( weight_desc, col_width );
    } else {
        cataimgui::draw_colored_text( _( "You do not remember the last time you weighed yourself." ),
                                      c_dark_gray, col_width );
    }

    if( debug_mode ) {
        ImGui::NewLine();
        ImGui::Separator();
        cataimgui::draw_colored_text( "Debug:", c_red );
        std::string txt;
        txt += string_format( "bodyweight: %.2f kg\n", units::to_kilogram( you.bodyweight() ) );
        const units::mass wornWeight = you.worn.weight();
        txt += string_format( "worn: %.2f kg\n", units::to_kilogram( wornWeight ) );

        const item_location &wield = you.get_wielded_item();
        if( wield ) {
            txt += string_format( "wielded: %.2f kg\n", units::to_kilogram( wield->weight() ) );
        }
        txt += string_format( "bionic: %.2f kg\n", units::to_kilogram( you.bionics_weight() ) );

        cataimgui::draw_colored_text( txt, col_width );
    }

}

void medical_ui::draw_limbs_details( const Character &you, const bodypart_id &bp )
{
    ImGui::BeginChild( "##Limb_Info" );

    cataimgui::scroll s = cataimgui::scroll::none;
    cataimgui::set_scroll( s );

    const int table_width = ImGui::GetContentRegionAvail().x;

    ImGui::SeparatorText( _( "Limb Effects:" ) );
    const std::string limb_effects = get_limb_effects( you, bp );
    if( !limb_effects.empty() ) {
        cataimgui::draw_colored_text( limb_effects, c_unset, table_width );
        ImGui::Separator();
    }

    ImGui::SeparatorText( _( "Limb Wounds:" ) );
    const std::string limb_wounds = get_limb_wounds( you, bp );
    if( !limb_wounds.empty() ) {
        cataimgui::draw_colored_text( limb_wounds, c_unset, table_width );
        ImGui::Separator();
    }

    if( debug_mode ) {
        print_debug_wound_info( you, bp );
    }

    ImGui::SeparatorText( _( "Limb Scores/Modifiers:" ) );
    const std::string limb_scores = get_limb_scores_and_modifiers( you, bp );
    if( !limb_scores.empty() ) {
        cataimgui::draw_colored_text( limb_scores, c_unset, table_width );
    }

    ImGui::EndChild();
}

namespace
{
class accessible_medical_ui : public cataimgui::window
{
    public:
        explicit accessible_medical_ui( Character *guy ) : cataimgui::window( _( "Medical" ),
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav ) {
            you = guy;
        };

        bool execute();
        a11y::semantic_snapshot reader_snapshot() const;
        std::optional<std::string> reader_intent( const a11y::semantic_intent &intent );

        void draw_limbs_list( const std::vector<bodypart_id> &bodyparts );
        void limb_tab( const std::vector<bodypart_id> &bodyparts );

        void draw_hint_section();

        std::string last_action;
    protected:

        void draw_controls() override;
        cataimgui::bounds get_bounds() override {

            const float window_width = std::clamp( float( str_width_to_pixels( EVEN_MINIMUM_TERM_WIDTH ) ),
                                                   ImGui::GetMainViewport()->Size.x / 2,
                                                   ImGui::GetMainViewport()->Size.x );
            const float window_height = std::clamp( float( str_height_to_pixels( EVEN_MINIMUM_TERM_HEIGHT ) ),
                                                    ImGui::GetMainViewport()->Size.y / 2,
                                                    ImGui::GetMainViewport()->Size.y );

            const cataimgui::bounds bounds{ -1.f, -1.f, window_width, window_height };
            return bounds;
        }

    private:
        Character *you;
        bodypart_id bp;
        input_context ctxt;
        cataimgui::scroll s = cataimgui::scroll::none;
        medical_tab_mode selected_tab = medical_tab_mode::TAB_LIMBS;

        float get_table_column_width() const {
            return ImGui::GetWindowSize().x / 2;
        }
};
} // namespace

bool Character::disp_medical()
{
    accessible_medical_ui medic( this );
    return medic.execute();
}

bool accessible_medical_ui::execute()
{
    ctxt.register_action( "UP", to_translation( "Move cursor up" ) );
    ctxt.register_action( "DOWN", to_translation( "Move cursor down" ) );
    ctxt.register_action( "SCROLL_UP", to_translation( "Move cursor up" ) );
    ctxt.register_action( "SCROLL_DOWN", to_translation( "Move cursor down" ) );
    ctxt.register_action( "PAGE_UP", to_translation( "Scroll description up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Scroll description down" ) );
    ctxt.register_action( "HOME", to_translation( "Scroll description to top" ) );
    ctxt.register_action( "END", to_translation( "Scroll description to bottom" ) );
    ctxt.register_leftright();
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "APPLY", to_translation( "Use item" ) );
    ctxt.register_action( "CONFIRM", to_translation( "Pick bodypart to be mended" ) );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.set_timeout( 16 );

    a11y::screen_scope reader( &ctxt, "medical",
    [this]( const a11y::semantic_intent &intent ) { return reader_intent( intent ); } );

    while( true ) {
        ui_manager::redraw_invalidated();
        if( a11y::enabled() ) {
            reader.update( reader_snapshot() );
        }
        last_action = ctxt.handle_input();
        if( last_action == "QUIT" || !get_is_open() ) {
            break;
        }

        // creating a new window within main loop of draw_controls() causes destructor to destroy wrong window
        // so moved here, outside of draw_controls()
        if( last_action == "CONFIRM" ) {
            if( !you->has_part( bp ) ) {
                if( a11y::enabled() ) {
                    add_msg( m_info, _( "No body part is selected." ) );
                } else {
                    popup( _( "No body part is selected." ) );
                }
                continue;
            }
            if( you->pick_wound_fix( bp ) ) {
                if( a11y::enabled() ) {
                    add_msg( m_info, _( "Treatment selected for %s. "
                                         "Follow the game's treatment progress and result messages." ),
                             body_part_name( bp ) );
                }
                return true;
            }
        }

        if( last_action == "APPLY" ) {
            if( you->is_avatar() ) {
                avatar_action::use_item( *you->as_avatar() );
            } else {
                popup( _( "Applying not implemented for NPCs." ) );
            }
        }

    }
    return false;
}


void accessible_medical_ui::draw_limbs_list( const std::vector<bodypart_id> &bodyparts )
{
    if( ImGui::BeginListBox( "##LISTBOX", ImGui::GetContentRegionAvail() ) ) {

        for( size_t i = 0; i < bodyparts.size(); i++ ) {
            const bodypart_id &part = bodyparts[i];
            const bool is_selected = bp == part;
            ImGui::PushID( i );

            if( ImGui::Selectable( "", is_selected ) ) {
                // if true, the option was selected, so set bp to what was selected
                bp = part;
            }

            if( is_selected ) {
                ImGui::SetScrollHereY();
                ImGui::SetItemDefaultFocus();
            }
            ImGui::SameLine();
            cataimgui::draw_colored_text( medical_ui::get_modified_limb_name( *you, part ), ImGui::GetContentRegionAvail().x );

            ImGui::PopID();
        }
        ImGui::EndListBox();
    }
}

void accessible_medical_ui::limb_tab( const std::vector<bodypart_id> &bodyparts )
{
    if( ImGui::BeginTable( "##Limbs", 2, ImGuiTableFlags_SizingFixedFit ) ) {
        ImGui::TableSetupColumn( _( "Limbs" ), ImGuiTableColumnFlags_WidthFixed,
                                 get_table_column_width() * 0.7 );

        const std::string limb_name = uppercase_first_letter( body_part_name( bp, 1 ) );
        ImGui::TableSetupColumn( limb_name.c_str(), ImGuiTableColumnFlags_WidthStretch );
        ImGui::TableHeadersRow();
        ImGui::TableNextColumn();
        draw_limbs_list( bodyparts );
        ImGui::TableNextColumn();
        ImGui::BeginChild( "##Limb_Info" );

        cataimgui::set_scroll( s );

        const int table_width = ImGui::GetContentRegionAvail().x;

        const std::string limb_effects = medical_ui::get_limb_effects( *you, bp );
        if( !limb_effects.empty() ) {
            cataimgui::draw_colored_text( limb_effects, c_unset, table_width );
            ImGui::Separator();
        }

        const std::string limb_wounds = medical_ui::get_limb_wounds( *you, bp );
        if( !limb_wounds.empty() ) {
            cataimgui::draw_colored_text( limb_wounds, c_unset, table_width );
            ImGui::Separator();
        }

        const std::string limb_scores = medical_ui::get_limb_scores_and_modifiers( *you, bp );
        if( !limb_scores.empty() ) {
            cataimgui::draw_colored_text( limb_scores, c_unset, table_width );
        }

        ImGui::EndChild();
        ImGui::EndTable();
    }
}

void accessible_medical_ui::draw_hint_section()
{
    const std::string desc = string_format(
                                 _( "[<color_yellow>%s</color>] Use item [<color_yellow>%s</color>] Treat, [<color_yellow>%s</color>] Keybindings" ),
                                 ctxt.get_desc( "APPLY" ), ctxt.get_desc( "CONFIRM" ), ctxt.get_desc( "HELP_KEYBINDINGS" ) );
    cataimgui::draw_colored_text( desc, c_unset );
    ImGui::Separator();
}

void accessible_medical_ui::draw_controls()
{
    // no need to draw anything if we are exiting
    if( last_action == "QUIT" ) {
        return;
    } else if( last_action == "NEXT_TAB" || last_action == "RIGHT" ) {
        s = cataimgui::scroll::begin;
        ++selected_tab;
    } else if( last_action == "PREV_TAB" || last_action == "LEFT" ) {
        s = cataimgui::scroll::begin;
        --selected_tab;
    }

    const std::vector<bodypart_id> bodyparts = you->get_all_body_parts( get_body_part_flags::sorted );
    size_t num_entries = bodyparts.size();
    if( num_entries == 0 ) {
        medical_ui::summary_tab( *you );
        return;
    }
    const int last_entry = num_entries == 0 ? 0 : ( static_cast<int>( num_entries ) - 1 );

    // initialize selected_limb either from already set bp, or from zero if not set
    auto it = std::find( bodyparts.begin(), bodyparts.end(), bp );
    int selected_limb = ( it != bodyparts.end() ) ? std::distance( bodyparts.begin(), it ) : 0;
    if( last_action == "UP" ) {
        selected_limb--;
    } else if( last_action == "DOWN" ) {
        selected_limb++;
    }

    if( last_action == "PAGE_UP" || last_action == "SCROLL_UP" ) {
        s = cataimgui::scroll::page_up;
    } else if( last_action == "PAGE_DOWN" || last_action == "SCROLL_DOWN" ) {
        s = cataimgui::scroll::page_down;
    } else if( last_action == "HOME" ) {
        s = cataimgui::scroll::begin;
    } else if( last_action == "END" ) {
        s = cataimgui::scroll::end;
    }

    if( selected_limb < 0 ) {
        selected_limb = last_entry;
    } else if( selected_limb > last_entry ) {
        selected_limb = 0;
    };

    bp = bodyparts[selected_limb];

    draw_hint_section();

    if( ImGui::BeginTabBar( "##Tabs" ) ) {
        if( cataimgui::BeginTabItem( _( "Limbs" ), selected_tab == medical_tab_mode::TAB_LIMBS ) ) {
            // if we picked the tab via mouse imput, update selected_tab
            // TODO there seems to be some lag appearing when you pick it with mouse?
            selected_tab = ImGui::IsItemActive() ? medical_tab_mode::TAB_LIMBS : selected_tab;
            limb_tab( bodyparts );
            ImGui::EndTabItem();
        }
        if( cataimgui::BeginTabItem( _( "Summary" ), selected_tab == medical_tab_mode::TAB_SUMMARY ) ) {
            selected_tab = ImGui::IsItemActive() ? medical_tab_mode::TAB_SUMMARY : selected_tab;
            medical_ui::summary_tab( *you );
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

a11y::semantic_snapshot accessible_medical_ui::reader_snapshot() const
{
    a11y::semantic_snapshot snapshot = a11y::command_snapshot( ctxt, _( "Medical" ) );
    snapshot.summary = selected_tab == medical_tab_mode::TAB_LIMBS ? _( "Limbs" ) : _( "Summary" );
    // Urgent conditions are announced before the full limb list. A status
    // estimate must never expose exact hit points or bypass treatment checks.
    const auto urgency = [this]( const bodypart_id &part ) {
        if( you->has_effect( effect_infected, part ) ) {
            return 5;
        }
        if( you->has_effect( effect_bleed, part ) ) {
            return 4;
        }
        if( you->is_limb_broken( part ) ) {
            return 3;
        }
        if( you->has_effect( effect_bite, part ) ) {
            return 2;
        }
        return you->has_wounds( part.id() ) ? 1 : 0;
    };
    std::vector<bodypart_id> urgent_first = you->get_all_body_parts( get_body_part_flags::sorted );
    std::stable_sort( urgent_first.begin(), urgent_first.end(),
    [&]( const bodypart_id &a, const bodypart_id &b ) {
        return urgency( a ) > urgency( b );
    } );
    int attention_count = 0;
    for( const bodypart_id &part : urgent_first ) {
        if( urgency( part ) == 0 ) {
            break;
        }
        ++attention_count;
        snapshot.summary += "\n" + string_format( _( "Needs attention: %1$s. %2$s" ),
                            body_part_name( part ), a11y::limb_health_description( *you, part ) );
    }
    if( attention_count == 0 ) {
        snapshot.summary += "\n" + std::string( _( "No reported wounds need attention." ) );
    } else {
        snapshot.summary += "\n" + std::string(
                                _( "Select a limb to see treatments; inspecting alone does not treat it." ) );
    }
    for( const auto &tab : { std::make_pair( "medical:tab:limbs", _( "Limbs" ) ),
                            std::make_pair( "medical:tab:summary", _( "Summary" ) ) } ) {
        a11y::semantic_node node;
        node.id = tab.first;
        node.role = "tab";
        node.label = tab.second;
        node.selected = ( selected_tab == medical_tab_mode::TAB_LIMBS ) ==
                        ( node.id == "medical:tab:limbs" );
        snapshot.nodes.push_back( std::move( node ) );
    }
    // Table contract (R02.2): limbs form one table whose columns are
    // declared once here. Condition text follows the medical knowledge
    // policy (coarse descriptions, never exact hit points), and cells are
    // presentation only -- selecting a row still uses the row node's own
    // focus/activate path in reader_intent, never a per-cell action.
    a11y::semantic_table limbs_table;
    limbs_table.id = "medical:limbs";
    limbs_table.columns = { { "part", _( "Body part" ), "" },
                            { "condition", _( "Condition" ), "" },
                            { "flags", _( "Status" ), "" } };
    snapshot.tables.push_back( limbs_table );
    const auto &parts = urgent_first;
    for( std::size_t i = 0; i < parts.size(); ++i ) {
        const bodypart_id &part = parts[i];
        a11y::semantic_node node;
        node.id = "medical:limb:" + part.id().str();
        node.label = body_part_name( part );
        const int priority = urgency( part );
        if( priority >= 3 ) {
            node.label = string_format( _( "Priority injury: %s" ), node.label );
        }
        const std::string condition = a11y::limb_health_description( *you, part );
        node.value = condition;
        std::string flags;
        const auto append_flag = [&flags]( const std::string &flag ) {
            if( !flags.empty() ) {
                flags += ", ";
            }
            flags += flag;
        };
        if( you->has_effect( effect_bleed, part ) ) {
            node.value += " " + std::string( _( "BLEEDING" ) );
            append_flag( _( "BLEEDING" ) );
        }
        if( you->has_effect( effect_bite, part ) ) {
            node.value += " " + std::string( _( "BITTEN" ) );
            append_flag( _( "BITTEN" ) );
        }
        if( you->has_effect( effect_infected, part ) ) {
            node.value += " " + std::string( _( "SEPTIC" ) );
            append_flag( _( "SEPTIC" ) );
        }
        if( you->has_wounds( part.id() ) ) {
            node.value += " " + std::string( _( "WOUNDED" ) );
            append_flag( _( "WOUNDED" ) );
        }
        node.table_id = "medical:limbs";
        node.row_index = static_cast<int>( i );
        node.cells = { { "part", node.label, "", "" },
                       { "condition", condition, "", "" },
                       { "flags", flags, "", "" } };
        node.detail = remove_color_tags( medical_ui::get_limb_effects( *you, part ) + "\n" +
                                        medical_ui::get_limb_wounds( *you, part ) + "\n" + medical_ui::get_limb_scores_and_modifiers( *you, part ) );
        node.selected = bp == part;
        node.position = i + 1;
        node.total = parts.size();
        if( node.selected && selected_tab == medical_tab_mode::TAB_LIMBS ) {
            snapshot.focus = node.id;
        }
        snapshot.nodes.push_back( std::move( node ) );
    }
    std::string summary = draw_speed_summary( *you ) + "\n" + draw_stats_summary( *you );
    const diag_value *weighing = you->maybe_get_value( "last_weighting_time" );
    if( weighing ) {
        const std::string weight = string_format( "%.0f %s", convert_weight(
                                      units::from_kilogram( you->get_value( "last_weighting_weight_kg" ).dbl() ) ),
                                      weight_units() );
        summary += "\n" + std::string( _( "Weight" ) ) + ": " + weight + ". " +
                   string_format( _( "last weighed %s ago." ),
                                  to_string_approx( calendar::turn - time_point( weighing->dbl() ) ) );
    } else {
        summary += "\n" + std::string( _( "You do not remember the last time you weighed yourself." ) );
    }
    a11y::append_document( snapshot, "medical:summary", _( "Summary" ), summary );
    if( selected_tab == medical_tab_mode::TAB_SUMMARY ) {
        snapshot.focus = "medical:summary";
    }
    return snapshot;
}

std::optional<std::string> accessible_medical_ui::reader_intent( const a11y::semantic_intent &intent )
{
    if( intent.action == "activate" &&
        ( intent.node == "medical:tab:limbs" || intent.node == "medical:tab:summary" ) ) {
        selected_tab = intent.node == "medical:tab:limbs" ? medical_tab_mode::TAB_LIMBS :
                       medical_tab_mode::TAB_SUMMARY;
        s = cataimgui::scroll::begin;
        return "TIMEOUT";
    }
    for( const bodypart_id &part : you->get_all_body_parts( get_body_part_flags::sorted ) ) {
        if( intent.node == "medical:limb:" + part.id().str() &&
            ( intent.action == "focus" || intent.action == "activate" ) ) {
            bp = part;
            selected_tab = medical_tab_mode::TAB_LIMBS;
            s = cataimgui::scroll::begin;
            // Inspection never starts treatment. The original wound menu and
            // its skill/proficiency/material checks run only on activation.
            return intent.action == "activate" ? "CONFIRM" : "TIMEOUT";
        }
    }
    return a11y::registered_intent( ctxt, intent );
}

namespace
{
struct healing_option {
    wound_type_id wd;
    wound_fix_id fix;
    bool doable;
    time_duration time_to_apply;
    int amount = 1;
};
} // namespace

bool Character::pick_wound_fix( const bodypart_id &bp_id )
{
    const auto report_treatment_issue = [this]( const std::string &message ) {
        if( is_avatar() && a11y::enabled() ) {
            add_msg_if_player( m_info, "%s", message );
        } else {
            popup( message );
        }
    };
    std::vector<healing_option> healing_options;
    const temp_crafting_inventory &inv = crafting_inventory();

    bodypart *bp = get_part( bp_id );
    const std::vector<wound> wounds = bp->get_wounds();

    if( debug_mode || has_trait( trait_DEBUG_HS ) ) {
        if( query_yn( "Open debug insertion and removing wounds?" ) ) {
            uilist menu;
            do {
                menu.reset();
                menu.text = "Toggle which wound?";
                std::vector<std::pair<wound_type_id, bool>> opts;
                for( const auto& [wound, weight] : bp_id->potential_wounds ) {
                    opts.emplace_back( wound.id, bp->has_wound( wound.id ) );
                    menu.addentry( -1, true, -1, string_format( opts.back().second ? "Treat: %s" : "Set: %s",
                                   wound.id.str() ) );
                }
                if( opts.empty() ) {
                    popup( string_format( "The %s doesn't have any wounds to toggle.", bp_id->name ) );
                    return false;
                }
                menu.query();
                if( menu.ret >= 0 ) {
                    if( opts[menu.ret].second ) {
                        bp->remove_wound( opts[menu.ret].first );
                    } else {
                        bp->add_wound( opts[menu.ret].first, *this, 0 );
                    }
                }
            } while( menu.ret >= 0 );
            return false;
        }
    }

    for( const wound &wd : wounds ) {
        for( const wound_fix_id &fix_id : wd.type->fixes ) {
            healing_option opt{wd.type, fix_id, true, fix_id->time};

            for( const wound_skill &req : fix_id->skills ) {
                const bool has_skill = get_greater_skill_or_knowledge_level( req.skill ) >= req.level;
                if( req.is_mandatory && !has_skill ) {
                    opt.doable = false;
                } else if( has_skill ) {
                    opt.time_to_apply *= req.time_save;
                }
            }

            for( const wound_proficiency &w_prof : fix_id->proficiencies ) {

                const bool has_prof = has_proficiency( w_prof.prof );

                if( w_prof.is_mandatory && !has_prof ) {
                    opt.doable = false;
                }

                if( has_prof ) {
                    opt.time_to_apply *= w_prof.time_save;
                }
            }

            opt.doable &= fix_id->get_requirements().can_make_with_inventory( this, inv,
                          is_crafting_component );

            // Group only the same wound type and treatment. A larger batch
            // must be affordable before it is presented as an enabled option.
            const auto same_option = [&opt]( const healing_option &stored ) {
                return stored.wd == opt.wd && stored.fix == opt.fix && stored.doable == opt.doable;
            };
            auto existing = std::find_if( healing_options.begin(), healing_options.end(), same_option );
            if( existing != healing_options.end() && opt.doable &&
                !fix_id->get_requirements().can_make_with_inventory( this, inv,
                        is_crafting_component, existing->amount + opt.amount ) ) {
                opt.doable = false;
                existing = std::find_if( healing_options.begin(), healing_options.end(), same_option );
            }
            if( existing != healing_options.end() ) {
                existing->amount += opt.amount;
                existing->time_to_apply += opt.time_to_apply;
            } else {
                healing_options.emplace_back( opt );
            }
        }
    }

    if( healing_options.empty() ) {
        if( bp->get_wounds().empty() ) {
            if( bp_id->has_flag( json_flag_BIONIC_LIMB ) ) {
                // should be popup, but popup() crashes this message, for some reason
                report_treatment_issue( string_format( _( "The %s doesn't have any damage to repair." ), body_part_name( bp_id ) ) );
                return false;
            } else {
                report_treatment_issue( string_format( _( "The %s doesn't have any wounds to treat." ), body_part_name( bp_id ) ) );
                return false;
            }
        } else {
            if( bp_id->has_flag( json_flag_BIONIC_LIMB ) ) {
                report_treatment_issue( string_format( _( "The %s damage cannot be repaired." ), body_part_name( bp_id ) ) );
                return false;
            } else {
                report_treatment_issue( string_format( _( "The %s wounds cannot be treated." ), body_part_name( bp_id ) ) );
                return false;
            }
        }
    }

    uilist menu;
    menu.title = _( "Treat which wound?" );
    menu.desc_enabled = true;

    constexpr int fold_width = 80;

    for( healing_option &opt : healing_options ) {

        const wound_fix &fix = *opt.fix;
        const requirement_data &reqs = fix.get_requirements();
        const nc_color col = opt.doable ? c_white : c_light_gray;

        const std::vector<std::string> tools = reqs.get_folded_tools_list( this, fold_width, col, inv,
                                               opt.amount );
        const std::vector<std::string> comps = reqs.get_folded_components_list( this, fold_width, col, inv,
                                               is_crafting_component, opt.amount );

        std::string descr = word_rewrap( fix.get_description(), 80 ) + "\n\n";

        descr += string_format( _( "Time required: <color_cyan>%s</color>\n" ),
                                to_string_approx( opt.time_to_apply ) );

        if( !fix.skills.empty() ) {
            descr += string_format( _( "Skills: %s\n" ), enumerate_as_string(
            fix.skills.begin(), fix.skills.end(), [&]( const wound_skill &sk ) {
                const int current = static_cast<int>( get_greater_skill_or_knowledge_level( sk.skill ) );
                const nc_color requirement_color = current >= sk.level ? c_green :
                                                   sk.is_mandatory ? c_red : c_light_gray;
                return string_format( pgettext( "skill requirement",
                                                //~ %1$s: skill name, %2$s: current skill level, %3$s: required skill level
                                                "%1$s %2$s" ),
                                      colorize( sk.skill->name(), c_cyan ),
                                      colorize( string_format( "(%1$d/%2$d)", current, sk.level ),
                                                requirement_color ) );
            } ) );
        }


        if( !fix.proficiencies.empty() ) {
            descr += string_format( _( "Proficiencies: %s\n" ), enumerate_as_string(
            fix.proficiencies.begin(), fix.proficiencies.end(), [&]( const wound_proficiency & w_prof ) {
                nc_color col;
                if( has_proficiency( w_prof.prof ) ) {
                    col = c_green;
                } else if( w_prof.is_mandatory ) {
                    col = c_red;
                } else {
                    col = c_light_gray;
                }
                return string_format( colorize( "%1$s", col ), w_prof.prof->name() );
            } ) );
        }

        for( const std::string &line : tools ) {
            descr += line + "\n";
        }
        for( const std::string &line : comps ) {
            descr += line + "\n";
        }
        if( a11y::enabled() ) {
            a11y::semantic_snapshot requirements;
            a11y::append_requirements( requirements, "treatment", reqs, *this, inv, is_crafting_component, opt.amount );
            for( const a11y::semantic_node &node : requirements.nodes ) {
                descr += "\n" + node.label + "\n" + node.detail;
            }
            for( const wound_proficiency &prof : fix.proficiencies ) {
                if( !has_proficiency( prof.prof ) ) {
                    descr += "\n" + string_format( prof.is_mandatory ?
                                                    _( "Missing required proficiency: %s" ) :
                                                    _( "Missing optional proficiency: %s" ), prof.prof->name() );
                }
            }
        }
        const std::string name = opt.amount > 1 ?
                                 string_format( "%s (x%d)", fix.get_name(), opt.amount ) : fix.get_name();
        menu.addentry_desc( -1, true, -1, name, colorize( descr, col ) );
        menu.entries.back().a11y_value = opt.doable ? _( "Treatment requirements are met." ) :
                                          _( "Treatment requirements are not met." );

    }

    menu.query();

    if( menu.ret < 0 ) {
        return false;
    }

    const healing_option &opt = healing_options[menu.ret];
    if( !opt.doable ) {
        add_msg( m_info, _( "You are currently unable to treat the %s this way." ),
                 opt.wd->get_name() );
        return false;
    }

    assign_activity( fix_wound_activity_actor( opt.time_to_apply, bp_id, opt.wd, opt.fix,
                     opt.amount ) );

    return true;
}
