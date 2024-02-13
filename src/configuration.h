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

#ifndef SCXT_SRC_CONFIGURATION_H
#define SCXT_SRC_CONFIGURATION_H

#include <cstdint>
#include "infrastructure/sse_include.h"

namespace scxt
{
static constexpr uint16_t blockSize{16};
static constexpr uint16_t blockSizeQuad{16 >> 2};
static constexpr uint16_t numParts{16};
static constexpr uint16_t numAux{4};
static constexpr uint16_t maxOutputs{1 + numParts + numAux}; // 16 parts, 4 aux, a main
static constexpr uint16_t maxBusses{maxOutputs};
static constexpr uint16_t mainOutput{0};
static constexpr uint16_t firstPartOutput{1};
static constexpr uint16_t firstAuxOutput{firstPartOutput + numParts};

static constexpr uint16_t numNonMainPluginOutputs{20};
static constexpr uint16_t numPluginOutputs{numNonMainPluginOutputs + 1};

static constexpr uint16_t maxVoices{256};

// some battles are not worth it
static constexpr uint16_t BLOCK_SIZE{blockSize};
static constexpr uint16_t BLOCK_SIZE_QUAD{blockSizeQuad};

static constexpr uint16_t maxBusEffectParams{12};
static constexpr uint16_t maxEffectsPerBus{12};
static constexpr uint16_t maxSendsPerBus{12};

static constexpr size_t maxProcessorFloatParams{9};
static constexpr size_t maxProcessorIntParams{3};

static constexpr uint16_t lfosPerZone{3};
static constexpr uint16_t maxSamplesPerZone{3};

static constexpr uint16_t lfosPerGroup{3};
static constexpr uint16_t egPerGroup{2};

static constexpr uint16_t processorsPerZoneAndGroup{4};

} // namespace scxt
#endif // __SCXT__CONFIGURATION_H
