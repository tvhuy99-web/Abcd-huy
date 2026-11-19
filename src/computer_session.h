#pragma once
#ifndef CATA_SRC_COMPUTER_SESSION_H
#define CATA_SRC_COMPUTER_SESSION_H

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "computer.h"
#include "cursesdef.h"
#include "type_id.h"

class Character;

class computer_session
{
    public:
        explicit computer_session( computer &comp );


        void use();

    private:
        computer &comp;

        catacurses::window win;
        const int left;
        const int top;
        const int width;
        const int height;
        std::vector<std::pair<int, std::string>> lines;



        bool hack_attempt( Character &you, int Security = -1 ) const;






        bool can_activate( computer_action action );


        void activate_function( computer_action action );

        void activate_failure( computer_failure_type fail );

        void activate_random_failure();


        void reset_terminal();

        void refresh();

        void print_newline();

        template<typename ...Args>
        void print_indented_line( int indent, int text_width, const std::string &text, Args &&... args );

        void print_gibberish_line();

        template<typename ...Args>
        void print_line( const std::string &text, Args &&... args );

        template<typename ...Args>
        void print_error( const std::string &text, Args &&... args );

        template<typename ...Args>
        void print_text( const std::string &text, Args &&... args );

        enum class ynq : int {
            yes,
            no,
            quit,
        };
        template<typename ...Args>
        ynq query_ynq( const std::string &text, Args &&... args );

        template<typename ...Args>
        bool query_bool( const std::string &text, Args &&... args );

        template<typename ...Args>
        bool query_any( const std::string &text, Args &&... args );

        bool query_any();




        std::string terminal_text() const;



        bool query_any_impl( std::string_view summary );

        void action_amigara_log();
        void action_amigara_start();
        void action_cascade();
        void action_complete_disable_external_power();
        void action_conveyor();
        void action_data_anal();
        void action_deactivate_shock_vent();
        void action_disconnect();
        void action_download_software();
        void action_elevator_on();
        void action_emerg_mess();
        void action_emerg_ref_center();
        void action_extract_rad_source();
        void action_geiger();
        void action_irradiator();
        void action_list_bionics();
        void action_list_mutations();
        void action_lock();
        void helper_map( bool ( *func )( const oter_id & ), const char *query,
                         enum computer_action action );
        void action_map_sewer();
        void action_map_subway();
        void action_maps();
        void action_miss_disarm();
        void action_miss_launch();
        void action_open();
        void action_open_gate();
        void action_close_gate();
        void action_open_disarm();
        void action_portal();
        void action_radio_archive();
        void helper_release( float radius );
        void action_release();
        void action_release_bionics();
        void action_release_disarm();
        void action_repeater_mod();
        void action_research();
        void action_sample();
        void action_shutters();
        void action_sr1_mess();
        void action_sr2_mess();
        void action_sr3_mess();
        void action_sr4_mess();
        void action_srcf_1_mess();
        void action_srcf_2_mess();
        void action_srcf_3_mess();
        void action_srcf_elevator();
        void action_srcf_seal();
        void action_srcf_seal_order();
        void action_terminate();
        void action_toll();
        void action_tower_unresponsive();
        void action_unlock();
        void action_unlock_disarm();
        static const std::map<computer_action, void( computer_session::* )()> computer_action_functions;

        void failure_alarm();
        void failure_amigara();
        void failure_damage();
        void failure_destroy_data();
        void failure_manhacks();
        void failure_pump_explode();
        void failure_pump_leak();
        void failure_secubots();
        void failure_shutdown();
        static const std::map<computer_failure_type, void( computer_session::* )()>
        computer_failure_functions;
};

#endif
