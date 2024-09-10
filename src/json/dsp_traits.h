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

#ifndef SCXT_SRC_JSON_DSP_TRAITS_H
#define SCXT_SRC_JSON_DSP_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "dsp/processor/processor.h"
#include "scxt_traits.h"
#include "extensions.h"

namespace scxt::json
{

SC_STREAMDEF(scxt::dsp::processor::ProcessorStorage, SC_FROM({
                 auto &t = from;
                 if (t.type == dsp::processor::proct_none)
                 {
                     v = tao::json::empty_object;
                 }
                 else
                 {
                     v = {{"type", scxt::dsp::processor::getProcessorStreamingName(t.type)},
                          {"mix", t.mix},
                          {"out", t.outputCubAmp},
                          {"isKeytracked", t.isKeytracked},
                          {"isTemposynced", t.isTemposynced},
                          {"floatParams", t.floatParams},
                          {"intParams", t.intParams},
                          {"deactivated", t.deactivated},
                          {"isActive", t.isActive}};
                 }
             }),
             SC_TO({
                 auto &result = to;
                 const auto &object = v.get_object();

                 std::string type;
                 findOrSet(
                     v, {"t", "type"},
                     scxt::dsp::processor::getProcessorStreamingName(dsp::processor::proct_none),
                     type);

                 auto optType = scxt::dsp::processor::fromProcessorStreamingName(type);

                 if (optType.has_value())
                     result.type = *optType;
                 else
                     result.type = scxt::dsp::processor::proct_none;

                 if (result.type == dsp::processor::proct_none)
                 {
                     result = scxt::dsp::processor::ProcessorStorage();
                     result.type = dsp::processor::proct_none;
                 }
                 else
                 {
                     findIf(v, "mix", result.mix);
                     findIf(v, "out", result.outputCubAmp);
                     fromArrayWithSizeDifference(v.at("floatParams"), result.floatParams);
                     fromArrayWithSizeDifference(v.at("intParams"), result.intParams);
                     findIf(v, "deactivated", result.deactivated);
                     findOrSet(v, "isActive", false, result.isActive);
                     findOrSet(v, "isKeytracked", false, result.isKeytracked);
                     findOrSet(v, "isTemposynced", false, result.isTemposynced);
                 }
             }))

SC_STREAMDEF(scxt::dsp::processor::ProcessorDescription,
             SC_FROM({ // Streaming this type as an int is fine since the processor storage
                 // stringifies definitively. This is just for in-session communication
                 v = {
                     {"id", (int32_t)t.id},          {"streamingName", t.streamingName},
                     {"displayName", t.displayName}, {"displayGroup", t.displayGroup},
                     {"groupOnly", t.groupOnly},
                 };
             }),
             SC_TO({
                 int32_t tr;
                 findIf(v, "id", tr);
                 result.id = (dsp::processor::ProcessorType)tr;
                 findIf(v, "streamingName", result.streamingName);
                 findIf(v, "displayName", result.displayName);
                 findIf(v, "displayGroup", result.displayGroup);
                 findIf(v, "groupOnly", result.groupOnly);
             }))

SC_STREAMDEF(
    scxt::dsp::processor::ProcessorControlDescription, SC_FROM({
        v = {
            {"type",
             (int32_t)t.type}, // these are process-lifetime only so the type is safe to stream
            {"typeDisplayName", t.typeDisplayName},
            {"numFloatParams", t.numFloatParams},
            {"floatControlDescriptions", t.floatControlDescriptions},
            {"numIntParams", t.numIntParams},
            {"intControlDescriptions", t.intControlDescriptions},
            {"supportsKeytrack", t.supportsKeytrack},
            {"supportsTemposync", t.supportsTemposync},
        };
    }),
    SC_TO({
        int tInt{dsp::processor::proct_none};
        findOrSet(v, "type", (int32_t)dsp::processor::proct_none, tInt);
        to.type = (dsp::processor::ProcessorType)tInt;
        findIf(v, "typeDisplayName", to.typeDisplayName);
        findIf(v, "numFloatParams", to.numFloatParams);
        findIf(v, "floatControlDescriptions", to.floatControlDescriptions);
        findIf(v, "numIntParams", to.numIntParams);
        findIf(v, "intControlDescriptions", to.intControlDescriptions);
        findIf(v, "supportsKeytrack", to.supportsKeytrack);
        findIf(v, "supportsTemposync", to.supportsTemposync);
    }))
} // namespace scxt::json
#endif // SHORTCIRCUIT_DSP_TRAITS_H
