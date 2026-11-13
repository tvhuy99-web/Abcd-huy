#pragma once
#ifndef CATA_SRC_A11Y_REQUIREMENTS_H
#define CATA_SRC_A11Y_REQUIREMENTS_H

#include <functional>
#include <string>

class Character;
class deduped_requirement_data;
class item;
struct requirement_data;
class temp_crafting_inventory;

namespace a11y
{
struct semantic_snapshot;

void append_requirements( semantic_snapshot &snapshot, const std::string &prefix,
                          const requirement_data &requirements, const Character &actor,
                          const temp_crafting_inventory &inventory,
                          const std::function<bool( const item & )> &component_filter,
                          int batch, const deduped_requirement_data *deduped = nullptr );
} // namespace a11y

#endif // CATA_SRC_A11Y_REQUIREMENTS_H
