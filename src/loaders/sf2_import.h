#pragma once

int get_sf2_patchlist(const wchar_t *filename, void **plist);
int get_dls_patchlist(const wchar_t *filename, void **plist);

struct midipatch
{
    char name[32];
    int PC, bank;
};