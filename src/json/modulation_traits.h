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
                 v = {{"deform", t.deform},   {"delay", t.delay},       {"attack", t.attack},
                      {"release", t.release}, {"unipolar", t.unipolar}, {"useenv", t.useenv}};
             }),
             SC_TO({
                 const auto &object = v.get_object();
                 findIf(v, "deform", result.deform);
                 findIf(v, "delay", result.delay);
                 findIf(v, "attack", result.attack);
                 findIf(v, "release", result.release);
                 findIf(v, "unipolar", result.unipolar);
                 findIf(v, "useenv", result.useenv);
             }))

SC_STREAMDEF(scxt::modulation::modulators::EnvLFOStorage, SC_FROM({
                 v = {{"deform", t.deform},  {"delay", t.delay}, {"attack", t.attack},
                      {"hold", t.hold},      {"decay", t.decay}, {"sustain", t.sustain},
                      {"release", t.release}};
             }),
             SC_TO({
                 const auto &object = v.get_object();
                 findIf(v, "deform", result.deform);
                 findIf(v, "delay", result.delay);
                 findIf(v, "attack", result.attack);
                 findIf(v, "hold", result.hold);
                 findIf(v, "decay", result.decay);
                 findIf(v, "sustain", result.sustain);
                 findIf(v, "release", result.release);
             }))

SC_STREAMDEF(modulation::modulators::AdsrStorage, SC_FROM({
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
                 v = {{"active", t.active},
                      {"source", t.source},
                      {"sourceVia", t.sourceVia},
                      {"target", t.target},
                      {"curve", t.curve},
                      {"depth", t.depth},
                      {"extraPayload", t.extraPayload}};
             }),
             SC_TO({
                 findIf(v, "active", result.active);
                 findIf(v, "source", result.source);
                 findIf(v, "sourceVia", result.sourceVia);
                 findIf(v, "target", result.target);
                 findIf(v, "curve", result.curve);
                 findIf(v, "depth", result.depth);
                 findIf(v, "extraPayload", result.extraPayload);
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
                 v = {{"active", t.active},
                      {"source", t.source},
                      {"sourceVia", t.sourceVia},
                      {"target", t.target},
                      {"curve", t.curve},
                      {"depth", t.depth},
                      {"extraPayload", t.extraPayload}};
             }),
             SC_TO({
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
