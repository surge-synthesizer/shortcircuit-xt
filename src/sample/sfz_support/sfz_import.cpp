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
    SCLOG(SCD(rootDir.u8string()));

    auto pt = std::clamp(e.getSelectionManager()->selectedPart, 0, (int)numParts);

    auto &part = e.getPatch()->getPart(pt);

    int groupId = -1;
    int firstGroupWithZonesAdded = -1;
    SFZParser::opCodes_t groupList;
    for (const auto &[r, list] : doc)
    {
        std::unordered_set<std::string> usedOpcodes;
        auto testAndGetFrom = [&usedOpcodes](const auto &key, auto &onto, const auto &l) {
            auto it = l.find(key);
            if (it != l.end())
            {
                usedOpcodes.insert(key);
                onto = it->second;
                return true;
            }
            return false;
        };
        auto testAndGet = [testAndGetFrom, &l = list](const auto &key, auto &onto) {
            return testAndGetFrom(key, onto, l);
        };

        auto testAndGetMidiNote = [testAndGet](const auto &key, auto defNote) {
            std::string val;
            if (testAndGet(key, val))
                return parseMidiNote(val);

            return defNote;
        };

        auto testAndGetXForm = [testAndGet](const auto &key, double defVal,
                                            std::function<double(double)> f) {
            std::string val;
            if (testAndGet(key, val))
            {
                return f(std::atoi(val.c_str()));
            }

            return defVal;
        };

        if (r.type != SFZParser::Header::region)
        {
            SCLOG("Header ----- <" << r.name << "> (" << list.size() << " opcodes) -------");
        }
        switch (r.type)
        {
        case SFZParser::Header::group:
        {
            groupList = list;
            groupId = part->addGroup() - 1;
            auto &group = part->getGroup(groupId);
            testAndGet("group_label", group->name) || testAndGet("name", group->name);
            // special case - we query for group sample below
            usedOpcodes.insert("sample");
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
            std::string sampleFileString = "";
            if (!testAndGet("sample", sampleFileString) &&
                !testAndGetFrom("sample", sampleFileString, groupList))
            {
                SCLOG("Unable to find sample in region or parent group");
                return false;
            }

            // fs always works with / and on windows also works with back
            std::replace(sampleFileString.begin(), sampleFileString.end(), '\\', '/');
            // Is it contained in quotes
            if (sampleFileString.back() == '"')
            {
                sampleFileString = sampleFileString.substr(0, sampleFileString.size() - 1);
            }
            if (sampleFileString[0] == '"')
            {
                sampleFileString = sampleFileString.substr(1);
            }
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
            std::string val;
            if (testAndGet("seq_position", val))
            {
                auto pos = std::atoi(val.c_str());
                if (pos > 1)
                {
                    roundRobinPosition = pos;
                };
            }

            if (roundRobinPosition > 0)
            {
                int16_t rk{0}, ks{0}, ke{0}, vs{0}, ve{0};
                for (auto &oc : list)
                {
                    rk = testAndGetMidiNote("pitch_keycenter", 0);
                    ks = testAndGetMidiNote("lokey", 0);
                    ke = testAndGetMidiNote("hikey", 0);
                    vs = testAndGetMidiNote("lovel", 0);
                    ve = testAndGetMidiNote("hivel", 0);
                }

                bool attached{false};

                for (const auto &z : *group)
                {
                    if (z->mapping.rootKey == rk && z->mapping.keyboardRange.keyStart == ks &&
                        z->mapping.keyboardRange.keyEnd == ke &&
                        z->mapping.velocityRange.velStart == vs &&
                        z->mapping.velocityRange.velEnd == ve)
                    {
                        int idx = roundRobinPosition - 1;
                        z->attachToSampleAtVariation(*e.getSampleManager(), sid, idx);
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
                auto zn = std::make_unique<engine::Zone>(sid);

                std::string key;

                zn->mapping.rootKey = testAndGetMidiNote("pitch_keycenter", 69);
                zn->mapping.keyboardRange.keyStart = testAndGetMidiNote("lokey", 0);
                zn->mapping.keyboardRange.keyEnd = testAndGetMidiNote("hikey", 127);
                zn->mapping.velocityRange.velStart = testAndGetMidiNote("lovel", 0);
                zn->mapping.velocityRange.velEnd = testAndGetMidiNote("hivel", 127);

                if (testAndGet("key", key))
                {
                    auto pm = parseMidiNote(key);
                    zn->mapping.rootKey = pm;
                    zn->mapping.keyboardRange.keyStart = pm;
                    zn->mapping.keyboardRange.keyEnd = pm;
                }

                auto etf = [](double d) { return scxt::modulation::secondsToNormalizedEnvTime(d); };
                zn->egStorage[0].a = testAndGetXForm("ampeg_attack", 0, etf);
                zn->egStorage[0].h = testAndGetXForm("ampeg_hold", 0, etf);
                zn->egStorage[0].h = testAndGetXForm("ampeg_decay", 0, etf);
                zn->egStorage[0].h =
                    testAndGetXForm("ampeg_sustain", 1, [](auto a) { return a * 0.01; });
                zn->egStorage[0].h = testAndGetXForm("ampeg_release", etf(0.01), etf);
                zn->egStorage[0].d = 0;
                zn->egStorage[0].s = 1;
                zn->egStorage[0].r = 0;

                zn->attachToSample(*e.getSampleManager());
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
            std::string vv;
            if (testAndGet("default_path", vv))
            {
                std::replace(vv.begin(), vv.end(), '\\', '/');
                sampleDir = rootDir / vv;
                SCLOG("Control: Resetting sample dir to " << sampleDir);
            }
        }
        break;
        default:
        {
            SCLOG("Ignoring SFZ Header " << r.name << " with " << list.size() << " keywords");
            for (const auto &oc : list)
            {
                SCLOG("  " << oc.first << " -> |" << oc.second << "|");
            }
        }
        break;
        }

        for (auto [k, v] : list)
        {
            if (usedOpcodes.find(k) == usedOpcodes.end())
            {
                SCLOG("Unused Opcode in <" << r.name << "> : '" << k << "' -> " << v);
            }
        }
    }

    e.getSelectionManager()->selectAction(selection::SelectionManager::SelectActionContents(
        pt, firstGroupWithZonesAdded, 0, true, true, true));
    return true;
}
} // namespace scxt::sfz_support