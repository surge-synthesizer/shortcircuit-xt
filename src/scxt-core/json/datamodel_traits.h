/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#ifndef SCXT_SRC_SCXT_CORE_JSON_DATAMODEL_TRAITS_H
#define SCXT_SRC_SCXT_CORE_JSON_DATAMODEL_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>
#include <unordered_map>

#include "scxt_traits.h"
#include "extensions.h"

namespace scxt::json
{

SC_STREAMDEF(datamodel::pmd, SC_FROM({
                 assert(SC_STREAMING_FOR_IN_PROCESS);
                 std::vector<std::pair<int, std::string>> dvStream;
                 for (const auto &[k, mv] : t.discreteValues)
                     dvStream.emplace_back(k, mv);

                 v = {{"t", (int)t.type},
                      {"n", t.name},
                      {"mn", t.minVal},
                      {"mx", t.maxVal},
                      {"df", t.defaultVal},
                      {"ts", t.canTemposync},
                      {"ex", t.canExtend},
                      {"de", t.canDeactivate},
                      {"ab", t.canAbsolute},
                      {"enb", t.enabled},
                      {"tsm", t.temposyncMultiplier},
                      {"ssc", t.supportsStringConversion},
                      {"dsc", (int)t.displayScale},
                      {"unit", t.unit},
                      {"qt", (int)t.quantization},
                      {"qtp", t.quantizationParam},
                      {"cval", t.customValueLabelsWithAccuracy},
                      {"discv", dvStream},
                      {"dep", t.decimalPlaces},
                      {"svA", t.svA},
                      {"svB", t.svB},
                      {"svC", t.svC},
                      {"svD", t.svD}};
             }),
             SC_TO({
                 findEnumIf(v, "t", to.type);
                 findIf(v, "n", to.name);
                 findIf(v, "mn", to.minVal);
                 findIf(v, "mx", to.maxVal);
                 findIf(v, "df", to.defaultVal);
                 findIf(v, "ts", to.canTemposync);
                 findIf(v, "de", to.canDeactivate);
                 findIf(v, "ex", to.canExtend);
                 findIf(v, "ab", to.canAbsolute);
                 findOrSet(v, "enb", true, to.enabled);
                 findIf(v, "tsm", to.temposyncMultiplier);
                 findIf(v, "ssc", to.supportsStringConversion);
                 findEnumIf(v, "dsc", to.displayScale);
                 findEnumIf(v, "qt", to.quantization);
                 findIf(v, "qtp", to.quantizationParam);
                 findIf(v, "unit", to.unit);
                 findIf(v, "cval", to.customValueLabelsWithAccuracy);

                 std::vector<std::pair<int, std::string>> dvStream;
                 findIf(v, "discv", dvStream);
                 for (const auto &[k, mv] : dvStream)
                     to.discreteValues[k] = mv;

                 findIf(v, "dep", to.decimalPlaces);
                 findIf(v, "svA", to.svA);
                 findIf(v, "svB", to.svB);
                 findIf(v, "svC", to.svC);
                 findIf(v, "svD", to.svD);
             }));
} // namespace scxt::json
#endif // SHORTCIRCUIT_DATAMODEL_TRAITS_H
