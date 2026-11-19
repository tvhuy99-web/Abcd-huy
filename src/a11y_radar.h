#pragma once
#ifndef CATA_SRC_A11Y_RADAR_H
#define CATA_SRC_A11Y_RADAR_H

#include <set>
#include <string>
#include "coords_fwd.h"

namespace a11y
{

// Called only from a world snapshot on the native game thread. The scan is
// read-only and does no terrain generation, movement, or LOS filtering.
void update_automatic_radar( const tripoint_bub_ms &origin );

namespace radar_detail
{
// A key denotes ONE real object, not a name or a compass direction.
// Re-entering after absence is a new event; remaining nearby is silent.
class entry_tracker
{
    public:
        std::set<std::string> entering( const std::set<std::string> &now ) {
            std::set<std::string> fresh;
            for( const std::string &key : now ) {
                if( previous.count( key ) == 0 ) {
                    fresh.insert( key );
                }
            }
            previous = now;
            return fresh;
        }
        void clear() {
            previous.clear();
        }
    private:
        std::set<std::string> previous;
};
} // namespace radar_detail

} // namespace a11y
#endif
