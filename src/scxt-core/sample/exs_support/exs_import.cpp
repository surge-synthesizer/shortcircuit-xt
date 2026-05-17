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

#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include "utils.h"
#include "exs_import.h"
#include "exs_parameters.h"
#include "sample/import_support/import_harness.h"
#include "sample/import_support/import_mapping.h"
#include "sample/import_support/import_loop.h"
#include "sample/import_support/import_envelope.h"
#include "sample/import_support/import_filter.h"
#include "sample/import_support/import_modulation.h"
#include "sample/import_support/import_numeric.h"
#include "dsp/processor/processor.h"
#include <cmath>

#include <messaging/messaging.h>

namespace scxt::exs_support
{
using byteData_t = std::vector<uint8_t>;

// EXS envelope time bytes (0..127) follow Logic's quartic time control, not
// the linear value/127*10 the Java ConvertWithMoss reference uses (which we
// initially mirrored — they have the same bug). Logic's max release is 10
// seconds (confirmed against Sampler); a power-law fit to readings from
// French Horns Legato.exs and Supergrain 5.exs gives k≈4 anchored at that
// upper bound:
//   v=0   -> 0 s       (hard-zeroed)
//   v=37  -> 72 ms     ✓
//   v=47  -> 188 ms    ✓
//   v=87  -> 2.20 s    ✓
//   v=115 -> 6.27 s    (formula gives 6.74 s; probably display rounding)
//   v=127 -> 10 s      (max)
// Fit: seconds = (v/127)^4 * 10.
inline float exsEnvByteToSeconds(int v)
{
    if (v <= 0)
        return 0.f;
    float frac = v / 127.f;
    return frac * frac * frac * frac * 10.f;
}

uint32_t readU32(std::ifstream &f, bool bigE)
{
    char c4[4];
    f.read(c4, 4);
    uint32_t res{0};
    if (bigE)
    {
        for (int i = 0; i < 4; ++i)
        {
            res = res << 8;
            res += (uint8_t)c4[i];
        }
    }
    else
    {
        for (int i = 3; i >= 0; --i)
        {
            res = res << 8;
            res += (uint8_t)c4[i];
        }
    }
    return res;
}

struct EXSBlock
{
    /** An instrument block. */
    static constexpr uint32_t TYPE_INSTRUMENT = 0x00;
    /** A zone block. */
    static constexpr uint32_t TYPE_ZONE = 0x01;
    /** A group block. */
    static constexpr uint32_t TYPE_GROUP = 0x02;
    /** A sample block. */
    static constexpr uint32_t TYPE_SAMPLE = 0x03;
    /** A parameters block. */
    static constexpr uint32_t TYPE_PARAMS = 0x04;
    /** An unknown block. */
    static constexpr uint32_t TYPE_UNKNOWN = 0x08;

    bool isBigEndian{true};
    uint32_t type{0};
    uint32_t size{0};
    uint32_t index{0};
    uint32_t flags{0};
    std::string name;
    byteData_t content;

    bool read(std::ifstream &f)
    {
#define CHECK(...)                                                                                 \
    __VA_ARGS__;                                                                                   \
    if (f.eof())                                                                                   \
        return false;

        uint8_t c;
        CHECK(f.read((char *)&c, 1));

        isBigEndian = (c == 0);

        uint8_t v1, v2, tp;
        CHECK(f.read((char *)&v1, 1));
        CHECK(f.read((char *)&v2, 1));
        CHECK(f.read((char *)&tp, 1));

        type = tp & 0x0F;

        if (v1 != 1 && v2 != 0)
        {
            SCLOG_IF(sampleCompoundParsers, "Unknown EXS Version: " << v1 << " " << v2);
            return false;
        }

        CHECK(size = readU32(f, isBigEndian));
        CHECK(index = readU32(f, isBigEndian));
        CHECK(flags = readU32(f, isBigEndian));

        char magic[4];
        CHECK(f.read(magic, 4));
        // TODO: Check

        char namec[64];
        CHECK(f.read(namec, 64));
        name = std::string(namec); // assume null termination

        content.resize(size);
        f.read((char *)(content.data()), size);
#undef CHECK
        return true;
    }
};

struct EXSObject
{
    EXSBlock within;
    EXSObject(EXSBlock in) : within(in) {}

    size_t position{0};
    void resetPosition() { position = 0; }
    void skip(int S)
    {
        position += S;
        assert(position <= within.size);
    }

    int32_t readByteToInt()
    {
        char c = within.content[position];
        position++;
        assert(position <= within.size);
        return (int32_t)c;
    }

    uint8_t readByte()
    {
        char c = within.content[position];
        position++;
        assert(position <= within.size);
        return (uint8_t)c;
    }

    int16_t readS16()
    {
        int16_t res = 0;
        if (within.isBigEndian)
        {
            for (auto i = position; i < position + 2; ++i)
            {
                res = res << 8;
                res += (uint8_t)within.content[i];
            }
        }
        else
        {
            for (auto i = (int)position + 1; i >= (int)position; --i)
            {
                res = res << 8;
                res += (uint8_t)within.content[i];
            }
        }
        position += 2;
        assert(position <= within.size);
        return res;
    }

    uint16_t readU16()
    {
        uint32_t res = 0;
        if (within.isBigEndian)
        {
            for (auto i = position; i < position + 2; ++i)
            {
                res = res << 8;
                res += (uint8_t)within.content[i];
            }
        }
        else
        {
            for (auto i = (int)position + 1; i >= (int)position; --i)
            {
                res = res << 8;
                res += (uint8_t)within.content[i];
            }
        }
        position += 2;
        assert(position <= within.size);
        return res;
    }
    uint32_t readU32()
    {
        uint32_t res = 0;
        if (within.isBigEndian)
        {
            for (auto i = position; i < position + 4; ++i)
            {
                res = res << 8;
                res += (uint8_t)within.content[i];
            }
        }
        else
        {
            // without this int cast at position = 0 this loop never terminates
            for (auto i = (int)position + 3; i >= (int)position; --i)
            {
                res = res << 8;
                res += (uint8_t)within.content[i];
            }
        }
        position += 4;
        assert(position <= within.size);
        return res;
    }
    std::string readStringNoTerm(size_t chars)
    {
        auto res = std::string(within.content[position], chars);
        position += chars;
        return res;
    }
    std::string readStringNullTerm(size_t chars)
    {
        // Sloppy
        char buf[512];
        assert(chars < 512);
        memset(buf, 0, sizeof(buf));
        memcpy(buf, within.content.data() + position, chars);
        position += chars;
        return std::string(buf);
    }
};
struct EXSInstrument : EXSObject
{
    EXSInstrument(EXSBlock in) : EXSObject(in) { assert(in.type == EXSBlock::TYPE_INSTRUMENT); }
};

struct EXSZone : EXSObject
{
    bool pitch;
    bool oneshot;
    bool reverse;
    int32_t key;
    int32_t fineTuning;
    int32_t pan;
    int32_t volumeAdjust;
    int32_t volumeScale;
    int32_t coarseTuning;
    int32_t keyLow;
    int32_t keyHigh;
    bool velocityRangeOn;
    int32_t velocityLow;
    int32_t velocityHigh;
    int32_t sampleStart;
    int32_t sampleEnd;
    int32_t loopStart;
    int32_t loopEnd;
    int32_t loopCrossfade;
    int32_t loopTune;
    bool loopOn;
    bool loopEqualPower;
    bool loopPlayToEndOnRelease;
    int32_t loopDirection;
    int32_t flexOptions;
    int32_t flexSpeed;
    int32_t tailTune;
    int32_t output;
    int32_t groupIndex;
    int32_t sampleIndex;
    int32_t sampleFadeOut = 0;
    int32_t offset = 0;

    EXSZone(EXSBlock in) : EXSObject(in)
    {
        assert(within.type == EXSBlock::TYPE_ZONE);
        parse();
    }
    void parse()
    {
        resetPosition();
        int zoneOpts = readByteToInt();
        pitch = (zoneOpts & 1 << 1) == 0;
        oneshot = (zoneOpts & 1 << 0) != 0;
        reverse = (zoneOpts & 1 << 2) != 0;
        velocityRangeOn = (zoneOpts & 1 << 3) != 0;

        key = readByteToInt();
        fineTuning = decodeTwosComplement(readByte());
        pan = decodeTwosComplement(readByte());
        volumeAdjust = decodeTwosComplement(readByte());
        volumeScale = readByteToInt();
        keyLow = readByteToInt();
        keyHigh = readByteToInt();
        skip(1);
        velocityLow = readByteToInt();
        velocityHigh = readByteToInt();
        skip(1);
        sampleStart = (int)readU32();
        sampleEnd = (int)readU32();
        loopStart = (int)readU32();
        loopEnd = (int)readU32();
        loopCrossfade = (int)readU32();
        loopTune = readByteToInt();

        int loopOptions = readByteToInt();
        loopOn = (loopOptions & 1) != 0;
        loopEqualPower = (loopOptions & 2) != 0;
        loopPlayToEndOnRelease = (loopOptions & 4) != 0;

        loopDirection = readByteToInt();
        skip(42);
        flexOptions = readByteToInt();
        flexSpeed = readByteToInt();
        tailTune = readByteToInt();
        coarseTuning = decodeTwosComplement(readByte());

        skip(1);

        output = readByteToInt();
        if ((zoneOpts & 1 << 6) == 0)
            output = -1;

        skip(5);

        groupIndex = (int)readU32();
        sampleIndex = (int)readU32();
    }

    static int32_t decodeTwosComplement(uint8_t value)
    {
        return (value & 0x80) != 0 ? value - 256 : value;
    }
};

struct EXSGroup : EXSObject
{
    int32_t volume = 0;
    int32_t pan = 0;
    int32_t polyphony = 0; // = Max
    int32_t exclusive = 0;
    int32_t minVelocity = 0;
    int32_t maxVelocity = 127;
    int32_t sampleSelectRandomOffset = 0;
    int32_t releaseTriggerTime = 0;
    int32_t velocityRangExFade = 0;
    int32_t velocityRangExFadeType = 0;
    int32_t keyrangExFadeType = 0;
    int32_t keyrangExFade = 0;
    int32_t enableByTempoLow = 80;
    int32_t enableByTempoHigh = 140;
    int32_t cutoffOffset = 0;
    int32_t resoOffset = 0;
    int32_t env1AttackOffset = 0;
    int32_t env1DecayOffset = 0;
    int32_t env1SustainOffset = 0;
    int32_t env1ReleaseOffset = 0;
    bool releaseTrigger = false;
    int32_t output = 0;
    int32_t enableByNoteValue = 0;
    int32_t roundRobinGroupPos = -1;
    int32_t enableByType = 0;
    int32_t enableByControlValue = 0;
    int32_t enableByControlLow = 0;
    int32_t enableByControlHigh = 0;
    int32_t startNote = 0;
    int32_t endNote = 127;
    int32_t enableByMidiChannel = 0;
    int32_t enableByArticulationValue = 0;
    int32_t enableByBenderLow = 0;
    int32_t enableByBenderHigh = 0;
    int32_t env1HoldOffset = 0;
    int32_t env2AttackOffset = 0;
    int32_t env2DecayOffset = 0;
    int32_t env2SustainOffset = 0;
    int32_t env2ReleaseOffset = 0;
    int32_t env2HoldOffset = 0;
    int32_t env1DelayOffset = 0;
    int32_t env2DelayOffset = 0;

    bool mute = false;
    bool releaseTriggerDecay = false;
    bool fixedSampleSelect = false;

    bool enableByNote = false;
    bool enableByRoundRobin = false;
    bool enableByControl = false;
    bool enableByBend = false;
    bool enableByChannel = false;
    bool enableByArticulation = false;
    bool enablebyTempo = false;

    EXSGroup(EXSBlock in) : EXSObject(in)
    {
        assert(within.type == EXSBlock::TYPE_GROUP);
        parse();
    }
    void parse()
    {
        resetPosition();
        volume = readByteToInt();
        pan = readByteToInt();
        polyphony = readByteToInt();
        int options = readByteToInt();
        mute = (options & 16) > 0;                // 0 = OFF, 1 = ON
        releaseTriggerDecay = (options & 64) > 0; // 0 = OFF, 1 = ON
        fixedSampleSelect = (options & 128) > 0;  // 0 = OFF, 1 = ON

        exclusive = readByteToInt();
        minVelocity = readByteToInt();
        maxVelocity = readByteToInt();
        sampleSelectRandomOffset = readByteToInt();

        skip(8);

        releaseTriggerTime = readU16();

        skip(14);

        velocityRangExFade = readByteToInt() - 128;
        velocityRangExFadeType = readByteToInt();
        keyrangExFadeType = readByteToInt();
        keyrangExFade = readByteToInt() - 128;

        skip(2);

        enableByTempoLow = readByteToInt();
        enableByTempoHigh = readByteToInt();

        skip(1);

        cutoffOffset = readByteToInt();
        skip(1);

        resoOffset = readByteToInt();
        skip(12);

        env1AttackOffset = (int)readU32();
        env1DecayOffset = (int)readU32();
        env1SustainOffset = (int)readU32();
        env1ReleaseOffset = (int)readU32();
        skip(1);

        releaseTrigger = readByteToInt() > 0;
        output = readByteToInt();
        enableByNoteValue = readByteToInt();

        if (position < within.size)
        {
            skip(4);

            roundRobinGroupPos = (int)readU32();
            enableByType = readByteToInt();
            enableByNote = enableByType == 1;
            enableByRoundRobin = enableByType == 2;
            enableByControl = enableByType == 3;
            enableByBend = enableByType == 4;
            enableByChannel = enableByType == 5;
            enableByArticulation = enableByType == 6;
            enablebyTempo = enableByType == 7;

            enableByControlValue = readByteToInt();
            enableByControlLow = readByteToInt();
            enableByControlHigh = readByteToInt();
            startNote = readByteToInt();
            endNote = readByteToInt();
            enableByMidiChannel = readByteToInt();
            enableByArticulationValue = readByteToInt();
        }
    }
};

struct EXSSample : EXSObject
{
    int waveDataStart;
    int length;
    int sampleRate;
    int bitDepth;
    int channels;
    int channels2;
    std::string type;
    int size;
    bool isCompressed = false;
    std::string filePath;
    std::string fileName;

    EXSSample(EXSBlock in) : EXSObject(in)
    {
        assert(in.type == EXSBlock::TYPE_SAMPLE);
        assert(in.size);
        parse();
    }
    void parse()
    {
        resetPosition();
        waveDataStart = readU32();
        length = readU32();
        sampleRate = readU32();
        bitDepth = readU32();

        channels = readU32();
        channels2 = readU32();

        skip(4);

        type = readStringNoTerm(4);
        size = readU32();
        isCompressed = readU32() > 0;

        skip(40);

        filePath = readStringNullTerm(256);

        // If not present the name from the header is used!
        fileName = (position < within.size) ? readStringNullTerm(256) : within.name;
    }
};

struct EXSParameters : EXSObject
{
    // Sparse map of (param-id -> int16 value). Named ids live in
    // exs_parameters.h; see that header (and ConvertWithMoss's
    // EXS24Parameters.java which it was transcribed from) for what each id
    // means.
    std::map<uint16_t, int16_t> params;

    // Typed accessor: returns the stored value if present, otherwise
    // `defaultVal`. Use the EXSParam enum so call sites read declaratively.
    int16_t get(EXSParam id, int16_t defaultVal = 0) const
    {
        auto it = params.find(static_cast<uint16_t>(id));
        return it != params.end() ? it->second : defaultVal;
    }
    bool has(EXSParam id) const { return params.find(static_cast<uint16_t>(id)) != params.end(); }

    EXSParameters(EXSBlock in) : EXSObject(in)
    {
        assert(in.type == EXSBlock::TYPE_PARAMS);
        assert(in.size);
        parse();
    }
    void parse()
    {
        int paramCount = (int)readU32();
        int paramBlockLength = paramCount * 3;

        // After the 4-byte count we just consumed, the chunk holds
        // `paramCount` 1-byte IDs followed by `paramCount` 2-byte LE values.
        // Offset both lookups by the current position so we read from
        // *after* the count, not from byte 0 of the chunk (which is the
        // count itself).
        const auto idsBase = position;
        const auto valuesBase = position + paramCount;

        for (int i = 0; i < paramCount; i++)
        {
            int paramId = within.content[idsBase + i] & 0xFF;
            if (paramId != 0)
            {
                auto offset = valuesBase + 2 * i;
                int16_t value = within.content[offset] | (within.content[offset + 1] << 8);
                params[paramId] = value;
            }
        }

        position += paramBlockLength;
        if (position < within.size)
        {
            paramCount = (int)readU32();
            if (paramCount > 0)
            {
                paramBlockLength = paramCount * 4;
                while (position < within.size)
                {
                    auto id = readU16();
                    auto val = readS16();
                    if (id != 0)
                    {
                        params[id] = val;
                    }
                }
            }
        }
    }
};
;

struct EXSInfo
{
    std::vector<EXSZone> zones;
    std::vector<EXSGroup> groups;
    std::vector<EXSSample> samples;
    std::optional<EXSParameters> parameters;
};

EXSInfo parseEXS(const fs::path &p)
{
    SCLOG_IF(sampleCompoundParsers, "Importing EXS from " << p.u8string());
    std::ifstream inputFile(p, std::ios_base::binary);

    auto block = EXSBlock();
    std::vector<EXSZone> zones;
    std::vector<EXSGroup> groups;
    std::vector<EXSSample> samples;
    std::optional<EXSParameters> parameters;

    while (block.read(inputFile))
    {
        switch (block.type)
        {
        case EXSBlock::TYPE_INSTRUMENT:
        {
            auto inst = EXSInstrument(block);
        }
        break;
        case EXSBlock::TYPE_ZONE:
        {
            zones.emplace_back(block);
        }
        break;
        case EXSBlock::TYPE_GROUP:
        {
            groups.emplace_back(block);
        }
        break;
        case EXSBlock::TYPE_SAMPLE:
        {
            samples.emplace_back(block);
        }
        break;
        case EXSBlock::TYPE_PARAMS:
        {
            parameters = EXSParameters(block);
        }
        break;
        default:
        {
            // SCLOG_IF(sampleCompoundParsers, "Un-parsed EXS block type " << block.type);
            break;
        }
        }
    }

    auto res = EXSInfo();
    res.zones = zones;
    res.groups = groups;
    res.samples = samples;
    res.parameters = parameters;
    return res;
}

std::vector<EXSCompoundElement> getEXSCompoundElements(const fs::path &p)
{
    auto info = parseEXS(p);
    std::vector<EXSCompoundElement> res;
    int idx = 0;
    for (const auto &s : info.samples)
    {
        res.push_back({s.within.name, idx++});
    }
    return res;
}

std::optional<scxt::SampleID> loadSampleFromEXS(const fs::path &p, int sampleIndex,
                                                sample::SampleManager *sm)
{
    auto info = parseEXS(p);
    if (sampleIndex < 0 || sampleIndex >= info.samples.size())
        return std::nullopt;

    auto &s = info.samples[sampleIndex];
    auto sp = fs::path{s.filePath} / s.fileName;
    return sm->loadSampleByPath(sp);
}

bool importEXS(const fs::path &p, engine::Engine &e)
{
    import_support::ImporterContext ctx(e, "Loading EXS '" + p.filename().u8string() + "'");

    auto info = parseEXS(p);

    std::vector<int> groupIndexByOrder;
    std::vector<SampleID> sampleIDByOrder;

    // EXS files store an absolute path captured when the patch was saved. When
    // the patch is moved between machines that path won't resolve, so we also
    // try the sample filename relative to the .exs's own directory.
    auto exsDir = p.parent_path();
    for (auto &s : info.samples)
    {
        auto absolutePath = fs::path{s.filePath} / s.fileName;
        auto siblingPath = exsDir / s.fileName;
        if (auto lsid = ctx.loadSampleFromDisk({absolutePath, siblingPath}))
            sampleIDByOrder.push_back(*lsid);
    }

    // Group-level config (volume/pan to GroupOutputInfo, polyphony to playMode,
    // exclusive to choke-group ID). Bumps Part::numExclusiveGroups for any
    // non-zero exclusive IDs we've seen.
    int maxExclusive = 0;
    for (auto &g : info.groups)
    {
        int scxtGid = ctx.addGroup(g.within.name);
        groupIndexByOrder.push_back(scxtGid);

        auto &grp = *ctx.getPart().getGroup(scxtGid);
        if (g.volume != 0)
            grp.outputInfo.amplitude = import_support::dBToCubicAttenuation((float)g.volume);
        if (g.pan != 0)
            grp.outputInfo.pan = g.pan / 50.f;

        if (g.polyphony == 1)
        {
            grp.outputInfo.playMode = engine::Group::PlayMode::MONO;
        }
        else if (g.polyphony > 1)
        {
            grp.outputInfo.hasIndependentPolyLimit = true;
            grp.outputInfo.polyLimit = g.polyphony;
        }
        // polyphony == 0: leave defaults (POLY, no limit).

        if (g.exclusive > 0)
        {
            grp.outputInfo.exclusiveGroup = g.exclusive;
            if (g.exclusive > maxExclusive)
                maxExclusive = g.exclusive;
        }
    }
    if (maxExclusive > 0)
    {
        auto &cfg = ctx.getPart().configuration;
        if (maxExclusive > cfg.numExclusiveGroups)
            cfg.numExclusiveGroups = maxExclusive;
    }

    // --- Global "parameters block" wiring ----------------------------------
    //
    // Logic stores program-wide settings (master tune, pitch-bend range,
    // global filter, global ENV1=amp / ENV2=filter envelopes) in the
    // parameters chunk. SCXT doesn't have a program-level filter or
    // envelope, so we project these onto every zone — every zone gets the
    // same ENV1/ENV2 + filter, and the global filter envelope (ENV2) is
    // wired to filter cutoff via a mod route.
    //
    // Param IDs are in exs_parameters.h (named from ConvertWithMoss's
    // EXS24Parameters.java); conversions follow the same reference unless
    // noted.
    const bool haveParams = info.parameters.has_value();
    const auto &params = info.parameters;

    float globalTuneSemis = 0;
    if (haveParams)
    {
        int coarse = params->get(EXSParam::COARSE_TUNE);
        int fine = params->get(EXSParam::FINE_TUNE);
        globalTuneSemis = coarse + import_support::centsToSemitones((float)fine);
    }

    int16_t globalPbUp = 2, globalPbDown = 2;
    bool haveGlobalPb = false;
    if (haveParams && params->has(EXSParam::PITCH_BEND_UP))
    {
        haveGlobalPb = true;
        int up = params->get(EXSParam::PITCH_BEND_UP, 2);
        int down = params->get(EXSParam::PITCH_BEND_DOWN, -1);
        globalPbUp = (int16_t)up;
        // EXS convention: PITCH_BEND_DOWN == -1 means "match PITCH_BEND_UP"
        globalPbDown = (int16_t)((down == -1) ? up : down);
    }
    if (haveGlobalPb)
    {
        for (int gid : groupIndexByOrder)
        {
            auto &grp = *ctx.getPart().getGroup(gid);
            grp.outputInfo.pbUp = globalPbUp;
            grp.outputInfo.pbDown = globalPbDown;
        }
    }

    const bool filterEnabled = haveParams && params->get(EXSParam::FILTER1_TOGGLE) > 0;
    import_support::FilterArgs filterArgs;
    if (filterEnabled)
    {
        // TODO: map FILTER1_TYPE (0..5: LP4/LP3/LP2/LP1/HP2/BP2) to the best
        // SCXT processor type rather than always CytomicSVF.
        filterArgs.type = dsp::processor::ProcessorType::proct_CytomicSVF;
        int rawCutoff = params->get(EXSParam::FILTER1_CUTOFF, 1000);
        int rawReso = params->get(EXSParam::FILTER1_RESO, 0);
        // Logic Sampler's filter cutoff is stored as 0..1000 (UI shows it as
        // a 0..100% percentage with no Hz reading). The raw value maps
        // logarithmically to Hz across the audio range: 0% → 20 Hz,
        // 100% → 20 kHz. The Java ConvertWithMoss reference uses a linear
        // 0..20kHz map which is wrong — they have the same bug.
        // Formula: hz = 20 * 1000^(rawCutoff/1000).
        float pct = std::clamp(rawCutoff / 1000.f, 0.f, 1.f);
        float hz = 20.f * std::pow(1000.f, pct);
        float cutoffSemis = 12.f * std::log2(hz / 440.f);
        filterArgs.cutoff = std::clamp(cutoffSemis, -69.f, 70.f);
        filterArgs.resonance = std::clamp(rawReso / 1000.f, 0.f, 1.f);
    }

    // Times go through exsEnvByteToSeconds (exponential curve, calibrated
    // against Logic Sampler — not the linear /127*10 the Java reference
    // uses). Sustain is a 0..127 byte → 0..1 level. Only write the field
    // if the file actually specified it (matches Java's null-default
    // behavior — important for sustain so an omitted param means "full
    // sustain" 1.0 not "silent" 0).
    auto envFromParams = [&](EXSParam delay, EXSParam attack, EXSParam hold, EXSParam decay,
                             EXSParam sustain, EXSParam release) -> import_support::EnvelopeArgs {
        import_support::EnvelopeArgs args;
        if (!haveParams)
            return args;
        if (params->has(delay))
            args.delaySeconds = exsEnvByteToSeconds(params->get(delay));
        if (params->has(attack))
            args.attackSeconds = exsEnvByteToSeconds(params->get(attack));
        if (params->has(hold))
            args.holdSeconds = exsEnvByteToSeconds(params->get(hold));
        if (params->has(decay))
            args.decaySeconds = exsEnvByteToSeconds(params->get(decay));
        if (params->has(sustain))
            args.sustainLevel = params->get(sustain) / 127.f;
        if (params->has(release))
            args.releaseSeconds = exsEnvByteToSeconds(params->get(release));
        return args;
    };
    auto env1Args =
        envFromParams(EXSParam::ENV1_DELAY_START, EXSParam::ENV1_ATK_HI_VEL, EXSParam::ENV1_HOLD,
                      EXSParam::ENV1_DECAY, EXSParam::ENV1_SUSTAIN, EXSParam::ENV1_RELEASE);
    auto env2Args =
        envFromParams(EXSParam::ENV2_DELAY_START, EXSParam::ENV2_ATK_HI_VEL, EXSParam::ENV2_HOLD,
                      EXSParam::ENV2_DECAY, EXSParam::ENV2_SUSTAIN, EXSParam::ENV2_RELEASE);

    bool warnedAboutRoundRobin = false;
    int zoneOrdinal = 0;
    for (auto &z : info.zones)
    {
        auto exsgi = z.groupIndex;
        auto exssi = z.sampleIndex;

        // EXS files sometimes leave sampleIndex == -1 expecting the loader to
        // fall back to the zone ordinal as the sample index (matches the
        // ConvertWithMoss reference implementation).
        if (exssi < 0)
            exssi = zoneOrdinal;
        ++zoneOrdinal;

        if (exsgi < 0 || exsgi >= (int)groupIndexByOrder.size())
        {
            SCLOG_IF(sampleCompoundParsers, "Out-of-bounds group index : " << SCD(exsgi));
            continue;
        }
        if (exssi < 0 || exssi >= (int)sampleIDByOrder.size())
        {
            SCLOG_IF(sampleCompoundParsers,
                     "Out-of-bounds sample index " << SCD(exsgi) << SCD(exssi));
            continue;
        }
        auto gi = groupIndexByOrder[exsgi];
        auto si = sampleIDByOrder[exssi];
        const auto &exsGroup = info.groups[exsgi];

        // EXS round-robin: groups flagged with enableByRoundRobin form a
        // sequence keyed by roundRobinGroupPos (-1, 0, 1, …). Each "position"
        // is a separate EXS group covering the same key/vel range; played
        // back in order on repeated note-ons. SCXT does this via Zone
        // variants — one zone with N variants — which would require gathering
        // multiple EXS zones into a single SCXT zone instead of one-per-EXS.
        // Not implemented yet; import the zone as a regular non-RR zone for
        // now so it still plays.
        if (exsGroup.enableByRoundRobin && !warnedAboutRoundRobin)
        {
            ctx.unsupported("EXS round-robin groups",
                            "imported as parallel zones; per-variant RR not yet supported");
            warnedAboutRoundRobin = true;
        }

        // velocityRangeOn=false means "use full velocity range"; otherwise
        // honor the per-zone velocityLow/High the EXS specifies.
        int velStart = z.velocityRangeOn ? z.velocityLow : 0;
        int velEnd = z.velocityRangeOn ? z.velocityHigh : 127;
        int keyStart = z.keyLow;
        int keyEnd = z.keyHigh;

        // Intersect with the group's vel/key range (per the ConvertWithMoss
        // reference). Skip the zone entirely if it falls outside the group's
        // range. The "!= 0" guards on the clamps preserve the reference
        // behavior of treating zero as "no bound applied" — see EXS24Detector
        // limitByGroupAttributes.
        if (velEnd < exsGroup.minVelocity || velStart > exsGroup.maxVelocity)
            continue;
        if (keyEnd < exsGroup.startNote || keyStart > exsGroup.endNote)
            continue;
        if (exsGroup.minVelocity != 0 && velStart < exsGroup.minVelocity)
            velStart = exsGroup.minVelocity;
        if (exsGroup.maxVelocity != 0 && velEnd > exsGroup.maxVelocity)
            velEnd = exsGroup.maxVelocity;
        if (exsGroup.startNote != 0 && keyStart < exsGroup.startNote)
            keyStart = exsGroup.startNote;
        if (exsGroup.endNote != 0 && keyEnd > exsGroup.endNote)
            keyEnd = exsGroup.endNote;

        auto zone = std::make_unique<engine::Zone>(si);
        zone->engine = &e;

        // Tuning: EXS only respects coarse/fine when the `pitch` flag is set
        // (mirrors the ConvertWithMoss reference). volumeAdjust is in dB.
        // Global COARSE_TUNE+FINE_TUNE adds on top.
        float perZoneTune = 0;
        if (z.pitch && (z.coarseTuning != 0 || z.fineTuning != 0))
            perZoneTune = z.coarseTuning + import_support::centsToSemitones((float)z.fineTuning);
        float totalTune = perZoneTune + globalTuneSemis;
        std::optional<float> pitchOffsetSemis;
        if (totalTune != 0)
            pitchOffsetSemis = totalTune;

        import_support::importZoneMapping(*zone, ctx,
                                          {
                                              .rootKey = z.key,
                                              .keyStart = keyStart,
                                              .keyEnd = keyEnd,
                                              .velStart = velStart,
                                              .velEnd = velEnd,
                                              .pitchOffsetSemitones = pitchOffsetSemis,
                                          });

        // EXS pan is -50..50 → -1..1.
        if (z.pan != 0)
            zone->outputInfo.pan = z.pan / 50.f;

        // volumeAdjust is in dB (range -96..+12).
        if (z.volumeAdjust != 0)
            zone->mapping.amplitude = (float)z.volumeAdjust;

        // Mask MAPPING and LOOP off attach — EXS supplies both itself. Leave
        // ENDPOINTS on so attach populates sample-length defaults; we override
        // below if the .exs explicitly set sampleStart/End.
        zone->attachToSample(*(e.getSampleManager()), 0, engine::Zone::ENDPOINTS);

        auto &variant = zone->variantData.variants[0];

        // Explicit sample bounds win over the attach-loaded defaults.
        if (z.sampleStart > 0)
            variant.startSample = z.sampleStart;
        if (z.sampleEnd > 0)
            variant.endSample = z.sampleEnd;

        variant.playReverse = z.reverse;
        if (exsGroup.releaseTrigger)
            variant.playMode = engine::Zone::PlayMode::ON_RELEASE;
        else if (z.oneshot)
            variant.playMode = engine::Zone::PlayMode::ONE_SHOT;

        if (z.loopOn)
        {
            // EXS loopCrossfade is in milliseconds. Convert via the loaded
            // sample's rate so it lands in samples for the engine.
            int64_t fadeSamples = 0;
            if (z.loopCrossfade > 0 && exssi < (int)info.samples.size())
            {
                const auto &smp = info.samples[exssi];
                fadeSamples = (int64_t)z.loopCrossfade * smp.sampleRate / 1000;
            }
            // EXS loopDirection: 0 = forward, non-zero treated as alternate
            // (spec doesn't enumerate other values; most loops are forward).
            auto direction = (z.loopDirection != 0)
                                 ? engine::Zone::LoopDirection::ALTERNATE_DIRECTIONS
                                 : engine::Zone::LoopDirection::FORWARD_ONLY;
            import_support::importZoneLoop(*zone, ctx, 0,
                                           {
                                               .mode = engine::Zone::LoopMode::LOOP_DURING_VOICE,
                                               .direction = direction,
                                               .startSamples = z.loopStart,
                                               .endSamples = z.loopEnd,
                                               .fadeSamples = fadeSamples,
                                               .active = true,
                                           });
        }

        // Global env1 (AmpEG) / env2 (FilEG) — every zone gets the same
        // global envelope shape.
        import_support::EGHandle egAmpHandle, egFilHandle;
        if (haveParams)
        {
            egAmpHandle = import_support::importZoneEnvelope(*zone, ctx, 0, env1Args);
            egFilHandle = import_support::importZoneEnvelope(*zone, ctx, 1, env2Args);
        }

        // Global filter + env2-to-cutoff mod route. Logic models env2 as the
        // filter's cutoff modulator with depth 1.0; SCXT routes it through
        // the mod matrix.
        if (filterEnabled)
        {
            auto filterHandle = import_support::importZoneFilter(*zone, ctx, 0, filterArgs);
            import_support::addImportedModRoute(
                *zone, ctx,
                {
                    .source = import_support::ImportedSource::fromEG(egFilHandle),
                    .target = import_support::ImportedTarget::filter(
                        filterHandle, import_support::FilterParam::Cutoff),
                    .depth = 1.0f,
                });
        }

        ctx.addZoneToGroup(gi, std::move(zone));
    }
    return ctx.finish();
}

void dumpEXSToLog(const fs::path &p)
{
    auto info = parseEXS(p);

    if (info.parameters.has_value())
    {
        SCLOG_IF(sampleCompoundParsers,
                 "Parameters block found with " << info.parameters->params.size() << " parameters");
    }
    else
    {
        SCLOG_IF(sampleCompoundParsers, "NO parameters block found");
    }
    for (auto &z : info.zones)
    {
        SCLOG_IF(sampleCompoundParsers, "Zone '" << z.within.name << "'");
        SCLOG_IF(sampleCompoundParsers,
                 "  KeyRange " << z.keyLow << "-" << z.key << "-" << z.keyHigh);
        SCLOG_IF(sampleCompoundParsers, "  VelRange " << z.velocityLow << "-" << z.velocityHigh);
        SCLOG_IF(sampleCompoundParsers, "  Group");
        SCLOG_IF(sampleCompoundParsers, "    gidx=" << z.groupIndex);
        const auto &grp = info.groups[z.groupIndex];
        SCLOG_IF(sampleCompoundParsers, "    gname='" << grp.within.name << "'");
        SCLOG_IF(sampleCompoundParsers, "  Sample");
        SCLOG_IF(sampleCompoundParsers, "    sidx=" << z.sampleIndex);
        const auto &smp = info.samples[z.sampleIndex];
        SCLOG_IF(sampleCompoundParsers, "    sname=" << smp.within.name);
        SCLOG_IF(sampleCompoundParsers, "    spath=" << smp.filePath);
        SCLOG_IF(sampleCompoundParsers, "    sname=" << smp.fileName);
    }
}
} // namespace scxt::exs_support