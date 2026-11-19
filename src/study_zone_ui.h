#pragma once
#ifndef CATA_SRC_STUDY_ZONE_UI_H
#define CATA_SRC_STUDY_ZONE_UI_H

#include <map>
#include <set>
#include <string>

#include "type_id.h"

enum class study_zone_ui_result {
    canceled,
    successful,
    changed,
};





study_zone_ui_result query_study_zone_skills( std::map<std::string, std::set<skill_id>>
        &npc_skill_preferences );

#endif
