//
// Created by Paul Walker on 4/25/23.
//

#ifndef SHORTCIRCUITXT_PART_FX_H
#define SHORTCIRCUITXT_PART_FX_H

#include <memory>
namespace scxt::engine
{
struct Engine;

struct PartEffectStorage
{
    static constexpr int maxPartEffectParams{12};
    float params[maxPartEffectParams];
};
struct PartEffect
{
    PartEffect(Engine *, PartEffectStorage *, float *) {}
    virtual ~PartEffect() = default;

    virtual void init() = 0;
    virtual void process(float *__restrict L, float *__restrict R) = 0;
};

enum AvailablePartEffects
{
    reverb1,
    flanger
};

std::unique_ptr<PartEffect> createEffect(AvailablePartEffects p, Engine *e, PartEffectStorage *s);
} // namespace scxt::engine

#endif // SHORTCIRCUITXT_PART_FX_H
