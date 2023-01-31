//
// Created by Paul Walker on 1/30/23.
//

#ifndef __SCXT__CONFIGURATION_H
#define __SCXT__CONFIGURATION_H

namespace scxt
{
static constexpr int blockSize{16};
static constexpr int blockSizeQuad{16 >> 2};
static constexpr int maxOutputs{16};
static constexpr int maxVoices{256};

// some battles are not worth it
static constexpr int BLOCK_SIZE{blockSize};
static constexpr int BLOCK_SIZE_QUAD{blockSizeQuad};

} // namespace scxt
#endif // __SCXT__CONFIGURATION_H
