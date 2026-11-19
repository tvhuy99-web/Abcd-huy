#pragma once
#ifndef CATA_SRC_GAMEMODE_H
#define CATA_SRC_GAMEMODE_H

#include <string>
#include <memory>

#include "enums.h"

enum action_id : int;
struct special_game;

std::string special_game_name( special_game_type id );
std::unique_ptr<special_game> get_special_game( special_game_type id );

struct special_game {
    virtual ~special_game() = default;
    virtual special_game_type id() {
        return special_game_type::NONE;
    }

    virtual bool init() {
        return true;
    }

    virtual void per_turn() { }


    virtual void pre_action( action_id & ) { }

    virtual void post_action( action_id ) { }

    virtual void game_over() { }

};

#endif
