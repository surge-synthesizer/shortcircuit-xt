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

#include "SCXTResources.h"
#include "utils.h"
#include "configuration.h"
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