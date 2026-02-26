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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_SHARED_PATCHMULTIIO_H
#define SCXT_SRC_SCXT_PLUGIN_APP_SHARED_PATCHMULTIIO_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "patch_io/patch_io.h"
#include "UIHelpers.h"

namespace scxt::ui::app::shared
{
template <typename T>
void doSaveMulti(T *that, std::unique_ptr<juce::FileChooser> &fileChooser,
                 patch_io::SaveStyles style)
{
    namespace cmsg = scxt::messaging::client;

    auto flags = juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::saveMode |
                 juce::FileBrowserComponent::warnAboutOverwriting;

    std::string ext = "*.scm";
    std::string title = "Save Multi";
    if (style == patch_io::SaveStyles::AS_SFZ)
    {
        SCLOG_IF(warnings, "Software error. MULTI as SFZ should not occur");
        return;
    }
    if (style == patch_io::SaveStyles::ONLY_COLLECT)
    {
        flags = juce::FileBrowserComponent::canSelectDirectories;
        title = "Collect Samples";
    }
    fileChooser = std::make_unique<juce::FileChooser>(
        title, juce::File(that->editor->browser.patchIODirectory.u8string()), "*.scm");
    fileChooser->launchAsync(
        flags, [style, w = juce::Component::SafePointer(that)](const juce::FileChooser &c) {
            auto result = c.getResults();
            if (result.isEmpty() || result.size() > 1)
            {
                return;
            }
            // send a 'save multi' message
            auto fsp = juceFileToFSPath(result[0]);
            w->sendToSerialization(cmsg::SaveMulti({fsp.u8string(), (int)style}));
        });
}

template <typename T> void doLoadMulti(T *that, std::unique_ptr<juce::FileChooser> &fileChooser)
{
    namespace cmsg = scxt::messaging::client;

    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Multi", juce::File(that->editor->browser.patchIODirectory.u8string()), "*.scm");
    fileChooser->launchAsync(juce::FileBrowserComponent::canSelectFiles |
                                 juce::FileBrowserComponent::openMode,
                             [w = juce::Component::SafePointer(that)](const juce::FileChooser &c) {
                                 auto result = c.getResults();
                                 if (result.isEmpty() || result.size() > 1)
                                 {
                                     return;
                                 }
                                 auto fsp = juceFileToFSPath(result[0]);
                                 w->sendToSerialization(cmsg::LoadMulti(fsp.u8string()));
                             });
}

template <typename T>
void doSavePart(T *that, std::unique_ptr<juce::FileChooser> &fileChooser, int part,
                patch_io::SaveStyles style)
{
    namespace cmsg = scxt::messaging::client;

    auto flags = juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::saveMode |
                 juce::FileBrowserComponent::warnAboutOverwriting;
    std::string ext = "*.scp";
    std::string title = "Save Part";
    if (style == patch_io::SaveStyles::AS_SFZ)
    {
        ext = "*.sfz";
        title = "Export to SFZ";
    }
    if (style == patch_io::SaveStyles::ONLY_COLLECT)
    {
        flags = juce::FileBrowserComponent::canSelectDirectories;
        title = "Collect Samples";
    }

    fileChooser = std::make_unique<juce::FileChooser>(
        title, juce::File(that->editor->browser.patchIODirectory.u8string()), ext);
    fileChooser->launchAsync(
        flags, [style, part, w = juce::Component::SafePointer(that)](const juce::FileChooser &c) {
            auto result = c.getResults();
            if (result.isEmpty() || result.size() > 1)
            {
                return;
            }
            // send a 'save multi' message
            auto fsp = juceFileToFSPath(result[0]);
            w->sendToSerialization(cmsg::SavePart({fsp.u8string(), part, (int)style}));
        });
}

template <typename T>
void doLoadPartInto(T *that, std::unique_ptr<juce::FileChooser> &fileChooser, int part)
{
    namespace cmsg = scxt::messaging::client;

    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Part", juce::File(that->editor->browser.patchIODirectory.u8string()), "*.scp");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::openMode,
        [part, w = juce::Component::SafePointer(that)](const juce::FileChooser &c) {
            auto result = c.getResults();
            if (result.isEmpty() || result.size() > 1)
            {
                return;
            }
            auto fsp = juceFileToFSPath(result[0]);
            w->sendToSerialization(cmsg::LoadPartInto({fsp.u8string(), part}));
        });
}
} // namespace scxt::ui::app::shared
#endif // PATCHMULTIIO_H
