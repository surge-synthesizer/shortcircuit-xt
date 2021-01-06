/*
** ShortCircuit3 is Free and Open Source Software
**
** ShortCircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** ShortCircuit was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made ShortCircuit
** open source in December 2020.
*/

#ifndef SHORTCIRCUIT_IMPORT_FS_H
#define SHORTCIRCUIT_IMPORT_FS_H

#if WINDOWS
#define GHC_USE_STD_FS
#include <filesystem>
namespace fs = std::filesystem;
#else
#ifndef GHC_USE_STD_FS
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif
#endif

inline std::string path_to_string(const fs::path &p )
{
    return p.u8string();
}
template<typename T>
inline fs::path string_to_path(const T& s)
{
    return fs::path{s};
}

#endif // SHORTCIRCUIT_IMPORT_FS_H
