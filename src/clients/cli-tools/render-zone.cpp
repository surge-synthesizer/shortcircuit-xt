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

#ifndef SCXT_SRC_CLIENTS_CLI_TOOLS_RENDER_ZONE
#define SCXT_SRC_CLIENTS_CLI_TOOLS_RENDER_ZONE

/*
 * Render one zone playing one note to a wav, so sample playback can be diffed against
 * another sampler's render of the same setup. The envelopes are flat by default, so what
 * comes out is the generator and nothing else.
 */

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "configuration.h"
#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "engine/patch.h"
#include "engine/zone.h"
#include "messaging/messaging.h"

namespace fs = std::filesystem;
using zone_t = scxt::engine::Zone;

namespace
{

void writeWavFloat32(const fs::path &out, const std::vector<float> &interleaved, double sampleRate)
{
    std::ofstream f(out, std::ios::binary);
    if (!f)
    {
        std::cerr << "cannot open " << out << " for writing\n";
        return;
    }

    uint32_t dataBytes = (uint32_t)(interleaved.size() * sizeof(float));
    uint32_t sr = (uint32_t)sampleRate;
    uint32_t riffSize = 4 + (8 + 16) + (8 + dataBytes);

    auto w32 = [&](uint32_t x) { f.write(reinterpret_cast<const char *>(&x), 4); };
    auto w16 = [&](uint16_t x) { f.write(reinterpret_cast<const char *>(&x), 2); };

    f.write("RIFF", 4);
    w32(riffSize);
    f.write("WAVE", 4);
    f.write("fmt ", 4);
    w32(16);
    w16(3); // IEEE float
    w16(2);
    w32(sr);
    w32(sr * 2 * 4);
    w16(2 * 4);
    w16(32);
    f.write("data", 4);
    w32(dataBytes);
    f.write(reinterpret_cast<const char *>(interleaved.data()), dataBytes);
}

void usage()
{
    std::cout << R"(render-zone - render one zone playing one note to a wav

  --sample PATH      sample to load                              (required)
  --out PATH         wav to write                                (required)
  --sr HZ            engine sample rate                          (48000)
  --key N            midi key to play                            (60)
  --root N           zone root key                               (60)
  --start-sample N   playback start                              (sample start)
  --end-sample N     playback end                                (sample end)
  --loop             enable looping
  --start-loop N     loop start
  --end-loop N       loop end
  --fade N           loop crossfade, in samples
  --alternate        ping-pong rather than forward-only looping
  --reverse          play the sample backwards
  --loop-mode M      during-voice | while-gated | count          (during-voice)
  --loop-count N     loops, when --loop-mode count
  --interp I         sinc | linear | zoh | zohaa                 (sinc)
  --hold SECONDS     how long the key is held                    (10)
  --tail SECONDS     how long to render after release            (1)
  --release R        AEG release, 0..1                           (0, instant)
)";
}

int64_t argInt(const std::map<std::string, std::string> &a, const std::string &k, int64_t dflt)
{
    auto p = a.find(k);
    return p == a.end() ? dflt : std::stoll(p->second);
}

double argDouble(const std::map<std::string, std::string> &a, const std::string &k, double dflt)
{
    auto p = a.find(k);
    return p == a.end() ? dflt : std::stod(p->second);
}

} // namespace

int main(int argc, char **argv)
{
    std::map<std::string, std::string> args;
    for (int i = 1; i < argc; ++i)
    {
        std::string a{argv[i]};
        if (a.rfind("--", 0) != 0)
        {
            std::cerr << "unexpected argument '" << a << "'\n";
            usage();
            return 1;
        }
        a = a.substr(2);
        // the flags take no value; everything else consumes the next argument
        if (a == "loop" || a == "alternate" || a == "reverse" || a == "help")
            args[a] = "1";
        else if (i + 1 < argc)
            args[a] = argv[++i];
        else
        {
            std::cerr << "--" << a << " needs a value\n";
            return 1;
        }
    }

    if (args.count("help") || !args.count("sample") || !args.count("out"))
    {
        usage();
        return args.count("help") ? 0 : 1;
    }

    auto samplePath = fs::path{args["sample"]};
    if (!fs::exists(samplePath))
    {
        std::cerr << "no such sample: " << samplePath << "\n";
        return 1;
    }

    auto sampleRate = argDouble(args, "sr", 48000);

    auto eng = std::make_unique<scxt::engine::Engine>();
    eng->prepareToPlay(sampleRate);
    eng->midikeyRetuner.setTuningMode(scxt::tuning::MidikeyRetuner::TWELVE_TET);

    auto &part = *eng->getPatch()->getPart(0);
    part.addGroup();
    auto *group = part.getGroup(0).get();

    auto z = std::make_unique<zone_t>();
    z->mapping.keyboardRange = {0, 127};
    z->mapping.velocityRange = {0, 127};
    z->mapping.rootKey = (int16_t)argInt(args, "root", 60);
    z->initialize();
    group->addZone(z);
    auto *zone = group->getZone(0).get();

    // loadSampleByPath asserts it is on the serial thread; this tool is single threaded
    auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
    auto sid = eng->getSampleManager()->loadSampleByPath(samplePath);
    if (!sid.has_value())
    {
        std::cerr << "cannot load " << samplePath << "\n";
        return 1;
    }

    auto &v = zone->variantData.variants[0];
    v.sampleID = *sid;
    v.active = true;
    if (!zone->attachToSample(*eng->getSampleManager(), 0,
                              zone_t::SampleInformationRead::ENDPOINTS))
    {
        std::cerr << "cannot attach " << samplePath << "\n";
        return 1;
    }

    v.startSample = argInt(args, "start-sample", v.startSample);
    v.endSample = argInt(args, "end-sample", v.endSample);
    v.playReverse = args.count("reverse") > 0;

    if (args.count("loop"))
    {
        v.loopActive = true;
        v.startLoop = argInt(args, "start-loop", v.startSample);
        v.endLoop = argInt(args, "end-loop", v.endSample);
        v.loopFade = argInt(args, "fade", 0);
        v.loopDirection =
            args.count("alternate") ? zone_t::ALTERNATE_DIRECTIONS : zone_t::FORWARD_ONLY;
        v.loopCountWhenCounted = (int)argInt(args, "loop-count", 0);

        auto lm = args.count("loop-mode") ? args["loop-mode"] : "during-voice";
        if (lm == "during-voice")
            v.loopMode = zone_t::LOOP_DURING_VOICE;
        else if (lm == "while-gated")
            v.loopMode = zone_t::LOOP_WHILE_GATED;
        else if (lm == "count")
            v.loopMode = zone_t::LOOP_COUNT;
        else
        {
            std::cerr << "unknown loop mode '" << lm << "'\n";
            return 1;
        }
    }

    if (args.count("interp"))
    {
        auto it = args["interp"];
        if (it == "sinc")
            v.interpolationType = scxt::dsp::InterpolationTypes::Sinc;
        else if (it == "linear")
            v.interpolationType = scxt::dsp::InterpolationTypes::Linear;
        else if (it == "zoh")
            v.interpolationType = scxt::dsp::InterpolationTypes::ZeroOrderHold;
        else if (it == "zohaa")
            v.interpolationType = scxt::dsp::InterpolationTypes::ZOHAA;
        else
        {
            std::cerr << "unknown interpolation '" << it << "'\n";
            return 1;
        }
    }

    // a flat envelope, so the wav is the generator and not the AEG
    zone->egStorage[0].a = 0.f;
    zone->egStorage[0].h = 0.f;
    zone->egStorage[0].d = 0.f;
    zone->egStorage[0].s = 1.f;
    zone->egStorage[0].r = (float)argDouble(args, "release", 0.0);

    auto key = (int)argInt(args, "key", 60);
    auto holdBlocks = (int64_t)(argDouble(args, "hold", 10.0) * sampleRate / scxt::blockSize);
    auto tailBlocks = (int64_t)(argDouble(args, "tail", 1.0) * sampleRate / scxt::blockSize);

    std::vector<float> out;
    out.reserve((holdBlocks + tailBlocks) * scxt::blockSize * 2);

    auto renderBlocks = [&](int64_t n) {
        for (int64_t b = 0; b < n; ++b)
        {
            eng->processAudio();
            const auto &mb = eng->getPatch()->busses.mainBus.output;
            for (int i = 0; i < scxt::blockSize; ++i)
            {
                out.push_back(mb[0][i]);
                out.push_back(mb[1][i]);
            }
        }
    };

    eng->processNoteOnEvent(0, 0, key, -1, 1.f, 0.f);
    renderBlocks(holdBlocks);
    eng->processNoteOffEvent(0, 0, key, -1, 0.f);
    renderBlocks(tailBlocks);

    writeWavFloat32(args["out"], out, sampleRate);
    std::cout << args["out"] << " : " << out.size() / 2 << " frames at " << (int)sampleRate << "\n";
    return 0;
}

#endif
