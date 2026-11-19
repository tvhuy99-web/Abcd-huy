#pragma once
#ifndef CATA_SRC_SETS_INTERSECT_H
#define CATA_SRC_SETS_INTERSECT_H

namespace cata
{

template<typename SetL, typename SetR>
bool sets_intersect( const SetL &l, const SetR &r )
{




    if( l.size() > r.size() ) {
        return sets_intersect( r, l );
    }

    for( const auto &elem : l ) {
        if( r.count( elem ) ) {
            return true;
        }
    }

    return false;
}

}

#endif
