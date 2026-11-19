#pragma once
#ifndef CATA_SRC_SYSTEM_LANGUAGE_H
#define CATA_SRC_SYSTEM_LANGUAGE_H

#include <optional>
#include <string>


namespace SystemLocale
{



std::optional<std::string> Language();



std::optional<bool> UseMetricSystem();

}

#endif
