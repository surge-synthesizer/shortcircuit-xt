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

    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Multi", juce::File(that->editor->browser.patchIODirectory.u8string()), "*.scm");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::saveMode |
            juce::FileBrowserComponent::warnAboutOverwriting,
        [style, w = juce::Component::SafePointer(that)](const juce::FileChooser &c) {
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

    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Selected Part", juce::File(that->editor->browser.patchIODirectory.u8string()),
        "*.scp");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::saveMode |
            juce::FileBrowserComponent::warnAboutOverwriting,
        [style, part, w = juce::Component::SafePointer(that)](const juce::FileChooser &c) {
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
