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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_SHARED_SAMPLEDROPHANDLER_H
#define SCXT_SRC_SCXT_PLUGIN_APP_SHARED_SAMPLEDROPHANDLER_H

#include <juce_gui_basics/juce_gui_basics.h>
// MultiInstrumentPrompt pulls in SCXTEditor.h → messaging/messaging.h which must come before
// structure_messages.h (structure_messages.h depends on the client_serial enums)
#include "app/shared/MultiInstrumentPrompt.h"
#include "browser/browser.h"
#include "sample/compound_file.h"
#include "app/browser-ui/BrowserPaneInterfaces.h"
#include "messaging/client/structure_messages.h"

namespace scxt::ui::app::shared
{

// Classifies a drag/drop source to drive visual affordances and message dispatch.
// Covers both external file drops (fs::path) and internal browser drags (WithSampleInfo).
struct SampleDropSource
{
    enum class Kind
    {
        None,       // Not a recognized sample source
        MultiFile,  // .scm — replaces the whole multi
        PartFile,   // .scp — loads into one part
        Instrument, // SF2/SFZ/GIG/EXS/AKP or CompoundElement::INSTRUMENT — replace-or-append
        Sample,     // Single audio file (WAV/FLAC/MP3/etc) or CompoundElement::SAMPLE
    };

    Kind kind{Kind::None};
    juce::String displayName;
    std::string pathStr;                                      // set if source is a file path
    std::optional<sample::compound::CompoundElement> element; // set if source is a compound element

    // ---- Predicates ----

    bool isNone() const { return kind == Kind::None; }
    bool isMultiFile() const { return kind == Kind::MultiFile; }
    bool isPartFile() const { return kind == Kind::PartFile; }
    // An instrument-container (SF2 preset / multi-sample file) that supports replace-or-append
    bool isInstrumentWhichCanReplace() const { return kind == Kind::Instrument; }
    // A plain audio sample — always appends as a new zone
    bool isSingleSample() const { return kind == Kind::Sample; }

    // True if the UI should show a top/bottom replace-vs-append split on a card
    bool showsReplaceSplit() const { return isInstrumentWhichCanReplace(); }
    // True if the UI should highlight the whole sidebar (replaces everything)
    bool showsFullSidebarHighlight() const { return isMultiFile(); }
    // True if this source can be dropped onto a group to add a zone
    bool isDroppableOnGroup() const { return isSingleSample(); }

    // ---- Factory methods ----

    // Classify from a filesystem path
    static SampleDropSource fromPath(const fs::path &p)
    {
        SampleDropSource s;
        s.pathStr = p.u8string();
        auto ps = juce::String::fromUTF8(p.u8string().c_str());
        s.displayName = juce::File(ps).getFileName();

        if (ps.endsWithIgnoreCase(".scm"))
            s.kind = Kind::MultiFile;
        else if (ps.endsWithIgnoreCase(".scp"))
            s.kind = Kind::PartFile;
        else if (browser::Browser::isLoadableMultiSample(p))
            s.kind = Kind::Instrument;
        else if (browser::Browser::isLoadableSingleSample(p))
            s.kind = Kind::Sample;

        return s;
    }

    // Classify from a single browser item — call only when !wsi->encompassesMultipleSampleInfos()
    static SampleDropSource fromBrowserItem(browser_ui::WithSampleInfo *wsi)
    {
        if (!wsi || wsi->encompassesMultipleSampleInfos())
            return {};

        if (wsi->getDirEnt().has_value())
        {
            try
            {
                return fromPath(wsi->getDirEnt()->path());
            }
            catch (fs::filesystem_error &)
            {
                return {};
            }
        }

        if (wsi->getCompoundElement().has_value())
        {
            auto el = wsi->getCompoundElement().value();
            SampleDropSource s;
            s.element = el;
            s.displayName = juce::String::fromUTF8(el.name.c_str());
            s.kind = (el.type == sample::compound::CompoundElement::Type::INSTRUMENT)
                         ? Kind::Instrument
                         : Kind::Sample;
            return s;
        }

        return {};
    }

    // ---- Execute methods ----

    // Drop onto targetPart. For Instrument/Sample: sends SelectPart first.
    // replace=true sends ClearPart before loading (only meaningful for Instrument kind).
    void dropOnPart(int targetPart, bool replace, HasEditor *editor) const
    {
        namespace cmsg = scxt::messaging::client;

        switch (kind)
        {
        case Kind::MultiFile:
            editor->sendToSerialization(cmsg::LoadMulti(pathStr));
            return;

        case Kind::PartFile:
            editor->sendToSerialization(cmsg::LoadPartInto({pathStr, (int16_t)targetPart}));
            return;

        case Kind::Instrument:
            editor->sendToSerialization(cmsg::SelectPart(targetPart));
            if (replace)
                editor->sendToSerialization(cmsg::ClearPart(targetPart));
            if (element.has_value())
            {
                editor->sendToSerialization(
                    cmsg::AddCompoundElementWithRange({*element, 60, 0, 127, 0, 127}));
            }
            else if (!pathStr.empty())
            {
                auto p = fs::path(fs::u8path(pathStr));
                auto inst = browser::Browser::getMultiInstrumentElements(p);
                if (inst.empty())
                {
                    editor->sendToSerialization(
                        cmsg::AddSampleWithRange({pathStr, 60, 0, 127, 0, 127}));
                }
                else
                {
                    auto e = editor;
                    promptForMultiInstrument(e->editor, inst, [e](const auto &elem) {
                        e->sendToSerialization(
                            cmsg::AddCompoundElementWithRange({elem, 60, 0, 127, 0, 127}));
                    });
                }
            }
            return;

        case Kind::Sample:
            editor->sendToSerialization(cmsg::SelectPart(targetPart));
            if (element.has_value())
            {
                editor->sendToSerialization(
                    cmsg::AddCompoundElementWithRange({*element, 60, 0, 127, 0, 127}));
            }
            else
            {
                editor->sendToSerialization(
                    cmsg::AddSampleWithRange({pathStr, 60, 0, 127, 0, 127}));
            }
            return;

        default:
            return;
        }
    }

    // Add as a new zone in the currently selected group.
    // Only meaningful for Sample kind — other kinds are no-ops.
    void dropAsZoneInCurrentGroup(HasEditor *editor) const
    {
        namespace cmsg = scxt::messaging::client;

        if (kind != Kind::Sample)
            return;
        if (element.has_value())
            editor->sendToSerialization(
                cmsg::AddCompoundElementWithRange({*element, 60, 0, 127, 0, 127}));
        else
            editor->sendToSerialization(cmsg::AddSample(pathStr));
    }

    // Add as a new zone in an explicitly specified group, bypassing selection state.
    // Only meaningful for Sample kind — other kinds are no-ops.
    void dropAsZoneInGroup(int part, int group, HasEditor *editor) const
    {
        namespace cmsg = scxt::messaging::client;

        if (kind != Kind::Sample)
            return;
        if (element.has_value())
            editor->sendToSerialization(
                cmsg::AddCompoundElementWithRange({*element, 60, 0, 127, 0, 127}));
        else
            editor->sendToSerialization(cmsg::AddSampleToGroup({pathStr, part, group}));
    }
};

// Execute a batch drop (encompassesMultipleSampleInfos()) onto a specific part.
inline void executeBatchDropOnPart(browser_ui::WithSampleInfo *wsi, int targetPart,
                                   HasEditor *editor)
{
    namespace cmsg = scxt::messaging::client;
    assert(wsi && wsi->encompassesMultipleSampleInfos());
    editor->sendToSerialization(cmsg::SelectPart(targetPart));
    for (auto *e : wsi->getMultipleSampleInfos())
    {
        if (e->getCompoundElement().has_value())
            editor->sendToSerialization(
                cmsg::AddCompoundElementWithRange({*e->getCompoundElement(), 60, 0, 127, 0, 127}));
        else if (e->getDirEnt().has_value())
            editor->sendToSerialization(
                cmsg::AddSampleWithRange({e->getDirEnt()->path().u8string(), 60, 0, 127, 0, 127}));
    }
}

// Execute a batch drop onto the currently selected group.
inline void executeBatchDropOnGroup(browser_ui::WithSampleInfo *wsi, HasEditor *editor)
{
    namespace cmsg = scxt::messaging::client;
    assert(wsi && wsi->encompassesMultipleSampleInfos());
    for (auto *e : wsi->getMultipleSampleInfos())
    {
        if (e->getCompoundElement().has_value())
            editor->sendToSerialization(
                cmsg::AddCompoundElementWithRange({*e->getCompoundElement(), 60, 0, 127, 0, 127}));
        else if (e->getDirEnt().has_value())
            editor->sendToSerialization(cmsg::AddSample(e->getDirEnt()->path().u8string()));
    }
}

// Execute a batch drop into an explicit group, bypassing selection state.
inline void executeBatchDropOnGroup(browser_ui::WithSampleInfo *wsi, int part, int group,
                                    HasEditor *editor)
{
    namespace cmsg = scxt::messaging::client;
    assert(wsi && wsi->encompassesMultipleSampleInfos());
    for (auto *e : wsi->getMultipleSampleInfos())
    {
        if (e->getCompoundElement().has_value())
            editor->sendToSerialization(
                cmsg::AddCompoundElementWithRange({*e->getCompoundElement(), 60, 0, 127, 0, 127}));
        else if (e->getDirEnt().has_value())
            editor->sendToSerialization(
                cmsg::AddSampleToGroup({e->getDirEnt()->path().u8string(), part, group}));
    }
}

} // namespace scxt::ui::app::shared

#endif // SCXT_SRC_SCXT_PLUGIN_APP_SHARED_SAMPLEDROPHANDLER_H
