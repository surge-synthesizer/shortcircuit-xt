//
// Created by Paul Walker on 2/22/23.
//

#include "parameter.h"
#include <cmath>
#include <fmt/core.h>

namespace scxt::datamodel
{
std::string ControlDescription::valueToString(float value) const
{
    if (type == INT)
    {
        // TODO obviouly
        assert(false);
        return "ERROR";
    }
    if (type == FLOAT)
    {
        auto mv = mapScale * value + mapBase;
        switch (displayMode)
        {
        case LINEAR:
            return fmt::format("{:8.4} {}", mv, unit);
        case TWO_TO_THE_X:
            // TODO improve these of course
            return fmt::format("{:8.4} {}", std::pow(2.0, mv), unit);
        default:
            return fmt::format("ERROR {} {} {} {}", __FILE__, __LINE__, mv, unit);
        }
    }

    assert(false);
    return "ERROR";
}

std::optional<float> ControlDescription::stringToValue(const std::string &s) const { return {}; }
} // namespace scxt::datamodel