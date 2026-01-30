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

#include <map>

#include "multisample_import.h"
#include "tinyxml/tinyxml.h"

#include <miniz.h>
#include "messaging/messaging.h"
#include "infrastructure/md5support.h"

namespace scxt::multisample_support
{

bool importMultisample(const fs::path &p, engine::Engine &engine)
{
    auto &messageController = engine.getMessageController();
    assert(messageController->threadingChecker.isSerialThread());

    auto cng = messaging::MessageController::ClientActivityNotificationGuard(
        "Loading Multisample '" + p.filename().u8string() + "'", *(messageController));

    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    auto status = mz_zip_reader_init_file(&zip_archive, p.u8string().c_str(), 0);

    if (!status)
        return false;

    auto md5 = infrastructure::createMD5SumFromFile(p);

    // Step one: Build a zip file to index map
    std::map<std::string, int> fileToIndex;

    for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); i++)
    {
        mz_zip_archive_file_stat file_stat;
        mz_zip_reader_file_stat(&zip_archive, i, &file_stat);

        fileToIndex[file_stat.m_filename] = i;
    }

    SCLOG_IF(sampleCompoundParsers, "Read multisample '"
                                        << p.filename().u8string() << "' with "
                                        << (int)mz_zip_reader_get_num_files(&zip_archive)
                                        << " components");

    // Step two grab the multisample.xml
    if (fileToIndex.find("multisample.xml") == fileToIndex.end())
    {
        SCLOG_IF(sampleCompoundParsers, "No Multisapmle.xml");
        engine.getMessageController()->reportErrorToClient("Multisample Error",
                                                           "Mo XML file in Multisample");
        return false;
    }

    size_t rsize{0};
    auto data =
        mz_zip_reader_extract_to_heap(&zip_archive, fileToIndex["multisample.xml"], &rsize, 0);
    std::string xml((const char *)data, rsize);
    free(data);

    // SCLOG_IF(sampleCompoundParsers,xml);
    auto doc = TiXmlDocument();
    if (!doc.Parse(xml.c_str()))
    {
        SCLOG_IF(sampleCompoundParsers, "XML Parse Fail");
        mz_zip_reader_end(&zip_archive);

        engine.getMessageController()->reportErrorToClient("Multisample Error",
                                                           "XML Failed to parse");
        return false;
    }

    auto pt = std::clamp(engine.getSelectionManager()->selectedPart, (int16_t)0, (int16_t)numParts);
    auto &part = engine.getPatch()->getPart(pt);

    std::vector<int> addedGroupIndices;

    auto rt = doc.RootElement();
    if (rt->ValueStr() != "multisample")
    {
        SCLOG_IF(sampleCompoundParsers, "XML is not a multisample document");
        mz_zip_reader_end(&zip_archive);
        engine.getMessageController()->reportErrorToClient("Multisample Error", "XML invalid");
        return false;
    }

    auto addSampleFromElement = [&p, &part, &engine, &zip_archive, &fileToIndex, &addedGroupIndices,
                                 &md5](TiXmlElement *fc, int32_t group_index = -1) {
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
            SCLOG_IF(sampleCompoundParsers, "Sample is not a file");
            engine.getMessageController()->reportErrorToClient("Multisample Error",
                                                               "Sample is not a file");
            return false;
        }

        size_t ssize{0};
        auto data = mz_zip_reader_extract_to_heap(&zip_archive, fileToIndex[fc->Attribute("file")],
                                                  &ssize, 0);

        auto lsid = engine.getSampleManager()->setupSampleFromMultifile(
            p, md5, fileToIndex[fc->Attribute("file")], data, ssize);
        free(data);

        if (!lsid.has_value())
        {
            SCLOG_IF(sampleCompoundParsers, "Wierd - no lsid value");
            return false;
        }

        engine.getSampleManager()->getSample(*lsid)->md5Sum = md5;

        auto kr{90}, ks{0}, ke{127}, vs{0}, ve{127};
        float ktrack{1.0}, ktune{0.0};
        auto klf{0}, khf{0}, vlf{0}, vhf{0};
        auto key = fc->FirstChildElement("key");
        auto vel = fc->FirstChildElement("velocity");
        auto loop = fc->FirstChildElement("loop");
        bool loopOn{false};
        float loopStart{0}, loopEnd{0};
        if (key)
        {
            key->QueryIntAttribute("root", &kr);
            key->QueryIntAttribute("low", &ks);
            key->QueryIntAttribute("high", &ke);
            key->QueryIntAttribute("low-fade", &klf);
            key->QueryIntAttribute("high-fade", &khf);
            key->QueryFloatAttribute("track", &ktrack);
            key->QueryFloatAttribute("tune", &ktune);
        }
        if (vel)
        {
            vel->QueryIntAttribute("low", &vs);
            vel->QueryIntAttribute("high", &ve);
            vel->QueryIntAttribute("low-fade", &vlf);
            vel->QueryIntAttribute("high-fade", &vhf);
        }
        if (loop)
        {
            auto md = loop->Attribute("mode");
            if (md)
            {
                auto smd = std::string(md);
                if (smd == "loop")
                {
                    loopOn = true;
                    loop->QueryFloatAttribute("start", &loopStart);
                    loop->QueryFloatAttribute("stop", &loopEnd);
                }
                else if (smd == "off")
                {
                }
                else
                {
                    SCLOG_IF(sampleCompoundParsers, "Ignoring loop mode " << md);
                }
            }
        }

        auto group_id = 0;
        if (group_index == -1)
        {
            auto group{0};
            fc->QueryIntAttribute("group", &group);

            if (group < 0 || group > addedGroupIndices.size())
            {
                SCLOG_IF(sampleCompoundParsers, "Bad group : " << group);
                return false;
            }
            group_id = addedGroupIndices[group];
        }

        auto &g = part->getGroup(group_id);
        auto z = std::make_unique<engine::Zone>(*lsid);
        z->givenName = fc->Attribute("file");
        z->mapping.rootKey = kr;
        z->mapping.keyboardRange.keyStart = ks;
        z->mapping.keyboardRange.keyEnd = ke;
        z->mapping.keyboardRange.fadeStart = klf;
        z->mapping.keyboardRange.fadeEnd = khf;
        z->mapping.velocityRange.velStart = vs;
        z->mapping.velocityRange.velEnd = ve;
        z->mapping.velocityRange.fadeStart = vlf;
        z->mapping.velocityRange.fadeEnd = vhf;
        z->mapping.tracking = ktrack;
        z->mapping.pitchOffset = ktune;

        z->attachToSample(*engine.getSampleManager());
        if (loopOn && loopStart + loopEnd > 0)
        {
            z->variantData.variants[0].loopActive = true;
            z->variantData.variants[0].loopMode = engine::Zone::LoopMode::LOOP_WHILE_GATED;
            z->variantData.variants[0].startLoop = loopStart;
            z->variantData.variants[0].endLoop = loopEnd;
        }
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
            SCLOG_IF(sampleCompoundParsers, "Ignored multisample field " << eln);
        }
        fc = fc->NextSiblingElement();
    }

    mz_zip_reader_end(&zip_archive);

    if (!addedGroupIndices.empty())
    {
        engine.getSelectionManager()->applySelectActions(
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