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

#include "perf_config.h"

#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include <tao/json/from_file.hpp>
#include <tao/json/to_string.hpp>
#include <tao/json/value.hpp>

#include "perf_fnv.h"

namespace scxt::perf
{

namespace
{
const tao::json::value *find(const tao::json::value &obj, const char *key)
{
    if (!obj.is_object())
        return nullptr;
    auto it = obj.get_object().find(key);
    return it == obj.get_object().end() ? nullptr : &it->second;
}

// readInto returns true if the key was present and assigned. Type pulled from
// taocpp::value::as<T> for arithmetic; bool/string use the explicit accessors
// because tao::json treats them distinctly from `as`.
template <typename T> bool readInto(const tao::json::value &obj, const char *key, T &out)
{
    if (auto *x = find(obj, key); x)
    {
        if constexpr (std::is_same_v<T, std::string>)
            out = x->get_string();
        else if constexpr (std::is_same_v<T, bool>)
            out = x->get_boolean();
        else if constexpr (std::is_same_v<T, float>)
            out = (float)x->as<double>();
        else
            out = x->as<T>();
        return true;
    }
    return false;
}

void warnUnknownKeys(const tao::json::value &obj, const char *context,
                     std::initializer_list<const char *> known)
{
    if (!obj.is_object())
        return;
    std::unordered_set<std::string> ok;
    for (auto k : known)
        ok.insert(k);
    for (const auto &[k, _] : obj.get_object())
        if (ok.find(k) == ok.end())
            std::cerr << "scxt-perf: unknown key '" << k << "' in " << context << " (ignored)\n";
}

LoadStep::Kind parseLoadKind(const std::string &s)
{
    if (s == "load_multi")
        return LoadStep::LoadMulti;
    if (s == "load_part_into")
        return LoadStep::LoadPartInto;
    if (s == "load_instrument" || s == "load_sample_to_part")
        return LoadStep::LoadInstrument;
    throw std::runtime_error("Unknown load kind: " + s);
}

SequenceSpec::Preset parsePreset(const std::string &s)
{
    if (s == "single_note")
        return SequenceSpec::SingleNote;
    if (s == "c_major_chord")
        return SequenceSpec::CMajorChord;
    if (s == "chromatic_blast")
        return SequenceSpec::ChromaticBlast;
    if (s == "custom")
        return SequenceSpec::Custom;
    throw std::runtime_error("Unknown sequence preset: " + s);
}

RunConfig::Mode parseMode(const std::string &s)
{
    if (s == "measure")
        return RunConfig::Measure;
    if (s == "profile")
        return RunConfig::Profile;
    throw std::runtime_error("Unknown run mode: " + s);
}

RunConfig::AudioThreadPriority parsePriority(const std::string &s)
{
    if (s == "default")
        return RunConfig::DefaultPriority;
    if (s == "realtime")
        return RunConfig::RealtimePriority;
    throw std::runtime_error("Unknown audio_thread_priority: " + s);
}
} // namespace

Config loadFromFile(const std::filesystem::path &p)
{
    auto v = tao::json::from_file(p);
    Config c;

    warnUnknownKeys(v, "top-level", {"name", "description", "engine", "load", "sequence", "run"});

    readInto(v, "name", c.name);
    readInto(v, "description", c.description);

    if (auto *eng = find(v, "engine"); eng)
    {
        warnUnknownKeys(*eng, "engine", {"sample_rate"});
        readInto(*eng, "sample_rate", c.engine.sampleRate);
    }

    if (auto *loads = find(v, "load"); loads && loads->is_array())
    {
        for (const auto &item : loads->get_array())
        {
            warnUnknownKeys(item, "load[]", {"kind", "path", "part", "preset", "preset_name"});
            LoadStep s;
            std::string kind;
            if (readInto(item, "kind", kind))
                s.kind = parseLoadKind(kind);
            std::string path;
            if (readInto(item, "path", path))
                s.path = std::filesystem::u8path(path);
            readInto(item, "part", s.part);

            int presetIdx{0};
            if (readInto(item, "preset", presetIdx))
                s.sf2Preset = presetIdx;
            std::string presetName;
            if (readInto(item, "preset_name", presetName))
                s.sf2PresetName = presetName;

            if (s.sf2Preset && s.sf2PresetName)
                throw std::runtime_error(
                    "load[]: 'preset' and 'preset_name' are mutually exclusive");
            if ((s.sf2Preset || s.sf2PresetName) && s.kind != LoadStep::LoadInstrument)
                throw std::runtime_error(
                    "load[]: 'preset'/'preset_name' only valid with kind=load_instrument");
            if ((s.sf2Preset || s.sf2PresetName) && s.path.extension() != ".sf2" &&
                s.path.extension() != ".SF2")
                throw std::runtime_error(
                    "load[]: 'preset'/'preset_name' only valid for .sf2 files");

            c.load.push_back(std::move(s));
        }
    }

    if (auto *seq = find(v, "sequence"); seq)
    {
        warnUnknownKeys(*seq, "sequence", {"preset", "params", "events"});
        std::string preset;
        if (readInto(*seq, "preset", preset))
            c.sequence.preset = parsePreset(preset);

        if (auto *params = find(*seq, "params"); params)
        {
            warnUnknownKeys(*params, "sequence.params",
                            {"lo", "hi", "note_len_s", "gap_s", "repeats", "velocity", "channel",
                             "chord_keys"});
            auto &sp = c.sequence.params;
            readInto(*params, "lo", sp.lo);
            readInto(*params, "hi", sp.hi);
            readInto(*params, "note_len_s", sp.noteLenS);
            readInto(*params, "gap_s", sp.gapS);
            readInto(*params, "repeats", sp.repeats);
            readInto(*params, "velocity", sp.velocity);
            readInto(*params, "channel", sp.channel);
            if (auto *x = find(*params, "chord_keys"); x && x->is_array())
            {
                sp.chordKeys.clear();
                for (const auto &k : x->get_array())
                    sp.chordKeys.push_back(k.as<int>());
            }
        }

        if (auto *events = find(*seq, "events"); events && events->is_array())
        {
            for (const auto &ev : events->get_array())
            {
                warnUnknownKeys(ev, "sequence.events[]", {"t", "type", "channel", "key", "vel"});
                CustomEvent e;
                readInto(ev, "t", e.t);
                std::string type;
                if (readInto(ev, "type", type))
                    e.noteOn = (type == "note_on");
                readInto(ev, "channel", e.channel);
                readInto(ev, "key", e.key);
                readInto(ev, "vel", e.velocity);
                c.sequence.custom.push_back(e);
            }
        }
    }

    if (auto *run = find(v, "run"); run)
    {
        warnUnknownKeys(*run, "run",
                        {"mode", "warmup_iterations", "measure_iterations", "tail_silence_s",
                         "profile_iterations", "wait_for_key", "audio_thread_priority",
                         "report_path", "wav_output_path"});
        auto &r = c.run;
        std::string mode;
        if (readInto(*run, "mode", mode))
            r.mode = parseMode(mode);
        readInto(*run, "warmup_iterations", r.warmupIterations);
        readInto(*run, "measure_iterations", r.measureIterations);
        readInto(*run, "tail_silence_s", r.tailSilenceS);
        readInto(*run, "profile_iterations", r.profileIterations);
        readInto(*run, "wait_for_key", r.waitForKey);
        std::string prio;
        if (readInto(*run, "audio_thread_priority", prio))
            r.audioThreadPriority = parsePriority(prio);
        readInto(*run, "report_path", r.reportPath);
        if (auto *x = find(*run, "wav_output_path"); x && !x->is_null())
            r.wavOutputPath = x->get_string();
    }

    return c;
}

std::string canonicalJson(const Config &c)
{
    tao::json::value v(tao::json::empty_object);
    v["name"] = c.name;
    v["description"] = c.description;
    v["engine"] = tao::json::value{{"sample_rate", c.engine.sampleRate}};

    tao::json::value loads(tao::json::empty_array);
    for (const auto &s : c.load)
    {
        const char *kind = s.kind == LoadStep::LoadMulti      ? "load_multi"
                           : s.kind == LoadStep::LoadPartInto ? "load_part_into"
                                                              : "load_instrument";
        tao::json::value entry{{"kind", kind}, {"path", s.path.u8string()}, {"part", s.part}};
        if (s.sf2Preset)
            entry["preset"] = *s.sf2Preset;
        if (s.sf2PresetName)
            entry["preset_name"] = *s.sf2PresetName;
        loads.push_back(std::move(entry));
    }
    v["load"] = std::move(loads);

    const char *presetName = c.sequence.preset == SequenceSpec::SingleNote       ? "single_note"
                             : c.sequence.preset == SequenceSpec::CMajorChord    ? "c_major_chord"
                             : c.sequence.preset == SequenceSpec::ChromaticBlast ? "chromatic_blast"
                                                                                 : "custom";
    tao::json::value seq(tao::json::empty_object);
    seq["preset"] = presetName;
    {
        const auto &p = c.sequence.params;
        tao::json::value params(tao::json::empty_object);
        params["lo"] = p.lo;
        params["hi"] = p.hi;
        params["note_len_s"] = p.noteLenS;
        params["gap_s"] = p.gapS;
        params["repeats"] = p.repeats;
        params["velocity"] = p.velocity;
        params["channel"] = p.channel;
        tao::json::value ck(tao::json::empty_array);
        for (auto k : p.chordKeys)
            ck.push_back(k);
        params["chord_keys"] = std::move(ck);
        seq["params"] = std::move(params);
    }
    v["sequence"] = std::move(seq);

    // The canonical form deliberately omits output destinations (report_path,
    // wav_output_path) and profile-only knobs (profile_iterations, wait_for_key)
    // so the hash identifies the SCENARIO — same hash ⇒ same audio expected.
    // CLI overrides like --out / --wav don't perturb the hash.
    tao::json::value run(tao::json::empty_object);
    run["mode"] = c.run.mode == RunConfig::Profile ? "profile" : "measure";
    run["warmup_iterations"] = c.run.warmupIterations;
    run["measure_iterations"] = c.run.measureIterations;
    run["tail_silence_s"] = c.run.tailSilenceS;
    // audio_thread_priority is canonical because it materially affects measured
    // timing — realtime vs default ⇒ apples-to-oranges.
    run["audio_thread_priority"] =
        c.run.audioThreadPriority == RunConfig::RealtimePriority ? "realtime" : "default";
    v["run"] = std::move(run);

    return tao::json::to_string(v);
}

uint64_t hashConfig(const Config &c)
{
    auto s = canonicalJson(c);
    return fnv1a(s.data(), s.size());
}

} // namespace scxt::perf
