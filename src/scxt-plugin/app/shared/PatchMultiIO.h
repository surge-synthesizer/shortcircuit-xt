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
#include <functional>
#include <optional>
#include <string>
#include "patch_io/patch_io.h"
#include "messaging/client/patch_io_messages.h"
#include "infrastructure/user_defaults.h"
#include "UIHelpers.h"

namespace scxt::ui::app::shared
{
// Save choosers open where the user last saved, not always in the user patch
// directory (#1839). Falls back to the patch directory if unset or stale.
template <typename T> juce::File lastSaveDirectory(T *that)
{
    auto lp = that->editor->defaultsProvider.getUserDefaultPath(
        infrastructure::DefaultKeys::lastSavedPath, fs::path{});
    if (!lp.empty())
    {
        auto f = fsPathToJuceFile(lp);
        if (f.isDirectory())
            return f;
    }
    return fsPathToJuceFile(that->editor->browser.patchIODirectory);
}

// The folder the item came from, if it still exists, else where we last saved
template <typename T>
juce::File currentFolderFor(T *that, const selection::SelectionManager::PatchFile &pf)
{
    if (!pf.path.empty())
    {
        auto f = fsPathToJuceFile(pf.path.parent_path());
        if (f.isDirectory())
            return f;
    }
    return lastSaveDirectory(that);
}

// The chooser opens on the name the item already has, so Save As is a confirm
inline juce::File startFileIn(const juce::File &dir, const std::string &name,
                              const std::string &ext)
{
    auto stem = scxt::sanitizeFilename(name);
    if (stem.empty())
        return dir;
    auto dotted = ext;
    if (!dotted.empty() && dotted.front() == '*')
        dotted = dotted.substr(1);
    return dir.getChildFile(juce::String::fromUTF8((stem + dotted).c_str()));
}

template <typename T> void rememberSaveDirectory(T *that, const juce::File &result)
{
    // In directory-select mode the result is the directory; in save mode it is the
    // (not yet created) target file.
    auto dir = result.isDirectory() ? result : result.getParentDirectory();
    that->editor->defaultsProvider.updateUserDefaultPath(infrastructure::DefaultKeys::lastSavedPath,
                                                         juceFileToFSPath(dir));
}

inline std::string multiExtension() { return "*.scm"; }

inline std::string partExtension(patch_io::SaveStyles style)
{
    return style == patch_io::SaveStyles::AS_SFZ ? "*.sfz" : "*.scp";
}

template <typename T>
void doSaveMulti(T *that, std::unique_ptr<juce::FileChooser> &fileChooser,
                 patch_io::SaveStyles style)
{
    namespace cmsg = scxt::messaging::client;

    auto flags = juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::saveMode |
                 juce::FileBrowserComponent::warnAboutOverwriting;

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
    auto dir = currentFolderFor(that, that->editor->patchFiles.multi);
    auto start = style == patch_io::SaveStyles::ONLY_COLLECT
                     ? dir
                     : startFileIn(dir, that->editor->patchFiles.multiName, multiExtension());
    fileChooser = std::make_unique<juce::FileChooser>(title, start, multiExtension());
    fileChooser->launchAsync(
        flags, [style, w = juce::Component::SafePointer(that)](const juce::FileChooser &c) {
            if (!w)
                return;
            auto result = c.getResults();
            if (result.isEmpty() || result.size() > 1)
            {
                return;
            }
            rememberSaveDirectory(w.getComponent(), result[0]);
            // send a 'save multi' message
            auto fsp = juceFileToFSPath(result[0]);
            // ONLY_COLLECT picks a directory, every other style names a file
            if (style != patch_io::SaveStyles::ONLY_COLLECT)
                fsp = scxt::guaranteeExtension(fsp, multiExtension());
            w->sendToSerialization(cmsg::SaveMulti({fsp.u8string(), (int)style}));
        });
}

template <typename T> void doLoadMulti(T *that, std::unique_ptr<juce::FileChooser> &fileChooser)
{
    namespace cmsg = scxt::messaging::client;

    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Multi", currentFolderFor(that, that->editor->patchFiles.multi), "*.scm");
    fileChooser->launchAsync(juce::FileBrowserComponent::canSelectFiles |
                                 juce::FileBrowserComponent::openMode,
                             [w = juce::Component::SafePointer(that)](const juce::FileChooser &c) {
                                 if (!w)
                                     return;
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
    std::string title = "Save Part";
    if (style == patch_io::SaveStyles::AS_SFZ)
    {
        title = "Export to SFZ";
    }
    if (style == patch_io::SaveStyles::ONLY_COLLECT)
    {
        flags = juce::FileBrowserComponent::canSelectDirectories;
        title = "Collect Samples";
    }

    auto dir = currentFolderFor(that, that->editor->patchFiles.parts[part]);
    auto start = style == patch_io::SaveStyles::ONLY_COLLECT
                     ? dir
                     : startFileIn(dir, that->editor->partNames[part].name, partExtension(style));
    fileChooser = std::make_unique<juce::FileChooser>(title, start, partExtension(style));
    fileChooser->launchAsync(
        flags, [style, part, w = juce::Component::SafePointer(that)](const juce::FileChooser &c) {
            if (!w)
                return;
            auto result = c.getResults();
            if (result.isEmpty() || result.size() > 1)
            {
                return;
            }
            rememberSaveDirectory(w.getComponent(), result[0]);
            // send a 'save multi' message
            auto fsp = juceFileToFSPath(result[0]);
            // ONLY_COLLECT picks a directory, every other style names a file
            if (style != patch_io::SaveStyles::ONLY_COLLECT)
                fsp = scxt::guaranteeExtension(fsp, partExtension(style));
            w->sendToSerialization(cmsg::SavePart({fsp.u8string(), part, (int)style}));
        });
}

/*
 * Save with no dialog: the item is written back to its own folder under its own
 * name. A rename since the last save lands as a new file beside it, which is what
 * Kontakt does, so an existing file of that name needs a nod first.
 */
template <typename T>
void doSaveInPlace(T *that, const fs::path &target, bool isCurrent, const std::string &what,
                   std::function<void()> send)
{
    if (isCurrent || !fs::exists(target))
    {
        send();
        return;
    }
    that->editor->promptOKCancel(
        "Overwrite " + what,
        target.filename().u8string() + " already exists in this folder. Overwrite it?", send);
}

// nullopt when there is nowhere to write yet and the caller has to open a dialog
inline std::optional<fs::path> inPlaceTarget(const selection::SelectionManager::PatchFile &pf,
                                             const std::string &name, const std::string &ext)
{
    auto stem = scxt::sanitizeFilename(name);
    if (pf.path.empty() || stem.empty())
        return std::nullopt;
    auto dir = pf.path.parent_path();
    try
    {
        if (!fs::is_directory(dir))
            return std::nullopt;
    }
    catch (const fs::filesystem_error &)
    {
        return std::nullopt;
    }
    return scxt::guaranteeExtension(dir / stem, ext);
}

template <typename T> void doSaveMultiInPlace(T *that, std::unique_ptr<juce::FileChooser> &fc)
{
    namespace cmsg = scxt::messaging::client;

    const auto &pf = that->editor->patchFiles.multi;
    // a monolith is read back from the file we would be replacing, so ask where
    // to put it until that is solved
    if (pf.monolith)
        return doSaveMulti(that, fc, patch_io::SaveStyles::AS_MONOLITH);

    auto target = inPlaceTarget(pf, that->editor->patchFiles.multiName, multiExtension());
    if (!target)
        return doSaveMulti(that, fc, patch_io::SaveStyles::NO_SAMPLES);

    doSaveInPlace(that, *target, *target == pf.path, "Multi",
                  [w = juce::Component::SafePointer(that), t = *target]() {
                      if (w)
                          w->sendToSerialization(cmsg::SaveMulti(
                              {t.u8string(), (int)patch_io::SaveStyles::NO_SAMPLES}));
                  });
}

template <typename T>
void doSavePartInPlace(T *that, std::unique_ptr<juce::FileChooser> &fc, int part)
{
    namespace cmsg = scxt::messaging::client;

    const auto &pf = that->editor->patchFiles.parts[part];
    if (pf.monolith)
        return doSavePart(that, fc, part, patch_io::SaveStyles::AS_MONOLITH);

    auto target = inPlaceTarget(pf, that->editor->partNames[part].name,
                                partExtension(patch_io::SaveStyles::NO_SAMPLES));
    if (!target)
        return doSavePart(that, fc, part, patch_io::SaveStyles::NO_SAMPLES);

    doSaveInPlace(that, *target, *target == pf.path, "Part",
                  [w = juce::Component::SafePointer(that), t = *target, part]() {
                      if (w)
                          w->sendToSerialization(cmsg::SavePart(
                              {t.u8string(), part, (int)patch_io::SaveStyles::NO_SAMPLES}));
                  });
}

template <typename T>
void doLoadPartInto(T *that, std::unique_ptr<juce::FileChooser> &fileChooser, int part)
{
    namespace cmsg = scxt::messaging::client;

    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Part", currentFolderFor(that, that->editor->patchFiles.parts[part]), "*.scp");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::openMode,
        [part, w = juce::Component::SafePointer(that)](const juce::FileChooser &c) {
            if (!w)
                return;
            auto result = c.getResults();
            if (result.isEmpty() || result.size() > 1)
            {
                return;
            }
            auto fsp = juceFileToFSPath(result[0]);
            w->sendToSerialization(cmsg::LoadPartInto({fsp.u8string(), part}));
        });
}
// The part I/O items, shared by the card, the PARTS hamburger and the disk menu
template <typename T>
void populatePartIOMenu(T *that, juce::PopupMenu &p, int part, bool withDeactivate = true)
{
    namespace cmsg = scxt::messaging::client;

    auto mono = that->editor->patchFiles.parts[part].monolith;
    p.addItem("Save Part", !mono, false, [w = juce::Component::SafePointer(that), part]() {
        if (w)
            doSavePartInPlace(w.getComponent(), w->fileChooser, part);
    });
    p.addItem("Save Part As...", [w = juce::Component::SafePointer(that), part]() {
        if (w)
            doSavePart(w.getComponent(), w->fileChooser, part, patch_io::SaveStyles::NO_SAMPLES);
    });
    p.addItem("Save Part as Monolith...", [w = juce::Component::SafePointer(that), part]() {
        if (w)
            doSavePart(w.getComponent(), w->fileChooser, part, patch_io::SaveStyles::AS_MONOLITH);
    });
    p.addItem("Save Part with Collected Samples...",
              [w = juce::Component::SafePointer(that), part]() {
                  if (w)
                      doSavePart(w.getComponent(), w->fileChooser, part,
                                 patch_io::SaveStyles::WITH_COLLECTED_SAMPLES);
              });
    p.addSeparator();
    p.addItem("Load Part...", [w = juce::Component::SafePointer(that), part]() {
        if (w)
            doLoadPartInto(w.getComponent(), w->fileChooser, part);
    });
    if (withDeactivate)
    {
        p.addSeparator();
        p.addItem("Deactivate Part", [w = juce::Component::SafePointer(that), part]() {
            if (w)
                w->sendToSerialization(cmsg::DeactivatePart(part));
        });
    }
}
} // namespace scxt::ui::app::shared
#endif // PATCHMULTIIO_H
