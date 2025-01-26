//
// Created by Paul Walker on 1/26/25.
//

#ifndef COMPOUND_FILE_H
#define COMPOUND_FILE_H

#include "sample/sample.h"

namespace scxt::engine
{
struct Engine;
}

namespace scxt::sample::compound
{
struct CompoundElement
{
    enum Type : int32_t
    {
        SAMPLE,
        INSTRUMENT
    } type{SAMPLE};

    std::string name;
    Sample::SampleFileAddress sampleAddress;
};

std::vector<CompoundElement> getSF2SampleAddresses(const fs::path &);
std::vector<CompoundElement> getSF2InstrumentAddresses(const fs::path &);

} // namespace scxt::sample::compound
#endif // COMPOUND_FILE_H
