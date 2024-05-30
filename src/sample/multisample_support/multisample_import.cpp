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

#include <map>

#include "multisample_import.h"
#include "tinyxml/tinyxml.h"

#include <miniz.h>
#include "messaging/messaging.h"

namespace scxt::multisample_support
{

bool importMultisample(const fs::path &p, engine::Engine &engine)
{
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    auto status = mz_zip_reader_init_file(&zip_archive, p.u8string().c_str(), 0);

    if (!status)
        return false;

    // Step one: Build a zip file to index map
    std::map<std::string, int> fileToIndex;

    for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); i++)
    {
        mz_zip_archive_file_stat file_stat;
        mz_zip_reader_file_stat(&zip_archive, i, &file_stat);

        fileToIndex[file_stat.m_filename] = i;
    }

    SCLOG("Read multisample '" << p.filename().u8string() << "' with "
                               << (int)mz_zip_reader_get_num_files(&zip_archive) << " components");

    // Step two grab the multisample.xml
    if (fileToIndex.find("multisample.xml") == fileToIndex.end())
    {
        SCLOG("No Multisapmle.xml");
        return false;
    }

    size_t rsize{0};
    auto data =
        mz_zip_reader_extract_to_heap(&zip_archive, fileToIndex["multisample.xml"], &rsize, 0);
    std::string xml((const char *)data);
    free(data);

    auto doc = TiXmlDocument();
    if (!doc.Parse(xml.c_str()))
    {
        SCLOG("XML Parse Fail");
        mz_zip_reader_end(&zip_archive);

        return false;
    }

    auto pt = std::clamp(engine.getSelectionManager()->selectedPart, 0, (int)numParts);
    auto &part = engine.getPatch()->getPart(pt);

    std::vector<int> addedGroupIndices;

    auto rt = doc.RootElement();
    if (rt->ValueStr() != "multisample")
    {
        SCLOG("XML is not a multisample document");
        mz_zip_reader_end(&zip_archive);
        return false;
    }

    auto addSampleFromElement = [&p, &part, &engine, &zip_archive, &fileToIndex,
                                 &addedGroupIndices](TiXmlElement *fc, int32_t group_index = -1) {
        /*
         * <sample file="60 Clavinet E5 05.wav" gain="-0.96" group="4" parameter-1="0.0000"
    parameter-2="0.0000" parameter-3="0.0000" reverse="false" sample-start="0.000"
    sample-stop="317919.000" zone-logic="always-play"> <key high="96" low="88" root="88"
    track="1.0000" tune="0.00"/> <velocity low="125"/> <select/> <loop fade="0.0000" mode="off"
    start="0.000" stop="317919.000"/>
    </sample>
         */
        if (!fc->Attribute("file"))
        {
            SCLOG("Sample is not a file");
            return false;
        }

        size_t ssize{0};
        auto data = mz_zip_reader_extract_to_heap(&zip_archive, fileToIndex[fc->Attribute("file")],
                                                  &ssize, 0);

        auto lsid = engine.getSampleManager()->setupSampleFromMultifile(
            p, fileToIndex[fc->Attribute("file")], data, ssize);
        free(data);

        if (!lsid.has_value())
        {
            SCLOG("Wierd - no lsid value");
            return false;
        }

        auto kr{90}, ks{0}, ke{127}, vs{0}, ve{127};
        auto key = fc->FirstChildElement("key");
        auto vel = fc->FirstChildElement("velocity");
        if (key)
        {
            key->QueryIntAttribute("root", &kr);
            key->QueryIntAttribute("low", &ks);
            key->QueryIntAttribute("high", &ke);
        }
        if (vel)
        {
            vel->QueryIntAttribute("low", &vs);
            vel->QueryIntAttribute("high", &ve);
        }

        auto group_id = 0;
        if (group_index == -1)
        {
            auto group{0};
            fc->QueryIntAttribute("group", &group);

            if (group < 0 || group > addedGroupIndices.size())
            {
                SCLOG("Bad group : " << group);
                return false;
            }
            group_id = addedGroupIndices[group];
        }

        auto &g = part->getGroup(group_id);
        auto z = std::make_unique<engine::Zone>(*lsid);
        z->mapping.rootKey = kr;
        z->mapping.keyboardRange.keyStart = ks;
        z->mapping.keyboardRange.keyEnd = ke;
        z->mapping.velocityRange.velStart = vs;
        z->mapping.velocityRange.velEnd = ve;
        z->attachToSample(*engine.getSampleManager());
        g->addZone(z);

        return true;
    };

    auto fc = rt->FirstChildElement();
    std::string name{};
    if (rt->Attribute("name"))
    {
        name = rt->Attribute("name");
        name += " ";
    }
    while (fc)
    {
        auto eln = fc->ValueStr();
        if (eln == "group")
        {
            auto groupId = part->addGroup() - 1;
            auto &group = part->getGroup(groupId);

            if (fc->Attribute("name"))
            {
                group->name = name + fc->Attribute("name");
            }
            addedGroupIndices.push_back(groupId);
        }
        else if (eln == "sample")
        {
            if (!addSampleFromElement(fc))
            {
                return false;
            }
        }
        else if (eln == "layer")
        {
            // Sigh. Presonus does something a little different
            auto groupId = part->addGroup() - 1;
            auto &group = part->getGroup(groupId);

            if (fc->Attribute("name"))
            {
                group->name = name + fc->Attribute("name");
            }
            addedGroupIndices.push_back(groupId);

            auto smp = fc->FirstChildElement("sample");
            while (smp)
            {
                addSampleFromElement(smp, groupId);
                smp = smp->NextSiblingElement("sample");
            }
        }
        else
        {
            SCLOG("Ignored multisample field " << eln);
        }
        fc = fc->NextSiblingElement();
    }

    mz_zip_reader_end(&zip_archive);

    if (!addedGroupIndices.empty())
    {
        engine.getSelectionManager()->selectAction(
            selection::SelectionManager::SelectActionContents(pt, addedGroupIndices.front(), 0,
                                                              true, true, true));

        return true;
    }
    else
    {
        return false;
    }
}
} // namespace scxt::multisample_support