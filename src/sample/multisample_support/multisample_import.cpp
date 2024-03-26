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