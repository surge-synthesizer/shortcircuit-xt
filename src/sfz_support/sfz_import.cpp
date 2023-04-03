//
// Created by Paul Walker on 4/2/23.
//

#include "sfz_import.h"
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

    auto sz = e.getSelectionManager()->getSelectedZone();
    auto pt = 0;
    if (sz.has_value())
        pt = sz->part;
    if (pt < 0 || pt >= engine::Patch::numParts)
        pt = 0;
    auto &part = e.getPatch()->getPart(pt);

    int groupId = -1;
    int firstGroupWithZonesAdded = -1;
    for (const auto &[r, list] : doc)
    {
        switch (r.type)
        {
        case SFZParser::Header::group:
        {
            groupId = part->addGroup() - 1;
            SCDBGCOUT << "Added group; ignoring " << list.size() << " keywords" << std::endl;
            for (auto &oc : list)
            {
                SCDBGCOUT << "    Skipped OpCode <group>: " << oc.name << " -> " << oc.value
                          << std::endl;
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
                else if (oc.name == "lowvel")
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
    e.getSelectionManager()->singleSelect(
        selection::SelectionManager::ZoneAddress(pt, firstGroupWithZonesAdded, 0));
    return true;
}
} // namespace scxt::sfz_support