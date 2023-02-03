#include "sample_manager.h"

namespace scxt::sample
{
std::optional<SampleID> SampleManager::loadSampleByPath(const fs::path &p)
{
    return loadSampleByPathToID(p, SampleID::next());
}


std::optional<SampleID> SampleManager::loadSampleByPathToID(const fs::path &p, const SampleID &id)
{
    for (const auto &[id, sm] : samples)
    {
        if (sm->getPath() == p)
        {
            return id;
        }
    }

    auto sp = std::make_shared<Sample>(id);

    if (!sp->load(p))
        return {};

    samples[sp->id] = sp;
    return sp->id;
}
} // namespace scxt::sample
