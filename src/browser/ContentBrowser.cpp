//
// Created by Paul Walker on 1/23/22.
//

#include "ContentBrowser.h"
#include <unordered_set>
#include <iostream>
#include <algorithm>

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
    const static auto patchsuffixes = std::unordered_set<std::string>({".sc2p"});
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
    }
}
} // namespace content
} // namespace scxt