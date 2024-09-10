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
#ifndef SCXT_SRC_UTILS_H
#define SCXT_SRC_UTILS_H

#include "fmt/core.h"
#include <string>
#include <sstream>
#include <cctype>
#include <atomic>
#include <functional>
#include <filesystem>
#include <thread>

#include <iostream>
#include <map>
#include <thread>
#include "unordered_map"
#include "filesystem/import.h"
#include <cassert>

namespace scxt
{
/**
 * This is just a little class to make sure we can have IDs for all of our items
 * which are quick to copy but type differentiated so we don't accidentally use
 * a ZoneID on a Sample or some such.
 */
template <int32_t idType_v> struct ID
{
    int32_t id{-1};
    int32_t idType{idType_v};

    static int32_t nextID;
    bool isValid() { return id > 0; }
    static ID<idType_v> next()
    {
        ID<idType_v> res;
        res.id = nextID;
        nextID++;
        return res;
    }

    static void guaranteeNextAbove(const ID<idType_v> &other)
    {
        if (nextID <= other.id)
            nextID = other.id + 1;
    }

    bool operator==(const ID<idType_v> &other) const { return (id == other.id); }
    bool operator!=(const ID<idType_v> &other) const { return !(id == other.id); }

    const std::string to_string() const { return fmt::format("{}::{}", display_name(), id); }
    const std::string display_name() const { return "ERROR(" + std::to_string(idType) + ")"; }
};

template <int idType> int32_t ID<idType>::nextID{1};

typedef ID<2> PatchID;
template <> inline const std::string PatchID::display_name() const { return "Patch"; }
typedef ID<3> PartID;
template <> inline const std::string PartID::display_name() const { return "Part"; }
typedef ID<4> GroupID;
template <> inline const std::string GroupID::display_name() const { return "Group"; }
typedef ID<5> ZoneID;
template <> inline const std::string ZoneID::display_name() const { return "Zone"; }
typedef ID<6> SampleID;
template <> inline const std::string SampleID::display_name() const { return "Sample"; }
typedef ID<7> EngineID;
template <> inline const std::string EngineID::display_name() const { return "Engine"; }

/**
 * A class which forces move semantics and optionally runs a naive leak detector
 *
 * @tparam T - use this in the standard mixin/CRTP way namely struct A : MoveableOnly<A>
 */
template <typename T> class MoveableOnly
{
  public:
    MoveableOnly(MoveableOnly &&) = default;
    MoveableOnly(const MoveableOnly &) = delete;
    MoveableOnly &operator=(const MoveableOnly &) = delete;

  protected:
    MoveableOnly() {}
    ~MoveableOnly() = default;
};

struct SCXTError : std::runtime_error
{
    SCXTError(const std::string &w) : std::runtime_error(w) {}
};
struct SampleRateSupport
{
    void setSampleRate(double sr)
    {
        if (sr == sampleRate)
            return;

        sampleRate = sr;
        sampleRateInv = 1.0 / sr;
        sync();
    }
    void setSampleRate(double sr, double sri)
    {
        if (sr == sampleRate)
            return;

        sampleRate = sr;
        sampleRateInv = sri;
        sync();
    }
    double getSampleRate() const { return sampleRate; }
    double getSampleRateInv() const { return sampleRateInv; }

    void sync()
    {
        samplerate = sampleRate;
        samplerate_inv = sampleRateInv;
        dsamplerate = sampleRate;
        dsamplerate_inv = sampleRateInv;
        onSampleRateChanged();
    }

    inline bool isSampleRateSet() { return sampleRate > 2; }
    inline void assertSampleRateSet() { assert(sampleRate > 2); }
    virtual void onSampleRateChanged() {}
    double dsamplerate, dsamplerate_inv;

    double sampleRate{1}, sampleRateInv{1};

    // TODO remove these aliase
    double samplerate, samplerate_inv;
};

struct ThreadingChecker
{
    // TODO: Remove this workaround once I have startup working
    std::atomic<bool> bypassThreadChecks{false};

    std::thread::id clientThreadId{}, serialThreadId{}, audioThreadId{};
    void registerAsClientThread() { clientThreadId = std::this_thread::get_id(); }
    inline bool isClientThread() const
    {
#if BUILD_IS_DEBUG
        return (bypassThreadChecks || clientThreadId == std::thread::id() ||
                clientThreadId == std::this_thread::get_id());
#else
        return true;
#endif
    }
    void registerAsSerialThread() { serialThreadId = std::this_thread::get_id(); }
    inline bool isSerialThread() const
    {
#if BUILD_IS_DEBUG
        return (bypassThreadChecks || serialThreadId == std::thread::id() ||
                serialThreadId == std::this_thread::get_id());
#else
        return true;
#endif
    }
    void registerAsAudioThread() { audioThreadId = std::this_thread::get_id(); }
    inline bool isAudioThread() const
    {
#if BUILD_IS_DEBUG
        return (bypassThreadChecks || audioThreadId == std::thread::id() ||
                audioThreadId == std::this_thread::get_id());
#else
        return true;
#endif
    }

    std::string threadName()
    {
#if BUILD_IS_DEBUG
        auto tid = std::this_thread::get_id();
        if (tid == audioThreadId)
            return "audio";
        if (tid == serialThreadId)
            return "serial";
        if (tid == clientThreadId)
            return "client";
        return "unknown";
#else
        return {};
#endif
    }
};

void postToLog(const std::string &s);
std::string getFullLog();
std::string logTimestamp();
#define SCLOG(...)                                                                                 \
    {                                                                                              \
        std::ostringstream oss_macr;                                                               \
        oss_macr << __FILE__ << ":" << __LINE__ << " [" << scxt::logTimestamp() << "] "            \
                 << __VA_ARGS__ << "\n";                                                           \
        scxt::postToLog(oss_macr.str());                                                           \
    }

#define SCLOG_ONCE(...)                                                                            \
    {                                                                                              \
        static bool x842132{false};                                                                \
        if (!x842132)                                                                              \
        {                                                                                          \
            std::ostringstream oss_macr;                                                           \
            oss_macr << __FILE__ << ":" << __LINE__ << " [" << scxt::logTimestamp() << "] "        \
                     << __VA_ARGS__ << " (Message will only appear once)\n";                       \
            scxt::postToLog(oss_macr.str());                                                       \
        }                                                                                          \
        x842132 = true;                                                                            \
    }

#define SCLOG_IF(x, ...)                                                                           \
    {                                                                                              \
        if constexpr (scxt::log::x)                                                                \
        {                                                                                          \
            SCLOG("[" << #x << "] " << __VA_ARGS__);                                               \
        }                                                                                          \
    }

#define SCLOG_WFUNC(...) SCLOG(__func__ << " " << __VA_ARGS__)
#define SCLOG_UNIMPL(...) SCLOG("\033[1;33mUnimpl [" << __func__ << "]\033[0m " << __VA_ARGS__);
#define SCLOG_UNIMPL_ONCE(...) SCLOG_ONCE("Unimpl [" << __func__ << "] " << __VA_ARGS__);
#define SCD(x) #x << "=" << (x) << " "

#define DECLARE_ENUM_STRING(E)                                                                     \
    static std::string toString##E(const E &);                                                     \
    static E fromString##E(const std::string &);

template <typename E, std::string (*F)(const E &)>
inline std::unordered_map<std::string, E> makeEnumInverse(const E &from, const E &to)
{
    std::unordered_map<std::string, E> res;
    for (auto i = (int32_t)from; i <= to; ++i)
    {
        res[F((E)i)] = (E)i;
    }
    return res;
}

void printStackTrace(int frameDepth = -1);

inline bool extensionMatches(const fs::path &p, const std::string &s)
{
    auto pes = p.extension().u8string();
    if (pes.size() != s.size())
        return false;

    auto cic = [](const auto &c1, const auto &c2) {
        if (c1 == c2)
            return true;
        else if (std::toupper(c1) == std::toupper(c2))
            return true;
        return false;
    };
    return std::equal(pes.begin(), pes.end(), s.begin(), cic);
}

inline std::string humanReadableVersion(uint64_t v)
{
    return fmt::format("{:04x}-{:02x}-{:02x}", (v >> 16) & 0xFFFF, (v >> 8) & 0xFF, v & 0xFF);
}
} // namespace scxt

// Make the ID hashable so we can use it as a map key
template <int idType> struct std::hash<scxt::ID<idType>>
{
    size_t operator()(const scxt::ID<idType> &v) const { return std::hash<int>()(v.id); }
};
#endif