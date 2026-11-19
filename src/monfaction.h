#pragma once
#ifndef CATA_SRC_MONFACTION_H
#define CATA_SRC_MONFACTION_H

#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "type_id.h"

class JsonObject;
class monfaction;
template <typename E> struct enum_traits;
template <typename T> class generic_factory;

enum mf_attitude {
    MFA_BY_MOOD = 0,
    MFA_NEUTRAL,
    MFA_FRIENDLY,
    MFA_HATE,

    MFA_SIZE
};

template<>
struct enum_traits<mf_attitude> {
    static constexpr mf_attitude last = MFA_SIZE;
};

using mfaction_att_map = std::map<mfaction_str_id, mf_attitude>;
using mfaction_att_vec = std::vector<uint8_t>;
static_assert( std::numeric_limits<mfaction_att_vec::value_type>::max() >
               enum_traits<mf_attitude>::last,
               "mfaction_att_vec element size is insufficient to represent all values of mf_attitude" );

namespace monfactions
{
const std::vector<monfaction> &get_all();
void reset();
void finalize();
void load_monster_faction( const JsonObject &jo, const std::string &src );
}

class monfaction
{
    public:


        mf_attitude attitude( const mfaction_id &other ) const;



        bool is_root() const;

        mfaction_str_id id = mfaction_str_id::NULL_ID();
        std::vector<std::pair<mfaction_str_id, mod_id>> src;
        mfaction_str_id base_faction = mfaction_str_id::NULL_ID();

    private:

        bool was_loaded = false;


        mutable mfaction_att_map attitude_map;
        std::set<mfaction_str_id> _att_by_mood;
        std::set<mfaction_str_id> _att_neutral;
        std::set<mfaction_str_id> _att_friendly;
        std::set<mfaction_str_id> _att_hate;




        mutable mfaction_att_vec attitude_vec;



        std::optional<mf_attitude> attitude_rec( const mfaction_str_id &other ) const;
        bool detect_base_faction_cycle( ) const;


        void inherit_parent_attitude_rec(
            std::set<mfaction_str_id> &processed,
            std::map<mfaction_str_id, mf_attitude> &child_attitudes
        ) const;


        void populate_attitude_vec() const;


        void load( const JsonObject &jo, std::string_view src );

        void finalize();

        friend void monfactions::finalize();
        friend class generic_factory<monfaction>;
};

#endif
