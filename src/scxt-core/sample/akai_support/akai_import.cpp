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
#include <fstream>
#include <map>
#include <algorithm>
#include "configuration.h"
#include "messaging/messaging.h"
#include "selection/selection_manager.h"

namespace scxt::akai_support
{

// Thanks https://burnit.co.uk/AKPspec/ plus AI

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
    auto &messageController = e.getMessageController();
    assert(messageController->threadingChecker.isSerialThread());

    auto cng = messaging::MessageController::ClientActivityNotificationGuard(
        "Loading AKP '" + path.filename().u8string() + "'", *(messageController));

    auto f = std::ifstream(path, std::ios::binary);
    if (!f.is_open())
    {
        return false;
    }
    f.seekg(0, std::ios::end);
    auto size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    f.read(buffer.data(), size);

    AKPFile akp;
    if (!akp.load(buffer.data(), size))
    {
        return false;
    }

    auto rootDir = path.parent_path();

    auto pt = std::clamp(e.getSelectionManager()->selectedPart, (int16_t)0, (int16_t)numParts);
    auto &part = e.getPatch()->getPart(pt);

    std::map<uint16_t, int> akaiGroupToSCGroup;
    std::vector<selection::SelectionManager::SelectActionContents> selectionActions;

    for (size_t i = 0; i < akp.keygroups.size(); ++i)
    {
        const auto &kg = akp.keygroups[i];

        int groupId = -1;
        if (akaiGroupToSCGroup.find(kg.kloc.grp) == akaiGroupToSCGroup.end())
        {
            groupId = part->addGroup() - 1;
            akaiGroupToSCGroup[kg.kloc.grp] = groupId;
            auto &group = part->getGroup(groupId);
            group->name = "AKP Group " + std::to_string(kg.kloc.grp);
        }
        else
        {
            groupId = akaiGroupToSCGroup[kg.kloc.grp];
        }

        auto &group = part->getGroup(groupId);

        for (const auto &z : kg.zones)
        {
            if (!z.mapped || z.sample.empty())
                continue;

            auto samplePath = rootDir / z.sample;
            // Akai samples often don't have extensions in the AKP file, but are .WAV or .AIF on
            // disk
            if (!fs::exists(samplePath))
            {
                for (auto ext : {".WAV", ".wav", ".AIF", ".aif"})
                {
                    if (fs::exists(rootDir / (z.sample + ext)))
                    {
                        samplePath = rootDir / (z.sample + ext);
                        break;
                    }
                }
            }

            SampleID sid;
            if (fs::exists(samplePath))
            {
                auto lsid = e.getSampleManager()->loadSampleByPath(samplePath);
                if (lsid.has_value())
                {
                    sid = *lsid;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                continue;
            }

            auto zn = std::make_unique<engine::Zone>(sid);
            zn->mapping.keyboardRange.keyStart = kg.kloc.lo;
            zn->mapping.keyboardRange.keyEnd = kg.kloc.hi;
            zn->mapping.velocityRange.velStart = z.lov;
            zn->mapping.velocityRange.velEnd = z.hiv;

            zn->mapping.rootKey = kg.kloc.lo + (kg.kloc.semi - 60); // This might be wrong, need to
                                                                    // check Akai root key logic
            // zn->mapping.rootKey = 60; // Default

            // If the sample itself has a root key, it will be loaded by attachToSample
            zn->attachToSample(*e.getSampleManager());

            // Adjust by AKP's tuning
            zn->mapping.pitchOffset = z.semiTune + z.fineTune * 0.01;

            int zoneIdx = group->getZones().size();
            group->addZone(zn);

            selectionActions.emplace_back(pt, groupId, zoneIdx, true, false, false);
        }
    }

    if (!selectionActions.empty())
    {
        selectionActions.back().selectingAsLead = true;
        e.getSelectionManager()->applySelectActions(selectionActions);
    }

    return true;
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
} // namespace scxt::akai_support