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
#include "selection_traits.h"
#include "engine/bus.h"
#include "messaging/messaging.h"

namespace scxt::json
{

struct EngineStreamGuard
{
    EngineStreamGuard(uint64_t sv)
    {
        SCLOG("Unstreaming engine with streaming version = " << std::hex << sv);
        engine::Engine::isFullEngineUnstream = true;
        engine::Engine::fullEngineUnstreamStreamingVersion = sv;
    }
    ~EngineStreamGuard()
    {
        engine::Engine::isFullEngineUnstream = false;
        engine::Engine::fullEngineUnstreamStreamingVersion = 0;
    }
};
SC_STREAMDEF(scxt::engine::Engine, SC_FROM({
                 v = {{"streamingVersion", scxt::json::currentStreamingVersion},
                      {"patch", from.getPatch()},
                      {"selectionManager", from.getSelectionManager()},
                      {"sampleManager", from.getSampleManager()}};
             }),
             SC_TO({
                 assert(to.getMessageController()->threadingChecker.isSerialThread());
                 auto sv{0};
                 findIf(v, "streamingVersion", sv);
                 EngineStreamGuard sg(sv);

                 // TODO: engine gets a SV? Guess maybe
                 // Order matters here. Samples need to be there before the patch and patch
                 // before selection

                 to.getSampleManager()->resetMissingList();
                 findIf(v, "sampleManager", *(to.getSampleManager()));
                 findIf(v, "patch", *(to.getPatch()));
                 findIf(v, "selectionManager", *(to.getSelectionManager()));

                 // Now we need to restore the bus effects
                 to.getPatch()->setupBussesOnUnstream(to);

                 // and finally set the sample rate
                 to.getPatch()->setSampleRate(to.getSampleRate());
             }))

SC_STREAMDEF(scxt::engine::Patch, SC_FROM({
                 v = {{"streamingVersion", scxt::json::currentStreamingVersion},
                      {"parts", from.getParts()},
                      {"busses", from.busses}};
             }),
             SC_TO({
                 auto &patch = to;
                 patch.resetToBlankPatch();
                 findIf(v, "streamingVersion", patch.streamingVersion);

                 // TODO: I could expose the array but I want to go to a limited stream in the
                 // future
                 auto vzones = v.at("parts").get_array();
                 size_t idx{0};
                 for (const auto vz : vzones)
                 {
                     vz.to(*(patch.getPart(idx)));
                     idx++;
                 }

                 findIf(v, "busses", patch.busses);
             }))

SC_STREAMDEF(scxt::engine::Part, SC_FROM({
                 // TODO: Do a non-empty part stream with the If variant
                 v = {{"channel", from.channel}, {"groups", from.getGroups()}};
             }),
             SC_TO({
                 auto &part = to;
                 part.clearGroups();

                 findIf(v, "channel", part.channel);
                 auto vzones = v.at("groups").get_array();
                 for (const auto vz : vzones)
                 {
                     auto idx = part.addGroup() - 1;
                     vz.to(*(part.getGroup(idx)));
                 }
             }))

SC_STREAMDEF(scxt::engine::Group::GroupOutputInfo, SC_FROM({
                 v = {{"amplitude", t.amplitude},   {"pan", t.pan},
                      {"oversample", t.oversample}, {"velocitySensitivity", t.velocitySensitivity},
                      {"muted", t.muted},           {"procRouting", t.procRouting},
                      {"routeTo", (int)t.routeTo}};
             }),
             SC_TO({
                 int rt;
                 findIf(v, "amplitude", result.amplitude);
                 findIf(v, "pan", result.pan);
                 findIf(v, "muted", result.muted);
                 findIf(v, "procRouting", result.procRouting);
                 findIf(v, "velocitySensitivity", result.velocitySensitivity);
                 findIf(v, "oversample", result.oversample);
                 findIf(v, "routeTo", rt);
                 result.routeTo = (engine::BusAddress)(rt);
             }));

template <> struct scxt_traits<scxt::engine::Group>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const scxt::engine::Group &t)
    {
        v = {{"zones", t.getZones()},
             {"name", t.getName()},
             {"outputInfo", t.outputInfo},
             {"routingTable", t.routingTable},
             {"gegStorage", t.gegStorage},
             {"modulatorStorage", t.modulatorStorage},
             {"processorStorage", t.processorStorage}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::engine::Group &group)
    {
        findIf(v, "name", group.name);
        findIf(v, "gegStorage", group.gegStorage);
        findIf(v, "outputInfo", group.outputInfo);
        findIf(v, "processorStorage", group.processorStorage);
        findIf(v, "routingTable", group.routingTable);
        findIf(v, "modulatorStorage", group.modulatorStorage);
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
        group.setupOnUnstream(*(group.parentPart->parentPatch->parentEngine));
    }
};

template <> struct scxt_traits<scxt::engine::Zone::ZoneOutputInfo>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v,
                       const scxt::engine::Zone::ZoneOutputInfo &t)
    {
        v = {{"amplitude", t.amplitude},
             {"pan", t.pan},
             {"muted", t.muted},
             {"procRouting", t.procRouting},
             {"routeTo", (int)t.routeTo}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::engine::Zone::ZoneOutputInfo &zo)
    {
        int rt;
        findIf(v, "amplitude", zo.amplitude);
        findIf(v, "pan", zo.pan);
        findIf(v, "muted", zo.muted);
        findIf(v, "procRouting", zo.procRouting);
        findIf(v, "routeTo", rt);
        zo.routeTo = (engine::BusAddress)(rt);
    }
};

template <> struct scxt_traits<scxt::engine::Zone::ZoneMappingData>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v,
                       const scxt::engine::Zone::ZoneMappingData &t)
    {
        v = {{"rootKey", t.rootKey},
             {"keyboardRange", t.keyboardRange},
             {"velocityRange", t.velocityRange},
             {"pbDown", t.pbDown},
             {"pbUp", t.pbUp},

             {"exclusiveGroup", t.exclusiveGroup},
             {"velocitySens", t.velocitySens},
             {"amplitude", t.amplitude},
             {"pan", t.pan},
             {"pitchOffset", t.pitchOffset}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v,
                   scxt::engine::Zone::ZoneMappingData &zmd)
    {
        findOrSet(v, "rootKey", 60, zmd.rootKey);
        findIf(v, "keyboardRange", zmd.keyboardRange);
        findIf(v, "velocityRange", zmd.velocityRange);
        findIf(v, "pbDown", zmd.pbDown);
        findIf(v, "pbUp", zmd.pbUp);

        findIf(v, "amplitude", zmd.amplitude);
        if (SC_UNSTREAMING_FROM_THIS_OR_OLDER(0x20240603))
        {
            // amplitude used to be in percent 0...1 and now
            // is in decibels -36 to 36.
            auto db = 20 * std::log10(zmd.amplitude);
            zmd.amplitude = std::clamp(db, -36.f, 36.f);
        }

        findIf(v, "pan", zmd.pan);
        findIf(v, "pitchOffset", zmd.pitchOffset);
        findOrSet(v, "velocitySens", 1.0, zmd.velocitySens);
        findOrSet(v, "exclusiveGroup", 0, zmd.exclusiveGroup);
    }
};

STREAM_ENUM(engine::Zone::PlayMode, engine::Zone::toStringPlayMode,
            engine::Zone::fromStringPlayMode);
STREAM_ENUM(engine::Zone::LoopMode, engine::Zone::toStringLoopMode,
            engine::Zone::fromStringLoopMode);
STREAM_ENUM(engine::Zone::LoopDirection, engine::Zone::toStringLoopDirection,
            engine::Zone::fromStringLoopDirection);
STREAM_ENUM(engine::Zone::ProcRoutingPath, engine::Zone::toStringProcRoutingPath,
            engine::Zone::fromStringProcRoutingPath);
STREAM_ENUM(engine::Group::ProcRoutingPath, engine::Group::toStringProcRoutingPath,
            engine::Group::fromStringProcRoutingPath);
STREAM_ENUM(engine::Zone::VariantPlaybackMode, engine::Zone::toStringVariantPlaybackMode,
            engine::Zone::fromStringVariantPlaybackMode);

template <> struct scxt_traits<scxt::engine::Zone::AssociatedSample>
{

    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v,
                       const scxt::engine::Zone::AssociatedSample &s)
    {
        if (s.active)
        {
            v = {{"active", s.active},
                 {"id", s.sampleID},
                 {"startSample", s.startSample},
                 {"endSample", s.endSample},
                 {"startLoop", s.startLoop},
                 {"endLoop", s.endLoop},
                 {"playMode", s.playMode},
                 {"loopActive", s.loopActive},
                 {"playReverse", s.playReverse},
                 {"loopMode", s.loopMode},
                 {"loopDirection", s.loopDirection},
                 {"loopCountWhenCounted", s.loopCountWhenCounted},
                 {"loopFade", s.loopFade}};
        }
        else
        {
            v = {{"active", s.active}};
        }
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::engine::Zone::AssociatedSample &s)
    {
        findOrSet(v, "active", false, s.active);
        if (s.active)
        {
            findIf(v, "id", s.sampleID);
            findOrSet(v, "startSample", -1, s.startSample);
            findOrSet(v, "endSample", -1, s.endSample);
            findOrSet(v, "startLoop", -1, s.startLoop);
            findOrSet(v, "endLoop", -1, s.endLoop);
            findOrSet(v, "playMode", engine::Zone::PlayMode::NORMAL, s.playMode);
            findOrSet(v, "loopActive", false, s.loopActive);
            findOrSet(v, "playReverse", false, s.playReverse);
            findOrSet(v, "loopMode", engine::Zone::LoopMode::LOOP_DURING_VOICE, s.loopMode);
            findOrSet(v, "loopDirection", engine::Zone::LoopDirection::FORWARD_ONLY,
                      s.loopDirection);
            findOrSet(v, "loopFade", 0, s.loopFade);
            findOrSet(v, "loopCountWhenCounted", 0, s.loopCountWhenCounted);
        }
        else
        {
            s = scxt::engine::Zone::AssociatedSample();
            assert(!s.active);
        }
    }
};

SC_STREAMDEF(scxt::engine::Zone::AssociatedSampleSet, SC_FROM({
                 v = {{"samples", t.samples}, {"variantPlaybackMode", t.variantPlaybackMode}};
             }),
             SC_TO({
                 findIf(v, "samples", result.samples);
                 findIf(v, "variantPlaybackMode", result.variantPlaybackMode);
             }));

template <> struct scxt_traits<scxt::engine::Zone>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const scxt::engine::Zone &t)
    {
        /*
        auto rtArray = toIndexedArrayIf<Traits>(t.routingTable, [](const auto &r) {
            // TODO we really don't need this we should just stream the table
            return (r.dst != scxt::modulation::vmd_none || r.src != scxt::modulation::vms_none ||
                    r.depth != 0 || !r.active || r.curve != scxt::modulation::modc_none);
        });
         */

        v = {{"sampleData", t.sampleData},     {"mappingData", t.mapping},
             {"outputInfo", t.outputInfo},     {"processorStorage", t.processorStorage},
             {"routingTable", t.routingTable}, {"modulatorStorage", t.modulatorStorage},
             {"aegStorage", t.egStorage[0]},   {"eg2Storage", t.egStorage[1]}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::engine::Zone &zone)
    {
        findIf(v, "sampleData", zone.sampleData);
        findIf(v, "mappingData", zone.mapping);
        findIf(v, "outputInfo", zone.outputInfo);
        fromArrayWithSizeDifference<Traits>(v.at("processorStorage"), zone.processorStorage);

        findIf(v, "routingTable", zone.routingTable);
        findIf(v, "modulatorStorage", zone.modulatorStorage);
        findOrDefault(v, "aegStorage", zone.egStorage[0]);
        findOrDefault(v, "eg2Storage", zone.egStorage[1]);
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
        findIf(v, "keyStart", r.keyStart);
        findIf(v, "keyEnd", r.keyEnd);
        findIf(v, "fadeStart", r.fadeStart);
        findIf(v, "fadeEnd", r.fadeEnd);
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
        findIf(v, "velStart", r.velStart);
        findIf(v, "velEnd", r.velEnd);
        findIf(v, "fadeStart", r.fadeStart);
        findIf(v, "fadeEnd", r.fadeEnd);
    }
};

template <> struct scxt_traits<engine::Engine::EngineStatusMessage>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v,
                       const engine::Engine::EngineStatusMessage &t)
    {
        v = {{"isAudioRunning", t.isAudioRunning},
             {"sampleRate", t.sampleRate},
             {"runningEnvironment", t.runningEnvironment}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, engine::Engine::EngineStatusMessage &r)
    {
        findIf(v, "isAudioRunning", r.isAudioRunning);
        findIf(v, "sampleRate", r.sampleRate);
        findIf(v, "runningEnvironment", r.runningEnvironment);
    }
};

template <> struct scxt_traits<engine::Bus>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const engine::Bus &t)
    {
        v = {{"busSendStorage", t.busSendStorage}, {"busEffectStorage", t.busEffectStorage}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, engine::Bus &r)
    {
        findIf(v, "busSendStorage", r.busSendStorage);
        findIf(v, "busEffectStorage", r.busEffectStorage);
        r.resetSendState();
    }
};

STREAM_ENUM(engine::Bus::BusSendStorage::AuxLocation,
            engine::Bus::BusSendStorage::toStringAuxLocation,
            engine::Bus::BusSendStorage::fromStringAuxLocation)

template <> struct scxt_traits<engine::Bus::BusSendStorage>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const engine::Bus::BusSendStorage &t)
    {
        v = {{"supportsSends", t.supportsSends},
             {"auxLocation", t.auxLocation},
             {"sendLevels", t.sendLevels},
             {"level", t.level},
             {"mute", t.mute},
             {"solo", t.solo},
             {"pan", t.pan},
             {"pluginOutputBus", t.pluginOutputBus}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, engine::Bus::BusSendStorage &r)
    {
        findIf(v, "supportsSends", r.supportsSends);
        findIf(v, "auxLocation", r.auxLocation);
        findIf(v, "sendLevels", r.sendLevels);
        findIf(v, "level", r.level);
        findIf(v, "mute", r.mute);
        findIf(v, "solo", r.solo);
        findIf(v, "pan", r.pan);
        findIf(v, "pluginOutputBus", r.pluginOutputBus);
    }
};

template <> struct scxt_traits<engine::BusEffectStorage>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const engine::BusEffectStorage &t)
    {
        if (t.type == scxt::engine::AvailableBusEffects::none)
        {
            v = {{"type", scxt::engine::toStringAvailableBusEffects(t.type)}};
        }
        else
        {
            v = {{"type", scxt::engine::toStringAvailableBusEffects(t.type)},
                 {"isActive", t.isActive},
                 {"params", t.params}};
        }
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, engine::BusEffectStorage &r)
    {
        std::string tp;
        findIf(v, "type", tp);
        r.type = scxt::engine::fromStringAvailableBusEffects(tp);
        if (r.type == engine::AvailableBusEffects::none)
        {
            r = engine::BusEffectStorage();
            r.type = engine::AvailableBusEffects::none;
        }
        else
        {
            findIf(v, "params", r.params);
            findOrSet(v, "isActive", true, r.isActive);
        }
    }
};

template <> struct scxt_traits<engine::Patch::Busses>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const engine::Patch::Busses &t)
    {
        // TODO fixme don't stream as int
        v = {{"mainBus", t.mainBus}, {"partBusses", t.partBusses}, {"auxBusses", t.auxBusses}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, engine::Patch::Busses &r)
    {
        findIf(v, "mainBus", r.mainBus);
        findIf(v, "partBusses", r.partBusses);
        findIf(v, "auxBusses", r.auxBusses);

        r.reconfigureSolo();
        r.reconfigureOutputBusses();
    }
};
} // namespace scxt::json
#endif // SHORTCIRCUIT_ENGINE_TRAITS_H
