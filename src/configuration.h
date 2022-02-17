//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "globals.h"
#include <string>
#include <filesystem/import.h>
#include "infrastructure/logfile.h"

class configuration
{
    fs::path mRelative;
    fs::path mConfFilename;

  public:
    // TODO probably this doesn't belong here in the object hierarchy
    scxt::log::StreamLogger &mLogger; // logger which is owned by sampler
    configuration(scxt::log::StreamLogger &logger);
    // replace <relative> in filename
    fs::path resolve_path(const fs::path &in);
    void set_relative_path(const fs::path &in);
};

// parse a path into components. All outputs are optional. Example:
// c:\file\name.EXT>1|2
// outputs:
//  out: c:\file\name.EXT (full valid path)
//  extension: ext (extension, lowercased)
//  name_only: name (name without ext)
//  path_only: c:\file (path without file)
//  program_id: 1
//  sample_id: 2
// returns the extension as lowercase
void decode_path(const fs::path &in, fs::path *out, std::string *extension = 0,
                 std::string *name_only = 0, fs::path *path_only = 0, int *program_id = 0,
                 int *sample_id = 0);

// construct a full path from dir and filename and optionally ext
// if ext is supplied it's delimited with .
fs::path build_path(const fs::path &in, const std::string &filename,
                    const std::string &ext = std::string());