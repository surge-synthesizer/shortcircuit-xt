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
#include "infrastructure/import_fs.h"
#include "infrastructure/logfile.h"

const int n_custom_controllers = 16;

enum midi_controller_type
{
    mct_none = 0,
    mct_cc,
    mct_rpn,
    mct_nrpn,
    n_ctypes,
};

const char ct_titles[n_ctypes][8] = {("none"), ("CC"), ("RPN"), ("NRPN")};

struct midi_controller
{
    midi_controller_type type;
    int number;
    char name[16];
};

class configuration
{
    fs::path mRelative;
    fs::path mConfFilename;

  public:
    // TODO probably this doesn't belong here in the object hierarchy
    SC3::Log::StreamLogger &mLogger; // logger which is owned by sampler
    configuration(SC3::Log::StreamLogger &logger);
    bool load(const fs::path &filename);
    bool save(const fs::path &filename);
    // replace <relative> in filename
    fs::path resolve_path(const fs::path &in);
    void set_relative_path(const fs::path &in);
    int stereo_outputs, mono_outputs;
    std::string pathlist[4];
    std::string skindir;
    int headroom;
    int keyboardmode;
    bool store_in_projdir;
    midi_controller MIDIcontrol[n_custom_controllers];
    float mPreviewLevel;
    bool mAutoPreview;
    bool mUseMiniDumper;
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