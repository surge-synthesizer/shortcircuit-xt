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

#ifndef SCXT_SRC_JSON_MODULATION_TRAITS_H
#define SCXT_SRC_JSON_MODULATION_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "modulation/voice_matrix.h"
#include "modulation/group_matrix.h"
#include "modulation/modulators/steplfo.h"
#include "modulation/modulators/curvelfo.h"
#include "modulation/modulator_storage.h"

namespace scxt::json
{

SC_STREAMDEF(scxt::modulation::modulators::StepLFOStorage, SC_FROM({
                 v = {{"data", t.data},
                      {"repeat", t.repeat},
                      {"rateIsForSingleStep", t.rateIsForSingleStep},
                      {"smooth", t.smooth}};
             }),
             SC_TO({
                 findIf(v, "data", result.data);
                 findIf(v, "repeat", result.repeat);
                 findIf(v, "smooth", result.smooth);
                 findIf(v, "rateIsForSingleStep", result.rateIsForSingleStep);
             }))

SC_STREAMDEF(scxt::modulation::modulators::CurveLFOStorage, SC_FROM({
                 v = tao::json::empty_object;
                 addUnlessDefault<val_t>(v, "attack", 0.f, from.attack);
                 addUnlessDefault<val_t>(v, "deform", 0.f, from.deform);
                 addUnlessDefault<val_t>(v, "delay", 0.f, from.delay);
                 addUnlessDefault<val_t>(v, "release", 0.f, from.release);
                 addUnlessDefault<val_t>(v, "unipolar", false, from.unipolar);
                 addUnlessDefault<val_t>(v, "useenv", false, from.useenv);
             }),
             SC_TO({
                 const auto &object = v.get_object();
                 findOrSet(v, "deform", 0.f, result.deform);
                 findOrSet(v, "delay", 0.f, result.delay);
                 findOrSet(v, "attack", 0.f, result.attack);
                 findOrSet(v, "release", 0.f, result.release);
                 findOrSet(v, "unipolar", false, result.unipolar);
                 findOrSet(v, "useenv", false, result.useenv);
             }))

SC_STREAMDEF(scxt::modulation::modulators::EnvLFOStorage, SC_FROM({
                 v = tao::json::empty_object;
                 addUnlessDefault<val_t>(v, "attack", 0.f, from.attack);
                 addUnlessDefault<val_t>(v, "decay", 0.f, from.decay);
                 addUnlessDefault<val_t>(v, "deform", 0.f, from.deform);
                 addUnlessDefault<val_t>(v, "delay", 0.f, from.delay);
                 addUnlessDefault<val_t>(v, "hold", 0.f, from.hold);
                 addUnlessDefault<val_t>(v, "release", 0.f, from.release);
                 addUnlessDefault<val_t>(v, "sustain", 1.f, from.sustain);
             }),
             SC_TO({
                 const auto &object = v.get_object();
                 findOrSet(v, "deform", 0.f, result.deform);
                 findOrSet(v, "delay", 0.f, result.delay);
                 findOrSet(v, "attack", 0.f, result.attack);
                 findOrSet(v, "hold", 0.f, result.hold);
                 findOrSet(v, "decay", 0.f, result.decay);
                 findOrSet(v, "sustain", 1.f, result.sustain);
                 findOrSet(v, "release", 0.f, result.release);
             }))

SC_STREAMDEF(modulation::modulators::AdsrStorage, SC_FROM({
                 v = tao::json::empty_object;
                 addUnlessDefault<val_t>(v, "a", 0.f, from.a);
                 addUnlessDefault<val_t>(v, "d", 0.f, from.d);
                 addUnlessDefault<val_t>(v, "h", 0.f, from.h);
                 addUnlessDefault<val_t>(v, "s", 1.f, from.s);
                 addUnlessDefault<val_t>(v, "r", 0.5f, from.r);
                 addUnlessDefault<val_t>(v, "aShape", 0.f, from.aShape);
                 addUnlessDefault<val_t>(v, "dShape", 0.f, from.dShape);
                 addUnlessDefault<val_t>(v, "rShape", 0.f, from.rShape);
                 addUnlessDefault<val_t>(v, "isDigital", true, from.isDigital);
             }),
             SC_TO({
                 findOrSet(v, "a", 0.f, result.a);
                 findOrSet(v, "h", 0.f, result.h);
                 findOrSet(v, "d", 0.f, result.d);
                 findOrSet(v, "s", 1.f, result.s);
                 findOrSet(v, "r", 0.5f, result.r);
                 findOrSet(v, "isDigital", true, result.isDigital);
                 findOrSet(v, "aShape", 0.f, result.aShape);
                 findOrSet(v, "dShape", 0.f, result.dShape);
                 findOrSet(v, "rShape", 0.f, result.rShape);
             }))

STREAM_ENUM(modulation::ModulatorStorage::ModulatorShape,
            modulation::ModulatorStorage::toStringModulatorShape,
            modulation::ModulatorStorage::fromStringModulatorShape);

STREAM_ENUM(modulation::ModulatorStorage::TriggerMode,
            modulation::ModulatorStorage::toStringTriggerMode,
            modulation::ModulatorStorage::fromStringTriggerMode);

SC_STREAMDEF(scxt::modulation::ModulatorStorage, SC_FROM({
                 v = {{"modulatorShape", t.modulatorShape},
                      {"triggerMode", t.triggerMode},
                      {"rate", t.rate},
                      {"start_phase", t.start_phase},
                      {"temposync", t.temposync},

                      {"curveLfoStorage", t.curveLfoStorage},
                      {"stepLfoStorage", t.stepLfoStorage},
                      {"envLfoStorage", t.envLfoStorage}};
             }),
             SC_TO({
                 const auto &object = v.get_object();
                 findIf(v, "modulatorShape", result.modulatorShape);
                 findIf(v, "triggerMode", result.triggerMode);
                 findIf(v, "rate", result.rate);
                 findIf(v, "start_phase", result.start_phase);
                 findIf(v, "temposync", result.temposync);
                 findIf(v, "curveLfoStorage", result.curveLfoStorage);
                 findIf(v, "stepLfoStorage", result.stepLfoStorage);
                 findIf(v, "envLfoStorage", result.envLfoStorage);

                 result.configureCalculatedState();
             }))

SC_STREAMDEF(scxt::modulation::shared::TargetIdentifier, SC_FROM({
                 v = {{"g", t.gid}, {"t", t.tid}, {"i", t.index}};
             }),
             SC_TO({
                 findIf(v, "g", result.gid);
                 findIf(v, "t", result.tid);
                 findIf(v, "i", result.index);
             }));

SC_STREAMDEF(scxt::modulation::shared::SourceIdentifier, SC_FROM({
                 v = {{"g", t.gid}, {"t", t.tid}, {"i", t.index}};
             }),
             SC_TO({
                 findIf(v, "g", result.gid);
                 findIf(v, "t", result.tid);
                 findIf(v, "i", result.index);
             }));

SC_STREAMDEF(scxt::modulation::shared::RoutingExtraPayload, SC_FROM({
                 v = {{"selC", t.selConsistent},
                      {"targetMetadata", t.targetMetadata},
                      {"targetBaseValue", t.targetBaseValue}};
             }),
             SC_TO({
                 findIf(v, "selC", result.selConsistent);
                 findIf(v, "targetMetadata", result.targetMetadata);
                 findIf(v, "targetBaseValue", result.targetBaseValue);
             }));

// Its a mild bummer we have to dup this for group and zone but they differ by trait so have
// distinct types Ahh well. Fixable with an annoying refactor but leave it for now
SC_STREAMDEF(scxt::voice::modulation::Matrix::RoutingTable::Routing, SC_FROM({
                 if (t.hasDefaultValues())
                 {
                     v = tao::json::empty_object;
                 }
                 else
                 {
                     v = {{"active", t.active},       {"source", t.source},
                          {"sourceVia", t.sourceVia}, {"target", t.target},
                          {"curve", t.curve},         {"depth", t.depth},
                          {"srcLMS", t.sourceLagMS},  {"srVLMS", t.sourceViaLagMS},
                          {"srcLE", t.sourceLagExp},  {"srVLE", t.sourceViaLagExp}};
                     if (SC_STREAMING_FOR_IN_PROCESS)
                         addToObject<val_t>(v, "extraPayload", t.extraPayload);
                 }
             }),
             SC_TO({
                 result = rt_t();
                 findIf(v, "active", result.active);
                 findIf(v, "source", result.source);
                 findIf(v, "sourceVia", result.sourceVia);
                 findIf(v, "target", result.target);
                 findIf(v, "curve", result.curve);
                 findIf(v, "depth", result.depth);
                 findIf(v, "extraPayload", result.extraPayload);
                 findOrSet(v, "srcLMS", 0, result.sourceLagMS);
                 findOrSet(v, "srVLMS", 0, result.sourceViaLagMS);
                 findOrSet(v, "srcLE", true, result.sourceLagExp);
                 findOrSet(v, "srVLE", true, result.sourceViaLagExp);
             }));

SC_STREAMDEF(scxt::voice::modulation::Matrix::RoutingTable, SC_FROM({
                 v = {{"routes", t.routes}};
             }),
             SC_TO({
                 const auto &object = v.get_object();

                 std::fill(result.routes.begin(), result.routes.end(), rt_t::Routing());
                 if (v.find("routes"))
                 {
                     fromArrayWithSizeDifference(v.at("routes"), result.routes);
                 }
             }));

SC_STREAMDEF(scxt::modulation::GroupMatrix::RoutingTable::Routing, SC_FROM({
                 if (t.hasDefaultValues())
                 {
                     v = tao::json::empty_object;
                 }
                 else
                 {
                     v = {{"active", t.active}, {"source", t.source}, {"sourceVia", t.sourceVia},
                          {"target", t.target}, {"curve", t.curve},   {"depth", t.depth}};
                     if (SC_STREAMING_FOR_IN_PROCESS)
                         addToObject<val_t>(v, "extraPayload", t.extraPayload);
                 }
             }),
             SC_TO({
                 result = rt_t();
                 findIf(v, "active", result.active);
                 findIf(v, "source", result.source);
                 findIf(v, "sourceVia", result.sourceVia);
                 findIf(v, "target", result.target);
                 findIf(v, "curve", result.curve);
                 findIf(v, "depth", result.depth);
                 findIf(v, "extraPayload", result.extraPayload);
             }));

SC_STREAMDEF(scxt::modulation::GroupMatrix::RoutingTable, SC_FROM({
                 v = {{"routes", t.routes}};
             }),
             SC_TO({
                 const auto &object = v.get_object();

                 std::fill(result.routes.begin(), result.routes.end(), rt_t::Routing());
                 if (v.find("routes"))
                 {
                     fromArrayWithSizeDifference(v.at("routes"), result.routes);
                 }
             }));

} // namespace scxt::json

#endif // SHORTCIRCUIT_MODULATION_TRAITS_H
