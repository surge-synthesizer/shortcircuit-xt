#pragma once
#include "infrastructure/import_fs.h"
#include "infrastructure/logfile.h"

int get_sf2_patchlist(const fs::path &filename, void **plist, SC3::Log::StreamLogger &logger);
int get_dls_patchlist(const fs::path &filename, void **plist);

struct midipatch
{
    char name[32];
    int PC, bank;
};