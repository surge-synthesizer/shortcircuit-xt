#if WINDOWS
#include "Precompiled_common.h"
#include <Windows.h>

#define forceinline __forceinline
#define restrict __restrict
#define frestrict __declspec(restrict)
#define Align16 _MM_ALIGN16
#define Align64 _CRT_ALIGN(64)
#endif
