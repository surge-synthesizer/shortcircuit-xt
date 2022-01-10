//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004-2006 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------
#pragma once

#if defined(__aarch64__)
#define SIMDE_ENABLE_NATIVE_ALIASES
#include "simde/x86/sse2.h"

// TODO: FixMe on all of these
#define _mm_malloc(a, b) malloc(a)
#define _mm_free(a) free(a)
#else
#include <xmmintrin.h>

#if LINUX
#include <immintrin.h>
#endif
#endif

#include <cmath>
#include <cstdint>

typedef uint32_t uint32;

const uint32 block_size = 32; // must be a multiple of 4 (SIMD)
const uint32 block_size_quad = block_size >> 2;
const float inv_block_size = 1.f / float(block_size);
const float inv_2block_size = 1.f / float(block_size << 1);
const __m128 inv_block_size_128 = _mm_set1_ps(inv_block_size);
const __m128 inv_2block_size_128 = _mm_set1_ps(inv_2block_size);

#ifdef SCFREE
const uint32 max_voices = 4;
#else
const uint32 max_voices = 256;
#endif

const uint32 max_samples = 2048;
const uint32 max_zones = 2048;
const uint32 max_groups = 64;
const uint32 max_outputs = 8; // stereo outs
// extern uint32 n_outputs;
const uint32 n_sampler_parts = 16;
const uint32 n_sampler_effects = 8;
const uint32 n_automation_parameters = 32;
const uint32 n_mute_groups = 64;
const unsigned int pathlength = 1024;

const float pi1 = 3.1415926535898f;
const float pi2 = 6.2831853071796f;
const float filter_freqrange = 20000.0f; // Hz
extern float samplerate;
extern float samplerate_inv;
extern float multiplier_freq2omega;

// directory under user's profile where sc3 config file(s) will be sought
// this should end up being same location that juce stores it's standalone app settings
// TODO AS for juce wrappers, we may consider migrating to using juce's cross platform
//  facility for loading/saving. see stochas configSerialization
const char SCXT_CONFIG_DIRECTORY[] = "ShortCircuitXT";

#if !WINDOWS
#include "windows_compat.h"
#endif
