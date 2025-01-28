//
// Created by Paul Walker on 1/26/25.
//

#include "sample/compound_file.h"

namespace scxt::sample::compound
{
std::vector<CompoundElement> getSF2SampleAddresses(const fs::path &p)
{
    auto riff = std::make_unique<RIFF::File>(p.u8string());
    auto sf = std::make_unique<sf2::File>(riff.get());

    auto sc = sf->GetSampleCount();
    std::vector<CompoundElement> result;
    result.reserve(sc);
    for (int i = 0; i < sc; ++i)
    {
        auto s = sf->GetSample(i);

        CompoundElement res;
        res.type = CompoundElement::Type::SAMPLE;
        res.name = s->GetName();
        res.sampleAddress = {Sample::SF2_FILE, p, "", -1, -1, i};
        result.push_back(res);
    }
    return result;
}

std::vector<CompoundElement> getSF2InstrumentAddresses(const fs::path &p)
{
    auto riff = std::make_unique<RIFF::File>(p.u8string());
    auto sf = std::make_unique<sf2::File>(riff.get());

    auto ic = sf->GetInstrumentCount();
    std::vector<CompoundElement> result;
    result.reserve(ic);
    for (int i = 0; i < ic; ++i)
    {
        auto in = sf->GetInstrument(i);

        CompoundElement res;
        res.type = CompoundElement::Type::INSTRUMENT;
        res.name = in->GetName();
        res.sampleAddress = {Sample::SF2_FILE, p, "", -1, i, -1};
        result.push_back(res);
    }

    return result;
}
} // namespace scxt::sample::compound
