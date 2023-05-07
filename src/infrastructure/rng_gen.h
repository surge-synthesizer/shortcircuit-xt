//
// Created by Paul Walker on 5/7/23.
//

#include <random>
#include <chrono>

#ifndef SHORTCIRCUITXT_RNG_GEN_H
#define SHORTCIRCUITXT_RNG_GEN_H

namespace scxt::infrastructure
{
struct RNGGen
{
    RNGGen()
        : g(std::chrono::system_clock::now().time_since_epoch().count()), pm1(-1.f, 1.f),
          z1(0.f, 1.f), u32(0, 0xFFFFFFFF)
    {
    }

    inline float rand01() { return z1(g); }
    inline float randPM1() { return pm1(g); }
    inline uint32_t randU32() { return u32(g); }

  private:
    std::minstd_rand g;
    std::uniform_real_distribution<float> pm1, z1;
    std::uniform_int_distribution<uint32_t> u32;
};
} // namespace scxt::infrastructure

#endif // SHORTCIRCUITXT_RNG_GEN_H
