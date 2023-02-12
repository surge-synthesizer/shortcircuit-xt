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

#include "ContentBrowser.h"
#include <unordered_set>
#include <iostream>
#include <algorithm>
#include <sst/plugininfra/strnatcmp.h>

namespace scxt
{
namespace content
{
void ContentBrowser::initialize(const fs::path &userDocumentPath)
{
    {
        auto lg = std::lock_guard(contentMutex);
        contentRoots.push_back(userDocumentPath / "Library");
    }
    rescanContentRoots();
}

void ContentBrowser::addContentRoot(const fs::path &here)
{
    auto lg = std::lock_guard(contentMutex);
}

void ContentBrowser::rescanContentRoots()
{
    auto lg = std::lock_guard(contentMutex);

    root = std::make_unique<Content>();
    root->type = Content::ROOT;
    for (const auto &r : contentRoots)
    {
        if (fs::is_directory(r))
        {
            auto child = std::make_unique<Content>();
            child->type = Content::DIR;
            child->fullPath = r;
            child->displayPath = r.filename();
            child->recursivelyPopulate();
            root->children.push_back(std::move(child));
        }
    }
}

void ContentBrowser::Content::recursivelyPopulate()
{
    if (type != DIR)
        return;

    const static auto samplesuffixes =
        std::unordered_set<std::string>({".wav", ".riff", ".sf2", ".sfz"});
    const static auto patchsuffixes = std::unordered_set<std::string>({".sc2p", ".sc2m"});
    for (const auto &d : fs::directory_iterator(fullPath))
    {
        auto dp = d.path();
        if (fs::is_directory(dp))
        {
            auto child = std::make_unique<Content>();
            child->type = DIR;
            child->fullPath = dp;
            child->displayPath = dp.filename();
            child->recursivelyPopulate();
            children.push_back(std::move(child));
        }
        else
        {
            auto l8 = dp.extension().u8string();
            std::for_each(l8.begin(), l8.end(), [](char &c) { c = ::tolower(c); });
            if (samplesuffixes.find(l8) != samplesuffixes.end())
            {
                auto child = std::make_unique<Content>();
                child->type = SAMPLE;
                child->fullPath = dp;
                child->displayPath = dp.filename();
                children.push_back(std::move(child));
            }
            else if (patchsuffixes.find(l8) != patchsuffixes.end())
            {
                auto child = std::make_unique<Content>();
                child->type = PATCH;
                child->fullPath = dp;
                child->displayPath = dp.filename();
                children.push_back(std::move(child));
            }
        }
        std::sort(children.begin(), children.end(), [](const auto &a, const auto &b) {
            return strnatcasecmp(a->displayPath.u8string().c_str(),
                                 b->displayPath.u8string().c_str()) < 0;
        });
    }
}
} // namespace content
} // namespace scxt