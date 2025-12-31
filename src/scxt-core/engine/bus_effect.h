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

#ifndef SCXT_SRC_SCXT_CORE_ENGINE_BUS_EFFECT_H
#define SCXT_SRC_SCXT_CORE_ENGINE_BUS_EFFECT_H

#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

#include "configuration.h"
#include "utils.h"

#include "datamodel/metadata.h"

namespace scxt::engine
{
struct Engine;

/*
 * When you add an effect here you also need to add it to
 * - here
 *      - in the toStringAvaialble
 * - bus_part_effect.cpp
 *      - as an include of the effect
 *      - in createEffect
 * - src-ui/json-layout/bus-effects
 *      - add 'foo.json' as a blank ("{}") json file
 * - src-ui/app/mixer-screen/MixerScreen.cpp
 *      - the menu switch
 *      - the call to 'add'
 *  - src-ui/app/mixer-screen/components/PartEffectsPane.cpp
 *      - in the rebuild switch with the name of the json (Look for CS(...))
 *
 */
enum AvailableBusEffects
{
    none,
    reverb1,
    reverb2,
    flanger,
    phaser,
    delay,
    treemonster,
    nimbus,
    rotaryspeaker,
    floatydelay,
    bonsai // if you make bonsai not last, make sure to update the range end
};
static constexpr auto LastAvailableBusEffect{bonsai};

struct BusEffectStorage
{
    BusEffectStorage() { std::fill(params.begin(), params.end(), 0.f); }
    static constexpr int maxBusEffectParams{scxt::maxBusEffectParams};
    AvailableBusEffects type{AvailableBusEffects::none};
    bool isActive{true};
    bool isTemposync{false};
    std::array<float, maxBusEffectParams> params{};
    std::array<bool, maxBusEffectParams> deact{};
    int16_t streamingVersion;

    inline bool isDeactivated(int idx) { return deact[idx]; }
    inline bool isExtended(int idx) { return false; }
};
struct BusEffect
{
    BusEffect(Engine *, BusEffectStorage *, float *) {}
    virtual ~BusEffect() = default;

    virtual void init(bool defaultsOverrideStorage) = 0;
    virtual void process(float *__restrict L, float *__restrict R) = 0;
    virtual size_t silentSamplesLength() = 0;
    virtual datamodel::pmd paramAt(int i) const = 0;
    virtual int numParams() const = 0;
    virtual void onSampleRateChanged() = 0;
};

std::unique_ptr<BusEffect> createEffect(AvailableBusEffects p, Engine *e, BusEffectStorage *s);
using busRemapFn_t = void (*)(int16_t, float *const);
std::pair<int16_t, busRemapFn_t> getBusEffectRemapStreamingFunction(AvailableBusEffects);
std::string getBusEffectStreamingName(AvailableBusEffects);
std::string getBusEffectDisplayName(AvailableBusEffects);

inline std::string toStringAvailableBusEffects(const AvailableBusEffects &p)
{
    return getBusEffectStreamingName(p);
}

inline AvailableBusEffects fromStringAvailableBusEffects(const std::string &s)
{
    static auto inverse = makeEnumInverse<AvailableBusEffects, toStringAvailableBusEffects>(
        AvailableBusEffects::none, LastAvailableBusEffect);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return none;
    return p->second;
}

} // namespace scxt::engine
#endif // BUS_PART_EFFECT_H
