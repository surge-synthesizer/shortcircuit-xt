/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#pragma once

#if defined(__aarch64__)
#define SIMDE_ENABLE_NATIVE_ALIASES
#include "simde/x86/sse2.h"
#else
#include <xmmintrin.h>

#if LINUX
#include <immintrin.h>
#endif
#endif

#include <cmath>
#include <cstdint>

static constexpr uint32_t BLOCK_SIZE = 32; // must be a multiple of 4 (SIMD)
static constexpr uint32_t BLOCK_SIZE_QUAD = BLOCK_SIZE >> 2;
static constexpr float INV_BLOCK_SIZE = 1.f / float(BLOCK_SIZE);
static constexpr float INV_2BLOCK_SIZE = 1.f / float(BLOCK_SIZE << 1);
static const __m128 INV_BLOCK_SIZE_128 = _mm_set1_ps(INV_BLOCK_SIZE);
static const __m128 INV_2BLOCK_SIZE_128 = _mm_set1_ps(INV_2BLOCK_SIZE);

static constexpr uint32_t MAX_VOICES = 256;

static constexpr uint32_t MAX_SAMPLES = 2048;
static constexpr uint32_t MAX_ZONES = 2048;
static constexpr uint32_t MAX_GROUPS = 64;
static constexpr uint32_t MAX_OUTPUTS = 8; // stereo outs
static constexpr uint32_t N_SAMPLER_PARTS = 16;
static constexpr uint32_t N_SAMPLER_EFFECTS = 8;
static constexpr uint32_t N_AUTOMATION_PARAMETERS = 32;
static constexpr uint32_t N_MUTE_GROUPS = 64;
static constexpr uint32_t PATHLENGTH = 1024;

static constexpr float PI_1 = 3.1415926535898f;
static constexpr float PI_2 = 6.2831853071796f;
static constexpr float FILTER_FREQRANGE = 20000.0f; // Hz
extern float samplerate;
extern float samplerate_inv;
extern float multiplier_freq2omega;

// directory under user's profile where sc3 config file(s) will be sought
// this should end up being same location that juce stores it's standalone app settings
// TODO AS for juce wrappers, we may consider migrating to using juce's cross platform
//  facility for loading/saving. see stochas configSerialization
static constexpr char SCXT_CONFIG_DIRECTORY[] = "ShortCircuitXT";

#if !WINDOWS
#include <cstring>
#if LINUX
#include <strings.h>
#endif

#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif
