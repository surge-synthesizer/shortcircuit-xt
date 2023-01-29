#include "sample_manager.h"

namespace scxt::sample
{
std::optional<SampleID> SampleManager::loadSampleByPath(const fs::path &p)
{
    auto sp = std::make_shared<Sample>();

    if (!sp->load(p))
        return {};

    samples[sp->id] = sp;
    return sp->id;
}
} // namespace scxt::sample
