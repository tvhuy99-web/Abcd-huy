#pragma once
#ifndef CATA_SRC_NPC_FAVOR_H
#define CATA_SRC_NPC_FAVOR_H

#include "type_id.h"

class JsonObject;
class JsonOut;

enum npc_favor_type {
    FAVOR_NULL,
    FAVOR_GENERAL,
    FAVOR_CASH,
    FAVOR_ITEM,
    FAVOR_TRAINING,
    NUM_FAVOR_TYPES
};

struct npc_favor {
    npc_favor_type type;
    int value;
    itype_id item_id;
    skill_id skill;

    npc_favor() {
        type = FAVOR_NULL;
        value = 0;
        item_id = itype_id::NULL_ID();
        skill = skill_id::NULL_ID();
    }

    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &jo );
};

#endif
