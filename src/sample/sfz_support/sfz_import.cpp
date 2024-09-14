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

#include "sfz_import.h"
#include "selection/selection_manager.h"
#include "sfz_parse.h"
#include "messaging/messaging.h"
#include "engine/engine.h"
#include <cctype>

namespace scxt::sfz_support
{
int parseMidiNote(const std::string &s)
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
        auto res = nsv + diff + (oct + 1) * 12;

        return res;
    }
    return std::atol(s.c_str());
}

bool importSFZ(const fs::path &f, engine::Engine &e)
{
    assert(e.getMessageController()->threadingChecker.isSerialThread());

    SFZParser parser;

    auto doc = parser.parse(f);
    auto rootDir = f.parent_path();
    auto sampleDir = rootDir;

    auto pt = std::clamp(e.getSelectionManager()->selectedPart, (int16_t)0, (int16_t)numParts);

    auto &part = e.getPatch()->getPart(pt);

    int groupId = -1;
    int firstGroupWithZonesAdded = -1;
    SFZParser::opCodes_t currentGroupOpcodes;
    for (const auto &[r, list] : doc)
    {
        if (r.type != SFZParser::Header::region)
        {
            SCLOG("Header ----- <" << r.name << "> (" << list.size() << " opcodes) -------");
        }
        switch (r.type)
        {
        case SFZParser::Header::group:
        {
            groupId = part->addGroup() - 1;
            currentGroupOpcodes = list;
            auto &group = part->getGroup(groupId);
            for (auto &oc : list)
            {
                if (oc.name == "group_label" || oc.name == "name")
                {
                    group->name = oc.value;
                }
                else
                {
                    // We take a second shot at this below
                    // SCLOG("     Skipped OpCode <group>: " << oc.name << " -> " << oc.value);
                }
            }
        }
        break;
        case SFZParser::Header::region:
        {
            if (groupId < 0)
            {
                groupId = part->addGroup() - 1;
            }
            auto &group = part->getGroup(groupId);

            // Find the sample
            std::string sampleFileString = "<-->";
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
            // fs always works with / and on windows also works with back. Quotes are
            // stripped by the parser now
            std::replace(sampleFileString.begin(), sampleFileString.end(), '\\', '/');
            auto sampleFile = fs::path{sampleFileString};
            auto samplePath = (sampleDir / sampleFile).lexically_normal();

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
                    SCLOG("Cannot load Sample : " << samplePath.u8string());
                    break;
                }
            }
            else if (fs::exists(sampleFile))
            {
                auto lsid = e.getSampleManager()->loadSampleByPath(sampleFile);
                if (lsid.has_value())
                {
                    sid = *lsid;
                }
                else
                {
                    SCLOG("Cannot load Sample : " << sampleFile);
                    return false;
                }
            }
            else
            {
                SCLOG("Unable to load either '" << samplePath.u8string() << "' or '"
                                                << sampleFile.u8string() << "'");
                return false;
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
            auto zn = std::make_unique<engine::Zone>(sid);
            // SFZ defaults
            zn->mapping.rootKey = 60;
            zn->mapping.keyboardRange.keyStart = 0;
            zn->mapping.keyboardRange.keyEnd = 127;
            zn->mapping.velocityRange.velStart = 0;
            zn->mapping.velocityRange.velEnd = 127;

            auto loadInfo = engine::Zone::LOOP | engine::Zone::ENDPOINTS | engine::Zone::MAPPING;

            int64_t loop_start{-1}, loop_end{-1};

            for (const auto &[n, ls] :
                 {std::make_pair(std::string("Group"), currentGroupOpcodes), {"Region", list}})
            {
                for (auto &oc : ls)
                {
                    if (oc.name == "pitch_keycenter")
                    {
                        zn->mapping.rootKey = parseMidiNote(oc.value);
                        loadInfo = loadInfo & ~engine::Zone::MAPPING;
                    }
                    else if (oc.name == "lokey")
                    {
                        zn->mapping.keyboardRange.keyStart = parseMidiNote(oc.value);
                        loadInfo = loadInfo & ~engine::Zone::MAPPING;
                    }
                    else if (oc.name == "hikey")
                    {
                        zn->mapping.keyboardRange.keyEnd = parseMidiNote(oc.value);
                        loadInfo = loadInfo & ~engine::Zone::MAPPING;
                    }
                    else if (oc.name == "key")
                    {
                        auto pmn = parseMidiNote(oc.value);
                        zn->mapping.rootKey = pmn;
                        zn->mapping.keyboardRange.keyStart = pmn;
                        zn->mapping.keyboardRange.keyEnd = pmn;
                        loadInfo = loadInfo & ~engine::Zone::MAPPING;
                    }
                    else if (oc.name == "lovel")
                    {
                        zn->mapping.velocityRange.velStart = std::atol(oc.value.c_str());
                        loadInfo = loadInfo & ~engine::Zone::MAPPING;
                    }
                    else if (oc.name == "hivel")
                    {
                        zn->mapping.velocityRange.velEnd = std::atol(oc.value.c_str());
                        loadInfo = loadInfo & ~engine::Zone::MAPPING;
                    }
                    else if (oc.name == "loop_start")
                    {
                        loop_start = std::atol(oc.value.c_str());
                        loadInfo = loadInfo & ~engine::Zone::LOOP;
                    }
                    else if (oc.name == "loop_end")
                    {
                        loop_end = std::atol(oc.value.c_str());
                        loadInfo = loadInfo & ~engine::Zone::LOOP;
                    }
                    else if (oc.name == "sample" || oc.name == "seq_position")
                    {
                        // dealt with above
                    }
                    else if (oc.name == "ampeg_sustain")
                    {
                        zn->egStorage[0].s = std::atof(oc.value.c_str()) * 0.01;
                    }
                    else if (oc.name == "volume")
                    {
                        zn->mapping.amplitude = std::atof(oc.value.c_str()); // decibels
                    }
                    else if (oc.name == "tune")
                    {
                        // Tune is supplied in cents. Our pitch offset is in semitones
                        zn->mapping.pitchOffset = std::atof(oc.value.c_str()) * 0.01;
                    }
                    else if (oc.name == "loop_mode")
                    {
                        if (oc.value == "loop_continuous")
                        {
                            // FIXME: In round robin looping modes this is probably wrong
                            zn->variantData.variants[0].loopActive = true;
                            zn->variantData.variants[0].loopMode = engine::Zone::LOOP_DURING_VOICE;
                        }
                        else
                        {
                            SCLOG("SFZ Unsupported loop_mode : " << oc.value);
                        }
                    }

#define APPLYEG(v, d, t)                                                                           \
    else if (oc.name == v)                                                                         \
    {                                                                                              \
        zn->egStorage[d].t =                                                                       \
            scxt::modulation::secondsToNormalizedEnvTime(std::atof(oc.value.c_str()));             \
    }

                    APPLYEG("ampeg_attack", 0, a)
                    APPLYEG("ampeg_decay", 0, d)
                    APPLYEG("ampeg_release", 0, r)
                    else
                    {
                        if (n != "Group")
                            SCLOG("    Skipped " << n << "-originated OpCode for region: "
                                                 << oc.name << " -> " << oc.value);
                    }
                }
            }
            int variantIndex{-1};
            if (roundRobinPosition > 0)
            {
                bool attached{false};

                for (const auto &z : *group)
                {
                    if (z->mapping.rootKey == zn->mapping.rootKey &&
                        z->mapping.keyboardRange == zn->mapping.keyboardRange &&
                        z->mapping.velocityRange == zn->mapping.velocityRange)
                    {
                        int idx = roundRobinPosition - 1;
                        variantIndex = idx;
                        z->attachToSampleAtVariation(*e.getSampleManager(), sid, idx, loadInfo);
                        attached = true;
                        break;
                    }
                }
                if (!attached)
                {
                    e.getMessageController()->reportErrorToClient(
                        "Mis-mapped SFZ Round Robin",
                        std::string("Unable to locate zone for sample ") + sampleFile.u8string());
                }
            }
            else
            {
                variantIndex = 0;
                zn->attachToSample(*e.getSampleManager(), 0, loadInfo);
            }

            if (variantIndex >= 0)
            {
                if (loop_start >= 0 || loop_end >= 0)
                {
                    zn->variantData.variants[variantIndex].startLoop =
                        std::max((int64_t)0, loop_start);
                    zn->variantData.variants[variantIndex].endLoop =
                        std::max({(int64_t)0, loop_start + 1, loop_end});
                }
            }

            if (roundRobinPosition <= 0)
            {
                group->addZone(zn);
            }

            if (firstGroupWithZonesAdded == -1)
            {
                firstGroupWithZonesAdded = groupId;
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
                    SCLOG("Control: Resetting sample dir to " << sampleDir);
                    ;
                }
                else
                {
                    SCLOG("    Skipped OpCode <control>: " << oc.name << " -> " << oc.value);
                }
            }
        }
        break;
        default:
        {
            SCLOG("Ignoring SFZ Header " << r.name << " with " << list.size() << " keywords");
            for (const auto &oc : list)
            {
                SCLOG("  " << oc.name << " -> |" << oc.value << "|");
            }
        }
        break;
        }
    }

    e.getSelectionManager()->selectAction(selection::SelectionManager::SelectActionContents(
        pt, firstGroupWithZonesAdded, 0, true, true, true));
    return true;
}
} // namespace scxt::sfz_support