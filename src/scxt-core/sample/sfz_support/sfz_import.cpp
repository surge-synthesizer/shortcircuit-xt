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
#include "selection/selection_manager.h"
#include "sfz_parse.h"
#include "messaging/messaging.h"
#include "engine/engine.h"
#include "configuration.h"
#include <cctype>
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
        // Tune is supplied in cents. Our pitch offset is in semitones
        mp.pitchOffset = std::atof(v->c_str()) * 0.01;
    }

    if (auto v = consumeOpcode(opCodes, "transpose"); v.has_value())
    {
        mp.pitchOffset = std::atoi(v->c_str());
    }
}

void zoneLoop(std::unique_ptr<engine::Zone> &zn, opCodeMap_t &opCodes, int &loadInfo,
              int64_t &loop_start, int64_t &loop_end)
{
    if (auto v = consumeOpcode(opCodes, "loop_start"); v.has_value())
    {
        loop_start = std::atol(v->c_str());
        loadInfo = loadInfo & ~engine::Zone::LOOP;
    }

    if (auto v = consumeOpcode(opCodes, "loop_end"); v.has_value())
    {
        loop_end = std::atol(v->c_str());
        loadInfo = loadInfo & ~engine::Zone::LOOP;
    }

    if (auto v = consumeOpcode(opCodes, "loop_mode"); v.has_value())
    {
        if (*v == "loop_continuous")
        {
            // FIXME: In round robin looping modes this is probably wrong
            zn->variantData.variants[0].loopActive = true;
            zn->variantData.variants[0].loopMode = engine::Zone::LOOP_DURING_VOICE;
        }
        else
        {
            SCLOG_IF(sampleCompoundParsers, "SFZ Unsupported loop_mode : " << *v);
        }
    }
}

void zoneEnvelope(std::unique_ptr<engine::Zone> &zn, opCodeMap_t &opCodes, int slot,
                  const std::string &prefix)
{
    auto &eg = zn->egStorage[slot];

    if (auto v = consumeOpcode(opCodes, prefix + "_sustain"); v.has_value())
        eg.s = std::atof(v->c_str()) * 0.01;

    if (auto v = consumeOpcode(opCodes, prefix + "_delay"); v.has_value())
        eg.dly = scxt::modulation::secondsToNormalizedEnvTime(std::atof(v->c_str()));

    if (auto v = consumeOpcode(opCodes, prefix + "_attack"); v.has_value())
        eg.a = scxt::modulation::secondsToNormalizedEnvTime(std::atof(v->c_str()));

    if (auto v = consumeOpcode(opCodes, prefix + "_decay"); v.has_value())
    {
        eg.d = scxt::modulation::secondsToNormalizedEnvTime(std::atof(v->c_str()));
        eg.dShape = -1;
    }

    if (auto v = consumeOpcode(opCodes, prefix + "_hold"); v.has_value())
        eg.h = scxt::modulation::secondsToNormalizedEnvTime(std::atof(v->c_str()));

    if (auto v = consumeOpcode(opCodes, prefix + "_release"); v.has_value())
    {
        eg.r = scxt::modulation::secondsToNormalizedEnvTime(std::atof(v->c_str()));
        eg.rShape = -1;
    }
}

bool importSFZ(const fs::path &f, engine::Engine &e)
{
    int octaveOffset{0};
    auto &messageController = e.getMessageController();
    assert(messageController->threadingChecker.isSerialThread());

    auto cng = messaging::MessageController::ClientActivityNotificationGuard(
        "Loading SFZ '" + f.filename().u8string() + "'", *(messageController));

    SFZParser parser;
    parser.onError = [&e](const auto &s) { RAISE_ERROR_ENGINE(e, "SFZ Import Error", s); };

    auto doc = parser.parse(f);
    auto rootDir = f.parent_path();
    auto sampleDir = rootDir;

    auto pt = std::clamp(e.getSelectionManager()->selectedPart, (int16_t)0, (int16_t)numParts);

    auto &part = e.getPatch()->getPart(pt);

    int groupId = -1;
    int firstGroupWithZonesAdded = -1;
    int regionCount{0};
    SFZParser::opCodes_t currentGroupOpcodes;
    std::set<std::string> unusedZoneOpcodes;
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
                    // SCLOG_IF(sampleCompoundParsers,"     Skipped OpCode <group>: " << oc.name <<
                    // " -> " << oc.value);
                }
            }
        }
        break;
        case SFZParser::Header::region:
        {
            regionCount++;
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

            if (!scxt::isValidUtf(sampleFileString))
            {
                SCLOG_IF(warnings, "Original file name is `" << sampleFileString << "`");
                RAISE_ERROR_ENGINE(e, "SFZ Import Error",
                                   "Sample filename in region " + std::to_string(regionCount) +
                                       " contains invalid UTF-8 sequence. "
                                       "Stopping SFZ import");
                return false;
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
                    SCLOG_IF(sampleCompoundParsers,
                             "Cannot load Sample : " << samplePath.u8string());
                    RAISE_ERROR_ENGINE(e, "SFZ Import Error",
                                       "Cannot load sample '" + samplePath.u8string() + "'");
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
                    SCLOG_IF(sampleCompoundParsers, "Cannot load Sample : " << sampleFile);
                    RAISE_ERROR_ENGINE(e, "SFZ Import Error",
                                       "Cannot load sample '" + sampleFile.u8string() + "'");
                    return false;
                }
            }
            else
            {
                SCLOG_IF(sampleCompoundParsers, "Unable to load either '"
                                                    << samplePath.u8string() << "' or '"
                                                    << sampleFile.u8string() << "'");
                RAISE_ERROR_ENGINE(e, "SFZ Import Error",
                                   "Unable to load either '" + samplePath.u8string() + "' or '" +
                                       sampleFile.u8string() + "'");
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

            opCodeMap_t mergedOpcodes;
            for (const auto &oc : currentGroupOpcodes)
                mergedOpcodes[oc.name] = {oc.value, false};
            for (const auto &oc : list)
                mergedOpcodes[oc.name] = {oc.value, false};

            // Already used
            consumeOpcode(mergedOpcodes, "sample");
            consumeOpcode(mergedOpcodes, "seq_position");

            auto onError = [&e](const std::string &title, const std::string &msg) {
                RAISE_ERROR_ENGINE(e, title, msg);
            };
            zoneGeometry(zn, mergedOpcodes, loadInfo, onError, octaveOffset);
            zonePlayback(zn, mergedOpcodes);
            zoneLoop(zn, mergedOpcodes, loadInfo, loop_start, loop_end);
            zoneEnvelope(zn, mergedOpcodes, 0, "ampeg");
            zoneEnvelope(zn, mergedOpcodes, 1, "fileg");

            for (auto &[n, m] : mergedOpcodes)
            {
                if (m.second)
                    continue;
                unusedZoneOpcodes.insert(n);
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
                    RAISE_ERROR_ENGINE(e, "Mis-mapped SFZ Round Robin",
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
    if (!unusedZoneOpcodes.empty())
    {
        std::ostringstream oss;
        std::string pfx = "";
        for (auto &oc : unusedZoneOpcodes)
        {
            oss << pfx << oc;
            pfx = ", ";
        }
        SCLOG_IF(sampleCompoundParsers || scxt::log::debug, "Unused SFZ OpCodes : " << oss.str())
    }

    e.getSelectionManager()->applySelectActions(selection::SelectionManager::SelectActionContents(
        pt, firstGroupWithZonesAdded, 0, true, true, true));
    return true;
}
} // namespace scxt::sfz_support