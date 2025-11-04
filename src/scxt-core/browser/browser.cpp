/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#include "browser.h"
#include "browser_db.h"
#include "utils.h"
#include "sample/compound_file.h"

#include "browser_db_impl.h"

namespace scxt::browser
{
Browser::Browser(BrowserDB &db, const infrastructure::DefaultsProvider &dp, const fs::path &ud,
                 errorReporter_t er)
    : browserDb(db), defaultsProvider(dp), userDirectory(ud), errorReporter(er)
{
    auto create = [&](const auto &l) -> fs::path {
        try
        {
            auto p = userDirectory / l;
            if (!fs::is_directory(p))
                fs::create_directories(p);
            return p;
        }
        catch (const fs::filesystem_error &e)
        {
            errorReporter(std::string("Unable to create ") + l + " directory", e.what());
        }
        return {};
    };
    patchIODirectory = create("Patches");
    themeDirectory = create("Themes");
    modulatorPresetDirectory = create("Modulator Presets");
}

std::vector<Browser::indexedRootPath_t> Browser::getRootPathsForDeviceView() const
{
    // TODO - append local favorites
    auto osdef = getOSDefaultRootPathsForDeviceView();
    std::vector<Browser::indexedRootPath_t> res;
    for (const auto &[p, s] : osdef)
        res.emplace_back(p, s, false);
    res.emplace_back(patchIODirectory, "User Patches", true);
    auto fav = browserDb.getBrowserLocations();
    for (const auto &p : fav)
        res.emplace_back(p.first, p.first.filename().u8string(), p.second);

    return res;
}

const std::vector<std::string> Browser::LoadableFile::singleSample{".wav", ".flac", ".mp3", ".aif",
                                                                   ".aiff"};

const std::vector<std::string> Browser::LoadableFile::multiSample{".sf2", ".sfz", ".multisample",
                                                                  ".exs", ".gig"};
// ".exs"};
const std::vector<std::string> Browser::LoadableFile::shortcircuitFormats{".scm", ".scp"};

bool Browser::isLoadableFile(const fs::path &p)
{
    return isLoadableSample(p) || isShortCircuitFormatFile(p);
}
bool Browser::isLoadableSample(const fs::path &p)
{
    return isLoadableSingleSample(p) || isLoadableMultiSample(p);
}
bool Browser::isLoadableSingleSample(const fs::path &p)
{
    return std::any_of(LoadableFile::singleSample.begin(), LoadableFile::singleSample.end(),
                       [p](auto e) { return extensionMatches(p, e); });
}
bool Browser::isLoadableMultiSample(const fs::path &p)
{
    return std::any_of(LoadableFile::multiSample.begin(), LoadableFile::multiSample.end(),
                       [p](auto e) { return extensionMatches(p, e); });
}

bool Browser::isShortCircuitFormatFile(const fs::path &p)
{
    return std::any_of(LoadableFile::shortcircuitFormats.begin(),
                       LoadableFile::shortcircuitFormats.end(),
                       [p](auto e) { return extensionMatches(p, e); });
}

void Browser::addRootPathForDeviceView(const fs::path &p, bool indexed,
                                       messaging::MessageController &mc)
{
    browserDb.addRemoveBrowserLocation(p, indexed, true);
    browserDb.addClientToSerialCallback(messaging::client::RequestBrowserUpdate{true});
}

void Browser::removeRootPathForDeviceView(const fs::path &p, messaging::MessageController &mc)
{
    browserDb.addRemoveBrowserLocation(p, false, false);
    browserDb.addClientToSerialCallback(messaging::client::RequestBrowserUpdate{true});
}

void Browser::reindexLocation(const fs::path &p) { browserDb.reindexLocation(p); }

std::vector<sample::compound::CompoundElement> Browser::expandForBrowser(const fs::path &p)
{
    if (extensionMatches(p, ".sf2"))
    {
        auto inst = sample::compound::getSF2InstrumentAddresses(p);
        auto smp = sample::compound::getSF2SampleAddresses(p);
        inst.insert(inst.end(), smp.begin(), smp.end());

        return inst;
    }
    if (extensionMatches(p, ".sfz"))
    {
        return sample::compound::getSFZCompoundList(p);
    }
    if (extensionMatches(p, ".gig"))
    {
        return sample::compound::getGIGCompoundList(p);
    }
    return {};
}
} // namespace scxt::browser
