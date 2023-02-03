#ifndef __SCXT_ENGINE_IDS_H
#define __SCXT_ENGINE_IDS_H

#include "fmt/core.h"
#include <string>
#include <cctype>
#include <atomic>
#include <functional>

#include <iostream>
#include <map>

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

#define USE_SIMPLE_LEAK_DETECTOR 1
#if USE_SIMPLE_LEAK_DETECTOR
extern std::map<std::string, std::pair<int, int>> allocLog;
void showLeakLog();
#endif

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
    MoveableOnly()
    {
#if USE_SIMPLE_LEAK_DETECTOR
        leakDetect(+1);
#endif
    };
    ~MoveableOnly()
    {
#if USE_SIMPLE_LEAK_DETECTOR
        leakDetect(-1);
#endif
    }
#if USE_SIMPLE_LEAK_DETECTOR
    void leakDetect(int dir)
    {
        auto tn = typeid(T).name();
        if (dir == 1)
            allocLog[tn].first++;
        else if (dir == -1)
            allocLog[tn].second++;
    }
#endif
};

struct SCXTError : std::runtime_error
{
    SCXTError(const std::string &w) : std::runtime_error(w) {}
};
struct SampleRateSupport
{
    void setSampleRate(double sr)
    {
        sampleRate = sr;
        sampleRateInv = 1.0 / sr;
        sync();
    }
    void setSampleRate(double sr, double sri)
    {
        sampleRate = sr;
        sampleRateInv = sri;
        sync();
    }

    void sync()
    {
        samplerate = sampleRate;
        samplerate_inv = sampleRateInv;
    }

  protected:
    double sampleRate{1}, sampleRateInv{1};

    // TODO remove these aliase
    double samplerate, samplerate_inv;
};

} // namespace scxt

// Make the ID hashable so we can use it as a map key
template <int idType> struct std::hash<scxt::ID<idType>>
{
    size_t operator()(const scxt::ID<idType> &v) const { return std::hash<int>()(v.id); }
};
#endif