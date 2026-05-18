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

#include "sfz_import.h"
#include "sample/import_support/import_harness.h"
#include "sample/import_support/import_mapping.h"
#include "sample/import_support/import_envelope.h"
#include "sample/import_support/import_filter.h"
#include "sample/import_support/import_generator.h"
#include "sample/import_support/import_loop.h"
#include "sample/import_support/import_modulation.h"
#include "sample/import_support/import_numeric.h"
#include "selection/selection_manager.h"
#include "sfz_parse.h"
#include "messaging/messaging.h"
#include "engine/engine.h"
#include "configuration.h"
#include <cctype>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <sstream>

namespace scxt::sfz_support
{
int parseMidiNote(const std::string &s, int offset)
{
    static constexpr int noteShift[7] = {-3, -1, 0, 2, 4, 5, 7};
    static constexpr int octShift[7] = {1, 1, 0, 0, 0, 0, 0};
    if ((s[0] >= 'a' && s[0] <= 'g') || (s[0] >= 'A' && s[0] <= 'G'))
    {
        auto bn = std::clamp((int)std::tolower(s[0]) - (int)'a', 0, 7);
        int oct = 4;
        auto diff = 0;
        if (s[1] == '#' || s[1] == 'b')
        {
            diff = s[1] == '#' ? 1 : 0;
            oct = std::atol(s.c_str() + 2);
        }
        else
        {
            oct = std::atol(s.c_str() + 1);
        }

        // C4 is 60 so
        auto nsv = noteShift[bn];
        oct += octShift[bn]; // so a4 is actually abve c4
        auto res = nsv + diff + (oct + 1 + offset) * 12;

        return res;
    }
    return std::atol(s.c_str());
}

using opCodeMap_t = std::map<std::string, std::pair<std::string, bool>>;

std::optional<std::string> consumeOpcode(opCodeMap_t &opCodes, const std::string &key)
{
    auto it = opCodes.find(key);
    if (it == opCodes.end())
        return std::nullopt;
    it->second.second = true;
    return it->second.first;
}

void zoneGeometry(std::unique_ptr<engine::Zone> &zn, opCodeMap_t &opCodes, int &loadInfo,
                  const std::function<void(const std::string &, const std::string &)> &onError,
                  int octaveOffset)
{
    auto &mp = zn->mapping;

    if (auto v = consumeOpcode(opCodes, "key"); v.has_value())
    {
        auto pmn = parseMidiNote(*v, octaveOffset);
        mp.rootKey = pmn;
        mp.keyboardRange.keyStart = pmn;
        mp.keyboardRange.keyEnd = pmn;
        loadInfo = loadInfo & ~engine::Zone::MAPPING;
    }

    if (auto v = consumeOpcode(opCodes, "pitch_keycenter"); v.has_value())
    {
        mp.rootKey = parseMidiNote(*v, octaveOffset);
        loadInfo = loadInfo & ~engine::Zone::MAPPING;
    }

    // SFZ pitch_keytrack: cents/key, default 100 (full natural keytrack).
    // Maps directly to zone.mapping.tracking (1.0 = perfect keytrack, 0.0 =
    // none), so divide by 100.
    if (auto v = consumeOpcode(opCodes, "pitch_keytrack"); v.has_value())
        mp.tracking = (float)std::atof(v->c_str()) / 100.f;

    if (auto v = consumeOpcode(opCodes, "lokey"); v.has_value())
    {
        mp.keyboardRange.keyStart = parseMidiNote(*v, octaveOffset);
        if (mp.keyboardRange.keyStart > mp.keyboardRange.keyEnd)
        {
            onError("SFZ Keyrange Error", "SFZ Keyrange Autofix for../ : lokey > hikey (" + *v +
                                              " > " + std::to_string(mp.keyboardRange.keyEnd) +
                                              ")");
            std::swap(mp.keyboardRange.keyStart, mp.keyboardRange.keyEnd);
        }
        loadInfo = loadInfo & ~engine::Zone::MAPPING;
    }

    if (auto v = consumeOpcode(opCodes, "hikey"); v.has_value())
    {
        mp.keyboardRange.keyEnd = parseMidiNote(*v, octaveOffset);
        loadInfo = loadInfo & ~engine::Zone::MAPPING;
        if (mp.keyboardRange.keyEnd < mp.keyboardRange.keyStart)
        {
            onError("SFZ Keyrange Error", "SFZ Keyrange Autofix for : hikey < lokey (" + *v +
                                              " < " + std::to_string(mp.keyboardRange.keyStart) +
                                              ")");
            std::swap(mp.keyboardRange.keyStart, mp.keyboardRange.keyEnd);
        }
    }

    if (auto v = consumeOpcode(opCodes, "lovel"); v.has_value())
    {
        mp.velocityRange.velStart = std::atol(v->c_str());
        loadInfo = loadInfo & ~engine::Zone::MAPPING;
        if (mp.velocityRange.velStart > mp.velocityRange.velEnd)
        {
            onError("SFZ Velocity Error", "SFZ Velocity Autifix for : lovel > hivel (" + *v +
                                              " > " + std::to_string(mp.velocityRange.velEnd) +
                                              ")");
            std::swap(mp.velocityRange.velStart, mp.velocityRange.velEnd);
        }
    }

    if (auto v = consumeOpcode(opCodes, "hivel"); v.has_value())
    {
        mp.velocityRange.velEnd = std::atol(v->c_str());
        loadInfo = loadInfo & ~engine::Zone::MAPPING;
        if (mp.velocityRange.velEnd < mp.velocityRange.velStart)
        {
            onError("SFZ Velocity Error", "SFZ Velocity Error AutoFix for : hivel < lovel (" + *v +
                                              " < " + std::to_string(mp.velocityRange.velStart) +
                                              ")");
            std::swap(mp.velocityRange.velStart, mp.velocityRange.velEnd);
        }
    }

    // OK so xfin_lovel xfin_hivel is how velocity fades occur but they
    // are distinct from the geometry so
    auto l = consumeOpcode(opCodes, "xfin_lovel");
    auto h = consumeOpcode(opCodes, "xfin_hivel");
    if (l.has_value() && h.has_value())
    {
        auto lv = std::atol(l->c_str());
        auto hv = std::atol(h->c_str());
        if (hv < lv)
        {
            onError("SFZ Velocity Error",
                    "SFZ Velocity Error AutoFix for : xfin_hivel < xfin_lovel (" + *h + " < " + *l +
                        ")");
            std::swap(lv, hv);
        }
        mp.velocityRange.velStart = lv;
        mp.velocityRange.fadeStart = hv - lv;
    }

    l = consumeOpcode(opCodes, "xfout_lovel");
    h = consumeOpcode(opCodes, "xfout_hivel");
    if (l.has_value() && h.has_value())
    {
        auto lv = std::atol(l->c_str());
        auto hv = std::atol(h->c_str());
        if (hv < lv)
        {
            onError("SFZ Velocity Error",
                    "SFZ Velocity Error AutoFix for : xfout_hivel < xfout_lovel (" + *h + " < " +
                        *l + ")");
            std::swap(lv, hv);
        }
        mp.velocityRange.velEnd = hv;
        mp.velocityRange.fadeEnd = hv - lv;
    }
}

void zonePlayback(std::unique_ptr<engine::Zone> &zn, opCodeMap_t &opCodes)
{
    auto &mp = zn->mapping;

    if (auto v = consumeOpcode(opCodes, "volume"); v.has_value())
    {
        mp.amplitude = std::atof(v->c_str()); // decibels
    }

    if (auto v = consumeOpcode(opCodes, "tune"); v.has_value())
    {
        mp.pitchOffset = import_support::centsToSemitones((float)std::atof(v->c_str()));
    }

    if (auto v = consumeOpcode(opCodes, "transpose"); v.has_value())
    {
        mp.pitchOffset = std::atoi(v->c_str());
    }
}

// Parse SFZ loop_* opcodes into LoopArgs. Side-effect: masks engine::Zone::LOOP
// out of loadInfo if explicit bounds were given (so the later attach call won't
// overwrite them from sample meta).
import_support::LoopArgs zoneLoopParse(opCodeMap_t &opCodes, int &loadInfo)
{
    import_support::LoopArgs args;

    if (auto v = consumeOpcode(opCodes, "loop_start"); v.has_value())
    {
        args.startSamples = std::atol(v->c_str());
        loadInfo = loadInfo & ~engine::Zone::LOOP;
    }

    if (auto v = consumeOpcode(opCodes, "loop_end"); v.has_value())
    {
        args.endSamples = std::atol(v->c_str());
        loadInfo = loadInfo & ~engine::Zone::LOOP;
    }

    if (auto v = consumeOpcode(opCodes, "loop_mode"); v.has_value())
    {
        if (*v == "loop_continuous")
        {
            args.active = true;
            args.mode = engine::Zone::LOOP_DURING_VOICE;
        }
        else
        {
            SCLOG_IF(sampleCompoundParsers, "SFZ Unsupported loop_mode : " << *v);
        }
    }

    return args;
}

void zoneEnvelope(std::unique_ptr<engine::Zone> &zn, import_support::ImporterContext &ctx,
                  opCodeMap_t &opCodes, int slot, const std::string &prefix)
{
    import_support::EnvelopeArgs args;

    if (auto v = consumeOpcode(opCodes, prefix + "_sustain"); v.has_value())
        args.sustainLevel = (float)(std::atof(v->c_str()) * 0.01);

    if (auto v = consumeOpcode(opCodes, prefix + "_delay"); v.has_value())
        args.delaySeconds = (float)std::atof(v->c_str());

    if (auto v = consumeOpcode(opCodes, prefix + "_attack"); v.has_value())
        args.attackSeconds = (float)std::atof(v->c_str());

    if (auto v = consumeOpcode(opCodes, prefix + "_decay"); v.has_value())
    {
        args.decaySeconds = (float)std::atof(v->c_str());
        args.decayShape = -1.f;
    }

    if (auto v = consumeOpcode(opCodes, prefix + "_hold"); v.has_value())
        args.holdSeconds = (float)std::atof(v->c_str());

    if (auto v = consumeOpcode(opCodes, prefix + "_release"); v.has_value())
    {
        args.releaseSeconds = (float)std::atof(v->c_str());
        args.releaseShape = -1.f;
    }

    import_support::importZoneEnvelope(*zn, ctx, slot, args);
}

// Returns the FilterHandle so the caller can wire CC/aftertouch mods onto
// it; FilterHandle{} (procSlot=-1) means no filter was installed. The
// `procSlot` argument lets the caller route the filter past a generator
// processor that already occupies slot 0 (see `installVirtualSample`).
import_support::FilterHandle zoneFilter(std::unique_ptr<engine::Zone> &zn,
                                        import_support::ImporterContext &ctx, opCodeMap_t &opCodes,
                                        std::set<std::string> &reportedFilTypes, int procSlot = 0)
{
    auto cutoffOpc = consumeOpcode(opCodes, "cutoff");
    auto resoOpc = consumeOpcode(opCodes, "resonance");
    auto typeOpc = consumeOpcode(opCodes, "fil_type");

    // Quick scan: also install a filter if any cutoff/resonance mod opcode
    // is present, so the mod has somewhere to land. (Don't consume here —
    // zoneFilterMods will do that.)
    bool hasFilterMod = false;
    for (const auto &[name, val] : opCodes)
    {
        if (val.second)
            continue;
        if (name.rfind("cutoff_", 0) == 0 || name.rfind("resonance_", 0) == 0)
        {
            hasFilterMod = true;
            break;
        }
    }

    // No filter opcodes set → don't install a filter at all.
    if (!cutoffOpc.has_value() && !resoOpc.has_value() && !typeOpc.has_value() && !hasFilterMod)
        return {};

    // SFZ defaults from the spec.
    float cutoffHz = cutoffOpc.has_value() ? (float)std::atof(cutoffOpc->c_str()) : 22050.f;
    float resoDb = resoOpc.has_value() ? (float)std::atof(resoOpc->c_str()) : 0.f;
    std::string filTypeStr = typeOpc.value_or("lpf_2p");

    static const std::unordered_map<std::string, import_support::FilterType> typeMap{
        {"lpf_1p", import_support::FilterType::LP6},
        {"lpf_2p", import_support::FilterType::LP12},
        {"lpf_2p_sv", import_support::FilterType::LP12},
        {"lpf_4p", import_support::FilterType::LP24},
        {"lpf_6p", import_support::FilterType::LP36},
        {"hpf_1p", import_support::FilterType::HP6},
        {"hpf_2p", import_support::FilterType::HP12},
        {"hpf_2p_sv", import_support::FilterType::HP12},
        {"hpf_4p", import_support::FilterType::HP24},
        {"hpf_6p", import_support::FilterType::HP36},
        {"bpf_1p", import_support::FilterType::BP6},
        {"bpf_2p", import_support::FilterType::BP12},
        {"bpf_2p_sv", import_support::FilterType::BP12},
        {"brf_1p", import_support::FilterType::Notch12},
        {"brf_2p", import_support::FilterType::Notch12},
        {"brf_2p_sv", import_support::FilterType::Notch12},
        {"apf_1p", import_support::FilterType::Allpass},
        {"pkf_2p", import_support::FilterType::Peak},
        {"comb", import_support::FilterType::Comb},
    };

    auto fType = import_support::FilterType::Other;
    if (auto it = typeMap.find(filTypeStr); it != typeMap.end())
    {
        fType = it->second;
    }
    else if (reportedFilTypes.insert(filTypeStr).second)
    {
        ctx.unsupported("SFZ fil_type", filTypeStr + " (using bypassed LP)");
    }

    // cutoff: SFZ Hz → SCXT semitones from MIDI 69 (A4=440Hz)
    float cutoffSemis = (cutoffHz > 1.f) ? 12.f * std::log2(cutoffHz / 440.f) : -69.f;
    cutoffSemis = std::clamp(cutoffSemis, -69.f, 70.f);

    // resonance: SFZ 0..40 dB → SCXT 0..1 (crude linear)
    float resonance = std::clamp(resoDb / 40.f, 0.f, 1.f);

    return import_support::importZoneFilter(*zn, ctx, procSlot,
                                            {
                                                .type = fType,
                                                .cutoff = cutoffSemis,
                                                .resonance = resonance,
                                            });
}

// Resolve an SFZ source-suffix opcode name into an ImportedSource. Handles:
//   <prefix>_oncc<N> → MidiCC(N) (or ModWheel for N==1)
//   <prefix>_chanaft → ChannelAT
//   <prefix>_polyaft → PolyAT (skipped; voice_matrix has a stale identifier)
// Returns nullopt if `name` doesn't match these forms.
std::optional<import_support::ImportedSource>
sfzSourceForSuffix(const std::string &name, const std::string &prefix,
                   import_support::ImporterContext &ctx)
{
    if (name.rfind(prefix + "_oncc", 0) == 0)
    {
        const auto rest = name.substr(prefix.size() + 5);
        if (rest.empty() ||
            !std::all_of(rest.begin(), rest.end(), [](char c) { return c >= '0' && c <= '9'; }))
            return std::nullopt;
        int cc = std::atoi(rest.c_str());
        if (cc < 0 || cc > 127)
            return std::nullopt;
        if (cc == 1)
            return import_support::ImportedSource::modWheel();
        return import_support::ImportedSource::midiCC(cc);
    }
    if (name == prefix + "_chanaft")
        return import_support::ImportedSource::channelAT();
    if (name == prefix + "_polyaft")
    {
        ctx.unsupported("SFZ polyaft", prefix);
        return std::nullopt;
    }
    return std::nullopt;
}

// SFZ <prefix>_keytrack / <prefix>_keycenter (e.g. fil_keytrack, amp_keytrack,
// pan_keytrack). Emits a KeyTrack→target mod route. `perKeyToDepth(v)` maps
// the SFZ per-key value into the target-native depth at one octave above
// rootKey (where SCXT's KeyTrack source = 1.0). keycenter defaults to 60;
// SCXT's KeyTrack is rooted at the zone's rootKey, so non-default keycenters
// that differ from rootKey are flagged as unsupported.
void zoneParamKeytrack(opCodeMap_t &opCodes, import_support::ImporterContext &ctx, engine::Zone &zn,
                       const std::string &prefix,
                       const std::function<import_support::ImportedTarget()> &targetMaker,
                       const std::function<float(float)> &perKeyToDepth)
{
    auto ktOpc = consumeOpcode(opCodes, prefix + "_keytrack");
    auto kcOpc = consumeOpcode(opCodes, prefix + "_keycenter");

    if (!ktOpc.has_value())
        return;

    float perKey = (float)std::atof(ktOpc->c_str());
    if (perKey == 0.f)
        return;

    if (kcOpc.has_value())
    {
        int kc = parseMidiNote(*kcOpc, 0);
        if (kc != zn.mapping.rootKey)
        {
            ctx.unsupported("SFZ " + prefix + "_keycenter",
                            "differs from zone rootKey; tracking centered on rootKey");
        }
    }

    import_support::addImportedModRoute(zn, ctx,
                                        {
                                            .source = import_support::ImportedSource::keyTrack(),
                                            .target = targetMaker(),
                                            .depth = perKeyToDepth(perKey),
                                        });
}

// Generic loop for the four "simple" mod-bearing SFZ params: cutoff,
// resonance, volume, pan, pitch. Scans opCodes for <prefix>_oncc<N>,
// _chanaft, _polyaft; converts the SFZ value to SCXT depth via `valueToDepth`
// (a value-in-SFZ-units → depth-in-target-units function); emits a mod route
// per match. Consumes any matched opcodes.
void zoneParamMods(opCodeMap_t &opCodes, import_support::ImporterContext &ctx, engine::Zone &zn,
                   const std::string &prefix,
                   const std::function<import_support::ImportedTarget()> &targetMaker,
                   const std::function<float(float)> &valueToDepth)
{
    std::vector<std::string> matchedKeys;
    for (const auto &[name, val] : opCodes)
    {
        if (val.second)
            continue;
        if (name.rfind(prefix + "_oncc", 0) == 0 || name == prefix + "_chanaft" ||
            name == prefix + "_polyaft")
            matchedKeys.push_back(name);
    }
    for (const auto &name : matchedKeys)
    {
        auto valOpt = consumeOpcode(opCodes, name);
        if (!valOpt)
            continue;
        auto src = sfzSourceForSuffix(name, prefix, ctx);
        if (!src)
            continue;
        float raw = (float)std::atof(valOpt->c_str());
        float depth = valueToDepth(raw);
        import_support::addImportedModRoute(zn, ctx,
                                            {
                                                .source = *src,
                                                .target = targetMaker(),
                                                .depth = depth,
                                            });
    }
}

// 25-second exponential envelope time mapping (matches as25SecondExpTime in
// ParamMetadata). seconds = (exp(A + norm*(B-A)) - 2)/1000.
static constexpr float kEgA = 0.6931471824646f;
static constexpr float kEgB = 10.1267113685608f;
inline float egNormToSeconds(float norm)
{
    return (std::exp(kEgA + norm * (kEgB - kEgA)) - 2.f) / 1000.f;
}
inline float egSecondsToNorm(float secs)
{
    return (std::log(std::max(0.f, secs) * 1000.f + 2.f) - kEgA) / (kEgB - kEgA);
}
// SFZ envelope mods describe a *seconds delta at full source*. SCXT's
// envelope time param is normalized 0..1 with exponential mapping, so the
// linear mod-matrix addition (norm += depth) doesn't map linearly to a
// seconds delta. We invert: at full source, the time should land at
// (base + deltaSec); depth = norm(base + deltaSec) - norm(base).
inline float egSecondsDeltaToDepth(float baseNorm, float deltaSec)
{
    float baseSec = egNormToSeconds(baseNorm);
    return egSecondsToNorm(baseSec + deltaSec) - baseNorm;
}

// Handles ampeg_/fileg_ envelope time CC and velocity mods. Pattern is
// different from the params above: `<env>_attackcc<N>=secs`, and
// `<env>_vel2attack=secs`. Time-shape mods (delay/hold/decay/sustain/release)
// follow the same shape.
void zoneEnvelopeMods(opCodeMap_t &opCodes, import_support::ImporterContext &ctx, engine::Zone &zn,
                      const std::string &prefix, int egSlot)
{
    const auto &eg = zn.egStorage[egSlot];

    struct Stage
    {
        const char *suffix; // e.g. "attack"
        import_support::EGWhich which;
        bool isTime;      // false = level (sustain), true = exp-time
        float baseNorm;   // base normalized value (for time-delta inversion)
        float levelScale; // level path: SFZ value * scale → depth
    };
    const Stage stages[] = {
        {"delay", import_support::EGWhich::Delay, true, eg.dly, 0.f},
        {"attack", import_support::EGWhich::Attack, true, eg.a, 0.f},
        {"hold", import_support::EGWhich::Hold, true, eg.h, 0.f},
        {"decay", import_support::EGWhich::Decay, true, eg.d, 0.f},
        // sustain: SFZ value is percent (0..100); target is 0..1 level.
        {"sustain", import_support::EGWhich::Sustain, false, eg.s, 0.01f},
        {"release", import_support::EGWhich::Release, true, eg.r, 0.f},
    };

    auto depthFor = [&](const Stage &st, float rawValue) {
        return st.isTime ? egSecondsDeltaToDepth(st.baseNorm, rawValue) : rawValue * st.levelScale;
    };

    for (const auto &st : stages)
    {
        auto target = import_support::ImportedTarget::egTime(egSlot, st.which);

        // <env>_vel2<stage>: velocity → stage, seconds (or percent for sustain)
        const std::string velKey = prefix + "_vel2" + st.suffix;
        if (auto v = consumeOpcode(opCodes, velKey))
        {
            float raw = (float)std::atof(v->c_str());
            import_support::addImportedModRoute(
                zn, ctx,
                {
                    .source = import_support::ImportedSource::velocity(),
                    .target = target,
                    .depth = depthFor(st, raw),
                });
        }

        // <env>_<stage>cc<N>: CC<N> → stage
        const std::string ccPrefix = prefix + "_" + st.suffix + "cc";
        std::vector<std::string> ccKeys;
        for (const auto &[name, val] : opCodes)
        {
            if (val.second)
                continue;
            if (name.rfind(ccPrefix, 0) == 0)
                ccKeys.push_back(name);
        }
        for (const auto &name : ccKeys)
        {
            const auto rest = name.substr(ccPrefix.size());
            if (rest.empty() ||
                !std::all_of(rest.begin(), rest.end(), [](char c) { return c >= '0' && c <= '9'; }))
                continue;
            int cc = std::atoi(rest.c_str());
            if (cc < 0 || cc > 127)
                continue;
            auto v = consumeOpcode(opCodes, name);
            if (!v)
                continue;
            float raw = (float)std::atof(v->c_str());
            auto src = (cc == 1) ? import_support::ImportedSource::modWheel()
                                 : import_support::ImportedSource::midiCC(cc);
            import_support::addImportedModRoute(zn, ctx,
                                                {
                                                    .source = src,
                                                    .target = target,
                                                    .depth = depthFor(st, raw),
                                                });
        }
    }
}

bool importSFZ(const fs::path &f, engine::Engine &e)
{
    int octaveOffset{0};
    import_support::ImporterContext ctx(e, "Loading SFZ '" + f.filename().u8string() + "'");

    SFZParser parser;
    parser.onError = [&ctx](const auto &s) { ctx.raise("SFZ Import Error", s); };

    auto doc = parser.parse(f);
    auto rootDir = f.parent_path();
    auto sampleDir = rootDir;

    int groupId = -1;
    int regionCount{0};
    SFZParser::opCodes_t currentGroupOpcodes;
    // Captures (opcode -> first observed value) across all regions; flushed
    // to ctx.recordUnusedItem at the end of the import.
    std::map<std::string, std::string> unusedZoneOpcodes;
    std::set<std::string> reportedSfzFilTypes;
    for (const auto &[r, list] : doc)
    {
        if (r.type != SFZParser::Header::region)
        {
            SCLOG_IF(sampleCompoundParsers,
                     "Header ----- <" << r.name << "> (" << list.size() << " opcodes) -------");
        }
        switch (r.type)
        {
        case SFZParser::Header::group:
        {
            groupId = ctx.addGroup();
            currentGroupOpcodes = list;
            auto &group = ctx.getPart().getGroup(groupId);
            for (auto &oc : list)
            {
                if (oc.name == "group_label" || oc.name == "name")
                {
                    group->name = oc.value;
                }
                else
                {
                    // We take a second shot at this below
                }
            }
        }
        break;
        case SFZParser::Header::region:
        {
            regionCount++;
            if (groupId < 0)
                groupId = ctx.addGroup();
            auto &group = ctx.getPart().getGroup(groupId);

            // Find the sample. Default to `*silence` so regions without a
            // `sample=` opcode (e.g. SFZ files that author only effects, or
            // stray empty `<region>` headers) come through as silent generator
            // zones rather than a sample-load failure.
            std::string sampleFileString = "*silence";
            for (auto &oc : currentGroupOpcodes)
            {
                if (oc.name == "sample")
                {
                    sampleFileString = oc.value;
                }
            }
            for (auto &oc : list)
            {
                if (oc.name == "sample")
                {
                    sampleFileString = oc.value;
                }
            }

            if (!scxt::isValidUtf(sampleFileString))
            {
                SCLOG_IF(warnings, "Original file name is `" << sampleFileString << "`");
                return ctx.fail("SFZ Import Error",
                                "Sample filename in region " + std::to_string(regionCount) +
                                    " contains invalid UTF-8 sequence. Stopping SFZ import");
            }

            // SFZ magic generator names (`*sine`, `*saw`, `*square`, `*triangle`,
            // `*tri`, `*noise`, `*silence`) skip the sample-loading path entirely;
            // the zone is built around a synthetic silent sample plus a generator
            // processor — see installVirtualSample.
            const bool isVirtual = import_support::isVirtualSampleName(sampleFileString);

            // fs always works with / and on windows also works with back. Quotes are
            // stripped by the parser now
            std::replace(sampleFileString.begin(), sampleFileString.end(), '\\', '/');
            auto sampleFile = fs::path{sampleFileString};
            auto samplePath = (sampleDir / sampleFile).lexically_normal();

            SampleID sid;
            if (!isVirtual)
            {
                auto lsid = ctx.loadSampleFromDisk({samplePath, sampleFile});
                if (!lsid)
                {
                    // TODO: this should trigger a missing-sample resolution flow
                    // (prompt the user to locate the file) rather than raising +
                    // skipping the region. Same shape applies to a load failure on
                    // a path that DID exist (e.g. a corrupt WAV). For now: raise a
                    // user-visible error and skip this region; the rest of the SFZ
                    // continues to import.
                    ctx.raise("SFZ Import Error", "Unable to load sample '" +
                                                      samplePath.u8string() + "' or '" +
                                                      sampleFile.u8string() + "'");
                    break;
                }
                sid = *lsid;
            }

            // OK so do we have a sequence position > 1
            int roundRobinPosition{-1};
            for (auto &oc : list)
            {
                if (oc.name == "seq_position")
                {
                    auto pos = std::atoi(oc.value.c_str());
                    if (pos > 1)
                    {
                        roundRobinPosition = pos;
                    };
                }
            }
            auto zn =
                isVirtual ? std::make_unique<engine::Zone>() : std::make_unique<engine::Zone>(sid);
            zn->engine = &e; // required by setProcessorType in zoneFilter / virtual
            if (isVirtual)
            {
                if (!import_support::installVirtualSample(*zn, ctx, sampleFileString))
                {
                    ctx.raise("SFZ Import Error",
                              "Unknown virtual sample name '" + sampleFileString + "'");
                    break;
                }
            }
            // SFZ default mapping (gets overwritten by zoneGeometry once opcodes are parsed).
            import_support::importZoneMapping(*zn, ctx,
                                              {
                                                  .rootKey = 60,
                                                  .keyStart = 0,
                                                  .keyEnd = 127,
                                                  .velStart = 0,
                                                  .velEnd = 127,
                                              });

            auto loadInfo = engine::Zone::LOOP | engine::Zone::ENDPOINTS | engine::Zone::MAPPING;

            opCodeMap_t mergedOpcodes;
            for (const auto &oc : currentGroupOpcodes)
                mergedOpcodes[oc.name] = {oc.value, false};
            for (const auto &oc : list)
                mergedOpcodes[oc.name] = {oc.value, false};

            // Already used
            consumeOpcode(mergedOpcodes, "sample");
            consumeOpcode(mergedOpcodes, "seq_position");

            // Group-level opcodes that may appear in either the <group>
            // header or a <region> (merged here). Apply to the parent group;
            // consuming dedups against the unused-opcodes histogram.
            if (auto v = consumeOpcode(mergedOpcodes, "group_label"); v.has_value())
                group->name = *v;
            if (auto v = consumeOpcode(mergedOpcodes, "name"); v.has_value())
                group->name = *v;
            if (auto v = consumeOpcode(mergedOpcodes, "amp_veltrack"); v.has_value())
            {
                // SCXT velocity sensitivity is a group-level scalar; SFZ
                // amp_veltrack can sit on either group or region. Apply
                // last-seen value; (veltrack/127)^2 is the SCXT-side calibration.
                float vt = (float)std::atof(v->c_str()) / 127.f;
                group->outputInfo.velocitySensitivity = vt * vt;
            }

            auto onError = [&ctx](const std::string &title, const std::string &msg) {
                ctx.raise(title, msg);
            };
            zoneGeometry(zn, mergedOpcodes, loadInfo, onError, octaveOffset);
            zonePlayback(zn, mergedOpcodes);
            auto loopArgs = zoneLoopParse(mergedOpcodes, loadInfo);
            zoneEnvelope(zn, ctx, mergedOpcodes, 0, "ampeg");
            zoneEnvelope(zn, ctx, mergedOpcodes, 1, "fileg");
            // Virtual generator zones occupy proc slot 0 with the oscillator;
            // route the filter to slot 1 so both can live in the same chain.
            int filterSlot = isVirtual ? 1 : 0;
            auto filterHandle = zoneFilter(zn, ctx, mergedOpcodes, reportedSfzFilTypes, filterSlot);

            // Envelope CC/velocity mods
            zoneEnvelopeMods(mergedOpcodes, ctx, *zn, "ampeg", 0);
            zoneEnvelopeMods(mergedOpcodes, ctx, *zn, "fileg", 1);

            // Filter cutoff/resonance mods (cents and dB respectively)
            auto cutoffTarget = [&]() {
                return import_support::ImportedTarget::filter(filterHandle,
                                                              import_support::FilterParam::Cutoff);
            };
            if (filterHandle.procSlot >= 0)
            {
                zoneParamMods(mergedOpcodes, ctx, *zn, "cutoff", cutoffTarget,
                              [](float v) { return v / 100.f; }); // cents → semitones
                zoneParamMods(
                    mergedOpcodes, ctx, *zn, "resonance",
                    [&]() {
                        return import_support::ImportedTarget::filter(
                            filterHandle, import_support::FilterParam::Resonance);
                    },
                    [](float v) { return v / 40.f; }); // dB → 0..1

                // fil_keytrack: KeyTrack → cutoff. SFZ value is cents/key;
                // depth at one octave = 12 * cents/100 = cents * 0.12 semis.
                zoneParamKeytrack(mergedOpcodes, ctx, *zn, "fil", cutoffTarget,
                                  [](float v) { return v * 0.12f; });
            }

            // Volume / pan / pitch mods
            auto volTarget = []() { return import_support::ImportedTarget::zoneAmplitude(); };
            auto panTarget = []() { return import_support::ImportedTarget::zonePan(); };
            zoneParamMods(mergedOpcodes, ctx, *zn, "volume", volTarget,
                          [](float v) { return v; }); // SFZ dB → SCXT dB
            zoneParamMods(mergedOpcodes, ctx, *zn, "pan", panTarget,
                          [](float v) { return v / 100.f; }); // % → -1..1
            zoneParamMods(
                mergedOpcodes, ctx, *zn, "pitch",
                []() { return import_support::ImportedTarget::pitch(); },
                [](float v) { return v / 100.f; }); // cents → semitones

            // amp_keytrack / pan_keytrack. SFZ pitch_keytrack is intentionally
            // NOT handled here — SCXT applies natural pitch keytrack via sample
            // playback rate, so an explicit KeyTrack→Pitch mod would stack.
            zoneParamKeytrack(mergedOpcodes, ctx, *zn, "amp", volTarget,
                              [](float v) { return v * 12.f; }); // dB/key → dB/oct
            zoneParamKeytrack(mergedOpcodes, ctx, *zn, "pan", panTarget,
                              [](float v) { return v * 0.12f; }); // %/key → unit/oct

            // pitch_veltrack: velocity → pitch, cents at vel=127
            if (auto v = consumeOpcode(mergedOpcodes, "pitch_veltrack"); v.has_value())
            {
                float depth = (float)std::atof(v->c_str()) / 100.f;
                import_support::addImportedModRoute(
                    *zn, ctx,
                    {
                        .source = import_support::ImportedSource::velocity(),
                        .target = import_support::ImportedTarget::pitch(),
                        .depth = depth,
                    });
            }

            for (auto &[n, m] : mergedOpcodes)
            {
                if (m.second)
                    continue;
                unusedZoneOpcodes.emplace(n, m.first);
            }
            int variantIndex{-1};
            if (isVirtual)
            {
                // installVirtualSample already attached the silent sample; just
                // record the variant index so the loop/etc. plumbing below works.
                variantIndex = 0;
            }
            else if (roundRobinPosition > 0)
            {
                bool attached{false};
                int idx = roundRobinPosition - 1;

                if (idx >= scxt::maxVariantsPerZone)
                {
                    // SCXT zones have a fixed cap on round-robin variants
                    // (scxt::maxVariantsPerZone). Anything past that would
                    // OOB-write into Zone::variantData; raise + skip instead.
                    ctx.raise("SFZ Round Robin",
                              "SCXT only supports " + std::to_string(scxt::maxVariantsPerZone) +
                                  " variations in round robin; seq_position=" +
                                  std::to_string(roundRobinPosition) + " dropped.");
                    break;
                }

                for (const auto &z : *group)
                {
                    if (z->mapping.rootKey == zn->mapping.rootKey &&
                        z->mapping.keyboardRange == zn->mapping.keyboardRange &&
                        z->mapping.velocityRange == zn->mapping.velocityRange)
                    {
                        variantIndex = idx;
                        z->attachToSampleAtVariation(*e.getSampleManager(), sid, idx, loadInfo);
                        attached = true;
                        break;
                    }
                }
                if (!attached)
                {
                    ctx.raise("Mis-mapped SFZ Round Robin",
                              std::string("Unable to locate zone for sample ") +
                                  sampleFile.u8string());
                }
            }
            else
            {
                variantIndex = 0;
                zn->attachToSample(*e.getSampleManager(), 0, loadInfo);
            }

            if (variantIndex >= 0)
            {
                // Apply SFZ's clamp/swap normalization to whatever bounds were
                // gathered before handing them to importZoneLoop.
                if (loopArgs.startSamples || loopArgs.endSamples)
                {
                    int64_t ls = loopArgs.startSamples.value_or(-1);
                    int64_t le = loopArgs.endSamples.value_or(-1);
                    loopArgs.startSamples = std::max((int64_t)0, ls);
                    loopArgs.endSamples = std::max({(int64_t)0, ls + 1, le});
                }
                import_support::importZoneLoop(*zn, ctx, variantIndex, loopArgs);
            }

            if (roundRobinPosition <= 0)
            {
                ctx.addZoneToGroup(groupId, std::move(zn));
            }
        }
        break;
        case SFZParser::Header::control:
        {
            for (const auto &oc : list)
            {
                if (oc.name == "default_path")
                {
                    auto vv = oc.value;
                    std::replace(vv.begin(), vv.end(), '\\', '/');
                    sampleDir = rootDir / vv;
                    SCLOG_IF(sampleCompoundParsers,
                             "Control: Resetting sample dir to " << sampleDir);
                }
                else if (oc.name == "octave_offset")
                {
                    octaveOffset = std::atoi(oc.value.c_str());
                }
                else
                {
                    SCLOG_IF(sampleCompoundParsers,
                             "    Skipped OpCode <control>: " << oc.name << " -> " << oc.value);
                }
            }
        }
        break;
        default:
        {
            SCLOG_IF(sampleCompoundParsers,
                     "Ignoring SFZ Header " << r.name << " with " << list.size() << " keywords");
            for (const auto &oc : list)
            {
                SCLOG_IF(sampleCompoundParsers, "  " << oc.name << " -> |" << oc.value << "|");
            }
        }
        break;
        }
    }
    for (auto &[k, v] : unusedZoneOpcodes)
        ctx.recordUnusedItem("sfz", k, v);

    return ctx.finish();
}
} // namespace scxt::sfz_support