#pragma once
#ifndef CATA_SRC_A11Y_BODY_H
#define CATA_SRC_A11Y_BODY_H
#include <string>
#include "type_id.h"
class Character;
namespace a11y
{
// Uses the medical UI's knowledge policy: feelings for pain immunity, coarse
// bar segments otherwise, and no exact underlying hit points or healing time.
std::string limb_health_description( const Character &who, const bodypart_id &part );
} // namespace a11y
#endif // CATA_SRC_A11Y_BODY_H
