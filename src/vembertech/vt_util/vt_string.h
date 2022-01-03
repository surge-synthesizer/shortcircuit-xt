#pragma once

#include <string>

void vtCopyString(char *Destination, const char *Source, size_t Length);
void vtCopyStringW(wchar_t *Destination, const wchar_t *Source, size_t Length);
void vtWStringToString(char *Destination, const wchar_t *Source, size_t Length);
void vtStringToWString(wchar_t *Destination, const char *Source, size_t Length);
