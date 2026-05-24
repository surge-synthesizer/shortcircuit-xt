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

#include "perf_runner.h"

#include <chrono>
#include <iostream>

#include <pthread.h>
#if defined(__APPLE__)
#include <pthread/qos.h>
#elif defined(__linux__)
#include <sched.h>
#endif

#include "configuration.h"
#include "engine/engine.h"

namespace scxt::perf
{

namespace
{
constexpr int kDrainExtraBlocks = 50;
constexpr int kDrainSafetyCap = 1 << 18;

// Best-effort: log failures to stderr but never throw — realtime priority is
// platform-dependent and may require elevated privileges (SCHED_FIFO on Linux
// typically needs CAP_SYS_NICE or root). Falling back to default priority is
// always safe; the user just gets less stable timing.
void requestRealtimePriority()
{
#if defined(__APPLE__)
    int rc = pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    if (rc != 0)
        std::cerr << "scxt-perf: pthread_set_qos_class_self_np failed (rc=" << rc
                  << "), running at default priority\n";
#elif defined(__linux__)
    sched_param p{};
    p.sched_priority = std::max(1, sched_get_priority_max(SCHED_FIFO) - 1);
    int rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &p);
    if (rc != 0)
        std::cerr << "scxt-perf: pthread_setschedparam(SCHED_FIFO) failed (rc=" << rc
                  << "); try running as root or with CAP_SYS_NICE — falling back to default\n";
#else
    std::cerr << "scxt-perf: audio_thread_priority=realtime is not implemented on this platform\n";
#endif
}

void dispatchEventsAt(scxt::engine::Engine &engine, const BakedSequence &seq, size_t &evIdx,
                      int64_t blockIdx)
{
    while (evIdx < seq.events.size() && seq.events[evIdx].first == blockIdx)
    {
        const auto &e = seq.events[evIdx].second;
        if (e.type == MidiEvent::NoteOn)
            engine.processNoteOnEvent(0, e.channel, e.key, -1, e.velocity, 0.f);
        else
            engine.processNoteOffEvent(0, e.channel, e.key, -1, 0.0);
        ++evIdx;
    }
}

int drainToQuiescence(scxt::engine::Engine &engine)
{
    int blocks{0};
    while (engine.activeVoices.load(std::memory_order_relaxed) > 0 && blocks < kDrainSafetyCap)
    {
        engine.processAudio();
        ++blocks;
    }
    for (int i = 0; i < kDrainExtraBlocks; ++i)
    {
        engine.processAudio();
        ++blocks;
    }
    return blocks;
}

// runOneIteration is the CLAP-process-shaped loop. The three sinks are
// optional so the same code path serves warmup, measurement, and profiling
// without separate copies.
IterationStats runOneIteration(scxt::engine::Engine &engine, const BakedSequence &seq,
                               Fingerprint *fp, std::vector<int64_t> *blockNs,
                               std::vector<float> *wavOut)
{
    IterationStats s;
    if (blockNs)
        blockNs->reserve(seq.totalBlocks);
    if (wavOut)
        wavOut->reserve(seq.totalBlocks * scxt::blockSize * 2);

    size_t evIdx = 0;
    uint32_t peak = 0;

    auto t0 = std::chrono::steady_clock::now();
    for (int64_t b = 0; b < seq.totalBlocks; ++b)
    {
        dispatchEventsAt(engine, seq, evIdx, b);

        auto bt0 = std::chrono::steady_clock::now();
        engine.processAudio();
        auto bt1 = std::chrono::steady_clock::now();

        const auto &out = engine.getPatch()->busses.mainBus.output;
        if (fp)
            fp->absorbBlock<scxt::blockSize>(out);
        if (wavOut)
            for (int i = 0; i < scxt::blockSize; ++i)
            {
                wavOut->push_back(out[0][i]);
                wavOut->push_back(out[1][i]);
            }
        if (blockNs)
            blockNs->push_back(
                std::chrono::duration_cast<std::chrono::nanoseconds>(bt1 - bt0).count());

        auto av = engine.activeVoices.load(std::memory_order_relaxed);
        if (av > peak)
            peak = av;
    }
    auto t1 = std::chrono::steady_clock::now();

    s.wallMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    auto sceneSec = seq.totalBlocks * scxt::blockSize / seq.sampleRate;
    s.realtimeRatio = sceneSec / (s.wallMs / 1000.0);
    s.peakVoices = peak;
    return s;
}
} // namespace

RunResult runMeasure(scxt::engine::Engine &engine, const BakedSequence &seq, const RunConfig &cfg)
{
    if (cfg.audioThreadPriority == RunConfig::RealtimePriority)
        requestRealtimePriority();

    RunResult r;
    r.scenario.sampleRate = seq.sampleRate;
    r.scenario.blockSize = scxt::blockSize;
    r.scenario.totalBlocks = seq.totalBlocks;
    r.scenario.totalSeconds = seq.totalBlocks * scxt::blockSize / seq.sampleRate;
    r.fingerprint.configure(seq.sampleRate, scxt::blockSize);

    // Fingerprint + WAV are captured during the FIRST measure iteration so the
    // user can point to a specific reported iteration. Warmup iterations only
    // run the loop (no sinks). Post-drain bit-equality means any measure
    // iteration produces the same hash, so the choice is just for clarity.
    auto runOne = [&](bool capture, bool wantTiming) {
        Fingerprint *fpSink = capture ? &r.fingerprint : nullptr;
        std::vector<float> *wavSink = (capture && cfg.wavOutputPath) ? &r.wavInterleaved : nullptr;
        std::vector<int64_t> blocks;
        auto s = runOneIteration(engine, seq, fpSink, wantTiming ? &blocks : nullptr, wavSink);
        s.blockNs = std::move(blocks);
        s.drainBlocks = drainToQuiescence(engine);
        if (capture)
            r.fingerprint.finalize();
        return s;
    };

    std::cout << "scxt-perf: starting " << cfg.warmupIterations << " warmup + "
              << cfg.measureIterations << " measure iteration(s) of " << r.scenario.totalSeconds
              << "s each" << std::endl;

    for (int i = 0; i < cfg.warmupIterations; ++i)
    {
        auto s = runOne(/*capture=*/false, /*wantTiming=*/false);
        std::cout << "  warmup  " << (i + 1) << "/" << cfg.warmupIterations << ": wall=" << s.wallMs
                  << "ms  realtime=" << s.realtimeRatio << "x" << std::endl;
        r.warmupIterations.push_back(std::move(s));
    }
    for (int i = 0; i < cfg.measureIterations; ++i)
    {
        auto s = runOne(/*capture=*/i == 0, /*wantTiming=*/true);
        std::cout << "  measure " << (i + 1) << "/" << cfg.measureIterations
                  << ": wall=" << s.wallMs << "ms  realtime=" << s.realtimeRatio << "x"
                  << std::endl;
        r.measureIterations.push_back(std::move(s));
    }

    return r;
}

void runProfile(scxt::engine::Engine &engine, const BakedSequence &seq, const RunConfig &cfg)
{
    if (cfg.audioThreadPriority == RunConfig::RealtimePriority)
        requestRealtimePriority();

    if (cfg.waitForKey)
    {
        std::cout << "scxt-perf: profile mode, PID=" << ::getpid()
                  << " — attach profiler then press Enter to begin." << std::endl;
        std::cin.get();
        std::cout << "scxt-perf: profile loop started." << std::endl;
    }
    int it = 0;
    while (cfg.profileIterations == 0 || it < cfg.profileIterations)
    {
        runOneIteration(engine, seq, nullptr, nullptr, nullptr);
        drainToQuiescence(engine);
        ++it;
    }
}

} // namespace scxt::perf
