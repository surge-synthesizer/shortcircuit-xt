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

#include "SCXTResources.h"
#include "utils.h"
#include "cmrc/cmrc.hpp"

CMRC_DECLARE(scxt_resources);

namespace scxt::ui::connectors::resources
{

juce::Typeface::Ptr loadTypeface(const std::string &path)
{
    try
    {
        auto fs = cmrc::scxt_resources::get_filesystem();
        auto fntf = fs.open(path);
        std::vector<char> fontData(fntf.begin(), fntf.end());

        auto res = juce::Typeface::createSystemTypefaceFor(fontData.data(), fontData.size());
        return res;
    }
    catch (std::exception &e)
    {
    }
    SCLOG_IF(uiTheme, "Font '" << path << "' failed to load");
    return nullptr;
}

std::unique_ptr<juce::Drawable> loadImageDrawable(const std::string &path)
{
    try
    {
        auto fs = cmrc::scxt_resources::get_filesystem();
        auto imf = fs.open(path);
        std::vector<char> imageData(imf.begin(), imf.end());

        auto res = juce::Drawable::createFromImageData(imageData.data(), imageData.size());

        return res;
    }
    catch (std::exception &e)
    {
    }
    SCLOG_IF(uiTheme, "Image '" << path << "' failed to load");
    return nullptr;
}

std::string loadTextFile(const std::string &path)
{
    try
    {
        auto fs = cmrc::scxt_resources::get_filesystem();
        auto txtf = fs.open(path);
        std::string res(txtf.begin(), txtf.end());

        return res;
    }
    catch (std::exception &e)
    {
    }
    SCLOG_IF(uiTheme, "Text File '" << path << "' failed to load");
    return {};
}

} // namespace scxt::ui::connectors::resources
  //