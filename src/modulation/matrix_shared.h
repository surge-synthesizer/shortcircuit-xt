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

#ifndef SCXT_SRC_MODULATION_MATRIX_SHARED_H
#define SCXT_SRC_MODULATION_MATRIX_SHARED_H

#include <cstdint>
#include <utility>
#include <ostream>
#include <string>

namespace scxt::engine
{
struct Engine;
};

namespace scxt::modulation::shared
{
struct SourceIdentifier
{
    uint32_t gid{0};
    uint32_t tid{0};
    uint32_t index{0};

    bool operator==(const SourceIdentifier &other) const
    {
        return gid == other.gid && tid == other.tid && index == other.index;
    }
};

struct TargetIdentifier
{
    uint32_t gid{0}; // this has to be unique and streaming stable
    uint32_t tid{0};
    uint32_t index{0};

    bool operator==(const TargetIdentifier &other) const
    {
        return gid == other.gid && tid == other.tid && index == other.index;
    }

    bool isProcessorTarget(size_t procNum) const
    {
        return (gid == 'proc' || gid == 'gprc') && index == procNum;
    }

    void setProcessorTargetTo(size_t t)
    {
        assert(gid == 'proc' || gid == 'gprc');
        index = t;
    }
};

struct RoutingExtraPayload
{
    bool selConsistent{false};
    datamodel::pmd targetMetadata;
    float targetBaseValue; // only valid on UI thread
};

inline std::string u2s(uint32_t u)
{
    std::string res = "";
    res += (char)((u & 0xFF000000) >> 24);
    res += (char)((u & 0x00FF0000) >> 16);
    res += (char)((u & 0x0000FF00) >> 8);
    res += (char)((u & 0x000000FF) >> 0);
    return res;
};

template <typename T>
static std::ostream &identifierToStream(std::ostream &os, const T &t, const char *nm)
{
    os << nm << "[" << u2s(t.gid) << "/" << u2s(t.tid) << "/" << t.index
       << " #=" << std::hash<T>{}(t) << "]";
    return os;
}
template <typename T> static std::string identifierToString(const T &t, const char *nm)
{
    std::ostringstream oss;
    identifierToStream(oss, t, nm);
    return oss.str();
}

template <typename T> static std::size_t identifierToHash(const T &t)
{
    auto h1 = std::hash<uint32_t>{}((int)t.gid);
    auto h2 = std::hash<uint32_t>{}((int)t.tid);
    auto h3 = std::hash<uint32_t>{}((int)t.index);
    return h1 ^ (h2 << 2) ^ (h3 << 5);
}

template <typename TG, uint32_t gn> struct EGTargetEndpointData
{
    uint32_t index{0};
    EGTargetEndpointData(uint32_t p)
        : index(p), dlyT{gn, 'dlay', p}, aT{gn, 'atck', p}, hT{gn, 'hld ', p}, dT{gn, 'dcay', p},
          sT{gn, 'sust', p}, rT{gn, 'rels', p}, asT{gn, 'atSH', p}, dsT{gn, 'dcSH', p},
          rsT{gn, 'rlSH', p}, retriggerT{gn, 'rtrg', p}
    {
    }

    TG dlyT, aT, hT, dT, sT, rT, asT, dsT, rsT, retriggerT;
    const float *dlyP{nullptr}, *aP{nullptr}, *hP{nullptr}, *dP{nullptr}, *sP{nullptr},
        *rP{nullptr};
    const float *asP{nullptr}, *dsP{nullptr}, *rsP{nullptr};
    const float *retriggerP{nullptr};

    float zeroBase{0.f};

    template <typename M, typename Z> void baseBind(M &m, Z &z);
};
template <typename TG, uint32_t gn> struct LFOTargetEndpointData
{
    LFOTargetEndpointData(uint32_t p)
        : index(p), rateT{gn, 'rate', p}, amplitudeT{gn, 'ampl', p}, retriggerT{gn, 'rtrg', p},
          curve(p), step(p), env(p)
    {
    }
    uint32_t index{0};

    TG rateT, amplitudeT, retriggerT;
    const float *rateP{nullptr};
    const float *amplitudeP{nullptr};
    const float *retriggerP{nullptr};
    float zeroBase{0.f};
    struct Curve
    {
        Curve(uint32_t p)
            : deformT{gn, 'cdfn', p}, angleT{gn, 'cang', p}, attackT{gn, 'catk', p},
              delayT{gn, 'cdel', p}, releaseT{gn, 'crel', p}
        {
        }
        TG deformT, angleT, delayT, attackT, releaseT;
        const float *deformP{nullptr}, *angleP{nullptr}, *delayP{nullptr}, *attackP{nullptr},
            *releaseP{nullptr};
    } curve;
    struct Step
    {
        Step(uint32_t p) : smoothT{gn, 'ssmt', p} {}
        TG smoothT;
        const float *smoothP{nullptr};
    } step;

    struct Env
    {
        Env(uint32_t p)
            : delayT{gn, 'edly', p}, attackT{gn, 'eatk', p}, holdT{gn, 'ehld', p},
              decayT{gn, 'edcy', p}, sustainT{gn, 'esus', p}, releaseT{gn, 'erel', p},
              aShapeT{gn, 'eash', p}, dShapeT{gn, 'edsh', p}, rShapeT{gn, 'ersh', p},
              rateMulT{gn, 'erml', p}
        {
        }
        TG delayT, attackT, holdT, decayT, sustainT, releaseT, aShapeT, dShapeT, rShapeT, rateMulT;
        const float *delayP{nullptr}, *attackP{nullptr}, *holdP{nullptr}, *decayP{nullptr},
            *sustainP{nullptr}, *releaseP{nullptr}, *aShapeP{nullptr}, *dShapeP{nullptr},
            *rShapeP{nullptr}, *rateMulP{nullptr};
    } env;

    template <typename M, typename Z> void baseBind(M &m, Z &z);
};

template <typename TG, uint32_t gn> struct ProcessorTargetEndpointData
{
    ProcessorTargetEndpointData(uint32_t p)
        : index{p}, mixT{gn, 'mix ', p}, outputLevelDbT{gn, 'oldb', p}
    {
        for (int i = 0; i < scxt::maxProcessorFloatParams; ++i)
        {
            fpT[i] = TG{gn, 'fp  ' + (uint32_t)(i + '0' - ' '), p};
        }
    }
    uint32_t index;
    TG mixT, outputLevelDbT, fpT[scxt::maxProcessorFloatParams];
    const float *mixP{nullptr};
    const float *outputLevelDbP{nullptr};
    const float *floatP[scxt::maxProcessorFloatParams]; // FIX CONSTANT
    float fp[scxt::maxProcessorFloatParams]{};

    void snapValues()
    {
        for (int i = 0; i < scxt::maxProcessorFloatParams; ++i)
            fp[i] = *floatP[i];
    }
};

template <typename Matrix, typename P>
void bindEl(Matrix &m, const P &payload, typename Matrix::TR::TargetIdentifier &tg, float &tgs,
            const float *&p, std::optional<datamodel::pmd> providedMetadata = std::nullopt)
{
    assert(tg.gid != 0); // hit this? You forgot to init your target ctor
    assert(tg.tid != 0);

    if (m.forUIMode)
    {
        datamodel::pmd tmd;
        if (!providedMetadata.has_value())
        {
            tmd = datamodel::describeValue(payload, tgs);
        }
        else
        {
            tmd = *providedMetadata;
        }
        m.activeTargetsToPMD[tg] = tmd;
        m.activeTargetsToBaseValue[tg] = tgs;
        return;
    }

    m.bindTargetBaseValue(tg, tgs);
    p = m.getTargetValuePointer(tg);

#if BUILD_IS_DEBUG
    /* Make sure every element has a description or a value provided.
     * This could be in an assert but you know, figure I'll just do it this
     * way
     * */
    if (!providedMetadata.has_value())
    {
        auto metaData = datamodel::describeValue(payload, tgs);
    }
#endif

    auto idxIt = m.targetToOutputIndex.find(tg);
    if (idxIt != m.targetToOutputIndex.end())
    {
        datamodel::pmd tmd;
        if (!providedMetadata.has_value())
        {
            tmd = datamodel::describeValue(payload, tgs);
        }
        else
        {
            tmd = *providedMetadata;
        }
        bool nonAdditive = tmd.hasSupportsMultiplicativeModulation();

        auto pt = m.getTargetValuePointer(tg);
        for (auto &r : m.routingValuePointers)
        {
            if (r.target == pt)
            {
                r.depthScale = tmd.maxVal - tmd.minVal;
                r.maxVal = tmd.maxVal;
                r.minVal = tmd.minVal;
                r.minVal = tmd.minVal;
                if (!nonAdditive)
                {
#if BUILD_IS_DEBUG
                    if (r.applicationMode ==
                        sst::basic_blocks::mod_matrix::ApplicationMode::MULTIPLICATIVE)
                    {
                        // In theory this should never happen
                        SCLOG("Somehow you set up multiplicative on an additive only param "
                              << tmd.name);
                        assert(false);
                    }
#endif
                    r.applicationMode = sst::basic_blocks::mod_matrix::ApplicationMode::ADDITIVE;
                }
            }
        }
    }
};

template <typename TG, uint32_t gn>
template <typename M, typename EG>
inline void EGTargetEndpointData<TG, gn>::baseBind(M &m, EG &eg)
{
    bindEl(m, eg, dlyT, eg.dly, dlyP);
    bindEl(m, eg, aT, eg.a, aP);
    bindEl(m, eg, hT, eg.h, hP);
    bindEl(m, eg, dT, eg.d, dP);
    bindEl(m, eg, sT, eg.s, sP);
    bindEl(m, eg, rT, eg.r, rP);
    bindEl(m, eg, asT, eg.aShape, asP);
    bindEl(m, eg, dsT, eg.dShape, dsP);
    bindEl(m, eg, rsT, eg.rShape, rsP);
    bindEl(m, eg, retriggerT, zeroBase, retriggerP,
           datamodel::pmd().withName("Retrigger").asPercent());
}

template <typename TG, uint32_t gn>
template <typename M, typename Z>
inline void LFOTargetEndpointData<TG, gn>::baseBind(M &m, Z &z)
{
    auto &ms = z.modulatorStorage[index];

    bindEl(m, ms, rateT, ms.rate, rateP);
    bindEl(m, ms, amplitudeT, ms.amplitude, amplitudeP);
    bindEl(m, ms, retriggerT, zeroBase, retriggerP,
           datamodel::pmd().withName("Retrigger").asPercent());

    bindEl(m, ms, curve.deformT, ms.curveLfoStorage.deform, curve.deformP);
    bindEl(m, ms, curve.angleT, ms.curveLfoStorage.angle, curve.angleP);
    bindEl(m, ms, curve.delayT, ms.curveLfoStorage.delay, curve.delayP);
    bindEl(m, ms, curve.attackT, ms.curveLfoStorage.attack, curve.attackP);
    bindEl(m, ms, curve.releaseT, ms.curveLfoStorage.release, curve.releaseP);

    bindEl(m, ms, step.smoothT, ms.stepLfoStorage.smooth, step.smoothP);

    bindEl(m, ms, env.delayT, ms.envLfoStorage.delay, env.delayP);
    bindEl(m, ms, env.attackT, ms.envLfoStorage.attack, env.attackP);
    bindEl(m, ms, env.holdT, ms.envLfoStorage.hold, env.holdP);
    bindEl(m, ms, env.decayT, ms.envLfoStorage.decay, env.decayP);
    bindEl(m, ms, env.sustainT, ms.envLfoStorage.sustain, env.sustainP);
    bindEl(m, ms, env.releaseT, ms.envLfoStorage.release, env.releaseP);
    bindEl(m, ms, env.aShapeT, ms.envLfoStorage.aShape, env.aShapeP);
    bindEl(m, ms, env.dShapeT, ms.envLfoStorage.dShape, env.dShapeP);
    bindEl(m, ms, env.rShapeT, ms.envLfoStorage.rShape, env.rShapeP);
    bindEl(m, ms, env.rateMulT, ms.envLfoStorage.rateMul, env.rateMulP);
}

template <typename SR, uint32_t gid,
          void (*registerSource)(scxt::engine::Engine *, const SR &, const std::string &,
                                 const std::string &)>
struct TransportSourceBase
{
    TransportSourceBase(scxt::engine::Engine *e)
    {
        for (uint32_t i = 0; i < scxt::phasorsPerGroupOrZone; ++i)
        {
            phasors[i] = SR{gid, 'phsr', i};
            std::string lab = "A";
            lab[0] += i;
            registerSource(e, phasors[i], "Phasors", "Phasor " + lab);
        }
    }

    std::array<SR, scxt::phasorsPerGroupOrZone> phasors;
};

template <typename SR, uint32_t gid,
          void (*registerSource)(scxt::engine::Engine *, const SR &, const std::string &,
                                 const std::string &)>
struct RNGSourceBase
{
    RNGSourceBase(scxt::engine::Engine *e)
    {
        for (uint32_t i = 0; i < scxt::phasorsPerGroupOrZone; ++i)
        {
            randoms[i] = SR{gid, 'rnds', i};
            std::string lab = "A";
            lab[0] += i;
            registerSource(e, randoms[i], "Random", "Random " + lab);
        }
    }

    std::array<SR, scxt::randomsPerGroupOrZone> randoms;
};

template <typename SR, uint32_t gid, size_t numLfo,
          void (*registerSource)(scxt::engine::Engine *, const SR &, const std::string &,
                                 const std::string &)>
struct LFOSourceBase
{
    LFOSourceBase(scxt::engine::Engine *e, const std::string &cat = "LFO",
                  const std::string &lfon = "LFO")
    {
        for (uint32_t i = 0; i < numLfo; ++i)
        {
            sources[i] = SR{gid, 'outp', i};
            registerSource(e, sources[i], cat, lfon + " " + std::to_string(i + 1));
        }
    }
    std::array<SR, numLfo> sources;

    template <typename M, typename S> void bind(M &m, S &s, float &zeroSource)
    {
        for (int i = 0; i < numLfo; ++i)
        {
            switch (s.lfoEvaluator[i])
            {
            case S::CURVE:
                m.bindSourceValue(sources[i], s.curveLfos[i].output);
                break;
            case S::STEP:
                m.bindSourceValue(sources[i], s.stepLfos[i].output);
                break;
            case S::ENV:
                m.bindSourceValue(sources[i], s.envLfos[i].output);
                break;
            case S::MSEG:
                m.bindSourceValue(sources[i], zeroSource);
                break;
            }
        }
    }
};

template <typename CF, typename SR, uint32_t gid,
          void (*registerSource)(scxt::engine::Engine *, const SR &, const std::string &,
                                 const std::string &)>
struct MIDICCBase
{
    static constexpr int numMidiCC{128};
    MIDICCBase(scxt::engine::Engine *e)
    {
        for (uint32_t i = 0; i < numMidiCC; ++i)
        {
            sources[i] = SR{gid, 'm1cc', i};
            registerSource(e, sources[i], "MIDI CCs", fmt::format("CC {:03}", i));
        }
    }
    std::array<SR, 128> sources;

    template <typename M, typename P> void bind(M &m, P &p)
    {
        for (int i = 0; i < numMidiCC; ++i)
        {
            m.bindSourceValue(sources[i], p.midiCCValues[i]);
            CF::setDefaultLagFor(sources[i], 50);
        }
    }
};
} // namespace scxt::modulation::shared

template <> struct std::hash<scxt::modulation::shared::TargetIdentifier>
{
    std::size_t operator()(const scxt::modulation::shared::TargetIdentifier &s) const noexcept
    {
        return scxt::modulation::shared::identifierToHash(s);
    }
};

template <> struct std::hash<scxt::modulation::shared::SourceIdentifier>
{
    std::size_t operator()(const scxt::modulation::shared::SourceIdentifier &s) const noexcept
    {
        return scxt::modulation::shared::identifierToHash(s);
    }
};

inline std::ostream &operator<<(std::ostream &os,
                                const scxt::modulation::shared::TargetIdentifier &tg)
{
    scxt::modulation::shared::identifierToStream(os, tg, "Target");
    return os;
}

inline std::ostream &operator<<(std::ostream &os,
                                const scxt::modulation::shared::SourceIdentifier &tg)
{
    scxt::modulation::shared::identifierToStream(os, tg, "Source");
    return os;
}

#endif // SHORTCIRCUITXT_MATRIX_SHARED_H
