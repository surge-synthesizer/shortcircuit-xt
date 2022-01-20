#pragma once
#include "filesystem/import.h"
#include "infrastructure/logfile.h"

int get_sf2_patchlist(const fs::path &filename, void **plist, scxt::log::StreamLogger &logger);
int get_dls_patchlist(const fs::path &filename, void **plist);

struct midipatch
{
    char name[32];
    int PC, bank;
};