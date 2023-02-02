//
// Created by Paul Walker on 2/1/23.
//

#include "voice_matrix.h"
#include "engine/zone.h"
#include "voice/voice.h"

namespace scxt::modulation
{
void VoiceModMatrix::clear()
{
    memset(sourcePointers, 0, sizeof(sourcePointers));
    memset(baseValues, 0, sizeof(baseValues));
    memset(modulatedValues, 0, sizeof(modulatedValues));
}

void VoiceModMatrix::snapRoutingFromZone(engine::Zone *z) { routingTable = z->routingTable; }

void VoiceModMatrix::copyBaseValuesFromZone(engine::Zone *z)
{
    for (int i = 0; i < engine::filtersPerZone; ++i)
    {
        baseValues[vmd_Filter1_Mix + i] = z->filterMix[i];
    }
}

void VoiceModMatrix::attachSourcesFromVoice(voice::Voice *v)
{
    std::cout << "Attaching to voice " << v << std::endl;
    sourcePointers[vms_none] = nullptr;
    for (int i = 0; i < engine::lfosPerZone; ++i)
    {
        sourcePointers[vms_LFO1 + i] = &(v->lfos[i].output);
    }
}

void VoiceModMatrix::process()
{
    memcpy(modulatedValues, baseValues, sizeof(modulatedValues));
    for (const auto &r : routingTable)
    {
        if (r.dst == vmd_none || r.src == vms_none)
            continue;
        modulatedValues[r.dst] += (*(sourcePointers[r.src])) * r.depth;
    }
}
} // namespace scxt::modulation