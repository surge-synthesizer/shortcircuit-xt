//
// Created by Paul Walker on 1/30/23.
//

#ifndef SCXT_SRC_CONFIGURATION_H
#define SCXT_SRC_CONFIGURATION_H

#include <cstdint>

namespace scxt
{
static constexpr uint16_t blockSize{16};
static constexpr uint16_t blockSizeQuad{16 >> 2};
static constexpr uint16_t maxOutputs{16};
static constexpr uint16_t maxVoices{256};

// some battles are not worth it
static constexpr uint16_t BLOCK_SIZE{blockSize};
static constexpr uint16_t BLOCK_SIZE_QUAD{blockSizeQuad};

} // namespace scxt
#endif // __SCXT__CONFIGURATION_H
