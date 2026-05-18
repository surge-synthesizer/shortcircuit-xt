/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_CLIENTS_CLI_TOOLS_CHECK_MULTI_LOADABILITY
#define SCXT_SRC_CLIENTS_CLI_TOOLS_CHECK_MULTI_LOADABILITY

#include <algorithm>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>

#include "configuration.h"
#include "console_harness.h"
#include "engine/engine.h"
#include "engine/part.h"
#include "engine/patch.h"
#include "messaging/client/client_messages.h"

namespace cmsg = scxt::messaging::client;

namespace
{
size_t countZones(const scxt::engine::Engine &eng)
{
    size_t n = 0;
    for (int p = 0; p < scxt::numParts; ++p)
    {
        const auto &part = eng.getPatch()->getPart(p);
        for (const auto &grp : *part)
            n += grp->getZones().size();
    }
    return n;
}

bool hasSuffix(const std::string &name, const std::string &suffix)
{
    if (suffix.size() > name.size())
        return false;
    auto a = name.end() - suffix.size();
    return std::equal(a, name.end(), suffix.begin(),
                      [](char x, char y) { return std::tolower(x) == std::tolower(y); });
}

// Collapse trailing CC number into a literal "N" so a histogram doesn't
// fragment across cc1..cc127. e.g. cutoff_oncc73 → cutoff_onccN,
// ampeg_attackcc11 → ampeg_attackccN, locc64 → loccN.
std::string normalizeUnusedKey(const std::string &key)
{
    static const std::regex ccTail{R"(cc\d+$)"};
    return std::regex_replace(key, ccTail, "ccN");
}
} // namespace

int main(int argc, char **argv)
{
    // Extract optional flags (only --unused-per-file for now), keeping positional args in argv.
    std::vector<std::string> args(argv + 1, argv + argc);
    bool unusedPerFile = false;
    args.erase(std::remove_if(args.begin(), args.end(),
                              [&](const std::string &a) {
                                  if (a == "--unused-per-file")
                                  {
                                      unusedPerFile = true;
                                      return true;
                                  }
                                  return false;
                              }),
               args.end());

    if (args.empty())
    {
        std::cerr << "Usage: " << argv[0] << " [--unused-per-file] <file>\n"
                  << "       " << argv[0] << " [--unused-per-file] <directory> <suffix>\n"
                  << "  Single-file mode: loads <file> and reports zone count + errors.\n"
                  << "  Directory mode: recursively loads every file under <directory>\n"
                  << "  whose name ends with <suffix> (case-insensitive, e.g. \".exs\").\n"
                  << "  --unused-per-file: list importer-recorded unused tokens per file in\n"
                  << "      addition to the rolled-up histogram.\n";
        return 1;
    }

    std::vector<fs::path> files;
    if (args.size() == 1)
    {
        fs::path target{args[0]};
        if (!fs::exists(target) || !fs::is_regular_file(target))
        {
            std::cerr << "Not a regular file: " << target << "\n";
            return 1;
        }
        files.push_back(target);
        std::cout << "Loading 1 file: " << target << "\n";
    }
    else
    {
        fs::path root{args[0]};
        std::string suffix{args[1]};
        if (!suffix.empty() && suffix[0] != '.')
            suffix = "." + suffix;

        if (!fs::exists(root) || !fs::is_directory(root))
        {
            std::cerr << "Not a directory: " << root << "\n";
            return 1;
        }

        for (auto &entry :
             fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied))
        {
            if (!entry.is_regular_file())
                continue;
            if (hasSuffix(entry.path().filename().u8string(), suffix))
                files.push_back(entry.path());
        }
        std::sort(files.begin(), files.end());
        std::cout << "Found " << files.size() << " " << suffix << " files under " << root << "\n";
    }

    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    struct FailureRecord
    {
        fs::path path;
        std::vector<scxt::messaging::client::s2cError_t> errors;
    };
    std::vector<FailureRecord> failures;
    std::vector<std::pair<fs::path, size_t>> loaded;

    // (format, normalized-key) → count. Filled from importer recordUnusedItem
    // calls (see ImporterContext::recordUnusedItem).
    std::map<std::pair<std::string, std::string>, int> unusedHisto;

    for (const auto &f : files)
    {
        // Print + flush before send so that a crash inside the importer
        // names the offending file. The OK/FAIL line below will overwrite
        // visually on successful return (different lead char).
        std::cerr << ">>> " << f.u8string() << std::endl;

        th.editor->clearErrors();
        th.editor->clearUnusedItems();
        size_t before = countZones(*th.engine);
        th.sendToSerialization(cmsg::AddSample(f.u8string()));
        th.stepUI(50);
        size_t after = countZones(*th.engine);
        size_t added = after - before;
        auto errs = th.editor->readErrors();
        auto unused = th.editor->readUnusedItems();

        // Aggregate unused tokens (after normalizing cc-suffixes).
        for (const auto &[format, key, value] : unused)
            unusedHisto[{format, normalizeUnusedKey(key)}]++;

        if (added == 0 || !errs.empty())
        {
            failures.push_back({f, errs});
            std::cout << "FAIL " << added << "\t" << f.u8string();
            if (!errs.empty())
                std::cout << "  [" << errs.size() << " error" << (errs.size() == 1 ? "" : "s")
                          << "]";
            std::cout << "\n";
            for (const auto &[title, body, source, line] : errs)
                std::cout << "       └ " << title << ": " << body << "\n";
        }
        else
        {
            loaded.emplace_back(f, added);
            std::cout << "OK   " << added << "\t" << f.u8string() << "\n";
        }

        if (unusedPerFile && !unused.empty())
        {
            // De-dup per file (importer can record the same key in many regions).
            std::map<std::pair<std::string, std::string>, int> perFile;
            for (const auto &[format, key, value] : unused)
                perFile[{format, normalizeUnusedKey(key)}]++;
            for (const auto &[k, c] : perFile)
                std::cout << "       · " << k.first << "  " << k.second << "  (x" << c << ")\n";
        }
    }

    std::cout << "\n=== Summary ===\n"
              << "Total : " << files.size() << "\n"
              << "OK    : " << loaded.size() << "\n"
              << "FAIL  : " << failures.size() << "\n";

    if (!failures.empty())
    {
        std::cout << "\nFailures:\n";
        for (const auto &fr : failures)
        {
            std::cout << "  " << fr.path.u8string() << "\n";
            for (const auto &[title, body, source, line] : fr.errors)
                std::cout << "      " << title << ": " << body << "\n";
        }
    }

    if (!unusedHisto.empty())
    {
        // Sort by descending count, then format, then key.
        std::vector<std::pair<std::pair<std::string, std::string>, int>> sorted(unusedHisto.begin(),
                                                                                unusedHisto.end());
        std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) {
            if (a.second != b.second)
                return a.second > b.second;
            return a.first < b.first;
        });
        std::cout << "\n=== Unused-item frequency (count, format, key) ===\n";
        for (const auto &[k, c] : sorted)
            std::cout << "  " << c << "\t" << k.first << "\t" << k.second << "\n";
    }

    return failures.empty() ? 0 : 2;
}

#endif
