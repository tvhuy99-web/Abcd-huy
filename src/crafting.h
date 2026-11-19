#pragma once
#ifndef CATA_SRC_CRAFTING_H
#define CATA_SRC_CRAFTING_H

#include <list>
#include <optional>
#include <string>
#include <vector>

#include "calendar.h"
#include "item_wakeup.h"

class Character;
class item;
class item_location;
class recipe;
struct attention_plan;
template <typename E> struct enum_traits;

enum class craft_flags : int {
    none = 0,
    start_only = 1,
};

template<>
struct enum_traits<craft_flags> {
    static constexpr bool is_flag_enum = true;
};



void remove_ammo( item &dis_item, Character &p );

void remove_ammo( std::list<item> &dis_items, Character &p );

void drop_or_handle( const item &newit, Character &p );






struct inflight_alarm_choices {
    time_duration remaining;
    bool finish_enabled = false;
    bool five_before_enabled = false;
    std::optional<time_duration> finish_offset;
    std::optional<time_duration> five_before_offset;
};





inflight_alarm_choices compute_inflight_alarm_choices(
    time_point passive_started_at, time_point live_ready_at,
    time_point now );








std::optional<std::vector<attention_plan>> show_craft_planning_modal(
        const recipe &rec, const Character &crafter, int batch, int from_step,
        const std::vector<attention_plan> &existing,
        const item *current_craft = nullptr );





void fire_step_complete_distraction( const std::string &interrupt_msg,
                                     const std::string &no_watch_msg,
                                     const item_location &loc );



std::vector<desired_wakeup> craft_enumerate_scheduled_wakeups(
    const item &craft, const item_location &loc );

void craft_relocated( const item_location &landed );

void craft_actualize_scheduled( item &craft, item_wakeup_kind kind,
                                time_point now, const item_location &loc );





void craft_resolve_overdue_passive( item &craft, time_point now, item_location &loc );




void craft_apply_resume_replan( item_location &loc );



void craft_collect_finalized( item_location &loc );






void craft_stamp_passive_entry( item &craft, const Character &crafter,
                                time_point now, const item_location &loc );

#endif
