#pragma once
#ifndef CATA_SRC_SKILL_UI
#define CATA_SRC_SKILL_UI

#include <vector>

class Skill;




struct HeaderSkill {
    const Skill *skill;
    bool is_header;
    HeaderSkill( const Skill *skill, bool is_header ): skill( skill ), is_header( is_header ) {
    }
};




std::vector<HeaderSkill> get_HeaderSkills( const std::vector<const Skill *> &sorted_skills );




template<typename V>
void skip_skill_headers( const std::vector<HeaderSkill> &skillslist, V &line, bool inc );

#endif
