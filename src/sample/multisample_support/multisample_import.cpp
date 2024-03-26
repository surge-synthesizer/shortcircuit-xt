//
// Created by Paul Walker on 3/25/24.
//

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
            /*
             * <sample file="60 Clavinet E5 05.wav" gain="-0.96" group="4" parameter-1="0.0000"
parameter-2="0.0000" parameter-3="0.0000" reverse="false" sample-start="0.000"
sample-stop="317919.000" zone-logic="always-play"> <key high="96" low="88" root="88" track="1.0000"
tune="0.00"/> <velocity low="125"/> <select/> <loop fade="0.0000" mode="off" start="0.000"
stop="317919.000"/>
</sample>
             */
            if (!fc->Attribute("file"))
            {
                SCLOG("Sample is not a file");
                return false;
            }

            size_t ssize{0};
            auto data = mz_zip_reader_extract_to_heap(
                &zip_archive, fileToIndex[fc->Attribute("file")], &ssize, 0);

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

            auto group{0};
            fc->QueryIntAttribute("group", &group);

            if (group < 0 || group > addedGroupIndices.size())
            {
                SCLOG("Bad group : " << group);
                return false;
            }

            auto &g = part->getGroup(addedGroupIndices[group]);
            auto z = std::make_unique<engine::Zone>(*lsid);
            z->mapping.rootKey = kr;
            z->mapping.keyboardRange.keyStart = ks;
            z->mapping.keyboardRange.keyEnd = ke;
            z->mapping.velocityRange.velStart = vs;
            z->mapping.velocityRange.velEnd = ve;
            z->attachToSample(*engine.getSampleManager());
            g->addZone(z);
        }
        else
        {
            // SCLOG("Unhandled multisample field " << eln);
        }
        fc = fc->NextSiblingElement();
    }

    mz_zip_reader_end(&zip_archive);
    return true;
}
} // namespace scxt::multisample_support