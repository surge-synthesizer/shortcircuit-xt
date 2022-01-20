#pragma once

#include <cstring>
#include <string>

inline void strncpy_0term(char *Destination, const char *Source, size_t Length)
{
    strncpy(Destination, Source, Length);
    Destination[Length - 1] = 0;
}
