/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

namespace scxt::sfz_support
{
int parseMidiNote(const std::string &s)
{
    if ((s[0] >= 'a' && s[0] <= 'g') || (s[0] >= 'A' && s[0] <= 'G'))
    {
        return 60;
    }
    return std::atol(s.c_str());
}

bool importSFZ(const std::filesystem::path &f, engine::Engine &e)
{
    assert(e.getMessageController()->threadingChecker.isSerialThread());

    SFZParser parser;

    auto doc = parser.parse(f);
    auto rootDir = f.parent_path();
    auto sampleDir = rootDir;
    SCDBGCOUT << SCD(rootDir.u8string()) << std::endl;

    auto pt = std::clamp(e.getSelectionManager()->selectedPart, 0, (int)numParts);

    auto &part = e.getPatch()->getPart(pt);

    int groupId = -1;
    int firstGroupWithZonesAdded = -1;
    for (const auto &[r, list] : doc)
    {
        if (r.type != SFZParser::Header::region)
        {
            SCDBGCOUT << "Header ----- <" << r.name << "> (" << list.size() << " opcodes) -------"
                      << std::endl;
        }
        switch (r.type)
        {
        case SFZParser::Header::group:
        {
            groupId = part->addGroup() - 1;
            auto &group = part->getGroup(groupId);
            for (auto &oc : list)
            {
                if (oc.name == "group_label" || oc.name == "name")
                {
                    group->name = oc.value;
                }
                else
                {
                    SCDBGCOUT << "     Skipped OpCode <group>: " << oc.name << " -> " << oc.value
                              << std::endl;
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
            std::string sampleFile = "<-->";
            for (auto &oc : list)
            {
                if (oc.name == "sample")
                {
                    sampleFile = oc.value;
                }
            }
            // std::filesystem always works with / and on windows also works with back
            std::replace(sampleFile.begin(), sampleFile.end(), '\\', '/');
            if (!fs::exists(sampleDir / sampleFile) && !fs::exists(sampleFile))
            {
                SCDBGCOUT << "Cannot find SampleFile [" << (sampleDir / sampleFile).u8string()
                          << "]" << std::endl;
                return false;
            }

            SampleID sid;
            if (fs::exists(sampleDir / sampleFile))
            {
                auto lsid = e.getSampleManager()->loadSampleByPath(sampleDir / sampleFile);
                if (lsid.has_value())
                {
                    sid = *lsid;
                }
                else
                {
                    SCDBGCOUT << "Cannot load Sample : " << (sampleDir / sampleFile).u8string()
                              << std::endl;
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
                    SCDBGCOUT << "Cannot load Sample : " << sampleFile << std::endl;
                    return false;
                }
            }

            auto zn = std::make_unique<engine::Zone>(sid);

            for (auto &oc : list)
            {
                if (oc.name == "pitch_keycenter")
                {
                    zn->mapping.rootKey = parseMidiNote(oc.value);
                }
                else if (oc.name == "lokey")
                {
                    zn->mapping.keyboardRange.keyStart = parseMidiNote(oc.value);
                }
                else if (oc.name == "hikey")
                {
                    zn->mapping.keyboardRange.keyEnd = parseMidiNote(oc.value);
                }
                else if (oc.name == "lovel")
                {
                    zn->mapping.velocityRange.velStart = std::atol(oc.value.c_str());
                }
                else if (oc.name == "hivel")
                {
                    zn->mapping.velocityRange.velEnd = std::atol(oc.value.c_str());
                }
                else if (oc.name == "sample")
                {
                    // dealt with above
                }
                else
                {
                    SCDBGCOUT << "    Skipped OpCode <region>: " << oc.name << " -> " << oc.value
                              << std::endl;
                }
            }
            zn->attachToSample(*e.getSampleManager());
            group->addZone(zn);
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
                    SCDBGCOUT << "Control: Resetting sample dir to " << sampleDir << std::endl;
                }
                else
                {
                    SCDBGCOUT << "    Skipped OpCode <control>: " << oc.name << " -> " << oc.value
                              << std::endl;
                }
            }
        }
        break;
        default:
        {
            SCDBGCOUT << "Ignoring SFZ Header " << r.name << " with " << list.size() << " keywords"
                      << std::endl;
            for (const auto &oc : list)
            {
                SCDBGCOUT << "  " << oc.name << " -> |" << oc.value << "|" << std::endl;
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