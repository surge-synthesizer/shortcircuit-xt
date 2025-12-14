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

#ifndef SCXT_SRC_SCXT_CORE_CONFIGURATION_H
#define SCXT_SRC_SCXT_CORE_CONFIGURATION_H

#include <stddef.h> // for size_t
#include <cstdint>  // for uint16_t etc...

namespace scxt
{
static constexpr uint64_t currentStreamingVersion{0x2025'11'12};

static constexpr uint16_t blockSize{16};
static constexpr uint16_t blockSizeQuad{16 >> 2};
static constexpr double blockSizeInv{1.0 / blockSize};
static constexpr uint16_t numParts{16};
static constexpr uint16_t numAux{4};
static constexpr uint16_t maxOutputs{1 + numParts + numAux}; // 16 parts, 4 aux, a main
static constexpr uint16_t maxBusses{maxOutputs};
static constexpr uint16_t mainOutput{0};
static constexpr uint16_t firstPartOutput{1};
static constexpr uint16_t firstAuxOutput{firstPartOutput + numParts};

static constexpr size_t macrosPerPart{16};

static constexpr uint16_t numNonMainPluginOutputs{20};
static constexpr uint16_t numPluginOutputs{numNonMainPluginOutputs + 1};

static constexpr uint16_t maxVoices{512};

// some battles are not worth it
static constexpr uint16_t BLOCK_SIZE{blockSize};
static constexpr uint16_t BLOCK_SIZE_QUAD{blockSizeQuad};

static constexpr uint16_t maxBusEffectParams{12};
static constexpr uint16_t maxEffectsPerBus{4};
static constexpr uint16_t maxSendsPerBus{4};

static constexpr uint16_t maxEffectsPerPart{4};

static constexpr size_t maxProcessorFloatParams{9};
static constexpr size_t maxProcessorIntParams{5};

static constexpr uint16_t lfosPerZone{4};
static constexpr uint16_t egsPerZone{5};
static constexpr uint16_t maxVariantsPerZone{16};

static constexpr uint16_t lfosPerGroup{4};
static constexpr uint16_t egsPerGroup{2};
static constexpr uint16_t processorsPerZoneAndGroup{4};

static constexpr uint16_t phasorsPerGroupOrZone{4};
static constexpr uint16_t randomsPerGroupOrZone{4};

static constexpr uint16_t triggerConditionsPerGroup{4};

static constexpr uint16_t maxGeneratorsPerVoice{64};

static constexpr size_t modMatrixRowsPerZone{12};
static constexpr size_t modMatrixRowsPerGroup{12};

// For tail detection use a full block below this level as silence
static constexpr float silenceThresh{1e-10f};

/*
 * This namespace guards some very useful debugging guards and logs in the code.
 * Some of them log in the realtime thread so please leave them all
 * false when you commit.
 */
namespace log
{
static constexpr bool selection{false};
static constexpr bool uiStructure{false};
static constexpr bool groupZoneMutation{false};
static constexpr bool memoryPool{false};
static constexpr bool voiceResponder{false};
static constexpr bool voiceLifecycle{false};
static constexpr bool generatorInitialization{false};
static constexpr bool sampleLoadAndPurge{false};
static constexpr bool sampleCompoundParsers{false};
static constexpr bool missingResolution{false};
static constexpr bool ringout{false};
static constexpr bool streaming{false};
static constexpr bool sqlDb{false};

static constexpr bool patchIO{false};
static constexpr bool jsonUI{false};
static constexpr bool uiTheme{false};

static constexpr bool plugin{false};

static constexpr bool warnings{true}; // make sure warnigns go to log
static constexpr bool cliTools{true}; // allow cli tools to use the log
} // namespace log

// The wireframe implies features beyond what we have. I started coding up the UI elements
// but this turns them off or providers simpler versions.
namespace hasFeature
{
static constexpr bool undoRedo{false};               // undo-redo
static constexpr bool memoryUsageExplanation{false}; // that chip in the header
static constexpr bool mappingPane11Controls{false};  // zone mapping header skipped for 1.0
static constexpr bool alternateEGModes{false};       // do we have DA only etc...
static constexpr bool hasBrowserSearch{false};
} // namespace hasFeature

} // namespace scxt
#endif // __SCXT__CONFIGURATION_H
