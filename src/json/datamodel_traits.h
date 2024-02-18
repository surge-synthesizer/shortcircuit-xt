/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_JSON_DATAMODEL_TRAITS_H
#define SCXT_SRC_JSON_DATAMODEL_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>
#include <unordered_map>

#include "scxt_traits.h"
#include "extensions.h"

#include "datamodel/adsr_storage.h"

namespace scxt::json
{

SC_STREAMDEF(datamodel::AdsrStorage, SC_FROM({
                 v = {{"a", t.a},           {"h", t.h},           {"d", t.d},
                      {"s", t.s},           {"r", t.r},           {"isDigital", t.isDigital},
                      {"aShape", t.aShape}, {"dShape", t.dShape}, {"rShape", t.rShape}};
             }),
             SC_TO({
                 findIf(v, "a", result.a);
                 findIf(v, "h", result.h);
                 findIf(v, "d", result.d);
                 findIf(v, "s", result.s);
                 findIf(v, "r", result.r);
                 findIf(v, "isDigital", result.isDigital);
                 findIf(v, "aShape", result.aShape);
                 findIf(v, "dShape", result.dShape);
                 findIf(v, "rShape", result.rShape);
             }))

SC_STREAMDEF(datamodel::pmd, SC_FROM({
                 std::vector<std::pair<int, std::string>> dvStream;
                 for (const auto &[k, mv] : t.discreteValues)
                     dvStream.emplace_back(k, mv);

                 v = {{"type", (int)t.type},
                      {"name", t.name},
                      {"minVal", t.minVal},
                      {"maxVal", t.maxVal},
                      {"defaultVal", t.defaultVal},
                      {"canTemposync", t.canTemposync},
                      {"supportsStringConversion", t.supportsStringConversion},
                      {"displayScale", (int)t.displayScale},
                      {"unit", t.unit},
                      {"customMinDisplay", t.customMinDisplay},
                      {"customMaxDisplay", t.customMaxDisplay},
                      {"discreteValues", dvStream},
                      {"decimalPlaces", t.decimalPlaces},
                      {"svA", t.svA},
                      {"svB", t.svB},
                      {"svC", t.svC},
                      {"svD", t.svD}};
             }),
             SC_TO({
                 findEnumIf(v, "type", to.type);
                 findIf(v, "name", to.name);
                 findIf(v, "minVal", to.minVal);
                 findIf(v, "maxVal", to.maxVal);
                 findIf(v, "defaultVal", to.defaultVal);
                 findIf(v, "canTemposync", to.canTemposync);
                 findIf(v, "supportsStringConversion", to.supportsStringConversion);
                 findEnumIf(v, "displayScale", to.displayScale);
                 findIf(v, "unit", to.unit);
                 findIf(v, "customMinDisplay", to.customMinDisplay);
                 findIf(v, "customMaxDisplay", to.customMaxDisplay);

                 std::vector<std::pair<int, std::string>> dvStream;
                 findIf(v, "discreteValues", dvStream);
                 for (const auto &[k, mv] : dvStream)
                     to.discreteValues[k] = mv;

                 findIf(v, "decimalPlaces", to.decimalPlaces);
                 findIf(v, "svA", to.svA);
                 findIf(v, "svB", to.svB);
                 findIf(v, "svC", to.svC);
                 findIf(v, "svD", to.svD);
             }));
} // namespace scxt::json
#endif // SHORTCIRCUIT_DATAMODEL_TRAITS_H
