#pragma once
#ifndef CATA_SRC_ACTIVITY_ACTOR_H
#define CATA_SRC_ACTIVITY_ACTOR_H

#include <memory>
#include <string>
#include <unordered_map>

#include "activity_type.h"
#include "clone_ptr.h"
#include "type_id.h"

class Character;
class JsonOut;
class JsonValue;
class player_activity;

class activity_actor
{
    private:








        virtual bool can_resume_with_internal( const activity_actor &,
                                               const Character & ) const {
            return false;
        }








        virtual void set_resume_values_internal( const activity_actor &,
                const Character & ) { }

    public:
        virtual ~activity_actor() = default;




        virtual const activity_id &get_type() const = 0;






        virtual void start( player_activity &act, Character &who ) = 0;





        virtual void do_turn( player_activity &act, Character &who ) = 0;





        virtual void finish( player_activity &act, Character &who ) = 0;





        virtual void canceled( player_activity &, Character & ) {}








        bool can_resume_with( const activity_actor &other, const Character &who ) const {
            if( other.get_type() == get_type() ) {
                return can_resume_with_internal( other, who );
            }

            return false;
        }







        void set_resume_values( const activity_actor &other, const Character &who ) {
            if( other.get_type() == get_type() ) {
                set_resume_values_internal( other, who );
            }
        }





        virtual std::string get_progress_message( const player_activity &act ) const;






        virtual float exertion_level() const {
            return get_type()->exertion_level();
        }







        virtual bool do_drop_invalid_inventory() const {
            return true;
        }












        virtual std::unique_ptr<activity_actor> clone() const = 0;






        virtual void serialize( JsonOut &jsout ) const = 0;
};

void serialize( const cata::clone_ptr<activity_actor> &actor, JsonOut &jsout );
void deserialize( cata::clone_ptr<activity_actor> &actor, const JsonValue &jsin );

namespace activity_actors
{


extern const std::unordered_map<activity_id, std::unique_ptr<activity_actor>( * )( JsonValue & )>
deserialize_functions;

}

#endif
