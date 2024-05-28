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

#ifndef SCXT_SRC_ENGINE_GROUP_AND_ZONE_H
#define SCXT_SRC_ENGINE_GROUP_AND_ZONE_H

#include <array>
#include "dsp/processor/processor.h"

/*
 * Base classes which provide functions shared by
 * groups and zones. This is a template class but it
 * needs to be explicitly instantiated since it has too
 * much stuff in the innards. So there's a group_and_zone_impl
 * which you should include in group and zone and explicitly
 * instantiate. Which we do.
 */

namespace scxt::engine
{
struct Engine;
struct Group;
struct Zone;

template <typename T> struct HasGroupZoneProcessors
{
    static constexpr bool forGroup{std::is_same_v<Group, T>};
    static constexpr bool forZone{std::is_same_v<Zone, T>};

    /*
     * This enum is here but we put it in the output info structure of
     * group and zone separately
     */
    enum ProcRoutingPath : int16_t
    {
        procRoute_linear,
        procRoute_bypass
    };
    DECLARE_ENUM_STRING(ProcRoutingPath);
    std::string getProcRoutingPathDisplayName(ProcRoutingPath p);

    T *asT() { return static_cast<T *>(this); }
    static constexpr int processorCount{scxt::processorsPerZoneAndGroup};
    void setProcessorType(int whichProcessor, dsp::processor::ProcessorType type);
    void setupProcessorControlDescriptions(int whichProcessor, dsp::processor::ProcessorType,
                                           dsp::processor::Processor *tmpProcessor = nullptr,
                                           bool reClampFloatValues = false);

    dsp::processor::Processor *spawnTempProcessor(int whichProcessor,
                                                  dsp::processor::ProcessorType type, uint8_t *mem,
                                                  float *pfp, int *ifp, bool initFromDefaults);

    // Returns true if I changed anything
    bool checkOrAdjustIntConsistency(int whichProcessor);
    bool checkOrAdjustBoolConsistency(int whichProcessor);

    std::array<dsp::processor::ProcessorStorage, processorCount> processorStorage;
    std::array<dsp::processor::ProcessorControlDescription, processorCount> processorDescription;
};

constexpr int processorCount{HasGroupZoneProcessors<Zone>::processorCount};

} // namespace scxt::engine
#endif // SHORTCIRCUITXT_GROUP_AND_ZONE_H
