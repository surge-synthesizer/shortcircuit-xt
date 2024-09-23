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
#include "engine/macros.h"
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

SC_STREAMDEF(scxt::engine::Engine, SC_FROM({
                 if (SC_STREAMING_FOR_IN_PROCESS)
                 {
                     /*
                      * If you see this warning you probably want to make an
                      * engine::Engine::StreamGuard with DAW or MULTi before you
                      * call your JSON conversion, unless its just a debug json
                      * dump, in which case this is OK
                      */
                     SCLOG("Warning: Engine is streaming for state 'IN_PROCESS'");
                 }

                 v = {{"streamingVersion", scxt::currentStreamingVersion},
                      {"streamingVersionHumanReadable",
                       scxt::humanReadableVersion(scxt::currentStreamingVersion)},
                      {"patch", from.getPatch()},
                      {"selectionManager", from.getSelectionManager()},
                      {"sampleManager", from.getSampleManager()}};
             }),
             SC_TO({
                 assert(to.getMessageController()->threadingChecker.isSerialThread());
                 auto sv{0};
                 findIf(v, "streamingVersion", sv);
                 SCLOG("Unstreaming engine state. Stream version : "
                       << scxt::humanReadableVersion(sv));

                 engine::Engine::UnstreamGuard sg(sv);

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
                 v = {{"parts", from.getParts()}, {"busses", from.busses}};
             }),
             SC_TO({
                 auto &patch = to;
                 patch.resetToBlankPatch();

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

SC_STREAMDEF(scxt::engine::Macro, SC_FROM({
                 v = {{"p", t.part}, {"i", t.index}, {"v", t.value}};

                 addUnlessDefault<val_t>(v, "bp", false, t.isBipolar);
                 addUnlessDefault<val_t>(v, "st", false, t.isStepped);
                 addUnlessDefault<val_t>(v, "sc", (size_t)1, t.stepCount);
                 addUnlessDefault<val_t>(v, "nm", scxt::engine::Macro::defaultNameFor(t.index),
                                         t.name);
             }),
             SC_TO({
                 findIf(v, "v", result.value);
                 findIf(v, "i", result.index);
                 findIf(v, "p", result.part);
                 findOrSet(v, "bp", false, result.isBipolar);
                 findOrSet(v, "st", false, result.isStepped);
                 findOrSet(v, "sc", 1, result.stepCount);
                 findOrSet(v, "nm", scxt::engine::Macro::defaultNameFor(result.index), result.name);
             }));

SC_STREAMDEF(
    scxt::engine::Part::PartConfiguration,
    SC_FROM(v = {{"a", from.active}, {"c", from.channel}, {"m", from.mute}, {"s", from.solo}};),
    SC_TO({
        findOrSet(v, "c", scxt::engine::Part::PartConfiguration::omniChannel, to.channel);
        findOrSet(v, "a", true, to.active);
        findOrSet(v, "m", false, to.mute);
        findOrSet(v, "s", false, to.solo);
    }));

SC_STREAMDEF(
    scxt::engine::Part, SC_FROM({
        // TODO: Do a non-empty part stream with the If variant
        v = {{"config", from.configuration}, {"groups", from.getGroups()}, {"macros", from.macros}};
        if (SC_STREAMING_FOR_PART)
        {
            addToObject<val_t>(v, "streamingVersion", currentStreamingVersion);
            addToObject<val_t>(v, "streamedForPart", true);
            assert(from.parentPatch->parentEngine);
            addToObject<val_t>(
                v, "samplesUsedByPart",
                from.parentPatch->parentEngine->getSampleManager()->getSampleAddressesFor(
                    from.getSamplesUsedByPart()));
            SCLOG("ToDo - stream sample manager subset");
        }
    }),
    SC_TO({
        auto &part = to;
        part.clearGroups();

        std::unique_ptr<engine::Engine::UnstreamGuard> sg;
        bool streamedForPart{false};
        findOrSet(v, "streamedForPart", false, streamedForPart);
        if (streamedForPart)
        {
            uint64_t partStreamingVersion{0};
            findIf(v, "streamingVersion", partStreamingVersion);
            SCLOG("Unstreaming part state. Stream version : "
                  << scxt::humanReadableVersion(partStreamingVersion));

            scxt::sample::SampleManager::sampleAddressesAndIds_t samples;
            findIf(v, "samplesUsedByPart", samples);
            to.parentPatch->parentEngine->getSampleManager()->restoreFromSampleAddressesAndIDs(
                samples);

            sg = std::make_unique<engine::Engine::UnstreamGuard>(partStreamingVersion);
        }

        if (SC_UNSTREAMING_FROM_PRIOR_TO(0x2024'08'18))
        {
            findIf(v, "channel", part.configuration.channel);
        }
        else
        {
            findIf(v, "config", part.configuration);
        }
        findIf(v, "macros", part.macros);
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
                 findIf(v, "amplitude", result.amplitude);
                 findIf(v, "pan", result.pan);
                 findIf(v, "muted", result.muted);
                 findIf(v, "procRouting", result.procRouting);
                 findIf(v, "velocitySensitivity", result.velocitySensitivity);
                 findIf(v, "oversample", result.oversample);
                 int rt{engine::BusAddress::DEFAULT_BUS};
                 findIf(v, "routeTo", rt);
                 result.routeTo = (engine::BusAddress)(rt);
             }));

SC_STREAMDEF(scxt::engine::Group, SC_FROM({
                 v = {{"zones", t.getZones()},
                      {"name", t.getName()},
                      {"outputInfo", t.outputInfo},
                      {"routingTable", t.routingTable},
                      {"gegStorage", t.gegStorage},
                      {"modulatorStorage", t.modulatorStorage},
                      {"processorStorage", t.processorStorage}};
             }),
             SC_TO({
                 auto &group = to;
                 findIf(v, "name", group.name);
                 findIf(v, "gegStorage", group.gegStorage);
                 findIf(v, "outputInfo", group.outputInfo);
                 findIf(v, "processorStorage", group.processorStorage);
                 findIf(v, "routingTable", group.routingTable);
                 findIfArray(v, "modulatorStorage", group.modulatorStorage);
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
                         group.getZone(idx)->setupOnUnstream(
                             *(group.parentPart->parentPatch->parentEngine));
                     }
                 }
                 group.setupOnUnstream(*(group.parentPart->parentPatch->parentEngine));
             }));

SC_STREAMDEF(scxt::engine::Zone::ZoneOutputInfo, SC_FROM({
                 v = {{"amp", t.amplitude}, {"pan", t.pan}, {"to", (int)t.routeTo}};
                 addUnlessDefault<val_t>(v, "prt", engine::Zone::ProcRoutingPath::procRoute_linear,
                                         t.procRouting);
                 addUnlessDefault<val_t>(v, "muted", false, t.muted);
             }),
             SC_TO({
                 auto &zo = to;
                 findIf(v, {"amp", "amplitude"}, zo.amplitude);
                 findIf(v, "pan", zo.pan);
                 findOrSet(v, "muted", false, zo.muted);
                 findOrSet(v, {"prt", "procRouting"},
                           engine::Zone::ProcRoutingPath::procRoute_linear, zo.procRouting);
                 int rt{engine::BusAddress::DEFAULT_BUS};
                 findIf(v, {"to", "routeTo"}, rt);

                 // There was an error which streamed garbage for a while.
                 // Might as well be defensive hereon out even though I
                 // could bump
                 if (rt >= engine::BusAddress::ERROR_BUS &&
                     rt <= engine::BusAddress::AUX_0 + numAux)
                     zo.routeTo = (engine::BusAddress)(rt);
                 else
                     zo.routeTo = engine::BusAddress::DEFAULT_BUS;
             }));

SC_STREAMDEF(scxt::engine::Zone::ZoneMappingData, SC_FROM({
                 v = {{"rootKey", t.rootKey},
                      {"keyR", t.keyboardRange},
                      {"velR", t.velocityRange},
                      {"pbDown", t.pbDown},
                      {"pbUp", t.pbUp},

                      {"exclusiveGroup", t.exclusiveGroup},
                      {"velocitySens", t.velocitySens},
                      {"amplitude", t.amplitude},
                      {"pan", t.pan},
                      {"pitchOffset", t.pitchOffset}};
             }),
             SC_TO({
                 auto &zmd = to;
                 findOrSet(v, "rootKey", 60, zmd.rootKey);
                 findIf(v, {"keyR", "keyboardRange"}, zmd.keyboardRange);
                 findIf(v, {"velR", "velocityRange"}, zmd.velocityRange);
                 findIf(v, "pbDown", zmd.pbDown);
                 findIf(v, "pbUp", zmd.pbUp);

                 findIf(v, "amplitude", zmd.amplitude);
                 if (SC_UNSTREAMING_FROM_THIS_OR_OLDER(0x2024'06'03))
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
             }));

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
STREAM_ENUM(dsp::InterpolationTypes, dsp::toStringInterpolationTypes,
            dsp::fromStringInterpolationTypes);

SC_STREAMDEF(scxt::engine::Zone::SingleVariant, SC_FROM({
                 auto &s = from;
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
                          {"interpolationType", s.interpolationType},
                          {"loopFade", s.loopFade},
                          {"normalizationAmplitude", s.normalizationAmplitude},
                          {"pitchOffset", s.pitchOffset},
                          {"amplitude", s.amplitude}};
                 }
                 else
                 {
                     v = tao::json::empty_object;
                 }
             }),
             SC_TO({
                 auto &s = to;
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
                     findOrSet(v, "loopMode", engine::Zone::LoopMode::LOOP_DURING_VOICE,
                               s.loopMode);
                     findOrSet(v, "loopDirection", engine::Zone::LoopDirection::FORWARD_ONLY,
                               s.loopDirection);
                     findOrSet(v, "loopFade", 0, s.loopFade);
                     findOrSet(v, "interpolationType", dsp::InterpolationTypes::Sinc,
                               s.interpolationType);
                     findOrSet(v, "loopCountWhenCounted", 0, s.loopCountWhenCounted);
                     findOrSet(v, "normalizationAmplitude", 0.f, s.normalizationAmplitude);
                     findOrSet(v, "pitchOffset", 0.f, s.pitchOffset);
                     findOrSet(v, "amplitude", 0.f, s.amplitude);
                 }
                 else
                 {
                     s = scxt::engine::Zone::SingleVariant();
                     assert(!s.active);
                 }
             }));

SC_STREAMDEF(scxt::engine::Zone::Variants, SC_FROM({
                 v = {{"variants", t.variants}, {"variantPlaybackMode", t.variantPlaybackMode}};
             }),
             SC_TO({
                 findIf(v, {"variants", "samples"}, result.variants);
                 findIf(v, "variantPlaybackMode", result.variantPlaybackMode);
             }));

SC_STREAMDEF(scxt::engine::Zone, SC_FROM({
                 // special case. before given name for empty samples we used id.
                 // crystalize this name for old patches. You can probably remove this
                 // code in october 2024.
                 auto useGivenName = t.givenName;
                 if (useGivenName.empty() && !t.variantData.variants[0].active)
                 {
                     SCLOG("Crystalizing given name on stream");
                     useGivenName = t.getName();
                 }
                 // but just that bit

                 v = {{"variantData", t.variantData},   {"mappingData", t.mapping},
                      {"outputInfo", t.outputInfo},     {"processorStorage", t.processorStorage},
                      {"routingTable", t.routingTable}, {"modulatorStorage", t.modulatorStorage},
                      {"aegStorage", t.egStorage[0]},   {"eg2Storage", t.egStorage[1]},
                      {"givenName", useGivenName}};
             }),
             SC_TO({
                 auto &zone = to;
                 findIf(v, {"variantData", "sampleData"}, zone.variantData);
                 findIf(v, "mappingData", zone.mapping);
                 findIf(v, "outputInfo", zone.outputInfo);
                 fromArrayWithSizeDifference<Traits>(v.at("processorStorage"),
                                                     zone.processorStorage);

                 findIf(v, "routingTable", zone.routingTable);
                 findIfArray(v, "modulatorStorage", zone.modulatorStorage);
                 findOrDefault(v, "aegStorage", zone.egStorage[0]);
                 findOrDefault(v, "eg2Storage", zone.egStorage[1]);
                 findOrSet(v, "givenName", "", zone.givenName);
                 zone.onRoutingChanged();
             }));

SC_STREAMDEF(scxt::engine::KeyboardRange, SC_FROM({
                 v = {{"ks", t.keyStart}, {"ke", t.keyEnd}, {"fs", t.fadeStart}, {"fe", t.fadeEnd}};
             }),
             SC_TO({
                 findIf(v, {"ks", "keyStart"}, to.keyStart);
                 findIf(v, {"ke", "keyEnd"}, to.keyEnd);
                 findIf(v, {"fs", "fadeStart"}, to.fadeStart);
                 findIf(v, {"fe", "fadeEnd"}, to.fadeEnd);
             }));
SC_STREAMDEF(scxt::engine::VelocityRange, SC_FROM({
                 v = {{"vs", t.velStart}, {"ve", t.velEnd}, {"fs", t.fadeStart}, {"fe", t.fadeEnd}};
             }),
             SC_TO({
                 findIf(v, {"vs", "velStart"}, to.velStart);
                 findIf(v, {"ve", "velEnd"}, to.velEnd);
                 findIf(v, {"fs", "fadeStart"}, to.fadeStart);
                 findIf(v, {"fe", "fadeEnd"}, to.fadeEnd);
             }));

SC_STREAMDEF(engine::Engine::EngineStatusMessage, SC_FROM({
                 v = {{"isAudioRunning", t.isAudioRunning},
                      {"sampleRate", t.sampleRate},
                      {"runningEnvironment", t.runningEnvironment}};
             }),
             SC_TO({
                 findIf(v, "isAudioRunning", to.isAudioRunning);
                 findIf(v, "sampleRate", to.sampleRate);
                 findIf(v, "runningEnvironment", to.runningEnvironment);
             }));

SC_STREAMDEF(
    engine::Bus, SC_FROM({
        v = {{"busSendStorage", t.busSendStorage}, {"busEffectStorage", t.busEffectStorage}};
    }),
    SC_TO({
        findIf(v, "busSendStorage", to.busSendStorage);
        findIf(v, "busEffectStorage", to.busEffectStorage);
        to.resetSendState();
    }));

STREAM_ENUM_WITH_DEFAULT(engine::Bus::BusSendStorage::AuxLocation,
                         engine::Bus::BusSendStorage::AuxLocation::POST_VCA,
                         engine::Bus::BusSendStorage::toStringAuxLocation,
                         engine::Bus::BusSendStorage::fromStringAuxLocation)

SC_STREAMDEF(engine::Bus::BusSendStorage, SC_FROM({
                 v = {{"supportsSends", t.supportsSends},
                      {"auxLocation", t.auxLocation},
                      {"sendLevels", t.sendLevels},
                      {"pluginOutputBus", t.pluginOutputBus}};
                 addUnlessDefault<val_t>(v, "mute", false, t.mute);
                 addUnlessDefault<val_t>(v, "solo", false, t.solo);
                 addUnlessDefault<val_t>(v, "level", 1.f, t.level);
                 addUnlessDefault<val_t>(v, "pan", 0.f, t.pan);
             }),
             SC_TO({
                 findIf(v, "supportsSends", to.supportsSends);
                 findIf(v, "auxLocation", to.auxLocation);
                 findIf(v, "sendLevels", to.sendLevels);
                 findOrSet(v, "level", 1.f, to.level);
                 findOrSet(v, "mute", false, to.mute);
                 findOrSet(v, "solo", false, to.solo);
                 findOrSet(v, "pan", 0.f, to.pan);
                 findIf(v, "pluginOutputBus", to.pluginOutputBus);
             }));

SC_STREAMDEF(engine::BusEffectStorage, SC_FROM({
                 if (from.type == scxt::engine::AvailableBusEffects::none)
                 {
                     v = tao::json::empty_object;
                 }
                 else
                 {
                     v = {{"t", scxt::engine::toStringAvailableBusEffects(from.type)},
                          {"isActive", from.isActive},
                          {"p", from.params},
                          {"deact", from.deact},
                          {"temposync", from.isTemposync},
                          {"streamingVersion", from.streamingVersion}};
                 }
             }),
             SC_TO({
                 std::string tp{
                     scxt::engine::toStringAvailableBusEffects(engine::AvailableBusEffects::none)};
                 findIf(v, {"t", "type"}, tp);
                 to.type = scxt::engine::fromStringAvailableBusEffects(tp);
                 if (to.type == engine::AvailableBusEffects::none)
                 {
                     to = engine::BusEffectStorage();
                     to.type = engine::AvailableBusEffects::none;
                 }
                 else
                 {
                     findIf(v, {"p", "params"}, to.params);
                     findOrSet(v, "isActive", true, to.isActive);
                     findIf(v, "deact", to.deact);
                     findIf(v, "temposync", to.isTemposync);

                     findOrSet(v, "streamingVersion", -1, result.streamingVersion);

                     /*
                      * If engine stream correct here
                      */
                     if (result.streamingVersion > 0)
                     {
                         auto [csv, fn] =
                             scxt::engine::getBusEffectRemapStreamingFunction(result.type);
                         if (csv != result.streamingVersion)
                         {
                             assert(fn);
                             if (fn)
                                 fn(result.streamingVersion, result.params.data());
                             result.streamingVersion = csv;
                         }
                     }
                 }
             }));

SC_STREAMDEF(
    engine::Patch::Busses, SC_FROM({
        v = {{"mainBus", t.mainBus}, {"partBusses", t.partBusses}, {"auxBusses", t.auxBusses}};
    }),
    SC_TO({
        findIf(v, "mainBus", to.mainBus);
        findIf(v, "partBusses", to.partBusses);
        findIf(v, "auxBusses", to.auxBusses);

        to.reconfigureSolo();
        to.reconfigureOutputBusses();
    }));

} // namespace scxt::json
#endif // SHORTCIRCUIT_ENGINE_TRAITS_H
