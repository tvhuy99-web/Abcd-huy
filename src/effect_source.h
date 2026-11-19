#pragma once
#ifndef CATA_SRC_EFFECT_SOURCE_H
#define CATA_SRC_EFFECT_SOURCE_H

#include <optional>

#include "character_id.h"
#include "type_id.h"

class Character;
class Creature;
class JsonObject;
class JsonOut;
class faction;
class monster;



class effect_source
{
    public:
        effect_source() = default;
        explicit effect_source( const monster *mon );
        explicit effect_source( const faction *fac );
        explicit effect_source( const Character *character );
        explicit effect_source( const Creature *creature );



        static effect_source empty();

        std::optional<character_id> get_character_id() const;
        std::optional<faction_id> get_faction_id() const;
        std::optional<mfaction_id> get_mfaction_id() const;




        Creature *resolve_creature() const;

        void serialize( JsonOut & ) const;
        void deserialize( const JsonObject &data );

    private:
        std::optional<character_id> character;
        std::optional<faction_id> fac;
        std::optional<mfaction_id> mfac;
};

#endif
