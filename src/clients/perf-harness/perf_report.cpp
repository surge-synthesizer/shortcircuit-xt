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

#include "perf_report.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <numeric>
#include <thread>

#include <fmt/core.h>

#include "sst/plugininfra/version_information.h"

#include <tao/json/events/from_value.hpp>
#include <tao/json/events/to_pretty_stream.hpp>
#include <tao/json/to_string.hpp>
#include <tao/json/value.hpp>

namespace scxt::perf
{

namespace
{
double percentile(std::vector<int64_t> v, double p)
{
    if (v.empty())
        return 0;
    std::sort(v.begin(), v.end());
    auto idx = (size_t)std::clamp((size_t)std::round(p * (v.size() - 1)), (size_t)0, v.size() - 1);
    return (double)v[idx];
}
double mean(const std::vector<int64_t> &v)
{
    if (v.empty())
        return 0;
    long double s = 0;
    for (auto x : v)
        s += x;
    return (double)(s / v.size());
}
double median(std::vector<double> v)
{
    if (v.empty())
        return 0;
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}
double stddev(const std::vector<double> &v)
{
    if (v.size() < 2)
        return 0;
    double m = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    double s = 0;
    for (auto x : v)
        s += (x - m) * (x - m);
    return std::sqrt(s / (v.size() - 1));
}

tao::json::value iterToJson(const IterationStats &s)
{
    tao::json::value v(tao::json::empty_object);
    v["wall_ms"] = s.wallMs;
    v["realtime_ratio"] = s.realtimeRatio;
    v["peak_voices"] = (int64_t)s.peakVoices;
    v["drain_blocks"] = s.drainBlocks;
    tao::json::value bn(tao::json::empty_object);
    bn["mean"] = mean(s.blockNs);
    bn["p50"] = percentile(s.blockNs, 0.50);
    bn["p95"] = percentile(s.blockNs, 0.95);
    bn["p99"] = percentile(s.blockNs, 0.99);
    bn["max"] = percentile(s.blockNs, 1.0);
    v["block_ns"] = std::move(bn);
    return v;
}

tao::json::value fingerprintToJson(const Fingerprint &fp)
{
    tao::json::value v(tao::json::empty_object);
    v["exact_fnv1a"] = fmt::format("{:016x}", fp.exact);
    tao::json::value tol(tao::json::empty_object);
    tol["samples"] = (int64_t)fp.samples;
    tol["sum"] = fp.sum;
    tol["rms"] = fp.rms();
    tol["peak"] = fp.peak;
    tol["zero_crossings"] = (int64_t)fp.zeroCrossings;
    tao::json::value env(tao::json::empty_array);
    for (auto e : fp.envelope)
        env.push_back((double)e);
    tol["envelope_per_100ms"] = std::move(env);
    v["tolerant"] = std::move(tol);
    return v;
}
} // namespace

void writeReport(const std::filesystem::path &out, const Config &cfg, const LoadResult &load,
                 const RunResult &run)
{
    tao::json::value v(tao::json::empty_object);
    v["scxt_version"] = sst::plugininfra::VersionInformation::project_version_and_hash;
#ifdef NDEBUG
    v["scxt_build_type"] = "Release";
#else
    v["scxt_build_type"] = "Debug";
#endif
    tao::json::value host(tao::json::empty_object);
    host["hardware_concurrency"] = (int)std::thread::hardware_concurrency();
#if defined(__APPLE__)
    host["os"] = "macOS";
#elif defined(__linux__)
    host["os"] = "Linux";
#elif defined(_WIN32)
    host["os"] = "Windows";
#else
    host["os"] = "Unknown";
#endif
    v["host"] = std::move(host);

    v["config_name"] = cfg.name;
    v["config_hash"] = fmt::format("{:016x}", hashConfig(cfg));

    tao::json::value loadJ(tao::json::empty_object);
    loadJ["wall_ms"] = load.wallMs;
    loadJ["file_hash"] = fmt::format("{:016x}", load.fileHash);
    loadJ["total_file_bytes"] = (int64_t)load.totalFileBytes;
    loadJ["num_parts"] = load.numParts;
    loadJ["num_groups"] = load.numGroups;
    loadJ["num_zones"] = load.numZones;
    loadJ["num_samples"] = load.numSamples;
    v["load"] = std::move(loadJ);

    tao::json::value sc(tao::json::empty_object);
    sc["sample_rate"] = run.scenario.sampleRate;
    sc["block_size"] = run.scenario.blockSize;
    sc["total_blocks"] = run.scenario.totalBlocks;
    sc["total_seconds"] = run.scenario.totalSeconds;
    uint32_t peakVoices = 0;
    for (const auto &it : run.measureIterations)
        peakVoices = std::max(peakVoices, it.peakVoices);
    sc["peak_voices"] = (int64_t)peakVoices;
    v["scenario"] = std::move(sc);

    tao::json::value warm(tao::json::empty_array);
    for (const auto &it : run.warmupIterations)
        warm.push_back(iterToJson(it));
    v["warmup_iterations"] = std::move(warm);

    tao::json::value meas(tao::json::empty_array);
    std::vector<double> wallMsList, p99List;
    for (const auto &it : run.measureIterations)
    {
        meas.push_back(iterToJson(it));
        wallMsList.push_back(it.wallMs);
        p99List.push_back(percentile(it.blockNs, 0.99));
    }
    v["measure_iterations"] = std::move(meas);

    tao::json::value agg(tao::json::empty_object);
    agg["wall_ms_median"] = median(wallMsList);
    agg["wall_ms_stddev"] = stddev(wallMsList);
    agg["block_ns_p99_median"] = median(p99List);
    v["aggregate"] = std::move(agg);

    v["fingerprint"] = fingerprintToJson(run.fingerprint);

    std::ofstream f(out);
    tao::json::events::to_pretty_stream consumer(f, 2);
    tao::json::events::from_value(consumer, v);
    f << "\n";
}

void writeWavFloat32(const std::filesystem::path &out, const std::vector<float> &interleaved,
                     double sampleRate)
{
    std::ofstream f(out, std::ios::binary);
    if (!f)
        return;

    uint32_t dataBytes = (uint32_t)(interleaved.size() * sizeof(float));
    uint16_t fmtCode = 3; // IEEE float
    uint16_t channels = 2;
    uint32_t sr = (uint32_t)sampleRate;
    uint16_t bitsPerSample = 32;
    uint32_t byteRate = sr * channels * (bitsPerSample / 8);
    uint16_t blockAlign = channels * (bitsPerSample / 8);
    uint32_t fmtChunkSize = 16;
    uint32_t riffSize = 4 + (8 + fmtChunkSize) + (8 + dataBytes);

    auto w32 = [&](uint32_t x) { f.write(reinterpret_cast<const char *>(&x), 4); };
    auto w16 = [&](uint16_t x) { f.write(reinterpret_cast<const char *>(&x), 2); };

    f.write("RIFF", 4);
    w32(riffSize);
    f.write("WAVE", 4);
    f.write("fmt ", 4);
    w32(fmtChunkSize);
    w16(fmtCode);
    w16(channels);
    w32(sr);
    w32(byteRate);
    w16(blockAlign);
    w16(bitsPerSample);
    f.write("data", 4);
    w32(dataBytes);
    f.write(reinterpret_cast<const char *>(interleaved.data()), dataBytes);
}

void printConsoleSummary(const Config &cfg, const LoadResult &load, const RunResult &run)
{
    fmt::print("scxt-perf  ({})\n", cfg.name.empty() ? "<unnamed>" : cfg.name);
    fmt::print("  load           : wall={:.2f}ms  parts={} groups={} zones={} samples={}\n",
               load.wallMs, load.numParts, load.numGroups, load.numZones, load.numSamples);
    fmt::print("  scenario       : sr={}  bs={}  blocks={}  ({:.3f}s)\n", run.scenario.sampleRate,
               run.scenario.blockSize, run.scenario.totalBlocks, run.scenario.totalSeconds);
    fmt::print("  warmup iters   : {}\n", run.warmupIterations.size());
    for (const auto &it : run.measureIterations)
    {
        fmt::print("  measure iter   : wall={:7.2f}ms  realtime={:5.1f}x  peak_voices={}  "
                   "drain={}\n",
                   it.wallMs, it.realtimeRatio, it.peakVoices, it.drainBlocks);
    }
    fmt::print("  fingerprint    : {:016x}  peak={:.4f}  rms={:.4f}  zc={}\n",
               run.fingerprint.exact, run.fingerprint.peak, run.fingerprint.rms(),
               run.fingerprint.zeroCrossings);
}

} // namespace scxt::perf
