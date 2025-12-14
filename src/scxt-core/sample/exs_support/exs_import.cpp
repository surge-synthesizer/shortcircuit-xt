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

#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include "utils.h"
#include "exs_import.h"

#include <messaging/messaging.h>

namespace scxt::exs_support
{
using byteData_t = std::vector<uint8_t>;

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
    // For vals see
    // https://github.com/git-moss/ConvertWithMoss/blob/main/src/main/java/de/mossgrabers/convertwithmoss/format/exs/EXS24Parameters.java
    std::map<uint16_t, int16_t> params;
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

        for (int i = 0; i < paramCount; i++)
        {
            int paramId = within.content[i] & 0xFF;
            if (paramId != 0)
            {
                auto offset = paramCount + 2 * i;
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

bool importEXS(const fs::path &p, engine::Engine &e)
{
    auto &messageController = e.getMessageController();
    assert(messageController->threadingChecker.isSerialThread());

    auto cng = messaging::MessageController::ClientActivityNotificationGuard(
        "Loading EXS '" + p.filename().u8string() + "'", *(messageController));

    auto info = parseEXS(p);
    auto pt = std::clamp(e.getSelectionManager()->selectedPart, (int16_t)0, (int16_t)numParts);
    auto &part = e.getPatch()->getPart(pt);

    std::vector<int> groupIndexByOrder;
    std::vector<SampleID> sampleIDByOrder;

    for (auto &s : info.samples)
    {
        auto p = fs::path{s.filePath} / s.fileName;
        auto lsid = e.getSampleManager()->loadSampleByPath(p);
        if (lsid.has_value())
        {
            sampleIDByOrder.push_back(*lsid);
        }
    }

    int gidx{0};
    for (auto &g : info.groups)
    {
        auto groupId = part->addGroup() - 1;
        auto &group = part->getGroup(groupId);
        group->name = g.within.name;
        groupIndexByOrder.push_back(groupId);
        ++gidx;
    }

    for (auto &z : info.zones)
    {
        auto exsgi = z.groupIndex;
        auto exssi = z.sampleIndex;

        if (exsgi < 0 || exsgi >= groupIndexByOrder.size())
        {
            SCLOG_IF(sampleCompoundParsers, "Out-of-bounds group index : " << SCD(exsgi));
            continue;
        }
        if (exssi < 0 || exssi >= sampleIDByOrder.size())
        {
            SCLOG_IF(sampleCompoundParsers,
                     "Out-of-bounds sample index " << SCD(exsgi) << SCD(exssi));
            continue;
        }
        auto gi = groupIndexByOrder[exsgi];
        auto si = sampleIDByOrder[exssi];
        auto &group = part->getGroup(gi);
        auto zone = std::make_unique<engine::Zone>(si);

        zone->mapping.rootKey = z.key;
        zone->mapping.keyboardRange.keyStart = z.keyLow;
        zone->mapping.keyboardRange.keyEnd = z.keyHigh;
        zone->mapping.velocityRange.velStart = z.velocityLow;
        zone->mapping.velocityRange.velEnd = z.velocityHigh;

        zone->attachToSample(*(e.getSampleManager()));
        group->addZone(std::move(zone));
    }

    return true;
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