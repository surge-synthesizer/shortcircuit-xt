/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#ifndef SHORTCIRCUIT_IMPORT_FS_H
#define SHORTCIRCUIT_IMPORT_FS_H

#include <utility>

#if SC3_USE_GHC_FILESYSTEM
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

inline std::string path_to_string(const fs::path &p)
{
#ifdef _WIN32
    return p.u8string();
#else
    return p.generic_string();
#endif
}

template <typename T> inline fs::path string_to_path(T &&s)
{
#ifdef _WIN32
    return fs::u8path(std::forward<T>(s));
#else
    return fs::path(std::forward<T>(s));
#endif
}

#endif // SHORTCIRCUIT_IMPORT_FS_H
