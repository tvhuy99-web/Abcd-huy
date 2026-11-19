#pragma once
#ifndef CATA_SRC_A11Y_BODY_H
#define CATA_SRC_A11Y_BODY_H
#include <string>
#include "type_id.h"
class Character;
namespace a11y
{


std::string limb_health_description( const Character &who, const bodypart_id &part );
}
#endif
