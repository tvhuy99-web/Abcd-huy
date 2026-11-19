#pragma once
#ifndef CATA_SRC_KILL_TRACKER_H
#define CATA_SRC_KILL_TRACKER_H

#include <map>
#include <string>
#include <vector>

#include "event_subscriber.h"
#include "type_id.h"

class JsonObject;
class JsonOut;

class kill_tracker : public event_subscriber
{

        friend class diary;
    public:
        kill_tracker() = default;
        void reset( const std::map<mtype_id, int> &kills,
                    const std::vector<std::string> &npc_kills );

        int kill_count( const mtype_id & ) const;

        int kill_count( const species_id & ) const;

        int guilt_kill_count() const;
        int monster_kill_count() const;
        int npc_kill_count() const;
        int total_kill_count() const;

        void clear();
        using event_subscriber::notify;
        void notify( const cata::event & ) override;

        void serialize( JsonOut & ) const;
        void deserialize( const JsonObject &data );

        std::map<mtype_id, int> kills;
        std::vector<std::string> npc_kills;
};

#endif
