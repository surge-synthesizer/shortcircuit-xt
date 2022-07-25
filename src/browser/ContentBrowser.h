/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#ifndef SHORTCIRCUIT_CONTENTBROWSER_H
#define SHORTCIRCUIT_CONTENTBROWSER_H

#include "filesystem/import.h"
#include <vector>
#include <thread>
#include <mutex>

/**
 * This is the one class still used from the old vt_gui stuff.
 */
struct database_samplelist
{
    char name[64];
    int id;
    size_t size;
    int refcount;
    int type; // hddref = 0, embedded = 1...
};

namespace scxt
{
namespace content
{

/**
 * The ContentBrowser is a replacement for the browser database vt_gui object
 * reworked with more, well, filesystemy goodness and stuff in mind.
 * It is thread safe, but at a cost which is it takes internal locks on rescans
 * and the like, so be careful with the thread conventions. But we only have two
 * threads so it's not *that* hard.
 */
struct ContentBrowser
{
    ContentBrowser() {}

    void initialize(const fs::path &userDocumentPath);
    void addContentRoot(const fs::path &here);
    void rescanContentRoots();

    std::vector<fs::path> contentRoots;

    struct Content
    {
        enum Type
        {
            ROOT,
            DIR,
            SAMPLE,
            PATCH
        } type;
        fs::path fullPath;
        fs::path displayPath;
        std::vector<std::unique_ptr<Content>> children;

        void recursivelyPopulate();
    };

    /*
     * This is only updated on init or by a user action from the wrapper
     */
    std::unique_ptr<Content> root;

    std::mutex contentMutex;
};
} // namespace content
} // namespace scxt

#endif // SHORTCIRCUIT_CONTENTBROWSER_H
