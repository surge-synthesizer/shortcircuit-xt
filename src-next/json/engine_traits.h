//
// Created by Paul Walker on 2/2/23.
//

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
        v = {{"zones", t.getZones()}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::engine::Group &group)
    {
        group.clearZones();

        auto vzones = v.at("zones").get_array();
        for (const auto vz : vzones)
        {
            auto idx = group.addZone(std::make_unique<scxt::engine::Zone>()) - 1;
            vz.to(*(group.getZone(idx)));
            if (group.parentPart && group.parentPart->parentPatch &&
                group.parentPart->parentPatch->parentEngine)
                group.getZone(idx)->attachToSample(
                    *(group.parentPart->parentPatch->parentEngine->getSampleManager()));
        }
    }
};

template <> struct scxt_traits<scxt::engine::Zone>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const scxt::engine::Zone &t)
    {
        auto rtArray = toIndexedArrayIf<Traits>(t.routingTable, [](const auto &r) {
            return (r.dst != scxt::modulation::vmd_none || r.src != scxt::modulation::vms_none ||
                    r.depth != 0);
        });

        v = {{"sampleID", t.sampleID}, //
             {"keyboardRange", t.keyboardRange},
             {"rootKey", t.rootKey},
             {"processorStorage", t.processorStorage},
             {"routingTable", rtArray},
             {"lfoStorage", t.lfoStorage}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::engine::Zone &zone)
    {
        const auto &object = v.get_object();
        // TODO : Stream and unstream IDs
        v.at("sampleID").to(zone.sampleID);
        v.at("keyboardRange").to(zone.keyboardRange);
        v.at("rootKey").to(zone.rootKey);
        v.at("processorStorage").to(zone.processorStorage);

        std::fill(zone.routingTable.begin(), zone.routingTable.end(),
                  scxt::modulation::VoiceModMatrix::Routing());
        fromIndexedArray<Traits>(v.at("routingTable"), zone.routingTable);

        v.at("lfoStorage").to(zone.lfoStorage);
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
        const auto &object = v.get_object();
        v.at("keyStart").to(r.keyStart);
        v.at("keyEnd").to(r.keyEnd);
        v.at("fadeStart").to(r.fadeStart);
        v.at("fadeEnd").to(r.fadeEnd);
    }
};
} // namespace scxt::json
#endif // SHORTCIRCUIT_ENGINE_TRAITS_H
