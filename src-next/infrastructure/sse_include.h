//
// Created by Paul Walker on 1/30/23.
//

#ifndef SHORTCIRCUIT_SSE_INCLUDE_H
#define SHORTCIRCUIT_SSE_INCLUDE_H

#if defined(__aarch64__)
#define SIMDE_ENABLE_NATIVE_ALIASES
#include "simde/x86/sse2.h"
#else
#include <xmmintrin.h>

#if LINUX
#include <immintrin.h>
#endif
#endif


#endif // SHORTCIRCUIT_SSE_INCLUDE_H
