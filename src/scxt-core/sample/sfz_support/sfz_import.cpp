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
#include "sample/import_support/import_loop.h"
#include "sample/import_support/import_numeric.h"
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
                return ctx.fail("SFZ Import Error",
                                "Sample filename in region " + std::to_string(regionCount) +
                                    " contains invalid UTF-8 sequence. Stopping SFZ import");
            }

            // fs always works with / and on windows also works with back. Quotes are
            // stripped by the parser now
            std::replace(sampleFileString.begin(), sampleFileString.end(), '\\', '/');
            auto sampleFile = fs::path{sampleFileString};
            auto samplePath = (sampleDir / sampleFile).lexically_normal();

            auto lsid = ctx.loadSampleFromDisk({samplePath, sampleFile});
            if (!lsid)
            {
                // TODO: this should trigger a missing-sample resolution flow
                // (prompt the user to locate the file) rather than raising +
                // skipping the region. Same shape applies to a load failure on
                // a path that DID exist (e.g. a corrupt WAV). For now: raise a
                // user-visible error and skip this region; the rest of the SFZ
                // continues to import.
                ctx.raise("SFZ Import Error", "Unable to load sample '" + samplePath.u8string() +
                                                  "' or '" + sampleFile.u8string() + "'");
                break;
            }
            SampleID sid = *lsid;

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

            auto onError = [&ctx](const std::string &title, const std::string &msg) {
                ctx.raise(title, msg);
            };
            zoneGeometry(zn, mergedOpcodes, loadInfo, onError, octaveOffset);
            zonePlayback(zn, mergedOpcodes);
            auto loopArgs = zoneLoopParse(mergedOpcodes, loadInfo);
            zoneEnvelope(zn, ctx, mergedOpcodes, 0, "ampeg");
            zoneEnvelope(zn, ctx, mergedOpcodes, 1, "fileg");

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
    for (auto &oc : unusedZoneOpcodes)
        ctx.unsupported("SFZ opcode", oc);

    return ctx.finish();
}
} // namespace scxt::sfz_support