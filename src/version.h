#ifndef __version__
#define __version__

namespace ShortCircuit
{
struct Build
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
};
} // namespace ShortCircuit

#endif //__version__
