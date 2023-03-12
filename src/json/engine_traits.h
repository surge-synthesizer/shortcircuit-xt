/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#ifndef SCXT_SRC_JSON_ENGINE_TRAITS_H
#define SCXT_SRC_JSON_ENGINE_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "stream.h"
#include "extensions.h"

#include "engine/patch.h"
#include "engine/part.h"
#include "engine/group.h"
#include "engine/zone.h"
#include "engine/keyboard.h"
#include "engine/engine.h"

#include "scxt_traits.h"

#include "utils_traits.h"
#include "sample_traits.h"
#include "dsp_traits.h"
#include "modulation_traits.h"
#include "datamodel_traits.h"

namespace scxt::json
{
template <> struct scxt_traits<scxt::engine::Engine>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const scxt::engine::Engine &e)
    {
        v = {{"streamingVersion", scxt::json::currentStreamingVersion},
             {"patch", e.getPatch()},
             {"sampleManager", e.getSampleManager()}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::engine::Engine &engine)
    {
        // TODO: engine gets a SV? Guess maybe
        // Order matters here. Samples need to be there before the patch
        v.at("sampleManager").to(*(engine.getSampleManager()));
        v.at("patch").to(*(engine.getPatch()));
    }
};

template <> struct scxt_traits<scxt::engine::Patch>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const scxt::engine::Patch &t)
    {
        v = {{"streamingVersion", scxt::json::currentStreamingVersion}, {"parts", t.getParts()}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::engine::Patch &patch)
    {
        patch.reset();
        v.at("streamingVersion").to(patch.streamingVersion);

        // TODO: I could expose the array but I want to go to a limited stream in the future
        auto vzones = v.at("parts").get_array();
        size_t idx{0};
        for (const auto vz : vzones)
        {
            vz.to(*(patch.getPart(idx)));
            idx++;
        }
    }
};

template <> struct scxt_traits<scxt::engine::Part>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const scxt::engine::Part &t)
    {
        // TODO: Do a non-empty part stream with the If variant
        v = {{"channel", t.channel}, {"groups", t.getGroups()}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::engine::Part &part)
    {
        part.clearGroups();

        v.at("channel").to(part.channel);
        auto vzones = v.at("groups").get_array();
        for (const auto vz : vzones)
        {
            auto idx = part.addGroup() - 1;
            vz.to(*(part.getGroup(idx)));
        }
    }
};

template <> struct scxt_traits<scxt::engine::Group>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const scxt::engine::Group &t)
    {
        v = {{"zones", t.getZones()}, {"name", t.getName()}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::engine::Group &group)
    {
        findIf(v, "name", group.name);
        group.clearZones();

        auto vzones = v.at("zones").get_array();
        for (const auto vz : vzones)
        {
            auto idx = group.addZone(std::make_unique<scxt::engine::Zone>()) - 1;
            vz.to(*(group.getZone(idx)));
            if (group.parentPart && group.parentPart->parentPatch &&
                group.parentPart->parentPatch->parentEngine)
            {
                // TODO: MOve this somewhere more intelligent
                group.getZone(idx)->setupOnUnstream(*(group.parentPart->parentPatch->parentEngine));
            }
        }
    }
};

template <> struct scxt_traits<scxt::engine::Zone::ZoneMappingData>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v,
                       const scxt::engine::Zone::ZoneMappingData &t)
    {
        v = {
            {"rootKey", t.rootKey},
            {"keyboardRange", t.keyboardRange},
            {"velocityRange", t.velocityRange},
            {"pbDown", t.pbDown},
            {"pbUp", t.pbUp},

            {"exclusiveGroup", t.exclusiveGroup},
            {"velocitySens", t.velocitySens},
            {"amplitude", t.amplitude},
            {"pan", t.pan},
            {"pitchOffset", t.pitchOffset},
        };
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v,
                   scxt::engine::Zone::ZoneMappingData &zmd)
    {
        findOrSet(v, "rootKey", 60, zmd.rootKey);
        v.at("keyboardRange").to(zmd.keyboardRange);
        v.at("velocityRange").to(zmd.velocityRange);
        v.at("pbDown").to(zmd.pbDown);
        v.at("pbUp").to(zmd.pbUp);
        v.at("amplitude").to(zmd.amplitude);
        v.at("pan").to(zmd.pan);
        v.at("pitchOffset").to(zmd.pitchOffset);
        findOrSet(v, "velocitySens", 1.0, zmd.velocitySens);
        findOrSet(v, "exclusiveGroup", 0, zmd.exclusiveGroup);
    }
};

template <> struct scxt_traits<scxt::engine::Zone::AssociatedSample>
{

    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v,
                       const scxt::engine::Zone::AssociatedSample &s)
    {
        v = {{"active", s.active}, {"id", s.sampleID}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::engine::Zone::AssociatedSample &s)
    {
        findOrSet(v, "active", false, s.active);
        findIf(v, "id", s.sampleID);
    }
};

template <> struct scxt_traits<scxt::engine::Zone>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const scxt::engine::Zone &t)
    {
        auto rtArray = toIndexedArrayIf<Traits>(t.routingTable, [](const auto &r) {
            // TODO we really don't need this we should just stream the table
            return (r.dst != scxt::modulation::vmd_none || r.src != scxt::modulation::vms_none ||
                    r.depth != 0 || !r.active || r.curve != scxt::modulation::vmc_none);
        });

        v = {{"associatedSamples", t.samples},
             {"mappingData", t.mapping},
             {"processorStorage", t.processorStorage},
             {"routingTable", rtArray},
             {"lfoStorage", t.lfoStorage},
             {"aegStorage", t.aegStorage},
             {"eg2Storage", t.eg2Storage}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::engine::Zone &zone)
    {
        findIf(v, "associatedSamples", zone.samples);
        v.at("mappingData").to(zone.mapping);
        fromArrayWithSizeDifference<Traits>(v.at("processorStorage"), zone.processorStorage);

        std::fill(zone.routingTable.begin(), zone.routingTable.end(),
                  scxt::modulation::VoiceModMatrix::Routing());
        fromIndexedArray<Traits>(v.at("routingTable"), zone.routingTable);

        v.at("lfoStorage").to(zone.lfoStorage);
        findOrDefault(v, "aegStorage", zone.aegStorage);
        findOrDefault(v, "eg2Storage", zone.eg2Storage);
    }
};

template <> struct scxt_traits<scxt::engine::KeyboardRange>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const scxt::engine::KeyboardRange &t)
    {
        v = {{"keyStart", t.keyStart},
             {"keyEnd", t.keyEnd},
             {"fadeStart", t.fadeStart},
             {"fadeEnd", t.fadeEnd}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::engine::KeyboardRange &r)
    {
        v.at("keyStart").to(r.keyStart);
        v.at("keyEnd").to(r.keyEnd);
        v.at("fadeStart").to(r.fadeStart);
        v.at("fadeEnd").to(r.fadeEnd);
    }
};

template <> struct scxt_traits<scxt::engine::VelocityRange>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const scxt::engine::VelocityRange &t)
    {
        v = {{"velStart", t.velStart},
             {"velEnd", t.velEnd},
             {"fadeStart", t.fadeStart},
             {"fadeEnd", t.fadeEnd}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::engine::VelocityRange &r)
    {
        v.at("velStart").to(r.velStart);
        v.at("velEnd").to(r.velEnd);
        v.at("fadeStart").to(r.fadeStart);
        v.at("fadeEnd").to(r.fadeEnd);
    }
};
} // namespace scxt::json
#endif // SHORTCIRCUIT_ENGINE_TRAITS_H
