/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_SCXT_CORE_ENGINE_GROUP_AND_ZONE_H
#define SCXT_SRC_SCXT_CORE_ENGINE_GROUP_AND_ZONE_H

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
        procRoute_linear, // 1 -> 2 -> 3 -> 4
        procRoute_ser2,   // -> { 1 | 2 } -> { 3 | 4 } ->
        procRoute_ser3,   // -> 1 -> { 2 | 3 } -> 4 ->
        procRoute_par1,   // -> { 1|2|3|4 } ->
        procRoute_par2,   // -> { { 1->2 } | { 3 -> 4 } } ->
        procRoute_par3,   // -> { 1 | 2 | 3 } -> 4
        procRoute_bypass  // bypass all procs.
    };
    DECLARE_ENUM_STRING(ProcRoutingPath);
    static std::string getProcRoutingPathDisplayName(ProcRoutingPath p);
    static std::string getProcRoutingPathShortName(ProcRoutingPath p);

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
    void updateRoutingTableAfterProcessorSwap(size_t f, size_t t);

    std::array<dsp::processor::ProcessorStorage, processorCount> processorStorage;
    std::array<dsp::processor::ProcessorControlDescription, processorCount> processorDescription;
};

constexpr int processorCount{HasGroupZoneProcessors<Zone>::processorCount};

} // namespace scxt::engine
#endif // SHORTCIRCUITXT_GROUP_AND_ZONE_H
