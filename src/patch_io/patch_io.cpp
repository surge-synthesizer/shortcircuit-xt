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

#include "RIFFWavWriter.hpp"

#include "utils.h"
#include "patch_io.h"
#include "engine/engine.h"
#include "messaging/messaging.h"

#include "json/engine_traits.h"

#include "cmrc/cmrc.hpp"
#include "browser/browser.h"

CMRC_DECLARE(scxt_resources_core);

namespace scxt::patch_io
{
static constexpr uint32_t scxtRIFFHeader{'SCXT'};
static constexpr uint32_t manifestChunk{'scmf'};
static constexpr uint32_t dataChunk{'scdt'};
static constexpr uint32_t sampleChunk{'scsm'};
static constexpr uint32_t samplePathsChunk{'scsp'};
static constexpr uint32_t sampleListChunk{'scsl'};
static constexpr uint32_t sampleFilenameChunk{'scsf'};
static constexpr uint32_t sampleDatChunk{'scsm'};

void addSCManifest(const std::unique_ptr<RIFF::File> &f, const std::string &type)
{
    std::map<std::string, std::string> manifest;
    manifest["version"] = "1";
    manifest["type"] = type;
    auto mmsg = tao::json::to_string(json::scxt_value(manifest));
    auto c = f->AddSubChunk(manifestChunk, mmsg.size());
    auto d = (uint8_t *)c->LoadChunkData();
    memcpy(d, mmsg.data(), mmsg.size());
}

std::unordered_map<std::string, std::string> readSCManifest(RIFF::File *f)
{
    auto c1 = f->GetSubChunk(manifestChunk);
    std::string s((char *)c1->LoadChunkData(), c1->GetSize());
    c1->ReleaseChunkData();

    tao::json::events::transformer<tao::json::events::to_basic_value<json::scxt_traits>> consumer;
    tao::json::events::from_string(consumer, s);
    auto jv = std::move(consumer.value);
    std::unordered_map<std::string, std::string> manifest;
    jv.to(manifest);

    return manifest;
}
std::unordered_map<std::string, std::string> readSCManifest(const std::unique_ptr<RIFF::File> &f)
{
    return readSCManifest(f.get());
}

void addSCDataChunk(const std::unique_ptr<RIFF::File> &f, const std::string &msg)
{
    auto c = f->AddSubChunk(dataChunk, msg.size());
    auto d = (uint8_t *)c->LoadChunkData();
    memcpy(d, msg.data(), msg.size());
}

std::string readSCDataChunk(const std::unique_ptr<RIFF::File> &f)
{
    auto cp = f->GetSubChunk(dataChunk);
    auto res = std::string((char *)cp->LoadChunkData(), cp->GetSize());
    cp->ReleaseChunkData();
    return res;
}

bool addMonolithBinaries(const std::unique_ptr<RIFF::File> &f, const engine::Engine &e,
                         const sample::SampleManager::sampleMap_t &addThese)
{
    SCLOG_IF(patchIO, "Adding " << addThese.size() << " samples to monolith");
    auto lst = f->AddSubList(sampleChunk);

    std::set<fs::path> paths;
    for (const auto &[sid, sample] : addThese)
    {
        if (!browser::Browser::isLoadableSingleSample(sample->getPath()))
        {
            e.getMessageController()->reportErrorToClient(
                "Unable to add multifile to monolith",
                "Monoliths currently only support single file (wav, flac, aiff, etc...)");
            return false;
        }
        paths.insert(sample->getPath());
    }
    std::vector<fs::path> sortedPaths(paths.begin(), paths.end());
    std::vector<std::string> sortedPathsStr;
    for (auto &p : sortedPaths)
        sortedPathsStr.push_back(p.u8string());

    auto mpaths = tao::json::to_string(json::scxt_value(sortedPathsStr));
    auto c = lst->AddSubChunk(samplePathsChunk, mpaths.size());
    auto d = (uint8_t *)c->LoadChunkData();
    memcpy(d, mpaths.data(), mpaths.size());

    auto smplst = lst->AddSubList(sampleListChunk);
    for (const auto &path : sortedPaths)
    {
        try
        {
            auto fsz = fs::file_size(path);
            std::string fn = path.filename().u8string();
            auto cex = smplst->AddSubChunk(sampleFilenameChunk, fn.size() + 1);
            auto dex = (uint8_t *)cex->LoadChunkData();
            strncpy((char *)dex, fn.c_str(), fn.size());
            dex[fn.size() + 1] = 0;

            auto c = smplst->AddSubChunk(sampleDatChunk, fsz);
            auto d = (uint8_t *)c->LoadChunkData();

            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return false;
            }

            // Check the file size
            file.seekg(0, std::ios::end);
            std::size_t fstreamSz = file.tellg();
            file.seekg(0, std::ios::beg);

            if (fstreamSz != fsz)
            {
                SCLOG("Mismatched sizes " << fsz << " " << fstreamSz);
                return false;
            };

            file.read((char *)d, fsz);
            if (!file)
            {
                return false;
            }

            SCLOG_IF(patchIO, "   - " << path.u8string() << " bytes=" << fsz);
            file.close();
        }
        catch (const fs::filesystem_error &fse)
        {
            return false;
        }
    }

    return true;
}

bool hasMonolithBinaries(const std::unique_ptr<RIFF::File> &f)
{
    auto lst = f->GetSubList(sampleChunk);
    return lst != nullptr;
}

std::vector<fs::path> readMonolithBinaryIndex(const std::unique_ptr<RIFF::File> &f,
                                              engine::Engine &e)
{
    auto lst = f->GetSubList(sampleChunk);
    if (lst == nullptr)
    {
        return {};
    }
    auto c = lst->GetSubChunk(samplePathsChunk);
    std::string s((char *)c->LoadChunkData(), c->GetSize());
    c->ReleaseChunkData();

    tao::json::events::transformer<tao::json::events::to_basic_value<json::scxt_traits>> consumer;
    tao::json::events::from_string(consumer, s);
    auto jv = std::move(consumer.value);
    std::vector<std::string> paths;
    jv.to(paths);

    SCLOG_IF(patchIO, "Monolith contains " << paths.size() << " paths");
    std::vector<fs::path> res;
    for (const auto &p : paths)
    {
        SCLOG_IF(patchIO, "  -> " << p);
        // Make sure to handle the c: shenanigans
        res.push_back(scxt::json::unstreamPathFromString(p));
        SCLOG_IF(patchIO, "  as " << res.back().u8string());
    }

    return res;
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

fs::path saveSubSample(const engine::Engine &e, sample::SampleManager::sampleMap_t &toCollect,
                       const SampleID &id, const std::shared_ptr<sample::Sample> &sp)
{
    SCLOG("Sample Collection from MultiThingy");
    auto writableDirectory = fs::temp_directory_path() / "scxt_multi_samples";

    fs::create_directories(writableDirectory);
    // So stream the thing to a wav with an appropriate name
    auto nm = sp->getPath().filename().replace_extension("").u8string();
    nm += " subsamp ";
    std::string pfx = "";
    for (int i = 0; i < 3; ++i)
    {
        if (id.multiAddress[i] >= 0)
        {
            nm += pfx + std::to_string(id.multiAddress[i]);
            pfx = "_";
        }
    }

    auto nf = writableDirectory / (nm + ".wav");

    SCLOG("Writing to " << nf.u8string());
    auto ch = sp->channels;
    if (sp->bitDepth == sample::Sample::BD_F32)
    {
        riffwav::RIFFWavWriter writer(nf, ch, riffwav::RIFFWavWriter::F32);
        if (!writer.openFile())
        {
            return {};
        }
        writer.writeRIFFHeader();
        writer.writeFMTChunk(sp->sample_rate);
        writer.startDataChunk();
        float d[2];
        if (ch == 1)
        {
            auto fd = sp->GetSamplePtrF32(0);
            for (int i = 0; i < sp->sample_length; ++i)
            {
                d[0] = fd[i];
                writer.pushSamplesF32(d);
            }
        }
        else if (ch == 2)
        {
            auto fl = sp->GetSamplePtrF32(0);
            auto fr = sp->GetSamplePtrF32(1);
            for (int i = 0; i < sp->sample_length; ++i)
            {
                d[0] = fl[i];
                d[1] = fr[i];
                writer.pushSamplesF32(d);
            }
        }
        if (!writer.closeFile())
        {
            return {};
        }
    }
    else if (sp->bitDepth == sample::Sample::BD_I16)
    {
        riffwav::RIFFWavWriter writer(nf, ch, riffwav::RIFFWavWriter::PCM16);
        if (!writer.openFile())
        {
            return {};
        }

        writer.writeRIFFHeader();
        writer.writeFMTChunk(sp->sample_rate);
        writer.startDataChunk();
        int16_t d[2];
        if (ch == 1)
        {
            auto fd = sp->GetSamplePtrI16(0);
            for (int i = 0; i < sp->sample_length; ++i)
            {
                d[0] = fd[i];
                writer.pushSamplesI16(d);
            }
        }
        else if (ch == 2)
        {
            auto fl = sp->GetSamplePtrI16(0);
            auto fr = sp->GetSamplePtrI16(1);
            for (int i = 0; i < sp->sample_length; ++i)
            {
                d[0] = fl[i];
                d[1] = fr[i];
                writer.pushSamplesI16(d);
            }
        }
        if (!writer.closeFile())
        {
            return {};
        }
    }
    else
    {
        SCLOG("Unsupported bit depth " << sp->bitDepth);
        return {};
    }

    auto sid = e.getSampleManager()->loadSampleByPath(nf);
    if (sid.has_value())
    {
        auto smp = e.getSampleManager()->getSample(*sid);
        toCollect.insert({*sid, smp});
        SCLOG("Re-pointing " << id.to_string() << " to "
                             << smp->getSampleFileAddress().path.u8string())
        e.getSampleManager()->remapIds[id] = {*sid, smp->getSampleFileAddress()};
    }
    else
    {
        SCLOG("Unable to load sample by path " << nf.u8string());
    }
    return nf;
}

sample::SampleManager::sampleMap_t getSamplePathsFor(const scxt::engine::Engine &e, int part,
                                                     std::vector<fs::path> &tempFilesCreated)
{
    sample::SampleManager::sampleMap_t toCollect;

    e.getSampleManager()->purgeUnreferencedSamples();

    if (part < 0)
    {
        for (auto curr = e.getSampleManager()->samplesBegin();
             curr != e.getSampleManager()->samplesEnd(); ++curr)
        {
            auto id = curr->first;
            auto sp = curr->second;
            if (sample::Sample::isSourceTypeSubSampleFromMonolith(sp->type))
            {
                tempFilesCreated.push_back(saveSubSample(e, toCollect, id, sp));
            }
            else
            {
                toCollect.insert(*curr);
            }
        }
    }
    else
    {
        const auto &pt = e.getPatch()->getPart(part);
        auto smp = pt->getSamplesUsedByPart();
        for (const auto &sid : smp)
        {
            auto sp = e.getSampleManager()->getSample(sid);
            if (sp)
            {
                if (sample::Sample::isSourceTypeSubSampleFromMonolith(sp->type))
                {
                    tempFilesCreated.push_back(saveSubSample(e, toCollect, sid, sp));
                }
                else
                {
                    toCollect.insert({sid, sp});
                }
            }
            else
            {
                SCLOG("Unable to find sample " << sid.to_string());
            }
        }
    }
    return toCollect;
}

void collectSamplesInto(const fs::path &collectDir, const scxt::engine::Engine &e, int part)
{
    std::vector<fs::path> tempFilesCreated;
    auto toCollect = getSamplePathsFor(e, part, tempFilesCreated);
    if (part < 0)
    {
        SCLOG_IF(patchIO, "Collecting all samples for multi to '" << collectDir.u8string() << "'");
    }
    else
    {
        SCLOG_IF(patchIO,
                 "Collecting samples for part " << part << " to '" << collectDir.u8string() << "'");
    }

    std::set<fs::path> uniquePaths;
    for (const auto &[_, sample] : toCollect)
    {
        uniquePaths.insert(sample->getPath());
    }

    std::set<fs::path> collectedFilenames;
    for (const auto &c : uniquePaths)
    {
        SCLOG_IF(patchIO, "   - " << c.u8string());
        if (collectedFilenames.find(c.filename()) != collectedFilenames.end())
        {
            SCLOG_IF(patchIO, "Duplicate Sample Name " << c.filename());
            e.getMessageController()->reportErrorToClient(
                "Unable to copy sample", "Your patch has two samples with the same filename ('" +
                                             c.filename().u8string() + "' with " +
                                             "different paths. This is currently unsupported for "
                                             "collect mode and needs fixing soonish!");
            return;
        }
        collectedFilenames.insert(c.filename());
        try
        {
            fs::copy_file(c, collectDir / c.filename(), fs::copy_options::overwrite_existing);
        }
        catch (fs::filesystem_error &fse)
        {
            SCLOG("Unable to copy " << c.u8string() << " " << fse.what());
            e.getMessageController()->reportErrorToClient("Unable to copy sample", fse.what());
            return;
        }
    }
    for (auto &f : tempFilesCreated)
    {
        try
        {
            fs::remove(f);
        }
        catch (fs::filesystem_error &fse)
        {
            SCLOG("Unable to remove " << f.u8string() << " " << fse.what());
        }
    }
}

bool saveMulti(const fs::path &p, const scxt::engine::Engine &e, SaveStyles style)
{
    fs::path riffPath = p;
    fs::path collectDir;

    e.getSampleManager()->remapIds.clear();

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

        auto f = std::make_unique<RIFF::File>(scxtRIFFHeader);
        f->SetByteOrder(RIFF::endian_little);
        addSCManifest(f, "multi");

        if (style == SaveStyles::AS_MONOLITH)
        {
            std::vector<fs::path> tmpf;
            auto smp = getSamplePathsFor(e, -1, tmpf);
            auto res = addMonolithBinaries(f, e, smp);
            for (auto &f : tmpf)
            {
                try
                {
                    fs::remove(f);
                }
                catch (fs::filesystem_error &fse)
                {
                    SCLOG("Unable to remove " << f.u8string() << " " << fse.what());
                }
            }
            if (!res)
            {
                return false;
            }
        }

        auto sg = scxt::engine::Engine::StreamGuard(engine::Engine::FOR_MULTI);
        auto msg = tao::json::msgpack::to_string(json::scxt_value(e));

        addSCDataChunk(f, msg);

        f->Save(riffPath.u8string());

        if (style == SaveStyles::COLLECT_SAMPLES)
        {
            e.getSampleManager()->clearReparenting();
        }
        e.getSampleManager()->remapIds.clear();
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

    e.getSampleManager()->remapIds.clear();
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

        auto f = std::make_unique<RIFF::File>(scxtRIFFHeader);
        f->SetByteOrder(RIFF::endian_little);
        addSCManifest(f, "part");

        if (style == SaveStyles::AS_MONOLITH)
        {
            std::vector<fs::path> tmpf;
            auto smp = getSamplePathsFor(e, part, tmpf);
            auto res = addMonolithBinaries(f, e, smp);
            for (auto &f : tmpf)
            {
                try
                {
                    fs::remove(f);
                }
                catch (fs::filesystem_error &fse)
                {
                    SCLOG("Unable to remove " << f.u8string() << " " << fse.what());
                }
            }
            if (!res)
            {
                return false;
            }
        }

        auto sg = scxt::engine::Engine::StreamGuard(engine::Engine::FOR_PART);
        auto &pt = e.getPatch()->getPart(part);
        auto &rid = e.getSampleManager()->remapIds;
        std::map<engine::Zone::SingleVariant *, SampleID> origIds;
        for (auto &g : *pt)
        {
            for (auto &z : *g)
            {
                for (auto &v : z->variantData.variants)
                {
                    if (v.active)
                    {
                        auto it = rid.find(v.sampleID);
                        if (it != rid.end())
                        {
                            origIds[&v] = v.sampleID;
                            v.sampleID = it->second.first;
                        }
                    }
                }
            }
        }
        auto msg = tao::json::msgpack::to_string(json::scxt_value(*(e.getPatch()->getPart(part))));

        for (auto &g : *pt)
        {
            for (auto &z : *g)
            {
                for (auto &v : z->variantData.variants)
                {
                    if (v.active)
                    {
                        auto it = origIds.find(&v);
                        if (it != origIds.end())
                        {
                            v.sampleID = it->second;
                        }
                    }
                }
            }
        }
        addSCDataChunk(f, msg);

        f->Save(riffPath.u8string());

        if (style == SaveStyles::COLLECT_SAMPLES)
        {
            e.getSampleManager()->clearReparenting();
        }
        e.getSampleManager()->remapIds.clear();
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
    std::vector<fs::path> monolithBinaryIndex;
    try
    {
        auto f = std::make_unique<RIFF::File>(p.u8string());
        auto manifest = readSCManifest(f);
        payload = readSCDataChunk(f);

        if (hasMonolithBinaries(f))
        {
            SCLOG_IF(patchIO, "Loading a multi with monolithic binaries");
            monolithBinaryIndex = readMonolithBinaryIndex(f, engine);
        }
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
            [payload, multiPath = p, monolithBinaryIndex, &nonconste = engine](auto &e) {
                try
                {
                    nonconste.getSampleManager()->setRelativeRoot(multiPath.parent_path());
                    nonconste.getSampleManager()->setMonolithBinaryIndex(multiPath,
                                                                         monolithBinaryIndex);
                    nonconste.immediatelyTerminateAllVoices();
                    scxt::json::unstreamEngineState(nonconste, payload, true);
                    nonconste.getSampleManager()->clearReparenting();
                    nonconste.getSampleManager()->clearMonolithBinaryIndex();
                    nonconste.getSampleManager()->purgeUnreferencedSamples();

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
    std::vector<fs::path> monolithBinaryIndex;

    try
    {
        auto f = std::make_unique<RIFF::File>(p.u8string());
        auto manifest = readSCManifest(f);
        payload = readSCDataChunk(f);

        if (hasMonolithBinaries(f))
        {
            SCLOG_IF(patchIO, "Loading a part with monolithic binaries");
            monolithBinaryIndex = readMonolithBinaryIndex(f, engine);
        }
    }
    catch (const RIFF::Exception &e)
    {
        SCLOG("RIFF::Exception " << e.Message);
        return false;
    }

    auto &cont = engine.getMessageController();
    if (cont->isAudioRunning)
    {
        cont->stopAudioThreadThenRunOnSerial([payload, multiPath = p, relP = p.parent_path(),
                                              monolithBinaryIndex, part,
                                              &nonconste = engine](auto &e) {
            try
            {
                nonconste.getSampleManager()->setRelativeRoot(relP);
                nonconste.getSampleManager()->setMonolithBinaryIndex(multiPath,
                                                                     monolithBinaryIndex);
                nonconste.immediatelyTerminateAllVoices();
                scxt::json::unstreamPartState(nonconste, part, payload, true);
                nonconste.getSampleManager()->clearReparenting();
                nonconste.getSampleManager()->purgeUnreferencedSamples();
                nonconste.getSampleManager()->clearMonolithBinaryIndex();

                auto &pt = nonconste.getPatch()->getPart(part);
                if (!pt->getGroups().empty() &&
                    part == nonconste.getSelectionManager()->selectedPart)
                {
                    nonconste.getSelectionManager()->selectAction({part, 0, -1});
                }
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

SCMonolithSampleReader::SCMonolithSampleReader(RIFF::File *f) : file(f)
{
    auto mani = readSCManifest(f);
    version = std::stoi(mani["version"]);
}

SCMonolithSampleReader::~SCMonolithSampleReader() {}

size_t SCMonolithSampleReader::getSampleCount()
{
    if (cacheSampleCountDone)
        return cacheSampleCount;
    cacheSampleCountDone = true;
    auto lst = file->GetSubList(sampleChunk);
    if (!lst)
    {
        cacheSampleCount = 0;
        return 0;
    }
    auto slst = lst->GetSubList(sampleListChunk);
    if (!slst)
    {
        cacheSampleCount = 0;
        return 0;
    }
    auto ck = slst->GetFirstSubChunk();
    auto ckCount{0};
    while (ck)
    {
        ck = slst->GetNextSubChunk();
        ckCount++;
    }
    cacheSampleCount = ckCount / 2;
    return cacheSampleCount;
}

bool SCMonolithSampleReader::getSampleData(size_t index, SampleData &data)
{
    auto lst = file->GetSubList(sampleChunk);
    if (!lst)
    {
        return false;
    }
    auto slst = lst->GetSubList(sampleListChunk);
    if (!slst)
    {
        return false;
    }

    auto ck = slst->GetFirstSubChunk();
    if (!ck)
        return false;
    for (int i = 0; i < index * 2; i++)
    {
        if (ck)
            ck = slst->GetNextSubChunk();
        if (!ck)
            return false;
    }

    if (ck->GetChunkID() != sampleFilenameChunk)
    {
        SCLOG("didn't count forward to filename chunk");
        return false;
    }

    data.filename = std::string((char *)ck->LoadChunkData());
    ck->ReleaseChunkData();

    ck = slst->GetNextSubChunk();
    if (!ck)
        return false;
    if (ck->GetChunkID() != sampleDatChunk)
    {
        SCLOG("didn't get data chunk id");
        return false;
    }
    auto sd = ck->LoadChunkData();
    data.data.assign((uint8_t *)sd, (uint8_t *)sd + ck->GetSize());
    ck->ReleaseChunkData();

    return true;
}

} // namespace scxt::patch_io