#pragma once
#ifndef CATA_SRC_EMIT_H
#define CATA_SRC_EMIT_H

#include <map>
#include <string>

#include "dialogue_helpers.h"
#include "type_id.h"

class JsonObject;
struct const_dialogue;

class emit
{
    public:
        emit();

        const emit_id &id() const {
            return id_;
        }


        bool is_null() const;


        bool is_valid() const;


        field_type_id field( const_dialogue const &d ) const {
            return field_type_id( field_.evaluate( d ) );
        }


        int intensity( const_dialogue const &d ) const {
            return intensity_.evaluate( d );
        }


        int qty( const_dialogue const &d ) const {
            return qty_.evaluate( d );
        }


        int chance( const_dialogue const &d ) const {
            return chance_.evaluate( d );
        }


        static void load_emit( const JsonObject &jo );


        static const std::map<emit_id, emit> &all();


        static void finalize();


        static void check_consistency();


        static void reset();

    private:
        emit_id id_;
        str_or_var field_;
        dbl_or_var intensity_;
        dbl_or_var qty_;
        dbl_or_var chance_;
};

#endif
