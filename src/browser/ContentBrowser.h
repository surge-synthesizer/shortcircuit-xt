//
// Created by Paul Walker on 1/23/22.
//

#ifndef SHORTCIRCUIT_CONTENTBROWSER_H
#define SHORTCIRCUIT_CONTENTBROWSER_H

#include "filesystem/import.h"
#include <vector>
#include <thread>
#include <mutex>

namespace scxt
{
namespace content
{
/*
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
