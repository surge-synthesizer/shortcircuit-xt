/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */
#ifndef SCXT_SRC_VERSION_H
#define SCXT_SRC_VERSION_H

namespace scxt
{
struct build
{
    static const char *MajorVersionStr;
    static const int MajorVersionInt;

    static const char *SubVersionStr;
    static const int SubVersionInt;

    static const char *ReleaseNumberStr;
    static const char *ReleaseStr;

    static const char *GitHash;
    static const char *GitBranch;

    static const char *BuildNumberStr;

    static const char *FullVersionStr;
    static const char *BuildHost;
    static const char *BuildArch;

    static const char *BuildLocation; // Local or Pipeline

    static const char *BuildDate;
    static const char *BuildTime;
    static const char *BuildYear;

    // Some features from cmake
    static const char *CMAKE_INSTALL_PREFIX;
    static const char *BuildCompiler;
};
} // namespace scxt

#endif //__version__
