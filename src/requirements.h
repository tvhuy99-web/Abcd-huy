#pragma once
#ifndef CATA_SRC_REQUIREMENTS_H
#define CATA_SRC_REQUIREMENTS_H

#include <cstdint>
#include <functional>
#include <map>
#include <numeric>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "crafting.h"
#include "translation.h"
#include "type_id.h"

class Character;
class JsonArray;
class JsonObject;
class JsonOut;
class JsonValue;
class item;
class item_components;
class nc_color;
class temp_crafting_inventory;
template <typename E> struct enum_traits;

enum class available_status : int {
    a_true = +1,
    a_false = -1,
    a_insufficient = 0,
};

enum class component_type : int {
    ITEM,
    TOOL,
    QUALITY,
};

struct quality {
    bool was_loaded = false;
    quality_id id;
    std::vector<std::pair<quality_id, mod_id>> src;
    translation name;

    std::vector<std::pair<int, std::string>> usages;

    void load( const JsonObject &jo, std::string_view src );

    static void reset();
    static void load_static( const JsonObject &jo, const std::string &src );
    static void finalize_all();
};


int quality_for_crafter( const item &it, const quality_id &qual, const Character *who,
                         bool strict_boiling );




int provider_quality_level( const item &cand, const quality_id &id, const Character *who,
                            bool strict_boiling );

struct component {
    itype_id type = itype_id::NULL_ID();
    int count = 0;


    mutable available_status available = available_status::a_false;
    bool recoverable = true;

    bool requirement = false;



    bool operator==( const component &rhs ) const {
        return std::forward_as_tuple( type, requirement, count, recoverable )
               == std::forward_as_tuple( rhs.type, rhs.requirement, rhs.count, rhs.recoverable );
    }
    bool operator!=( const component &rhs ) const {
        return !operator==( rhs );
    }

    bool operator<( const component &rhs ) const {

        return std::forward_as_tuple( type.str(), requirement, count, recoverable )
               < std::forward_as_tuple( rhs.type.str(), rhs.requirement, rhs.count, rhs.recoverable );
    }

    component() = default;
    component( const itype_id &TYPE, int COUNT ) : type( TYPE ), count( COUNT ) { }
    component( const itype_id &TYPE, int COUNT, bool RECOVERABLE ) :
        type( TYPE ), count( COUNT ), recoverable( RECOVERABLE ) { }
    void check_consistency( const std::string &display_name ) const;
};

struct tool_comp : public component {
    tool_comp() = default;
    tool_comp( const itype_id &TYPE, int COUNT ) : component( TYPE, COUNT ) { }

    void load( const JsonValue &value );
    void dump( JsonOut &jsout ) const;
    bool has( const Character *actor, const temp_crafting_inventory &crafting_inv,
              const std::function<bool( const item & )> &filter,
              int batch = 1, craft_flags = craft_flags::none,
              const std::function<void( int )> &visitor = std::function<void( int )>() ) const;
    std::string to_string( int batch = 1, int avail = 0 ) const;
    nc_color get_color( const Character *actor, bool has_one,
                        const temp_crafting_inventory &crafting_inv,
                        const std::function<bool( const item & )> &filter, int batch = 1 ) const;
    bool by_charges() const;
    component_type get_component_type() const {
        return component_type::TOOL;
    }
};

struct item_comp : public component {
    item_comp() = default;
    item_comp( const itype_id &TYPE, int COUNT ) : component( TYPE, COUNT ) { }

    void load( const JsonValue &value );
    void dump( JsonOut &jsout ) const;
    bool has( const Character *actor, const temp_crafting_inventory &crafting_inv,
              const std::function<bool( const item & )> &filter,
              int batch = 1, craft_flags = craft_flags::none,
              const std::function<void( int )> &visitor = std::function<void( int )>() ) const;
    std::string to_string( int batch = 1, int avail = 0 ) const;
    nc_color get_color( const Character *actor, bool has_one,
                        const temp_crafting_inventory &crafting_inv,
                        const std::function<bool( const item & )> &filter, int batch = 1 ) const;
    component_type get_component_type() const {
        return component_type::ITEM;
    }
};

struct quality_requirement {
    quality_id type = quality_id( "UNKNOWN" );
    int count = 1;
    int level = 1;
    mutable available_status available = available_status::a_false;
    bool requirement = false;



    bool operator==( const quality_requirement &rhs ) const {
        return std::forward_as_tuple( type, requirement, count, level )
               == std::forward_as_tuple( rhs.type, rhs.requirement, rhs.count, rhs.level );
    }
    bool operator!=( const quality_requirement &rhs ) const {
        return !operator==( rhs );
    }

    bool operator<( const quality_requirement &rhs ) const {
        return std::forward_as_tuple( type, requirement, count, level )
               < std::forward_as_tuple( rhs.type, rhs.requirement, rhs.count, rhs.level );
    }

    quality_requirement() = default;
    quality_requirement( const quality_id &TYPE, int COUNT, int LEVEL ) : type( TYPE ), count( COUNT ),
        level( LEVEL ) { }

    void load( const JsonValue &value );
    void dump( JsonOut &jsout ) const;
    bool has( const Character *actor, const temp_crafting_inventory &crafting_inv,
              const std::function<bool( const item & )> &filter,
              int = 0, craft_flags = craft_flags::none,
              const std::function<void( int )> &visitor = std::function<void( int )>() ) const;
    std::string to_string( int batch = 1, int avail = 0 ) const;
    std::string to_colored_string() const;
    void check_consistency( const std::string &display_name ) const;
    nc_color get_color( const Character *actor, bool has_one,
                        const temp_crafting_inventory &crafting_inv,
                        const std::function<bool( const item & )> &filter, int = 0 ) const;
    component_type get_component_type() const {
        return component_type::QUALITY;
    }
};

enum class requirement_display_flags : int {
    none = 0,
    no_unavailable = 1,
};

template<>
struct enum_traits<requirement_display_flags> {
    static constexpr bool is_flag_enum = true;
};






























struct requirement_data {



        friend class vpart_info;

        using alter_tool_comp_vector = std::vector<std::vector<tool_comp> >;
        using alter_quali_req_vector = std::vector<std::vector<quality_requirement> >;
        using alter_item_comp_vector = std::vector<std::vector<item_comp> >;

    private:
        alter_tool_comp_vector tools;
        alter_quali_req_vector qualities;
        alter_item_comp_vector components;

    public:

        requirement_data() = default;
        requirement_data( const alter_tool_comp_vector &tools, const alter_quali_req_vector &qualities,
                          const alter_item_comp_vector &components ) : tools( tools ), qualities( qualities ),
            components( components ) {}

        template <
            typename Container,
            typename = std::enable_if_t <
                std::is_same_v <
                    typename Container::value_type, std::pair<requirement_id, int >> ||
                std::is_same_v <
                    typename Container::value_type, std::pair<const requirement_id, int >>
                >
            >
        explicit requirement_data( const Container &cont ) :
            requirement_data(
                std::accumulate(
                    cont.begin(), cont.end(), requirement_data() ) )
        {}

        const requirement_id &id() const {
            return id_;
        }



        const std::string &display_name() const;
        bool has_display_name() const;


        bool is_null() const {
            return id_.is_null();
        }


        bool is_empty() const {
            return tools.empty() && components.empty() && qualities.empty();
        }

        bool is_blacklisted() const {
            return blacklisted;
        }


        requirement_data operator*( unsigned scalar ) const;


        requirement_data operator+( const requirement_data &rhs ) const;



        requirement_data operator+( const std::pair<const requirement_id, int> &rhs ) const;
        requirement_data operator+( const std::pair<requirement_id, int> &rhs ) const;







        static void load_requirement( const JsonObject &jsobj,
                                      const requirement_id &id = requirement_id::NULL_ID(),
                                      bool check_extend = false,
                                      bool is_abstract = false );






        static void save_requirement( const requirement_data &req,
                                      const requirement_id &id = requirement_id::NULL_ID(),
                                      const requirement_data *extend = nullptr );
        static std::vector<requirement_data> get_all();



        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );

        static const std::map<requirement_id, requirement_data> &all();


        static void finalize();


        static void check_consistency();


        static void reset();





        std::string list_all() const;





        std::string list_missing() const;






        void blacklist_item( const itype_id &id );




        void replace_items( const std::unordered_map<itype_id, itype_id> &replacements );

        const alter_tool_comp_vector &get_tools() const;
        const alter_quali_req_vector &get_qualities() const;
        const alter_item_comp_vector &get_components() const;
        alter_item_comp_vector &get_components();









        bool can_make_with_inventory( const Character *actor,
                                      const temp_crafting_inventory &crafting_inv,
                                      const std::function<bool( const item & )> &filter, int batch = 1,
                                      craft_flags = craft_flags::none, bool restrict_volume = true ) const;




        bool check_enough_materials( const Character *actor, const item_comp &comp,
                                     const temp_crafting_inventory &crafting_inv,
                                     const std::function<bool( const item & )> &filter, int batch = 1 ) const;


        std::vector<std::string> get_folded_components_list( const Character *actor, int width,
                nc_color col,
                const temp_crafting_inventory &crafting_inv, const std::function<bool( const item & )> &filter,
                int batch = 1, std::string_view hilite = {},
                requirement_display_flags = requirement_display_flags::none ) const;

        std::vector<std::string> get_folded_tools_list( const Character *actor, int width, nc_color col,
                const temp_crafting_inventory &crafting_inv, int batch = 1 ) const;





        requirement_data disassembly_requirements() const;





        static requirement_data continue_requirements( const std::vector<item_comp> &required_comps,
                const item_components &remaining_comps );





        void consolidate();





        bool has_same_requirements_as( const requirement_data &that ) const;




        void dump( JsonOut &jsout ) const;

        uint64_t make_hash() const;

    private:
        requirement_id id_ = requirement_id::NULL_ID(); // NOLINT(cata-serialize)
        translation name_; // NOLINT(cata-serialize)

        bool blacklisted = false;

        bool check_enough_materials( const Character *actor,
                                     const temp_crafting_inventory &crafting_inv,
                                     const std::function<bool( const item & )> &filter, int batch = 1,
                                     bool restrict_volume = true ) const;

        template<typename T>
        static void check_consistency( const std::vector< std::vector<T> > &vec,
                                       const std::string &display_name );
        template<typename T>
        static void finalize( std::vector< std::vector<T> > &vec );
        template<typename T>
        static std::string print_all_objs( const std::string &header,
                                           const std::vector< std::vector<T> > &objs );
        template<typename T>
        static std::string print_missing_objs( const std::string &header,
                                               const std::vector< std::vector<T> > &objs );
        template<typename T>
        static bool has_comps( const Character *actor,
                               const temp_crafting_inventory &crafting_inv, const std::vector< std::vector<T> > &vec,
                               const std::function<bool( const item & )> &filter, int batch = 1,
                               craft_flags = craft_flags::none );

        template<typename T>
        std::vector<std::string> get_folded_list( const Character *actor, int width,
                const temp_crafting_inventory &crafting_inv,
                const std::function<bool( const item & )> &filter,
                const std::vector< std::vector<T> > &objs, int batch = 1,
                std::string_view hilite = {},
                requirement_display_flags = requirement_display_flags::none ) const;

        template<typename T>
        static bool any_marked_available( const std::vector<T> &comps );
        template<typename T>
        static void load_obj_list( const JsonArray &jsarr, std::vector< std::vector<T> > &objs );
        template<typename T, typename ID>
        static const T *find_by_type( const std::vector< std::vector<T> > &vec, const ID &type );
};
























class deduped_requirement_data
{
    public:
        using alter_item_comp_vector = requirement_data::alter_item_comp_vector;

        deduped_requirement_data() = default;
        deduped_requirement_data( const requirement_data &, const recipe_id &context );

        std::vector<requirement_data> const &alternatives() const {
            return alternatives_;
        }

        std::vector<const requirement_data *> feasible_alternatives(
            const Character *actor, const temp_crafting_inventory &crafting_inv,
            const std::function<bool( const item & )> &filter,
            int batch = 1, craft_flags = craft_flags::none ) const;

        const requirement_data *select_alternative(
            Character &, const std::function<bool( const item & )> &filter, int batch = 1,
            craft_flags = craft_flags::none ) const;

        const requirement_data *select_alternative(
            Character &, const temp_crafting_inventory &, const std::function<bool( const item & )> &filter,
            int batch = 1, craft_flags = craft_flags::none ) const;

        bool can_make_with_inventory(
            const Character *actor,
            const temp_crafting_inventory &crafting_inv, const std::function<bool( const item & )> &filter,
            int batch = 1, craft_flags = craft_flags::none ) const;

        bool is_too_complex() const {
            return is_too_complex_;
        }
    private:
        bool is_too_complex_ = false;
        std::vector<requirement_data> alternatives_;
};

#endif
