#pragma once
#ifndef CATA_SRC_CONTENTS_CHANGE_HANDLER_H
#define CATA_SRC_CONTENTS_CHANGE_HANDLER_H

#include <vector>

#include "item_location.h"

class Character;
class JsonOut;
class JsonValue;







class contents_change_handler
{
    public:
        contents_change_handler() = default;




        void add_unsealed( const item_location &loc );






        void unseal_pocket_containing( const item_location &loc );




        void handle_by( Character &guy );



        void serialize( JsonOut &jsout ) const;



        void deserialize( const JsonValue &jsin );
    private:
        std::vector<item_location> unsealed;
};

#endif
