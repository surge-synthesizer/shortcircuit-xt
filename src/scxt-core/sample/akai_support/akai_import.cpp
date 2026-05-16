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

#include "akai_import.h"
#include "sample/import_support/import_harness.h"
#include "sample/import_support/import_mapping.h"
#include "sample/import_support/import_envelope.h"
#include "sample/import_support/import_filter.h"
#include "sample/import_support/import_numeric.h"
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <algorithm>
#include "configuration.h"
#include "messaging/messaging.h"
#include "selection/selection_manager.h"

namespace scxt::akai_support
{

// Thanks https://burnit.co.uk/AKPspec/ plus AI

// AKP envelope/LFO/filter values are bytes in 0..100 (sometimes 0..127 or
// 0..12), with curve and absolute units left undocumented in the spec.
// These conversions are best-effort approximations — they get a reasonable
// envelope shape but absolute times may need refinement once we audition
// real AKP-imported instruments side-by-side with a hardware S5/S6.
namespace
{
// AKP envelope time bytes (0..100) follow an exponential curve. Calibration
// from internet google search: 0→0ms, 30→12ms, 50→58ms, 70→295ms, 100→3300ms.
// Log-linear fit: log10(seconds) = -2.966 + 0.0349 * v, with v=0 hard-zeroed.
inline float akpTimeToSeconds(uint16_t v)
{
    if (v == 0)
        return 0.f;
    return 0.00108f * std::exp(0.0803f * (float)v);
}
// AKP cutoff byte (0..100) → CytomicSVF cutoff (semitones offset from MIDI 69).
constexpr float akpCutoffToSemis(uint16_t v) { return (v / 100.f) * 127.f - 69.f; }

// AKP S5000/S6000 filter type enum, per https://burnit.co.uk/AKPspec/.
// We currently default everything to CytomicSVF LP12 in the import; this table
// lets us tell the user which mode they actually asked for when it differs.
constexpr const char *akpFilterTypeName(uint16_t v)
{
    switch (v)
    {
    case 0:
        return "2-POLE LP";
    case 1:
        return "4-POLE LP";
    case 2:
        return "2-POLE LP+";
    case 3:
        return "2-POLE BP";
    case 4:
        return "4-POLE BP";
    case 5:
        return "2-POLE BP+";
    case 6:
        return "1-POLE HP";
    case 7:
        return "2-POLE HP";
    case 8:
        return "1-POLE HP+";
    case 9:
        return "LO<>HI";
    case 10:
        return "LO<>BAND";
    case 11:
        return "BAND<>HI";
    case 12:
        return "NOTCH 1";
    case 13:
        return "NOTCH 2";
    case 14:
        return "NOTCH 3";
    case 15:
        return "WIDE NOTCH";
    case 16:
        return "BI-NOTCH";
    case 17:
        return "PEAK 1";
    case 18:
        return "PEAK 2";
    case 19:
        return "PEAK 3";
    case 20:
        return "WIDE PEAK";
    case 21:
        return "BI-PEAK";
    case 22:
        return "PHASER 1";
    case 23:
        return "PHASER 2";
    case 24:
        return "BI-PHASE";
    case 25:
        return "VOWELISER";
    default:
        return "UNKNOWN";
    }
}
} // namespace

struct AKPContext
{
    const char *data;
    size_t size;
};

struct KLOC
{
    uint16_t lo, hi;
    uint16_t semi, fine;
    uint16_t grp;

    void load(const char *data, uint32_t size)
    {
        if (size < 15)
            return;
        lo = (uint8_t)data[4];
        hi = (uint8_t)data[5];
        semi = (uint8_t)data[6];
        fine = (uint8_t)data[7];
        grp = (uint8_t)data[14];
    }
};

struct ZONE
{
    bool mapped{false};
    std::string sample;
    uint16_t lov{0}, hiv{127};
    int16_t fineTune, semiTune;
    int16_t filter, pan;
    uint8_t playback;
    uint8_t output;
    int16_t level;
    uint8_t kbTrack;
    int16_t velToStart;

    void load(const char *data, uint32_t size)
    {
        if (size < 46)
            return;
        if (data[1] > 0)
        {
            mapped = true;
            auto slen = (uint8_t)data[1];
            for (int i = 0; i < slen && (2 + i < size) && i < 20; ++i)
                if (data[2 + i])
                    sample += (char)data[2 + i];

            lov = (uint8_t)data[34];
            hiv = (uint8_t)data[35];
            fineTune = (int8_t)data[36];
            semiTune = (int8_t)data[37];
            filter = (int8_t)data[38];
            pan = (int8_t)data[39];
            playback = (uint8_t)data[40];
            output = (uint8_t)data[41];
            level = (int8_t)data[42];
            kbTrack = (uint8_t)data[43];

            // velToStart is 16-bit at 44,45? Spec says 0152: 00 Velocity->Start LSB, 0153: 00
            // Velocity->Start MSB
            velToStart = (int16_t)((uint8_t)data[44] | ((uint8_t)data[45] << 8));
        }
    }
};

struct EG
{
    uint16_t attack, decay, sustain, release;
    int16_t velToAttack, keyScale, velToRel;

    void load(const char *data, uint32_t size)
    {
        if (size < 18)
            return;
        attack = (uint8_t)data[1];
        decay = (uint8_t)data[3];
        release = (uint8_t)data[4];
        sustain = (uint8_t)data[7];

        velToAttack = (int8_t)data[10];
        keyScale = (int8_t)data[12];
        velToRel = (int8_t)data[14];
    }
};

struct FILTER
{
    uint16_t type;
    uint16_t cutoff;
    uint16_t resonance;
    int16_t keyTrack;

    void load(const char *data, uint32_t size)
    {
        if (size < 10)
            return;
        type = (uint8_t)data[1];
        cutoff = (uint8_t)data[2];
        resonance = (uint8_t)data[3];
        keyTrack = (int8_t)data[4];
    }
};

struct LFO
{
    uint16_t type;
    uint16_t rate;
    uint16_t depth;
    uint16_t delay;

    void load(const char *data, uint32_t size)
    {
        if (size < 8)
            return;
        type = (uint8_t)data[1];
        rate = (uint8_t)data[2];
        delay = (uint8_t)data[3];
        depth = (uint8_t)data[4];
    }
};

struct MODULATION
{
    uint16_t source;
    uint16_t destination;
    int16_t amount;

    void load(const char *data, uint32_t size)
    {
        if (size < 8)
            return;
        source = (uint8_t)data[4];
        destination = (uint8_t)data[5];
        amount = (int8_t)data[6];
    }
};

struct KEYGROUP
{
    KLOC kloc;
    std::vector<ZONE> zones;
    std::vector<EG> egs;
    std::vector<FILTER> filters;
    std::vector<LFO> lfos;
    std::vector<MODULATION> modulations;

    void parse(const char *data, uint32_t size);
};

struct GLOBAL_OUT
{
    uint8_t loudness;
    uint8_t ampMod1, ampMod2;
    uint8_t panMod1, panMod2, panMod3;
    int8_t velSens;

    void load(const char *data, uint32_t size)
    {
        if (size < 8)
            return;
        loudness = (uint8_t)data[1];
        ampMod1 = (uint8_t)data[2];
        ampMod2 = (uint8_t)data[3];
        panMod1 = (uint8_t)data[4];
        panMod2 = (uint8_t)data[5];
        panMod3 = (uint8_t)data[6];
        velSens = (int8_t)data[7];
    }
};

struct GLOBAL_TUNE
{
    int8_t semi, fine;
    int8_t detune[12];
    uint8_t pbUp, pbDown;
    uint8_t bendMode;
    int8_t aftertouch;

    void load(const char *data, uint32_t size)
    {
        if (size < 19)
            return;
        semi = (int8_t)data[1];
        fine = (int8_t)data[2];
        for (int i = 0; i < 12; ++i)
            detune[i] = (int8_t)data[3 + i];
        pbUp = (uint8_t)data[15];
        pbDown = (uint8_t)data[16];
        bendMode = (uint8_t)data[17];
        aftertouch = (int8_t)data[18];
    }
};

struct GLOBAL_LFO
{
    uint8_t waveform;
    uint8_t rate, delay, depth;
    uint8_t sync;      // LFO 1 only
    uint8_t retrigger; // LFO 2 only
    uint8_t modwheel, aftertouch;
    int8_t rateMod, delayMod, depthMod;

    void load(const char *data, uint32_t size, int index)
    {
        if (size < 7)
            return;
        waveform = (uint8_t)data[1];
        rate = (uint8_t)data[2];
        delay = (uint8_t)data[3];
        depth = (uint8_t)data[4];
        if (index == 0)
        {
            sync = (uint8_t)data[5];
            if (size >= 12)
            {
                modwheel = (uint8_t)data[7];
                aftertouch = (uint8_t)data[8];
                rateMod = (int8_t)data[9];
                delayMod = (int8_t)data[10];
                depthMod = (int8_t)data[11];
            }
        }
        else
        {
            retrigger = (uint8_t)data[6];
            if (size >= 12)
            {
                rateMod = (int8_t)data[9];
                delayMod = (int8_t)data[10];
                depthMod = (int8_t)data[11];
            }
        }
    }
};

struct GLOBAL_MODS
{
    uint8_t ampMod1Src, ampMod2Src;
    uint8_t panMod1Src, panMod2Src, panMod3Src;
    uint8_t lfo1RateModSrc, lfo1DelayModSrc, lfo1DepthModSrc;
    uint8_t lfo2RateModSrc, lfo2DelayModSrc, lfo2DepthModSrc;

    void load(const char *data, uint32_t size)
    {
        if (size < 25)
            return;
        ampMod1Src = (uint8_t)data[5];
        ampMod2Src = (uint8_t)data[7];
        panMod1Src = (uint8_t)data[9];
        panMod2Src = (uint8_t)data[11];
        panMod3Src = (uint8_t)data[13];
        lfo1RateModSrc = (uint8_t)data[15];
        lfo1DelayModSrc = (uint8_t)data[17];
        lfo1DepthModSrc = (uint8_t)data[19];
        lfo2RateModSrc = (uint8_t)data[21];
        lfo2DelayModSrc = (uint8_t)data[23];
        lfo2DepthModSrc = (uint8_t)data[25];
    }
};

struct AKPFile
{
    std::string programName;
    GLOBAL_OUT out;
    GLOBAL_TUNE tune;
    std::vector<GLOBAL_LFO> lfos;
    GLOBAL_MODS mods;
    std::vector<KEYGROUP> keygroups;

    bool load(const char *data, size_t size);
};

std::string fourCC(const char *c)
{
    std::string fourCC;
    fourCC += c[0];
    fourCC += c[1];
    fourCC += c[2];
    fourCC += c[3];
    return fourCC;
}

std::pair<std::string, uint32_t> fourCCSize(const char *c)
{
    std::string fourCC;
    fourCC += c[0];
    fourCC += c[1];
    fourCC += c[2];
    fourCC += c[3];
    uint32_t size = 0;
    for (int i = 0; i < 4; ++i)
        size += (uint32_t((uint8_t)c[i + 4]) << (i * 8));
    return {fourCC, size};
}

void KEYGROUP::parse(const char *data, uint32_t size)
{
    uint32_t pos = 0;
    while (pos + 8 <= size)
    {
        auto [id, sz] = fourCCSize(data + pos);
        if (pos + 8 + sz > size)
            break;

        const char *chunkData = data + pos + 8;
        if (id == "kloc")
        {
            kloc.load(chunkData, sz);
        }
        else if (id == "zone")
        {
            ZONE z;
            z.load(chunkData, sz);
            zones.push_back(z);
        }
        else if (id == "env ")
        {
            EG e;
            e.load(chunkData, sz);
            egs.push_back(e);
        }
        else if (id == "filt")
        {
            FILTER f;
            f.load(chunkData, sz);
            filters.push_back(f);
        }
        else if (id == "lfo ")
        {
            LFO l;
            l.load(chunkData, sz);
            lfos.push_back(l);
        }
        else if (id == "modl")
        {
            MODULATION m;
            m.load(chunkData, sz);
            modulations.push_back(m);
        }
        pos += 8 + sz;
    }
}

bool AKPFile::load(const char *data, size_t size)
{
    if (size < 12)
        return false;
    if (fourCC(data) != "RIFF" || fourCC(data + 8) != "APRG")
        return false;

    uint32_t pos = 12;
    while (pos + 8 <= size)
    {
        auto [id, sz] = fourCCSize(data + pos);
        if (pos + 8 + sz > size)
            break;

        const char *chunkData = data + pos + 8;
        if (id == "pnam")
        {
            programName = std::string(chunkData, sz);
        }
        else if (id == "out ")
        {
            out.load(chunkData, sz);
        }
        else if (id == "tune")
        {
            tune.load(chunkData, sz);
        }
        else if (id == "lfo ")
        {
            GLOBAL_LFO l;
            l.load(chunkData, sz, lfos.size());
            lfos.push_back(l);
        }
        else if (id == "mods")
        {
            mods.load(chunkData, sz);
        }
        else if (id == "kgrp")
        {
            KEYGROUP kg;
            kg.parse(chunkData, sz);
            keygroups.push_back(kg);
        }
        pos += 8 + sz;
    }
    return true;
}

bool importAKP(const fs::path &path, engine::Engine &e)
{
    import_support::ImporterContext ctx(e, "Loading AKP '" + path.filename().u8string() + "'");

    auto f = std::ifstream(path, std::ios::binary);
    if (!f.is_open())
        return ctx.fail("AKP Load Error", "Cannot open file");

    f.seekg(0, std::ios::end);
    auto size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    f.read(buffer.data(), size);

    AKPFile akp;
    if (!akp.load(buffer.data(), size))
        return ctx.fail("AKP Load Error", "Failed to parse AKP file");

    auto rootDir = path.parent_path();
    std::map<uint16_t, int> akaiGroupToSCGroup;
    std::set<uint16_t> reportedFilterTypes; // dedupe filter-type warnings per import

    for (size_t i = 0; i < akp.keygroups.size(); ++i)
    {
        const auto &kg = akp.keygroups[i];

        int groupId = import_support::getOrCreateGroup(ctx, akaiGroupToSCGroup, kg.kloc.grp, [&] {
            return "AKP Group " + std::to_string(kg.kloc.grp);
        });

        for (const auto &z : kg.zones)
        {
            if (!z.mapped || z.sample.empty())
                continue;

            // AKP files reference samples by name without extension; the actual
            // file on disk is .WAV or .AIF (and case may vary).
            auto lsid =
                ctx.loadSampleFromDisk({rootDir / z.sample}, {".WAV", ".wav", ".AIF", ".aif"});
            if (!lsid)
                continue;

            auto zn = std::make_unique<engine::Zone>(*lsid);
            zn->engine = &e;
            import_support::importZoneMapping(
                *zn, ctx,
                {
                    // This might be wrong, need to check Akai root key logic
                    .rootKey = kg.kloc.lo + (kg.kloc.semi - 60),
                    .keyStart = kg.kloc.lo,
                    .keyEnd = kg.kloc.hi,
                    .velStart = z.lov,
                    .velEnd = z.hiv,
                    .pitchOffsetSemitones =
                        z.semiTune + import_support::centsToSemitones((float)z.fineTune),
                });

            // Zone-level pan and level (per ZONE struct). AKP pan is -50..50;
            // level is -100..100 with no spec on units — treat as dB-ish.
            if (z.pan != 0)
                zn->outputInfo.pan = z.pan / 50.f;
            if (z.level != 0)
                zn->mapping.amplitude = z.level / 2.f;

            // Envelopes: AKP keygroups carry up to three envs (amp/filt/aux).
            // SCXT egStorage[0] is the AmpEG; [1] is FilEG.
            auto envelopeFromAkp = [](const EG &eg) -> import_support::EnvelopeArgs {
                return {
                    .attackSeconds = akpTimeToSeconds(eg.attack),
                    .decaySeconds = akpTimeToSeconds(eg.decay),
                    .sustainLevel = eg.sustain / 100.f,
                    .releaseSeconds = akpTimeToSeconds(eg.release),
                };
            };
            if (kg.egs.size() >= 1)
                import_support::importZoneEnvelope(*zn, ctx, 0, envelopeFromAkp(kg.egs[0]));
            if (kg.egs.size() >= 2)
                import_support::importZoneEnvelope(*zn, ctx, 1, envelopeFromAkp(kg.egs[1]));

            // Filter: AKP type is an enum 0..25 (see akpFilterTypeName above).
            // We currently always set a CytomicSVF LP12; if the AKP asked for
            // anything other than type 0 (2-POLE LP) we raise once per unique
            // type so the user knows their filter mode wasn't honored.
            if (!kg.filters.empty())
            {
                const auto &flt = kg.filters[0];
                if (flt.cutoff < 100 || flt.resonance > 0)
                {
                    if (flt.type != 0 && reportedFilterTypes.insert(flt.type).second)
                    {
                        ctx.raise("AKP Filter Type Not Mapped",
                                  std::string("AKP requested filter type ") +
                                      std::to_string(flt.type) + " (" +
                                      akpFilterTypeName(flt.type) +
                                      "); defaulting to CytomicSVF LP12.");
                    }
                    import_support::importZoneFilter(
                        *zn, ctx, 0,
                        {
                            .type = dsp::processor::ProcessorType::proct_CytomicSVF,
                            .cutoff = akpCutoffToSemis(flt.cutoff),
                            .resonance = flt.resonance / 12.f,
                        });
                }
            }

            // MAPPING is masked off because the AKP file already supplied root/key/vel
            // and we don't want a smpl-chunk in the referenced WAV to clobber them.
            zn->attachToSample(*e.getSampleManager(), 0,
                               engine::Zone::ENDPOINTS | engine::Zone::LOOP);

            ctx.addZoneToGroup(groupId, std::move(zn));
        }
    }
    return ctx.finish();
}

void dumpAkaiToLog(const fs::path &path)
{
    SCLOG_IF(cliTools, "Dumping " << path.u8string());
    auto f = std::ifstream(path, std::ios::binary);
    if (!f.is_open())
    {
        SCLOG_IF(cliTools, "Failed to open file");
        return;
    }
    f.seekg(0, std::ios::end);
    auto size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    f.read(buffer.data(), size);
    SCLOG_IF(cliTools, "File size: " << size);

    AKPFile akp;
    if (!akp.load(buffer.data(), size))
    {
        SCLOG_IF(cliTools, "Failed to load AKP file");
        return;
    }

    SCLOG_IF(cliTools, "AKP Program: " << akp.programName);
    SCLOG_IF(cliTools, "  Out: loudness: " << (int)akp.out.loudness << " ampMod: "
                                           << (int)akp.out.ampMod1 << " " << (int)akp.out.ampMod2
                                           << " panMod: " << (int)akp.out.panMod1 << " "
                                           << (int)akp.out.panMod2 << " " << (int)akp.out.panMod3
                                           << " velSens: " << (int)akp.out.velSens);
    SCLOG_IF(cliTools, "  Tune: semi: " << (int)akp.tune.semi << " fine: " << (int)akp.tune.fine
                                        << " pbUp/Dn: " << (int)akp.tune.pbUp << "/"
                                        << (int)akp.tune.pbDown
                                        << " bendMode: " << (int)akp.tune.bendMode
                                        << " AT: " << (int)akp.tune.aftertouch);
    for (size_t i = 0; i < akp.lfos.size(); ++i)
    {
        const auto &l = akp.lfos[i];
        SCLOG_IF(cliTools, "  LFO " << (i + 1) << ": wave: " << (int)l.waveform
                                    << " rate: " << (int)l.rate << " delay: " << (int)l.delay
                                    << " depth: " << (int)l.depth
                                    << (i == 0 ? " sync: " : " retrig: ")
                                    << (i == 0 ? (int)l.sync : (int)l.retrigger));
        SCLOG_IF(cliTools, "        modwheel: " << (int)l.modwheel << " AT: " << (int)l.aftertouch
                                                << " rateMod: " << (int)l.rateMod
                                                << " delayMod: " << (int)l.delayMod
                                                << " depthMod: " << (int)l.depthMod);
    }
    SCLOG_IF(cliTools,
             "  Mods: ampSrc: " << (int)akp.mods.ampMod1Src << " " << (int)akp.mods.ampMod2Src
                                << " panSrc: " << (int)akp.mods.panMod1Src << " "
                                << (int)akp.mods.panMod2Src << " " << (int)akp.mods.panMod3Src);
    SCLOG_IF(cliTools, "        lfo1Mod: " << (int)akp.mods.lfo1RateModSrc << " "
                                           << (int)akp.mods.lfo1DelayModSrc << " "
                                           << (int)akp.mods.lfo1DepthModSrc
                                           << " lfo2Mod: " << (int)akp.mods.lfo2RateModSrc << " "
                                           << (int)akp.mods.lfo2DelayModSrc << " "
                                           << (int)akp.mods.lfo2DepthModSrc);

    SCLOG_IF(cliTools, "Keygroups: " << akp.keygroups.size());

    for (size_t i = 0; i < akp.keygroups.size(); ++i)
    {
        const auto &kg = akp.keygroups[i];
        SCLOG_IF(cliTools, "  Keygroup " << i);
        SCLOG_IF(cliTools, "    range: " << kg.kloc.lo << " " << kg.kloc.hi
                                         << " tune:  " << kg.kloc.semi << " " << kg.kloc.fine
                                         << " group: " << kg.kloc.grp);
        for (const auto &z : kg.zones)
        {
            if (z.mapped)
            {
                SCLOG_IF(cliTools, "    Zone: sample: '" << z.sample << "' vel range " << z.lov
                                                         << "-" << z.hiv);
                SCLOG_IF(cliTools, "          tune: " << z.semiTune << " " << z.fineTune
                                                      << " filter: " << z.filter
                                                      << " pan: " << z.pan);
                SCLOG_IF(cliTools, "          playback: " << (int)z.playback << " output: "
                                                          << (int)z.output << " level: " << z.level
                                                          << " kbTrack: " << (int)z.kbTrack);
                SCLOG_IF(cliTools, "          velToStart: " << z.velToStart);
            }
        }
        for (const auto &e : kg.egs)
        {
            SCLOG_IF(cliTools, "    EG: A:" << e.attack << " D:" << e.decay << " R:" << e.release
                                            << " S:" << e.sustain);
            SCLOG_IF(cliTools, "        Vel>Attack: " << e.velToAttack << " KeyScale: "
                                                      << e.keyScale << " Vel>Rel: " << e.velToRel);
        }
        for (const auto &fl : kg.filters)
        {
            SCLOG_IF(cliTools, "    Filter: T:" << fl.type << " C:" << fl.cutoff
                                                << " R:" << fl.resonance << " KT:" << fl.keyTrack);
        }
        for (const auto &l : kg.lfos)
        {
            SCLOG_IF(cliTools, "    LFO: T:" << l.type << " R:" << l.rate << " D:" << l.depth
                                             << " Dy:" << l.delay);
        }
        for (const auto &m : kg.modulations)
        {
            SCLOG_IF(cliTools,
                     "    Mod: S:" << m.source << " D:" << m.destination << " A:" << m.amount);
        }
    }
}

/*
 * TODO — AKP import gap list (parsed by AKPFile but not yet wired into the engine):
 *
 * Per-keygroup data
 * -----------------
 *  - kg.egs[2] (AUX env): would populate egStorage[2]. Currently unused; needs
 *    a mod target to be useful (it isn't routed automatically).
 *  - kg.egs[*].velToAttack / keyScale / velToRel: per-env velocity and key
 *    scaling sensitivities. Need mod routes (velocity -> egN.attack/release).
 *  - kg.filters[0].type: AKP enum 0..25 (see akpFilterTypeName). Currently
 *    every type collapses to CytomicSVF LP12 with a user-visible warning.
 *    Real fix needs a mapping table AKP-type -> SCXT ProcessorType and the
 *    appropriate cutoff/resonance scaling per SCXT filter.
 *  - kg.filters[0].keyTrack: filter keytrack amount. Needs a key-track mod
 *    route to filter cutoff.
 *  - kg.lfos[]: per-keygroup LFOs (type/rate/depth/delay). Should call
 *    importZoneLFO and then a mod route to whatever the AKP MODULATION block
 *    targets — none of which is wired yet.
 *  - kg.modulations[]: source/destination/amount triples. Source enum 0..14
 *    (NO_SOURCE, MODWHEEL, BEND, AFTERTOUCH, EXTERNAL, VELOCITY, KEYBOARD,
 *    LFO1, LFO2, AMP_ENV, FILT_ENV, AUX_ENV, dMODWHEEL, dBEND, dEXTERNAL).
 *    Destination enum is also AKP-specific. Requires extending
 *    import_modulation's ImportedSourceKind / ImportedTargetKind (currently
 *    only LFO/EG sources and Pitch/FilterParam targets) before AKP mod routes
 *    can be expressed.
 *
 * Per-zone data
 * -------------
 *  - z.filter: per-zone filter offset/level. Today only the keygroup-level
 *    filter block is read; the zone-level filter offset is ignored.
 *  - z.playback: playback mode (one-shot, looped, etc.). Maps to Zone loop /
 *    playmode but the AKP enum values aren't decoded here yet.
 *  - z.output: output bus assignment. Could map to Zone routeTo.
 *  - z.kbTrack: per-zone keyboard tracking. Maps to mapping.tracking.
 *  - z.velToStart: velocity -> sample-start-offset. Needs a mod route from
 *    Velocity to SampleStartPos (target kind not in import_modulation yet).
 *  - z.level scaling: currently z.level / 2.f as mapping.amplitude in dB.
 *    AKP spec says -100..100 with no documented units; needs hardware
 *    calibration like akpTimeToSeconds got.
 *
 * Global (program-level) data
 * ---------------------------
 *  - akp.out (loudness, ampMod1/2, panMod1/2/3, velSens): program-wide
 *    output settings. Some map to Part-level config, some to Group-level.
 *  - akp.tune (semi, fine, detune[12], pbUp, pbDown, bendMode, aftertouch):
 *    program-wide tuning + per-note detune. pbUp/pbDown could go to Zone
 *    pbUp/pbDown; detune[12] is a per-pitch-class tuning table SCXT doesn't
 *    currently have a direct equivalent for.
 *  - akp.lfos[0..1] (GLOBAL_LFO): the two AKP "global" LFOs distinct from
 *    keygroup LFOs. Need a Group-level or Part-level LFO host, plus the same
 *    mod-route expansion noted above.
 *  - akp.mods (GLOBAL_MODS): cross-references that say which source modulates
 *    each of the GLOBAL_OUT / GLOBAL_LFO targets. Resolution depends on all
 *    the destinations above being wired first.
 *
 * Curve calibration
 * -----------------
 *  - akpCutoffToSemis: currently linear (v/100)*127 - 69. Likely needs a log
 *    fit like akpTimeToSeconds got, once we audition against hardware.
 *
 * Helper layer
 * ------------
 *  - Extend ImportedSourceKind: MidiCC, Velocity, ChannelPressure, PolyAT,
 *    KeyTrack, PitchTrack, NoteExpression. Required for AKP mod routes AND
 *    SF2 modulators block AND SFZ *_oncc* family — landing this unlocks
 *    Phase B progress on three formats at once.
 *  - Extend ImportedTargetKind: Pan, Amplitude, PlaybackRatio, LFORate,
 *    LFODepth, EGStage, SampleStartPos.
 */

} // namespace scxt::akai_support