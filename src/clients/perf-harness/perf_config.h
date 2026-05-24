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

#ifndef SCXT_SRC_CLIENTS_PERF_HARNESS_PERF_CONFIG_H
#define SCXT_SRC_CLIENTS_PERF_HARNESS_PERF_CONFIG_H

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace scxt::perf
{

struct EngineConfig
{
    double sampleRate{48000.0};
};

struct LoadStep
{
    enum Kind
    {
        LoadInstrument, // any extension Engine::loadSampleIntoSelectedPartAndGroup handles
        LoadMulti,      // .scm explicit
        LoadPartInto    // .scp explicit, into part 0
    } kind{LoadInstrument};
    std::filesystem::path path;
    int part{0};
    // SF2-only: pick a single preset to import. At most one may be set; if both
    // were provided in JSON the parser throws. Name match is case-sensitive
    // after trimming trailing whitespace.
    std::optional<int> sf2Preset;
    std::optional<std::string> sf2PresetName;
};

struct SequenceParams
{
    int lo{60}, hi{60};   // chromatic_blast / single_note key
    double noteLenS{0.5}; // on time
    double gapS{0.5};     // off time between repeats / blast notes
    int repeats{20};      // single_note / c_major_chord
    float velocity{100.f};
    int channel{0};
    std::vector<int> chordKeys{60, 64, 67, 72};
};

struct CustomEvent
{
    double t;
    bool noteOn;
    int channel{0};
    int key{60};
    float velocity{100.f};
};

struct SequenceSpec
{
    enum Preset
    {
        SingleNote,
        CMajorChord,
        ChromaticBlast,
        Custom
    } preset{SingleNote};
    SequenceParams params;
    std::vector<CustomEvent> custom;
};

struct RunConfig
{
    enum Mode
    {
        Measure,
        Profile
    } mode{Measure};

    enum AudioThreadPriority
    {
        DefaultPriority,
        RealtimePriority // pthread_set_qos (macOS) / SCHED_FIFO (Linux)
    } audioThreadPriority{DefaultPriority};

    int warmupIterations{2};
    int measureIterations{5};
    double tailSilenceS{0.5};

    // Profile mode: 0 means loop forever
    int profileIterations{0};
    bool waitForKey{false};

    std::string reportPath{"perf_report.json"};
    std::optional<std::string> wavOutputPath;
};

struct Config
{
    std::string name;
    std::string description;
    EngineConfig engine;
    std::vector<LoadStep> load;
    SequenceSpec sequence;
    RunConfig run;
};

Config loadFromFile(const std::filesystem::path &);
std::string canonicalJson(const Config &);
uint64_t hashConfig(const Config &);

} // namespace scxt::perf
#endif
