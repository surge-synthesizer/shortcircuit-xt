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
#include "sample/import_support/import_harness.h"
#include "sample/import_support/import_mapping.h"
#include "sample/import_support/import_loop.h"
#include "tinyxml/tinyxml.h"

#include <miniz.h>
#include "messaging/messaging.h"
#include "infrastructure/md5support.h"

namespace scxt::multisample_support
{

bool importMultisample(const fs::path &p, engine::Engine &engine)
{
    import_support::ImporterContext ctx(engine,
                                        "Loading Multisample '" + p.filename().u8string() + "'");

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
        return ctx.fail("Multisample Error", "No XML file in Multisample");

    size_t rsize{0};
    auto data =
        mz_zip_reader_extract_to_heap(&zip_archive, fileToIndex["multisample.xml"], &rsize, 0);
    std::string xml((const char *)data, rsize);
    free(data);

    auto doc = TiXmlDocument();
    if (!doc.Parse(xml.c_str()))
    {
        mz_zip_reader_end(&zip_archive);
        return ctx.fail("Multisample Error", "XML Failed to parse");
    }

    auto rt = doc.RootElement();
    if (rt->ValueStr() != "multisample")
    {
        mz_zip_reader_end(&zip_archive);
        return ctx.fail("Multisample Error", "XML invalid");
    }

    std::vector<int> addedGroupIndices;

    auto addSampleFromElement = [&p, &ctx, &engine, &zip_archive, &fileToIndex, &addedGroupIndices,
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
            return ctx.fail("Multisample Error", "Sample is not a file");

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

        auto group_id = group_index;
        if (group_index == -1)
        {
            auto group{0};
            fc->QueryIntAttribute("group", &group);

            if (group < 0 || group > (int)addedGroupIndices.size())
            {
                SCLOG_IF(sampleCompoundParsers, "Bad group : " << group);
                return false;
            }
            group_id = addedGroupIndices[group];
        }

        auto z = std::make_unique<engine::Zone>(*lsid);
        z->givenName = fc->Attribute("file");
        import_support::importZoneMapping(*z, ctx,
                                          {
                                              .rootKey = kr,
                                              .keyStart = ks,
                                              .keyEnd = ke,
                                              .keyFadeStart = klf,
                                              .keyFadeEnd = khf,
                                              .velStart = vs,
                                              .velEnd = ve,
                                              .velFadeStart = vlf,
                                              .velFadeEnd = vhf,
                                              .tracking = ktrack,
                                              .pitchOffsetSemitones = ktune,
                                          });

        // MAPPING is supplied by the multisample.xml; mask it off so a smpl
        // chunk in the embedded WAV can't clobber root/key/vel. LOOP is masked
        // off only when the multisample explicitly sets loop bounds.
        bool willWriteLoop = loopOn && loopStart + loopEnd > 0;
        int32_t loadInfo = engine::Zone::ENDPOINTS;
        if (!willWriteLoop)
            loadInfo |= engine::Zone::LOOP;
        z->attachToSample(*engine.getSampleManager(), 0, loadInfo);
        if (willWriteLoop)
        {
            import_support::importZoneLoop(*z, ctx, 0,
                                           {
                                               .mode = engine::Zone::LoopMode::LOOP_WHILE_GATED,
                                               .startSamples = (int64_t)loopStart,
                                               .endSamples = (int64_t)loopEnd,
                                               .active = true,
                                           });
        }
        ctx.addZoneToGroup(group_id, std::move(z));

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
            auto groupName = fc->Attribute("name") ? (name + fc->Attribute("name")) : "";
            addedGroupIndices.push_back(ctx.addGroup(groupName));
        }
        else if (eln == "sample")
        {
            if (!addSampleFromElement(fc))
            {
                mz_zip_reader_end(&zip_archive);
                return false;
            }
        }
        else if (eln == "layer")
        {
            // Sigh. Presonus does something a little different
            auto groupName = fc->Attribute("name") ? (name + fc->Attribute("name")) : "";
            auto groupId = ctx.addGroup(groupName);
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
            ctx.unsupported("multisample field", eln);
        }
        fc = fc->NextSiblingElement();
    }

    mz_zip_reader_end(&zip_archive);
    return ctx.finish();
}
} // namespace scxt::multisample_support