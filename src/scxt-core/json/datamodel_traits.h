/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
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
                      {"ftr", t.features},
                      {"svA", t.svA},
                      {"svB", t.svB},
                      {"svC", t.svC},
                      {"svD", t.svD},
                      {"asw", (int)t.alternateScaleWhen},
                      {"asc", t.alternateScaleCutoff},
                      {"asr", t.alternateScaleRescaling},
                      {"asu", t.alternateScaleUnits},
                      {"asd", t.alternateScaleIsDefaultFromString},
                      {"asdp", t.alternateScaleDecimalPlaces}};
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
                 findIf(v, "ftr", to.features);
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

                 findEnumIf(v, "asw", to.alternateScaleWhen);
                 findIf(v, "asc", to.alternateScaleCutoff);
                 findIf(v, "asr", to.alternateScaleRescaling);
                 findIf(v, "asu", to.alternateScaleUnits);
                 findIf(v, "asd", to.alternateScaleIsDefaultFromString);
                 findIf(v, "asdp", to.alternateScaleDecimalPlaces);
             }));
} // namespace scxt::json
#endif // SHORTCIRCUIT_DATAMODEL_TRAITS_H
