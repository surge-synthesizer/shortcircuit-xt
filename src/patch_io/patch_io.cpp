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

#include <fstream>
#include <set>

#include "tao/json/msgpack/consume_string.hpp"
#include "tao/json/msgpack/from_binary.hpp"
#include "tao/json/msgpack/from_input.hpp"
#include "tao/json/msgpack/from_string.hpp"
#include "tao/json/msgpack/parts_parser.hpp"
#include "tao/json/msgpack/to_stream.hpp"
#include "tao/json/msgpack/to_string.hpp"

#include "RIFF.h" // from libgig

#include "utils.h"
#include "patch_io.h"
#include "engine/engine.h"
#include "messaging/messaging.h"

#include "json/engine_traits.h"

#include "cmrc/cmrc.hpp"

CMRC_DECLARE(scxt_resources_core);

namespace scxt::patch_io
{
void addSCManifest(const std::unique_ptr<RIFF::File> &f, const std::string &type)
{
    std::map<std::string, std::string> manifest;
    manifest["version"] = "1";
    manifest["type"] = type;
    auto mmsg = tao::json::to_string(json::scxt_value(manifest));
    auto c = f->AddSubChunk('scmf', mmsg.size());
    auto d = (uint8_t *)c->LoadChunkData();
    memcpy(d, mmsg.data(), mmsg.size());
}

std::unordered_map<std::string, std::string> readSCManifest(const std::unique_ptr<RIFF::File> &f)
{
    auto c1 = f->GetSubChunk('scmf');
    std::string s((char *)c1->LoadChunkData(), c1->GetSize());

    tao::json::events::transformer<tao::json::events::to_basic_value<json::scxt_traits>> consumer;
    tao::json::events::from_string(consumer, s);
    auto jv = std::move(consumer.value);
    std::unordered_map<std::string, std::string> manifest;
    jv.to(manifest);

    return manifest;
}

void addSCDataChunk(const std::unique_ptr<RIFF::File> &f, const std::string &msg)
{
    auto c = f->AddSubChunk('scdt', msg.size());
    auto d = (uint8_t *)c->LoadChunkData();
    memcpy(d, msg.data(), msg.size());
}

std::string readSCDataChunk(const std::unique_ptr<RIFF::File> &f)
{
    auto cp = f->GetSubChunk('scdt');
    return std::string((char *)cp->LoadChunkData(), cp->GetSize());
}

std::tuple<fs::path, fs::path, std::string> setupForCollection(const fs::path &path)
{
    auto collectDir = path;
    collectDir.replace_extension("");
    auto riffPath = collectDir / path.filename();
    collectDir /= "samples";
    try
    {
        fs::create_directories(collectDir);
    }
    catch (const fs::filesystem_error &fse)
    {
        return {{},
                {},
                std::string("Unable to create collect directory: " + collectDir.u8string() + " " +
                            fse.what())};
    }
    return {riffPath, collectDir, ""};
}

void collectSamplesInto(const fs::path &collectDir, const scxt::engine::Engine &e, int part)
{
    std::set<fs::path> toCollect;
    e.getSampleManager()->purgeUnreferencedSamples();

    if (part < 0)
    {
        SCLOG("Collecting all samples into " << collectDir.u8string());
        for (auto sit = e.getSampleManager()->samplesBegin();
             sit != e.getSampleManager()->samplesEnd(); ++sit)
        {
            toCollect.insert(sit->second->getPath());
        }
    }
    else
    {
        SCLOG("Collecting part " << part << " samples into " << collectDir.u8string());

        const auto &pt = e.getPatch()->getPart(part);
        auto smp = pt->getSamplesUsedByPart();
        for (const auto &sid : smp)
        {
            auto sp = e.getSampleManager()->getSample(sid);
            if (sp)
            {
                toCollect.insert(sp->getPath());
            }
            else
            {
                SCLOG("Unable to find sample " << sid.to_string());
            }
        }
    }

    for (auto &c : toCollect)
    {
        SCLOG("   - " << c.u8string());
        try
        {
            fs::copy_file(c, collectDir / c.filename());
        }
        catch (fs::filesystem_error &fse)
        {
            SCLOG("Unable to copy " << c.u8string() << " " << fse.what());
            e.getMessageController()->reportErrorToClient("Unable to copy sample", fse.what());
            return;
        }
    }
}

bool saveMulti(const fs::path &p, const scxt::engine::Engine &e, SaveStyles style)
{
    fs::path riffPath = p;
    fs::path collectDir;

    if (style == SaveStyles::AS_MONOLITH)
    {
        e.getMessageController()->reportErrorToClient("Monolith not yet implemented",
                                                      "Coming Soon");
        return false;
    }

    if (style == SaveStyles::COLLECT_SAMPLES)
    {
        auto [r, c, emsg] = setupForCollection(p);
        if (emsg.empty())
        {
            riffPath = r;
            collectDir = c;
        }
        else
        {
            e.getMessageController()->reportErrorToClient("Unable to create collect dir", emsg);
        }
    }

    try
    {
        if (style == SaveStyles::COLLECT_SAMPLES)
        {
            collectSamplesInto(collectDir, e, -1);
            e.getSampleManager()->reparentSamplesOnStreamToRelative("samples/");
        }
        auto sg = scxt::engine::Engine::StreamGuard(engine::Engine::FOR_MULTI);
        auto msg = tao::json::msgpack::to_string(json::scxt_value(e));

        auto f = std::make_unique<RIFF::File>('SCXT');
        f->SetByteOrder(RIFF::endian_little);
        addSCManifest(f, "multi");
        addSCDataChunk(f, msg);

        f->Save(riffPath.u8string());

        if (style == SaveStyles::COLLECT_SAMPLES)
        {
            e.getSampleManager()->clearReparenting();
        }
    }
    catch (const RIFF::Exception &e)
    {
        SCLOG(e.Message);
    }
    return true;
}

bool savePart(const fs::path &p, const scxt::engine::Engine &e, int part,
              patch_io::SaveStyles style)
{
    fs::path riffPath = p;
    fs::path collectDir;

    if (style == SaveStyles::AS_MONOLITH)
    {
        e.getMessageController()->reportErrorToClient("Monolith not yet implemented",
                                                      "Coming Soon");
        return false;
    }

    if (style == SaveStyles::COLLECT_SAMPLES)
    {
        auto [r, c, emsg] = setupForCollection(p);
        if (emsg.empty())
        {
            riffPath = r;
            collectDir = c;
        }
        else
        {
            e.getMessageController()->reportErrorToClient("Unable to create collect dir", emsg);
        }
    }

    try
    {
        if (style == SaveStyles::COLLECT_SAMPLES)
        {
            collectSamplesInto(collectDir, e, part);
            e.getSampleManager()->reparentSamplesOnStreamToRelative("samples/");
        }
        auto sg = scxt::engine::Engine::StreamGuard(engine::Engine::FOR_PART);
        auto msg = tao::json::msgpack::to_string(json::scxt_value(*(e.getPatch()->getPart(part))));

        auto f = std::make_unique<RIFF::File>('SCXT');
        f->SetByteOrder(RIFF::endian_little);
        addSCManifest(f, "part");
        addSCDataChunk(f, msg);

        f->Save(riffPath.u8string());

        if (style == SaveStyles::COLLECT_SAMPLES)
        {
            e.getSampleManager()->clearReparenting();
        }
    }
    catch (const RIFF::Exception &e)
    {
        SCLOG(e.Message);
    }
    return true;
}

bool initFromResourceBundle(scxt::engine::Engine &engine, const std::string &file)
{
    std::string payload;

    try
    {
        auto fs = cmrc::scxt_resources_core::get_filesystem();
        auto fntf = fs.open("init_states/" + file);
        payload = std::string(fntf.begin(), fntf.end());
    }
    catch (std::exception &e)
    {
        SCLOG("Exception opening payload");
        return false;
    }

    auto &cont = engine.getMessageController();
    if (cont->isAudioRunning)
    {
        cont->stopAudioThreadThenRunOnSerial([payload, &nonconste = engine](auto &e) {
            try
            {
                nonconste.immediatelyTerminateAllVoices();
                scxt::json::unstreamEngineState(nonconste, payload, true);
                auto &cont = *e.getMessageController();
                cont.restartAudioThreadFromSerial();
            }
            catch (std::exception &err)
            {
                SCLOG("Unable to load [" << err.what() << "]");
            }
        });
    }
    else
    {
        try
        {
            engine.immediatelyTerminateAllVoices();
            scxt::json::unstreamEngineState(engine, payload, true);
        }
        catch (std::exception &err)
        {
            SCLOG("Unable to load [" << err.what() << "]");
        }
    }
    return true;
}

bool loadMulti(const fs::path &p, scxt::engine::Engine &engine)
{
    std::string payload;
    try
    {
        auto f = std::make_unique<RIFF::File>(p.u8string());
        auto manifest = readSCManifest(f);
        payload = readSCDataChunk(f);
    }
    catch (const RIFF::Exception &e)
    {
        SCLOG("RIFF::Exception " << e.Message);
        return false;
    }

    auto &cont = engine.getMessageController();
    if (cont->isAudioRunning)
    {
        cont->stopAudioThreadThenRunOnSerial(
            [payload, relP = p.parent_path(), &nonconste = engine](auto &e) {
                try
                {
                    nonconste.getSampleManager()->setRelativeRoot(relP);
                    nonconste.immediatelyTerminateAllVoices();
                    scxt::json::unstreamEngineState(nonconste, payload, true);
                    nonconste.getSampleManager()->clearReparenting();
                    auto &cont = *e.getMessageController();
                    cont.restartAudioThreadFromSerial();
                }
                catch (std::exception &err)
                {
                    SCLOG("Unable to load [" << err.what() << "]");
                }
            });
    }
    else
    {
        try
        {
            engine.immediatelyTerminateAllVoices();
            scxt::json::unstreamEngineState(engine, payload, true);
        }
        catch (std::exception &err)
        {
            SCLOG("Unable to load [" << err.what() << "]");
        }
    }
    return true;
}

bool loadPartInto(const fs::path &p, scxt::engine::Engine &engine, int part)
{
    std::string payload;
    try
    {
        auto f = std::make_unique<RIFF::File>(p.u8string());
        auto manifest = readSCManifest(f);
        payload = readSCDataChunk(f);
    }
    catch (const RIFF::Exception &e)
    {
        SCLOG("RIFF::Exception " << e.Message);
        return false;
    }

    auto &cont = engine.getMessageController();
    if (cont->isAudioRunning)
    {
        cont->stopAudioThreadThenRunOnSerial(
            [payload, relP = p.parent_path(), part, &nonconste = engine](auto &e) {
                try
                {
                    nonconste.getSampleManager()->setRelativeRoot(relP);
                    nonconste.immediatelyTerminateAllVoices();
                    scxt::json::unstreamPartState(nonconste, part, payload, true);
                    nonconste.getSampleManager()->clearReparenting();
                    auto &cont = *e.getMessageController();
                    cont.restartAudioThreadFromSerial();
                }
                catch (std::exception &err)
                {
                    SCLOG("Unable to load [" << err.what() << "]");
                }
            });
    }
    else
    {
        try
        {
            engine.immediatelyTerminateAllVoices();
            scxt::json::unstreamPartState(engine, part, payload, true);
        }
        catch (std::exception &err)
        {
            SCLOG("Unable to load [" << err.what() << "]");
        }
    }

    return true;
}
} // namespace scxt::patch_io