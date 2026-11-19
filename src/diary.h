#pragma once
#ifndef CATA_SRC_DIARY_H
#define CATA_SRC_DIARY_H

#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
#include <stddef.h>
#include <string>
#include <vector>

#include "calendar.h"
#include "mutation.h"
#include "type_id.h"
#include "units.h"

class JsonOut;
class JsonValue;
class Skill;

namespace catacurses
{
class window;
}


template<typename T>
class id_translated_name_comp
{
    public:
        bool operator()( const string_id<T> &a, const string_id<T> &b ) const {
            const std::string &a_name = a->name();
            const std::string &b_name = b->name();
            if( a_name == b_name ) {

                // NOLINTNEXTLINE(cata-use-localized-sorting)
                return a.str() < b.str();
            }
            // NOLINTNEXTLINE(cata-use-localized-sorting)
            return a->name() < b->name();
        }
};




struct diary_page {
    diary_page();
    virtual ~diary_page() = default;

    std::string m_text;
    time_point previous_Page_turn;
    std::vector<std::string> diff_to_previous_page;

    time_point turn;


    time_accuracy time_acc = time_accuracy::NONE;

    std::vector<int> mission_completed;
    std::vector<int> mission_active;
    std::vector<int> mission_failed;

    std::map<mtype_id, int> kills;

    std::vector<std::string> npc_kills;

    bool male = false;

    int strength = 0;
    int dexterity = 0;
    int intelligence = 0;
    int perception = 0;

    std::vector<trait_and_var> traits;

    std::map<spell_id, int> known_spells;

    std::vector<bionic_id> bionics;

    std::map<skill_id, int, id_translated_name_comp<Skill>> skillsL;

    std::vector<proficiency_id> known_profs;
    std::vector<proficiency_id> learning_profs;

    units::energy max_power_level;

    virtual std::string entry_name() const;
    virtual bool is_summary() const {
        return false;
    }
};

struct diary_page_summary : public diary_page {
    std::string entry_name() const override;
    bool is_summary() const override {
        return true;
    }
};







class diary
{

    private:

        std::string owner;

        std::vector< std::unique_ptr<diary_page>> pages;


        int opened_page = 0; // NOLINT(cata-serialize)

        std::vector<std::string> change_list; // NOLINT(cata-serialize)

        std::map<int, std::string> desc_map; // NOLINT(cata-serialize)


    public:
        diary();
        virtual ~diary() = default;

        static void show_diary_ui( diary *c_diary );

        void death_entry();


        bool store() const;
        void load();
        void serialize( std::ostream &fout ) const;
        void deserialize( const JsonValue &jsin );
        void serialize( JsonOut &jsout ) const;

    private:
        void add_summary_page();
        void open_summary_page();
        time_accuracy time_acc() const;


        void edit_page_ui( const std::function<catacurses::window()> &create_window );

        int set_opened_page( int pagenum );

        void new_page();

        void set_page_text( std::string text );

        void delete_page();


        int get_opened_page_num() const;

        std::vector<std::string> get_pages_list() const;

        const std::vector<std::string> &get_change_list();

        std::map<int, std::string> &get_desc_map();


        diary_page *get_page_ptr( int offset = 0, bool allow_summary = true );

        std::string get_page_text();

        std::string get_desc_or_page_text( int desc );

        std::string get_head_text( bool is_summary = false );

        template<typename Container, typename Fn>
        void changes( Container diary_page::* member, Fn &&get_entry,
                      const std::string &heading_first, const std::string &headings_first,
                      const std::string &heading, const std::string &headings,
                      diary_page *currpage, diary_page *prevpage );
        template<typename Container, typename Fn>
        void changes( Container diary_page::* member, Fn &&get_entry,
                      const std::string &heading_first, const std::string &headings_first,
                      const std::string &heading, const std::string &headings );
        template<typename Container, typename Fn>
        void changes( Container diary_page::* member, Fn &&get_entry,
                      const std::string &heading_first, const std::string &headings_first );

        void skill_changes();
        void mon_kill_changes();
        void npc_kill_changes();
        void trait_changes();
        void bionic_changes();
        void stat_changes();
        void prof_changes();
        void mission_changes();
        void spell_changes();


        void export_to_txt( bool lastexport = false );

        size_t add_to_change_list( const std::string &entry, const std::string &desc = "" );

        void update_change_list( size_t index, const std::string &entry, const std::string &desc = "" );
};
#endif
