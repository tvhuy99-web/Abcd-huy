#pragma once
#ifndef CATA_SRC_MAP_FIELD_H
#define CATA_SRC_MAP_FIELD_H

#include <vector>

#include "coords_fwd.h"

class field_entry;
struct field_proc_data;
struct field_type;

namespace map_field_processing
{





using FieldProcessorPtr = void( * )( const tripoint_bub_ms &p, field_entry &cur,
                                     field_proc_data &pd );




std::vector<FieldProcessorPtr> processors_for_type( const field_type &ft );

}

#endif
