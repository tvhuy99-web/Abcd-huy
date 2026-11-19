#pragma once
#ifndef CATA_SRC_GUN_MODE_H
#define CATA_SRC_GUN_MODE_H

#include <set>
#include <string>

#include "translations.h"

class item;

class gun_mode
{
    private:
        translation name_;

    public:

        item *target = nullptr;

        int qty = 0;

        std::set<std::string> flags;

        gun_mode() = default;
        gun_mode( const translation &n, item *target, int qty, const std::set<std::string> &flags ) :
            name_( n ),
            target( target ),
            qty( qty ),
            flags( flags ) {}


        bool melee() const {
            return flags.count( "MELEE" );
        }

        explicit operator bool() const {
            return target != nullptr;
        }

        // NOLINTNEXTLINE(readability-make-member-function-const)
        item &operator*() {
            return *target;
        }
        const item &operator*() const {
            return *target;
        }

        // NOLINTNEXTLINE(readability-make-member-function-const)
        item *operator->() {
            return target;
        }
        const item *operator->() const {
            return target;
        }

        std::string tname() const {
            return name_.translated();
        }
};

#endif
