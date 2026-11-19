#pragma once
#ifndef CATA_SRC_PLAYER_DIFFICULTY_H
#define CATA_SRC_PLAYER_DIFFICULTY_H

#include <map>
#include <string>
#include <utility>

#include "npc.h"

class Character;
class avatar;


constexpr int HIGH_STAT = 12;


enum class pool_type {
    FREEFORM = 0,
    TRANSFER = 3,
};

enum class rating_category {
    Defense,
    Combat,
    Genetics,
    Expertise,
    Social
};


enum class OP_level {
    Underpowered,
    Weak,
    Average,
    Strong,
    Powerful,
    Overpowered
};

class player_difficulty
{
    private:
        player_difficulty();
        ~player_difficulty() = default;


        std::pair<OP_level, std::string> get_defense_difficulty( const Character &u );
        std::pair<OP_level, std::string> get_combat_difficulty( const Character &u );
        std::pair<OP_level, std::string> get_genetics_difficulty( const Character &u );
        std::pair<OP_level, std::string> get_expertise_difficulty( const Character &u );
        std::pair<OP_level, std::string> get_social_difficulty( const Character &u );


        static double calc_armor_value( const Character &u );
        static double calc_dps_value( const Character &u );
        static int calc_social_value( const Character &u, const npc &compare );


        static void reset_npc( Character &dummy );
        static void npc_from_avatar( const avatar &u, npc &dummy );

        npc average;

        std::map<rating_category, OP_level> ratings;

    public:
        const npc &get_average_npc();

        player_difficulty( const player_difficulty & ) = delete;
        player_difficulty &operator= ( const player_difficulty & ) = delete;

        static player_difficulty &getInstance() {
            static player_difficulty instance;
            return instance;
        }

        std::map<rating_category, OP_level> get_ratings() const;




        static std::string format_text_for( OP_level overpowered, float per = -9999.9f );


        std::string difficulty_to_string( const avatar &u );
};

#endif
