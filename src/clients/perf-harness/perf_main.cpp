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

#include <cstring>
#include <exception>
#include <filesystem>
#include <string>

#include <fmt/core.h>

#include "configuration.h"
#include "engine/engine.h"

#include "perf_config.h"
#include "perf_generator.h"
#include "perf_loader.h"
#include "perf_report.h"
#include "perf_runner.h"

namespace
{
void printUsage(const char *argv0)
{
    fmt::print("Usage: {} <config.json> [--mode measure|profile] [--iters N] [--out path]\n",
               argv0);
    fmt::print("             [--wav path] [--wait-for-key] [--profile-iters N] [--realtime]\n");
    fmt::print("\n");
    fmt::print("Runs a scxt-core scenario described by <config.json>. Two modes:\n");
    fmt::print("  measure  warmup + N iterations, per-block timing, JSON report + fingerprint\n");
    fmt::print("  profile  loop the scenario for profiler attach; --wait-for-key prompts\n");
    fmt::print("           before entering the loop and prints PID for attaching;\n");
    fmt::print("           --profile-iters caps the loop (0/default = forever)\n");
    fmt::print("\n");
    fmt::print("  --realtime  request realtime thread priority (USER_INTERACTIVE qos on macOS,\n");
    fmt::print("              SCHED_FIFO on Linux — needs CAP_SYS_NICE or root)\n");
}
} // namespace

int main(int argc, char **argv)
{
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")
    {
        printUsage(argv[0]);
        return argc < 2 ? 1 : 0;
    }

    std::filesystem::path configPath = argv[1];
    scxt::perf::Config cfg;
    try
    {
        cfg = scxt::perf::loadFromFile(configPath);
    }
    catch (const std::exception &e)
    {
        fmt::print(stderr, "Failed to parse {}: {}\n", configPath.u8string(), e.what());
        return 1;
    }

    // CLI overrides
    for (int i = 2; i < argc; ++i)
    {
        std::string a = argv[i];
        auto next = [&]() -> const char * {
            if (i + 1 >= argc)
            {
                fmt::print(stderr, "Missing value for {}\n", a);
                std::exit(1);
            }
            return argv[++i];
        };
        if (a == "--mode")
        {
            std::string m = next();
            cfg.run.mode =
                (m == "profile") ? scxt::perf::RunConfig::Profile : scxt::perf::RunConfig::Measure;
        }
        else if (a == "--iters")
        {
            cfg.run.measureIterations = std::atoi(next());
        }
        else if (a == "--out")
        {
            cfg.run.reportPath = next();
        }
        else if (a == "--wav")
        {
            cfg.run.wavOutputPath = next();
        }
        else if (a == "--wait-for-key")
        {
            cfg.run.waitForKey = true;
        }
        else if (a == "--profile-iters")
        {
            cfg.run.profileIterations = std::atoi(next());
        }
        else if (a == "--realtime")
        {
            cfg.run.audioThreadPriority = scxt::perf::RunConfig::RealtimePriority;
        }
        else
        {
            fmt::print(stderr, "Unknown arg: {}\n", a);
            return 1;
        }
    }

    scxt::engine::Engine engine;
    engine.prepareToPlay(cfg.engine.sampleRate);

    auto load = scxt::perf::applyLoad(engine, cfg.load);
    if (!load.ok)
    {
        fmt::print(stderr, "Load failed: {}\n", load.error);
        return 2;
    }

    auto seq = scxt::perf::bake(cfg.sequence, cfg.engine.sampleRate, scxt::blockSize,
                                cfg.run.tailSilenceS);

    if (cfg.run.mode == scxt::perf::RunConfig::Profile)
    {
        scxt::perf::runProfile(engine, seq, cfg.run);
        return 0;
    }

    auto run = scxt::perf::runMeasure(engine, seq, cfg.run);

    scxt::perf::printConsoleSummary(cfg, load, run);
    scxt::perf::writeReport(cfg.run.reportPath, cfg, load, run);
    fmt::print("  report         : {}\n", cfg.run.reportPath);
    if (cfg.run.wavOutputPath)
    {
        scxt::perf::writeWavFloat32(*cfg.run.wavOutputPath, run.wavInterleaved,
                                    cfg.engine.sampleRate);
        fmt::print("  wav            : {} ({} interleaved floats)\n", *cfg.run.wavOutputPath,
                   run.wavInterleaved.size());
    }
    return 0;
}
