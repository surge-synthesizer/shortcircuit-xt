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

#include <fstream>
#include <map>
#include <optional>
#include <set>

#include "tao/json/to_string.hpp"
#include "tao/json/from_string.hpp"

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
#include "infrastructure/user_defaults.h"

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

std::string readSCManifestString(RIFF::File *f)
{
    // libgig returns null for an absent chunk; opening a RIFF that isn't
    // an SCXT patch would deref null. Throw so the callers' RIFF::Exception
    // handlers report an error instead of crashing.
    auto c1 = f->GetSubChunk(manifestChunk);
    if (!c1)
        throw RIFF::Exception("File has no SCXT manifest chunk");
    std::string s((char *)c1->LoadChunkData(), c1->GetSize());
    c1->ReleaseChunkData();
    return s;
}

std::unordered_map<std::string, std::string> readSCManifest(RIFF::File *f)
{
    auto c1 = f->GetSubChunk(manifestChunk);
    if (!c1)
        throw RIFF::Exception("File has no SCXT manifest chunk");
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
    if (!cp)
        throw RIFF::Exception("File has no SCXT data chunk");
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
            RAISE_ERROR_ENGINE(
                e, "Unable to add multifile to monolith",
                "Monoliths currently only support single file (wav, flac, aiff, etc...)");
            return false;
        }
        paths.insert(sample->getPath());
    }
    std::vector<fs::path> sortedPaths(paths.begin(), paths.end());
    std::vector<std::string> sortedPathsStr;
    for (auto &p : sortedPaths)
    {
        auto pp = fs::path(fs::u8path(p.u8string()));
        sortedPathsStr.push_back(pp.u8string());
    }

    auto mpaths = tao::json::to_string(json::scxt_value(sortedPathsStr));
    auto c = lst->AddSubChunk(samplePathsChunk, mpaths.size());
    auto d = (uint8_t *)c->LoadChunkData();
    memcpy(d, mpaths.data(), mpaths.size());

    auto unableToRead = [&e](const fs::path &path, const std::string &why) {
        RAISE_ERROR_ENGINE(e, "Unable to add sample to monolith", path.u8string() + "\n" + why);
        return false;
    };

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
            dex[fn.size()] = 0;

            auto c = smplst->AddSubChunk(sampleDatChunk, fsz);
            auto d = (uint8_t *)c->LoadChunkData();

            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return unableToRead(path, "Could not open file");
            }

            // Check the file size
            file.seekg(0, std::ios::end);
            std::size_t fstreamSz = file.tellg();
            file.seekg(0, std::ios::beg);

            if (fstreamSz != fsz)
            {
                SCLOG_IF(patchIO, "Mismatched sizes " << fsz << " " << fstreamSz);
                return unableToRead(path, "File size changed while reading");
            };

            file.read((char *)d, fsz);
            if (!file)
            {
                return unableToRead(path, "Could not read file");
            }

            SCLOG_IF(patchIO, "   - " << path.u8string() << " bytes=" << fsz);
            file.close();
        }
        catch (const fs::filesystem_error &fse)
        {
            return unableToRead(path, fse.what());
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
    if (!c)
        throw RIFF::Exception("Monolith sample list has no paths chunk");
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

std::tuple<fs::path, fs::path, std::string, std::string> setupForCollection(const fs::path &path)
{
    auto re = path.filename().replace_extension("");
    auto riffPath = path;
    auto sampleDir = (re.u8string() + " Samples");
    auto collectDir = path.parent_path() / sampleDir;

    try
    {
        fs::create_directories(collectDir);
    }
    catch (const fs::filesystem_error &fse)
    {
        return {{},
                {},
                {},
                std::string("Unable to create collect directory: " + collectDir.u8string() + " " +
                            fse.what())};
    }
    SCLOG_IF(patchIO, "Saving collected '" << path.u8string() << "' with sample dir '"
                                           << collectDir.u8string() << "'")
    return {riffPath, collectDir, sampleDir + "/", ""};
}

fs::path saveSubSample(const engine::Engine &e, sample::SampleManager::sampleMap_t &toCollect,
                       const SampleID &id, const std::shared_ptr<sample::Sample> &sp)
{
    SCLOG_IF(patchIO, "Sample Collection from MultiThingy " << id.to_string());
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

    SCLOG_IF(patchIO, "Writing to " << nf.u8string());
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
            for (int i = 0; i < sp->sampleLengthPerChannel; ++i)
            {
                d[0] = fd[i];
                writer.pushSamplesF32(d);
            }
        }
        else if (ch == 2)
        {
            auto fl = sp->GetSamplePtrF32(0);
            auto fr = sp->GetSamplePtrF32(1);
            for (int i = 0; i < sp->sampleLengthPerChannel; ++i)
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
            for (int i = 0; i < sp->sampleLengthPerChannel; ++i)
            {
                d[0] = fd[i];
                writer.pushSamplesI16(d);
            }
        }
        else if (ch == 2)
        {
            auto fl = sp->GetSamplePtrI16(0);
            auto fr = sp->GetSamplePtrI16(1);
            for (int i = 0; i < sp->sampleLengthPerChannel; ++i)
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
        SCLOG_IF(patchIO, "Unsupported bit depth " << sp->bitDepth);
        return {};
    }

    auto sid = e.getSampleManager()->loadSampleByPath(nf);
    if (sid.has_value())
    {
        auto smp = e.getSampleManager()->getSample(*sid);
        toCollect.insert({*sid, smp});
        SCLOG_IF(patchIO, "Re-pointing " << id.to_string() << " to "
                                         << smp->getSampleFileAddress().path.u8string())
        e.getSampleManager()->remapIds[id] = {*sid, smp->getSampleFileAddress()};
    }
    else
    {
        SCLOG_IF(patchIO, "Unable to load sample by path " << nf.u8string());
    }
    return nf;
}

sample::SampleManager::sampleMap_t samplesToSave(const scxt::engine::Engine &e, int part)
{
    sample::SampleManager::sampleMap_t res;

    e.getSampleManager()->purgeUnreferencedSamples();

    if (part < 0)
    {
        auto lk = e.getSampleManager()->acquireMapLock();
        for (auto curr = e.getSampleManager()->samplesBegin();
             curr != e.getSampleManager()->samplesEnd(); ++curr)
        {
            res.insert(*curr);
        }
    }
    else
    {
        const auto &pt = e.getPatch()->getPart(part);
        for (const auto &sid : pt->getSamplesUsedByPart())
        {
            auto sp = e.getSampleManager()->getSample(sid);
            if (sp)
            {
                res.insert({sid, sp});
            }
            else
            {
                SCLOG_IF(patchIO, "Unable to find sample " << sid.to_string());
            }
        }
    }
    return res;
}

sample::SampleManager::sampleMap_t getSamplePathsFor(const scxt::engine::Engine &e, int part,
                                                     std::vector<fs::path> &tempFilesCreated)
{
    sample::SampleManager::sampleMap_t toCollect;

    // saveSubSample adds to the sample manager, so don't walk it live
    for (const auto &[id, sp] : samplesToSave(e, part))
    {
        if (sample::Sample::isSourceTypeSubSampleFromMonolith(sp->type))
        {
            tempFilesCreated.push_back(saveSubSample(e, toCollect, id, sp));
        }
        else
        {
            toCollect.insert({id, sp});
        }
    }
    return toCollect;
}

void removeTempFiles(const std::vector<fs::path> &files)
{
    for (auto &f : files)
    {
        try
        {
            fs::remove(f);
        }
        catch (fs::filesystem_error &fse)
        {
            SCLOG_IF(patchIO, "Unable to remove " << f.u8string() << " " << fse.what());
        }
    }
}

bool sampleFilesPresentForSave(const scxt::engine::Engine &e, int part, SaveStyles style)
{
    auto what = part < 0 ? std::string("the multi") : "part " + std::to_string(part + 1);
    std::string action;
    switch (style)
    {
    case NO_SAMPLES:
        return true;
    case WITH_COLLECTED_SAMPLES:
        action = "save " + what + " with collected samples";
        break;
    case AS_MONOLITH:
        action = "save " + what + " as a monolith";
        break;
    case AS_SFZ:
        action = "export " + what + " as SFZ";
        break;
    case ONLY_COLLECT:
        action = "collect the samples for " + what;
        break;
    }

    std::set<fs::path> missing;
    for (const auto &[id, sp] : samplesToSave(e, part))
    {
        if (sp->isMissingPlaceholder)
        {
            missing.insert(sp->getPath());
        }
        else if (!sample::Sample::isSourceTypeSubSampleFromMonolith(sp->type))
        {
            // sub-samples are rewritten from memory, everything else is read from disk
            std::error_code ec;
            if (!fs::exists(sp->getPath(), ec))
                missing.insert(sp->getPath());
        }
    }

    if (missing.empty())
        return true;

    static constexpr size_t maxListed{8};
    auto msg = "Unable to " + action + ". " +
               (missing.size() == 1 ? std::string("A sample file has")
                                    : std::to_string(missing.size()) + " sample files have") +
               " moved or been deleted since loading, so nothing was written.\n";
    size_t listed{0};
    for (const auto &p : missing)
    {
        if (listed == maxListed)
        {
            msg += "\n... and " + std::to_string(missing.size() - maxListed) + " more";
            break;
        }
        msg += "\n" + p.u8string();
        listed++;
    }
    SCLOG_IF(patchIO, msg);
    RAISE_ERROR_ENGINE(e, "Missing Sample Files", msg);
    return false;
}

std::optional<std::unordered_map<SampleID, fs::path>>
collectSamplesInto(const fs::path &collectDir, const scxt::engine::Engine &e, int part)
{
    std::vector<fs::path> tempFilesCreated;
    std::unordered_map<SampleID, fs::path> res;
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

    std::map<fs::path, SampleID> uniquePaths;
    for (const auto &[sid, sample] : toCollect)
    {
        uniquePaths.insert({sample->getPath(), sid});
    }

    bool ok{true};
    std::set<fs::path> collectedFilenames;
    for (const auto &[c, sid] : uniquePaths)
    {
        SCLOG_IF(patchIO, "   - " << c.u8string());
        if (collectedFilenames.find(c.filename()) != collectedFilenames.end())
        {
            SCLOG_IF(patchIO, "Duplicate Sample Name " << c.filename());
            RAISE_ERROR_ENGINE(e, "Unable to copy sample",
                               "Your patch has two samples with the same filename ('" +
                                   c.filename().u8string() + "' with " +
                                   "different paths. This is currently unsupported for "
                                   "collect mode and needs fixing soonish!");
            ok = false;
            break;
        }
        collectedFilenames.insert(c.filename());
        try
        {
            auto to = collectDir / c.filename();
            if (c != to)
                fs::copy_file(c, collectDir / c.filename(), fs::copy_options::overwrite_existing);
            res.insert({sid, to});
        }
        catch (fs::filesystem_error &fse)
        {
            SCLOG_IF(patchIO, "Unable to copy " << c.u8string() << " " << fse.what());
            RAISE_ERROR_ENGINE(e, "Unable to copy sample", fse.what());
            ok = false;
            break;
        }
    }
    removeTempFiles(tempFilesCreated);
    if (!ok)
        return std::nullopt;
    return res;
}

bool onlyCollect(const fs::path &p, scxt::engine::Engine &e, int part)
{
    if (!sampleFilesPresentForSave(e, part, SaveStyles::ONLY_COLLECT))
        return false;

    try
    {
        if (!fs::is_directory(p))
            fs::create_directories(p);
    }
    catch (const fs::filesystem_error &fse)
    {
        RAISE_ERROR_ENGINE(e, "Unable to create directory", p.u8string() + "\n" + fse.what());
        return false;
    }
    auto collectMap = patch_io::collectSamplesInto(p, e, part);
    if (!collectMap)
        return false;
    if (collectMap->empty())
    {
        RAISE_ERROR_ENGINE(e, "No Samples Collected", "No samples were saved to " + p.u8string());
        return false;
    }
    return true;
}

bool saveMulti(const fs::path &p, scxt::engine::Engine &e, SaveStyles style)
{
    if (style == SaveStyles::ONLY_COLLECT)
    {
        return onlyCollect(p, e, -1);
    }
    if (!sampleFilesPresentForSave(e, -1, style))
    {
        return false;
    }

    fs::path riffPath = p;
    fs::path collectDir;

    e.getSampleManager()->remapIds.clear();
    e.prepareToStream();

    std::string reparentInto;
    if (style == SaveStyles::WITH_COLLECTED_SAMPLES)
    {
        auto [r, c, rp, emsg] = setupForCollection(p);
        if (emsg.empty())
        {
            riffPath = r;
            collectDir = c;
            reparentInto = rp;
        }
        else
        {
            RAISE_ERROR_ENGINE(e, "Unable to create collect dir", emsg);
            e.getSampleManager()->remapIds.clear();
            return false;
        }
    }

    try
    {
        if (style == SaveStyles::WITH_COLLECTED_SAMPLES)
        {
            if (!collectSamplesInto(collectDir, e, -1))
            {
                e.getSampleManager()->remapIds.clear();
                return false;
            }
            e.getSampleManager()->reparentSamplesOnStreamToRelative(reparentInto);
        }

        auto f = std::make_unique<RIFF::File>(scxtRIFFHeader);
        f->SetByteOrder(RIFF::endian_little);
        addSCManifest(f, "multi");

        if (style == SaveStyles::AS_MONOLITH)
        {
            std::vector<fs::path> tmpf;
            auto smp = getSamplePathsFor(e, -1, tmpf);
            auto res = addMonolithBinaries(f, e, smp);
            removeTempFiles(tmpf);
            if (!res)
            {
                e.getSampleManager()->remapIds.clear();
                return false;
            }
        }

        auto sg = scxt::engine::Engine::StreamGuard(engine::Engine::FOR_MULTI);
        auto msg = tao::json::msgpack::to_string(json::scxt_value(e));

        addSCDataChunk(f, msg);

        f->Save(riffPath.u8string());

        // the multi is now named for the file it lives in
        e.getSelectionManager()->setMultiFile(riffPath, style == SaveStyles::AS_MONOLITH);
        e.getSelectionManager()->sendPatchFilesToClient();

        if (style == SaveStyles::WITH_COLLECTED_SAMPLES)
        {
            e.getSampleManager()->clearReparenting();
        }
        e.getSampleManager()->remapIds.clear();
    }
    catch (const RIFF::Exception &exc)
    {
        SCLOG_IF(patchIO, exc.Message);
        RAISE_ERROR_ENGINE(e, "Exception saving Multi",
                           riffPath.u8string() + " threw " + exc.Message);
        // A failed save must report failure and leave the SampleManager out of
        // reparent mode, else subsequent saves corrupt their paths.
        if (style == SaveStyles::WITH_COLLECTED_SAMPLES)
            e.getSampleManager()->clearReparenting();
        e.getSampleManager()->remapIds.clear();
        return false;
    }
    return true;
}

bool savePart(const fs::path &p, scxt::engine::Engine &e, int part, patch_io::SaveStyles style)
{
    if (style == SaveStyles::ONLY_COLLECT)
    {
        if (part < 0 || part >= numParts)
            return false;
        return onlyCollect(p, e, part);
    }
    if (!sampleFilesPresentForSave(e, part, style))
    {
        return false;
    }

    fs::path riffPath = p;
    fs::path collectDir;
    std::string reparentInto;
    auto priorName = e.getPatch()->getPart(part)->names;

    e.getSampleManager()->remapIds.clear();
    e.prepareToStream();
    if (style == SaveStyles::WITH_COLLECTED_SAMPLES)
    {
        auto [r, c, rp, emsg] = setupForCollection(p);
        if (emsg.empty())
        {
            riffPath = r;
            collectDir = c;
            reparentInto = rp;
        }
        else
        {
            RAISE_ERROR_ENGINE(e, "Unable to create collect dir", emsg);
            e.getSampleManager()->remapIds.clear();
            return false;
        }
    }

    try
    {
        if (style == SaveStyles::WITH_COLLECTED_SAMPLES)
        {
            if (!collectSamplesInto(collectDir, e, part))
            {
                e.getSampleManager()->remapIds.clear();
                return false;
            }
            e.getSampleManager()->reparentSamplesOnStreamToRelative(reparentInto);
        }

        auto f = std::make_unique<RIFF::File>(scxtRIFFHeader);
        f->SetByteOrder(RIFF::endian_little);
        addSCManifest(f, "part");

        if (style == SaveStyles::AS_MONOLITH)
        {
            std::vector<fs::path> tmpf;
            auto smp = getSamplePathsFor(e, part, tmpf);
            auto res = addMonolithBinaries(f, e, smp);
            removeTempFiles(tmpf);
            if (!res)
            {
                e.getSampleManager()->remapIds.clear();
                return false;
            }
        }

        auto sg = scxt::engine::Engine::StreamGuard(engine::Engine::FOR_PART);
        auto &pt = e.getPatch()->getPart(part);

        // the part takes the name of the file, and the file carries it
        auto priorName = pt->names;
        auto stem = riffPath.filename().replace_extension("").u8string();
        snprintf(pt->names.name, sizeof(pt->names.name), "%s", stem.c_str());

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

        e.getSelectionManager()->setPartFile(part, riffPath, style == SaveStyles::AS_MONOLITH);
        e.getSelectionManager()->sendPatchFilesToClient();
        e.sendPartNamesToClient(part);

        if (style == SaveStyles::WITH_COLLECTED_SAMPLES)
        {
            e.getSampleManager()->clearReparenting();
        }
        e.getSampleManager()->remapIds.clear();
    }
    catch (const RIFF::Exception &exc)
    {
        SCLOG_IF(patchIO, exc.Message);
        RAISE_ERROR_ENGINE(e, "Exception saving Part",
                           riffPath.u8string() + " threw " + exc.Message);
        // A failed save must report failure and leave the SampleManager out of
        // reparent mode, else subsequent saves corrupt their paths.
        if (style == SaveStyles::WITH_COLLECTED_SAMPLES)
            e.getSampleManager()->clearReparenting();
        e.getSampleManager()->remapIds.clear();
        e.getPatch()->getPart(part)->names = priorName;
        return false;
    }
    return true;
}

/*
 * A reset comes up untitled, so the first save has to ask where to put it. The
 * built-in templates carry a name anyway, and their parts start out unnamed.
 */
void recordResetEngine(scxt::engine::Engine &e, const std::string &file)
{
    static const std::map<std::string, std::string> templateNames{
        {"InitSampler.dat", "Init"},
        {"InitSamplerMulti.dat", "Init 16 Part"},
        {"InitSynth.dat", "Init Synth"}};

    selection::SelectionManager::PatchFiles pf;
    auto it = templateNames.find(file);
    if (it != templateNames.end())
        pf.multiName = it->second;
    e.getSelectionManager()->setPatchFiles(std::move(pf));

    for (auto &part : *(e.getPatch()))
        part->names.setName(engine::Part::PartNames::defaultName);
    if (file == "InitSynth.dat")
        e.getPatch()->getPart(0)->names.setName("Synth");

    e.getSelectionManager()->sendPatchFilesToClient();
    for (int16_t p = 0; p < (int16_t)numParts; ++p)
        e.sendPartNamesToClient(p);
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
        SCLOG_IF(patchIO, "Exception opening payload");
        return false;
    }

    auto &cont = engine.getMessageController();
    if (cont->isAudioRunning)
    {
        cont->stopAudioThreadThenRunOnSerial([payload, file, &nonconste = engine](auto &e) {
            try
            {
                nonconste.immediatelyTerminateAllVoices();
                scxt::json::unstreamEngineState(nonconste, payload, true);
                recordResetEngine(nonconste, file);
            }
            catch (std::exception &err)
            {
                SCLOG_IF(patchIO, "Unable to load [" << err.what() << "]");
            }
            // Always restart, else a failed load leaves audio permanently stopped.
            e.getMessageController()->restartAudioThreadFromSerial();
        });
    }
    else
    {
        // audio can start before the serialization thread notices
        engine.stopEngineRequests++;
        try
        {
            engine.immediatelyTerminateAllVoices();
            scxt::json::unstreamEngineState(engine, payload, true);
            recordResetEngine(engine, file);
        }
        catch (std::exception &err)
        {
            SCLOG_IF(patchIO, "Unable to load [" << err.what() << "]");
        }
        engine.stopEngineRequests--;
    }
    return true;
}

bool initFromStartupPatch(scxt::engine::Engine &engine)
{
    auto &defaults = *engine.defaults;
    auto pathString =
        defaults.getUserDefaultValue(infrastructure::DefaultKeys::startupPatchPath, std::string());
    if (pathString.empty())
        return false;

    auto p = fs::path(fs::u8path(pathString));
    auto isMulti = extensionMatches(p, ".scm");
    auto isPart = extensionMatches(p, ".scp");

    bool present{false};
    try
    {
        present = (isMulti || isPart) && fs::is_regular_file(p);
    }
    catch (const fs::filesystem_error &)
    {
    }

    if (present)
    {
        if (isMulti)
        {
            present = loadMulti(p, engine);
        }
        else
        {
            initFromResourceBundle(engine, emptyEngineResource);
            present = loadPartInto(p, engine, 0);
        }
    }

    if (!present)
    {
        RAISE_ERROR_ENGINE(engine, "Startup Patch Unavailable",
                           "Unable to load startup patch '" + pathString +
                               "'. Starting with an empty engine and clearing the startup patch.");
        defaults.updateUserDefaultValueOrOverride(infrastructure::DefaultKeys::startupPatchPath,
                                                  std::string());
        initFromResourceBundle(engine, emptyEngineResource);
    }
    return true;
}

std::optional<std::pair<std::string, std::string>> retrieveSCManifestAndPayload(const fs::path &p)
{
    try
    {
        auto f = std::make_unique<RIFF::File>(p.u8string());
        if (!f)
            return std::nullopt;
        auto manifest = readSCManifestString(f.get());
        auto pl = readSCDataChunk(f);
        return std::make_pair(manifest, pl);
    }
    catch (const RIFF::Exception &e)
    {
        SCLOG_IF(patchIO, "RIFF::Exception " << e.Message);
        return std::nullopt;
    }
}

/*
 * A multi carries the screen and tab it was saved on, but loading one shouldn't
 * move you: you were looking at something for a reason. The part you were on is
 * kept too, unless the multi you just loaded has nothing there.
 */
struct ClientViewAcrossLoad
{
    selection::SelectionManager::otherTabSelection_t tabs;
    int16_t selectedPart{0};

    static ClientViewAcrossLoad capture(const scxt::engine::Engine &e)
    {
        const auto &sm = e.getSelectionManager();
        return {sm->otherTabSelection, sm->selectedPart};
    }

    void restore(scxt::engine::Engine &e) const
    {
        // nothing was on screen to preserve, as at startup, so let the file decide
        if (tabs.empty())
            return;

        const auto &sm = e.getSelectionManager();
        for (const auto &k : {"main_screen", "multi.pgz"})
        {
            auto p = tabs.find(k);
            if (p != tabs.end())
                sm->otherTabSelection[k] = p->second;
        }
        sm->sendOtherTabsSelectionToClient();

        if (selectedPart != sm->selectedPart &&
            e.getPatch()->getPart(selectedPart)->configuration.active)
        {
            sm->selectPart(selectedPart);
        }
    }
};

/*
 * The multi is named for the file it came from. A part path that no longer resolves
 * is pointed at the multi's own folder, which is where a shared multi's parts sit if
 * they are anywhere, and keeps the name the part was saved under.
 */
void recordLoadedMulti(scxt::engine::Engine &e, const fs::path &p, bool monolith)
{
    auto &sm = e.getSelectionManager();
    auto pf = sm->getPatchFiles();
    pf.multi = {p, monolith};
    pf.multiName = p.filename().replace_extension("").u8string();
    for (auto &part : pf.parts)
    {
        if (part.path.empty())
            continue;
        try
        {
            if (!fs::is_directory(part.path.parent_path()))
                part.path = p.parent_path() / part.path.filename();
        }
        catch (const fs::filesystem_error &)
        {
            part.path = fs::path{};
        }
    }
    sm->setPatchFiles(std::move(pf));
    sm->sendPatchFilesToClient();
}

void recordLoadedPart(scxt::engine::Engine &e, int part, const fs::path &p, bool monolith)
{
    auto &pt = e.getPatch()->getPart(part);
    auto stem = p.filename().replace_extension("").u8string();
    snprintf(pt->names.name, sizeof(pt->names.name), "%s", stem.c_str());

    e.getSelectionManager()->setPartFile(part, p, monolith);
    e.getSelectionManager()->sendPatchFilesToClient();
    e.sendPartNamesToClient(part);
}

// the audio thread must be stopped around both of these
static void unstreamMultiPayload(scxt::engine::Engine &engine, const std::string &payload,
                                 const fs::path &multiPath,
                                 const std::vector<fs::path> &monolithBinaryIndex)
{
    auto &sm = *engine.getSampleManager();
    try
    {
        auto g = messaging::MessageController::ClientActivityNotificationGuard(
            "Loading Multi from " + multiPath.filename().u8string(),
            *engine.getMessageController());

        sm.setRelativeRoot(multiPath.parent_path());
        sm.setMonolithBinaryIndex(multiPath, monolithBinaryIndex);
        engine.immediatelyTerminateAllVoices();
        scxt::json::unstreamEngineState(engine, payload, true);
        sm.clearReparenting();
        sm.clearMonolithBinaryIndex();
        sm.purgeUnreferencedSamples();
        recordLoadedMulti(engine, multiPath, !monolithBinaryIndex.empty());
    }
    catch (std::exception &err)
    {
        SCLOG_IF(patchIO, "Unable to load [" << err.what() << "]");
        RAISE_ERROR_ENGINE(engine, "Unable to load Multi", err.what());
        sm.clearReparenting();
        sm.clearMonolithBinaryIndex();
    }
}

static void unstreamPartPayload(scxt::engine::Engine &engine, int part, const std::string &payload,
                                const fs::path &partPath,
                                const std::vector<fs::path> &monolithBinaryIndex)
{
    auto &smgr = *engine.getSampleManager();
    try
    {
        auto pg = messaging::MessageController::ClientActivityNotificationGuard(
            "Loading Part from " + partPath.filename().u8string(), *engine.getMessageController());

        smgr.setRelativeRoot(partPath.parent_path());
        smgr.setMonolithBinaryIndex(partPath, monolithBinaryIndex);
        engine.immediatelyTerminateAllVoices();
        scxt::json::unstreamPartState(engine, part, payload, true);
        smgr.clearReparenting();
        smgr.purgeUnreferencedSamples();
        smgr.clearMonolithBinaryIndex();

        auto &pt = engine.getPatch()->getPart(part);
        auto &sm = engine.getSelectionManager();
        // Only default-select group 0 when loading into the active part AND
        // the stream didn't restore a selection of its own. SCPs from
        // 0x2026'05'29 on carry a per-part selection slice; clobbering it
        // here would lose the saved selection.
        if (!pt->getGroups().empty() && part == sm->selectedPart &&
            !sm->currentLeadZone(engine).has_value() && !sm->currentLeadGroup(engine).has_value())
        {
            sm->applySelectActions({part, 0, -1});
        }
        recordLoadedPart(engine, part, partPath, !monolithBinaryIndex.empty());
    }
    catch (std::exception &err)
    {
        SCLOG_IF(patchIO, "Unable to load [" << err.what() << "]");
        RAISE_ERROR_ENGINE(engine, "Unable to load Part", err.what());
        smgr.clearReparenting();
        smgr.clearMonolithBinaryIndex();
    }
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
        SCLOG_IF(patchIO, "RIFF::Exception " << e.Message);
        return false;
    }

    auto priorView = ClientViewAcrossLoad::capture(engine);

    auto &cont = engine.getMessageController();
    if (cont->isAudioRunning)
    {
        cont->stopAudioThreadThenRunOnSerial(
            [payload, multiPath = p, monolithBinaryIndex, priorView, &nonconste = engine](auto &e) {
                unstreamMultiPayload(nonconste, payload, multiPath, monolithBinaryIndex);
                priorView.restore(nonconste);
                // Always restart, else a failed load leaves audio permanently stopped.
                e.getMessageController()->restartAudioThreadFromSerial();
            });
    }
    else
    {
        // audio can start before the serialization thread notices
        engine.stopEngineRequests++;
        unstreamMultiPayload(engine, payload, p, monolithBinaryIndex);
        engine.stopEngineRequests--;
        priorView.restore(engine);
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
        SCLOG_IF(patchIO, "RIFF::Exception " << e.Message);
        return false;
    }

    auto &cont = engine.getMessageController();
    if (cont->isAudioRunning)
    {
        cont->stopAudioThreadThenRunOnSerial(
            [payload, partPath = p, monolithBinaryIndex, part, &nonconste = engine](auto &e) {
                unstreamPartPayload(nonconste, part, payload, partPath, monolithBinaryIndex);
                // Always restart, else a failed load leaves audio permanently stopped.
                e.getMessageController()->restartAudioThreadFromSerial();
            });
    }
    else
    {
        // audio can start before the serialization thread notices
        engine.stopEngineRequests++;
        unstreamPartPayload(engine, part, payload, p, monolithBinaryIndex);
        engine.stopEngineRequests--;
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
        addError("didn't count forward to filename chunk");
        return false;
    }

    data.filename = std::string((char *)ck->LoadChunkData());
    ck->ReleaseChunkData();

    ck = slst->GetNextSubChunk();
    if (!ck)
        return false;
    if (ck->GetChunkID() != sampleDatChunk)
    {
        addError("didn't get data chunk id");
        return false;
    }
    auto sd = ck->LoadChunkData();
    data.data.assign((uint8_t *)sd, (uint8_t *)sd + ck->GetSize());
    ck->ReleaseChunkData();

    return true;
}

} // namespace scxt::patch_io