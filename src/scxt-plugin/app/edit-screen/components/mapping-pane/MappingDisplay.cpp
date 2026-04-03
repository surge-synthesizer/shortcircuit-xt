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

#include "MappingDisplay.h"
#include "app/edit-screen/components/MacroMappingVariantPane.h"
#include "app/browser-ui/BrowserPaneInterfaces.h"
#include "sst/jucegui/screens/ModalBase.h"
#include "ZoneLayoutDisplay.h"
#include "ZoneLayoutKeyboard.h"
#include "engine/zone.h"
#include "app/shared/MultiInstrumentPrompt.h"

#include <app/edit-screen/EditScreen.h>

namespace scxt::ui::app::edit_screen
{
namespace cmsg = scxt::messaging::client;
namespace jcmp = sst::jucegui::components;

MappingDisplay::MappingDisplay(MacroMappingVariantPane *p)
    : HasEditor(p->editor), mappingView(p->mappingView), parentPane(p)
{
    auto tmpLOK = std::make_unique<LayoutAndKeyboard>(this);
    mappingZones = tmpLOK->mappingZones.get();
    keyboard = tmpLOK->keyboard.get();
    zoneLayoutViewport = std::make_unique<jcmp::ZoomContainer>(std::move(tmpLOK));
    zoneLayoutViewport->setHZoomFloor((36 + 2) * 1.f / 128.f);
    zoneLayoutViewport->setVZoomFloor(32.f / 128.f);
    addAndMakeVisible(*zoneLayoutViewport);

    zoneHeader = std::make_unique<MappingZoneHeader>(editor);
    addAndMakeVisible(*zoneHeader);

    using ffac =
        connectors::SingleValueFactory<floatAttachment_t, cmsg::UpdateZoneMappingFloatValue>;
    using ifac =
        connectors::SingleValueFactory<int16Attachment_t, cmsg::UpdateZoneMappingInt16TValue>;

    auto makeLabel = [this](auto &l, const std::string &t) {
        l = std::make_unique<sst::jucegui::components::Label>();
        l->setText(t);
        addAndMakeVisible(*l);
    };

    auto makeGlyph = [this](auto &c, sst::jucegui::components::GlyphPainter::GlyphType g) {
        c = std::make_unique<sst::jucegui::components::GlyphPainter>(g);
        addAndMakeVisible(*c);
    };

    // Just a little shorthand to save the two args repeating
    auto iAdd = [this](auto &v, auto &a, auto &w) {
        ifac::attachAndAdd(mappingView, v, this, a, w);
        auto origOnBeginEdit = w->onBeginEdit;
        auto origOnEndEdit = w->onEndEdit;
        w->onBeginEdit = [this, origOnBeginEdit]() {
            SCLOG_IF(undoRedo, "about to mutate");
            mayBeAboutToMutate = true;
            if (origOnBeginEdit)
                origOnBeginEdit();
        };
        w->onEndEdit = [this, origOnEndEdit]() {
            SCLOG_IF(undoRedo, "about to mutate over");
            mayBeAboutToMutate = false;
            if (origOnEndEdit)
                origOnEndEdit();
        };
    };
    auto iAddConstrained = [this, iAdd](auto &v, auto &a, auto &w,
                                        std::function<bool(int)> accept) {
        iAdd(v, a, w);
        connectors::addGuiStepBeforeSend(*a, [&aWrite = a, accept](const auto &a) {
            if (!accept(aWrite->value))
                aWrite->value = a.prevValue;
        });
    };

    auto fAdd = [this](auto &v, auto &a, auto &w) {
        ffac::attachAndAdd(mappingView, v, this, a, w);
    };

    iAdd(mappingView.rootKey, intAttachments.RootKey, textEds.RootKey);
    makeLabel(labels.RootKey, "Root Key");

    iAddConstrained(mappingView.keyboardRange.keyStart, intAttachments.KeyStart, textEds.KeyStart,
                    [w = juce::Component::SafePointer(this)](auto v) {
                        if (!w)
                            return true;
                        return v <= w->mappingView.keyboardRange.keyEnd;
                    });

    auto addDeltas = [this](auto &to, auto &wid, auto dir) {
        auto og = to->onGuiValueChanged;
        to->onGuiValueChanged = [dr = dir, w = juce::Component::SafePointer(wid.get()), og,
                                 this](const auto &a) {
            auto isDrag = true;
            auto isCtrl = false;
            if (w)
            {
                isDrag = w->isSetFromDrag();
                isCtrl = juce::ModifierKeys::getCurrentModifiers().isCtrlDown();
            }
            if (isDrag || !isCtrl)
            {
                int d = a.value - a.prevValue;
                int dx{0}, dy{0};
                if (dr == engine::Zone::ChangeDimension::KEY_RANGE_START ||
                    dr == engine::Zone::ChangeDimension::KEY_RANGE_END ||
                    dr == engine::Zone::ChangeDimension::KEY_FADE_START ||
                    dr == engine::Zone::ChangeDimension::KEY_FADE_END)
                    dx = d;
                else
                    dy = d;
                auto res = applyDeltaToSelectedZones(dr, dx, dy, true);

                if ((dr == engine::Zone::ChangeDimension::KEY_RANGE_START ||
                     dr == engine::Zone::ChangeDimension::KEY_RANGE_END ||
                     dr == engine::Zone::ChangeDimension::KEY_FADE_START ||
                     dr == engine::Zone::ChangeDimension::KEY_FADE_END) &&
                    (res == KEYRANGE_CHANGED))
                    d = dx;
                else if (res == VELOCITY_CHANGED)
                    d = dy;
                else
                    d = 0;

                a.value = a.prevValue + d;
                // explicitly dont call og here since above delta applies to lead
                // and we don't need server round trip during drag for constraints
                repaint();
            }
            else
            {
                int v = a.value;
                int vx{0}, vy{0};
                if (dr == engine::Zone::ChangeDimension::KEY_RANGE_START ||
                    dr == engine::Zone::ChangeDimension::KEY_RANGE_END ||
                    dr == engine::Zone::ChangeDimension::KEY_FADE_START ||
                    dr == engine::Zone::ChangeDimension::KEY_FADE_END)
                    vx = v;
                else
                    vy = v;
                if (!applyAbsoluteToSelectedZones(dr, vx, vy, false))
                {
                    a.value = a.prevValue;
                }
                else
                {
                    og(a);
                }
            }
        };
        wid->onEndEdit = [this]() {
            sendToSerialization(cmsg::RequestZoneMapping({editor->selectedPart}));
        };
    };
    addDeltas(intAttachments.KeyStart, textEds.KeyStart,
              engine::Zone::ChangeDimension::KEY_RANGE_START);

    iAddConstrained(mappingView.keyboardRange.keyEnd, intAttachments.KeyEnd, textEds.KeyEnd,
                    [w = juce::Component::SafePointer(this)](auto v) {
                        if (!w)
                            return true;
                        return v >= w->mappingView.keyboardRange.keyStart;
                    });
    addDeltas(intAttachments.KeyEnd, textEds.KeyEnd, engine::Zone::ChangeDimension::KEY_RANGE_END);

    makeLabel(labels.KeyStart, "Key Range");
    iAdd(mappingView.keyboardRange.fadeStart, intAttachments.FadeStart, textEds.FadeStart);
    addDeltas(intAttachments.FadeStart, textEds.FadeStart,
              engine::Zone::ChangeDimension::KEY_FADE_START);

    iAdd(mappingView.keyboardRange.fadeEnd, intAttachments.FadeEnd, textEds.FadeEnd);
    addDeltas(intAttachments.FadeEnd, textEds.FadeEnd, engine::Zone::ChangeDimension::KEY_FADE_END);
    makeLabel(labels.FadeStart, "Crossfade");

    iAddConstrained(mappingView.velocityRange.velStart, intAttachments.VelStart, textEds.VelStart,
                    [w = juce::Component::SafePointer(this)](auto v) {
                        if (!w)
                            return true;
                        return v <= w->mappingView.velocityRange.velEnd;
                    });
    addDeltas(intAttachments.VelStart, textEds.VelStart,
              engine::Zone::ChangeDimension::VEL_RANGE_START);
    iAddConstrained(mappingView.velocityRange.velEnd, intAttachments.VelEnd, textEds.VelEnd,
                    [w = juce::Component::SafePointer(this)](auto v) {
                        if (!w)
                            return true;
                        return v >= w->mappingView.velocityRange.velStart;
                    });
    addDeltas(intAttachments.VelEnd, textEds.VelEnd, engine::Zone::ChangeDimension::VEL_RANGE_END);

    makeLabel(labels.VelStart, "Vel Range");

    iAdd(mappingView.velocityRange.fadeStart, intAttachments.VelFadeStart, textEds.VelFadeStart);
    addDeltas(intAttachments.VelFadeStart, textEds.VelFadeStart,
              engine::Zone::ChangeDimension::VEL_FADE_START);
    iAdd(mappingView.velocityRange.fadeEnd, intAttachments.VelFadeEnd, textEds.VelFadeEnd);
    addDeltas(intAttachments.VelFadeEnd, textEds.VelFadeEnd,
              engine::Zone::ChangeDimension::VEL_FADE_END);

    makeLabel(labels.VelFadeStart, "CrossFade");

    iAdd(mappingView.pbDown, intAttachments.PBDown, textEds.PBDown);
    iAdd(mappingView.pbUp, intAttachments.PBUp, textEds.PBUp);
    makeLabel(labels.PBDown, "Pitch Bend");

    fAdd(mappingView.amplitude, floatAttachments.Level, textEds.Level);
    makeGlyph(glyphs.Level, sst::jucegui::components::GlyphPainter::VOLUME);

    fAdd(mappingView.pan, floatAttachments.Pan, textEds.Pan);
    makeGlyph(glyphs.Pan, sst::jucegui::components::GlyphPainter::PAN);

    fAdd(mappingView.pitchOffset, floatAttachments.Pitch, textEds.Pitch);
    textEds.Pitch->setDragScaleFromMinMaxHeuristic();
    makeGlyph(glyphs.Pitch, sst::jucegui::components::GlyphPainter::TUNING);

    fAdd(mappingView.tracking, floatAttachments.Tracking, textEds.Tracking);
    makeLabel(labels.Tracking, "KT");
    labels.Tracking->setJustification(juce::Justification::centredLeft);

    setAccessible(true);
    setTitle("Mapping");
    setDescription("Mapping");
    setWantsKeyboardFocus(true);
}

MappingDisplay::~MappingDisplay() = default;

MappingDisplay::LayoutAndKeyboard::LayoutAndKeyboard(
    scxt::ui::app::edit_screen::MappingDisplay *that)
{
    mappingZones = std::make_unique<ZoneLayoutDisplay>(that);
    addAndMakeVisible(*mappingZones);
    keyboard = std::make_unique<ZoneLayoutKeyboard>(that);
    addAndMakeVisible(*keyboard);
}

void MappingDisplay::LayoutAndKeyboard::setHorizontalZoom(float pctStart, float zoomFactor)
{
    mappingZones->setHorizontalZoom(pctStart, zoomFactor);
    keyboard->setHorizontalZoom(pctStart, zoomFactor);
    repaint();
}

void MappingDisplay::LayoutAndKeyboard::setVerticalZoom(float pctStart, float zoomFactor)
{
    mappingZones->setVerticalZoom(pctStart, zoomFactor);
    repaint();
}

void MappingDisplay::LayoutAndKeyboard::resized()
{
    auto lo = getLocalBounds().withTrimmedBottom(ZoneLayoutKeyboard::keyboardHeight);
    auto kb = getLocalBounds().withTrimmedTop(getHeight() - ZoneLayoutKeyboard::keyboardHeight);

    mappingZones->setBounds(lo);
    keyboard->setBounds(kb);
}

void MappingDisplay::resized()

{
    static constexpr int controlSize{140};
    static constexpr int headerSize{20};

    // Header
    auto b = getLocalBounds();
    auto z = b;
    zoneHeader->setBounds(b.withHeight(headerSize));

    // Mapping Display
    z = b.withTrimmedTop(headerSize);
    auto viewArea = z.withWidth(getWidth() - controlSize - 5);
    zoneLayoutViewport->setBounds(viewArea);
    /*
     * mappingViewport->setBounds(viewArea);
    auto kbdY = viewArea.getBottom() - mappingViewport->viewport->getScrollBarThickness() -
                ZoneLayoutKeyboard::keyboardHeight;
    auto kbdW = viewArea.getWidth() - mappingViewport->viewport->getScrollBarThickness();
    keyboardViewPort->setBounds({0, kbdY, kbdW, ZoneLayoutKeyboard::keyboardHeight});
*/

    // Side Pane
    static constexpr int rowHeight{16}, rowMargin{4};
    static constexpr int typeinWidth{32};
    static constexpr int typeinPad{4}, typeinMargin{2};
    auto r = z.withLeft(getWidth() - controlSize);
    auto cr = r.withHeight(rowHeight);

    auto co2 = [=](auto &c) {
        return c.withWidth(c.getWidth() - typeinPad - 2 * typeinWidth - 2 * typeinMargin);
    };
    auto c2 = [=](auto &c) {
        return c.withLeft(c.getRight() - typeinPad - typeinMargin - 2 * typeinWidth)
            .withWidth(typeinWidth);
    };
    auto co3 = [=](auto &c) {
        return c.withWidth(c.getWidth() - typeinPad - typeinWidth - typeinMargin);
    };
    auto c3 = [=](auto &c) {
        return c.withLeft(c.getRight() - typeinPad - typeinWidth).withWidth(typeinWidth);
    };

    labels.RootKey->setBounds(co3(cr));
    textEds.RootKey->setBounds(c3(cr));

    cr = cr.translated(0, rowHeight + rowMargin);
    textEds.KeyStart->setBounds(c2(cr));
    textEds.KeyEnd->setBounds(c3(cr));
    labels.KeyStart->setBounds(co2(cr));

    cr = cr.translated(0, rowHeight + rowMargin);
    textEds.FadeStart->setBounds(c2(cr));
    textEds.FadeEnd->setBounds(c3(cr));
    labels.FadeStart->setBounds(co2(cr));

    cr = cr.translated(0, rowHeight + rowMargin);
    textEds.VelStart->setBounds(c2(cr));
    textEds.VelEnd->setBounds(c3(cr));
    labels.VelStart->setBounds(co2(cr));

    cr = cr.translated(0, rowHeight + rowMargin);
    textEds.VelFadeStart->setBounds(c2(cr));
    textEds.VelFadeEnd->setBounds(c3(cr));
    labels.VelFadeStart->setBounds(co2(cr));

    cr = cr.translated(0, rowHeight + rowMargin);
    textEds.PBDown->setBounds(c2(cr));
    textEds.PBUp->setBounds(c3(cr));
    labels.PBDown->setBounds(co2(cr));

    auto cQ = [&](int i) {
        auto w = cr.getWidth() / 4.0;
        return cr.withTrimmedLeft(w * i).withWidth(w).reduced(1);
    };
    cr = cr.translated(0, rowHeight + rowMargin);
    //  (volume)

    cr = cr.translated(0, rowHeight + rowMargin);
    glyphs.Level->setBounds(cQ(2));
    textEds.Level->setBounds(cQ(3));

    cr = cr.translated(0, rowHeight + rowMargin);
    glyphs.Pan->setBounds(cQ(2));
    textEds.Pan->setBounds(cQ(3));

    cr = cr.translated(0, rowHeight + rowMargin);
    glyphs.Pitch->setBounds(cQ(2));
    textEds.Pitch->setBounds(cQ(3));

    // FIXME - make this match wireframe better
    cr = cr.translated(0, rowHeight + rowMargin);
    labels.Tracking->setBounds(cQ(2));
    textEds.Tracking->setBounds(cQ(3));
}

void MappingDisplay::mappingChangedFromGUI()
{
    sendToSerialization(cmsg::UpdateLeadZoneMapping(mappingView));
}

void MappingDisplay::setGroupZoneMappingSummary(const engine::Part::zoneMappingSummary_t &d)
{
    summary = d;
    if (editor->currentLeadZoneSelection.has_value())
        setLeadSelection(*(editor->currentLeadZoneSelection));
    if (mappingZones)
    {
        mappingZones->mappingWasReset();
    }
    repaint();
}

void MappingDisplay::setLeadSelection(const selection::SelectionManager::ZoneAddress &za)
{
    // but we need to call setLeadZoneBounds to make the hotspots so rename that too
    bool foundZone{false};
    for (const auto &s : summary)
    {
        if (s.address == za)
        {
            foundZone = true;
            if (mappingZones)
                mappingZones->setLeadZoneBounds(s);
        }
    }

    if (mappingZones && !foundZone)
    {
        mappingZones->clearLeadZoneBounds();
    }
}

int MappingDisplay::voiceCountFor(const selection::SelectionManager::ZoneAddress &z)
{
    auto p = editor->editScreen->voiceCountByZoneAddress.find(z);
    if (p != editor->editScreen->voiceCountByZoneAddress.end())
        return p->second;

    return 0;
}

bool MappingDisplay::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    auto r = browser_ui::asSampleInfo(dragSourceDetails.sourceComponent);
    if (r)
    {
        auto wsi = browser_ui::asSampleInfo(dragSourceDetails.sourceComponent);
        if (wsi->encompassesMultipleSampleInfos())
        {
            dropElementCount = wsi->getMultipleSampleInfos().size();
        }
        else
        {
            dropElementCount = 1;
        }
    }
    return browser_ui::hasSampleInfo(dragSourceDetails.sourceComponent);
}
void MappingDisplay::itemDropped(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    isUndertakingDrop = false;
    currentDragSource = {};
    // Recompute replace vs add from drop position — do not rely on saved drag state
    bool savedReplace = dragSourceDetails.localPosition.x <
                        (zoneLayoutViewport->getX() + zoneLayoutViewport->getWidth() / 2);

    auto wsi = browser_ui::asSampleInfo(dragSourceDetails.sourceComponent);
    if (wsi)
    {
        auto r = mappingZones->rootAndRangeForPosition(dragSourceDetails.localPosition,
                                                       dropElementCount, false);
        assert(r.size() > 0);
        assert(r.size() == dropElementCount);
        if (wsi->encompassesMultipleSampleInfos())
        {
            auto els = wsi->getMultipleSampleInfos();
            auto nEls = els.size();
            assert(nEls == dropElementCount);
            int idx{0};
            for (auto e : els)
            {
                auto &loc = r[std::min(idx++, (int)nEls - 1)];

                if (e->getCompoundElement().has_value())
                {
                    sendToSerialization(cmsg::AddCompoundElementWithRange(
                        {*e->getCompoundElement(), loc.root, loc.lo, loc.hi, loc.vlo, loc.vhi}));
                }
                else if (e->getDirEnt().has_value())
                {
                    sendToSerialization(
                        cmsg::AddSampleWithRange({e->getDirEnt()->path().u8string(), loc.root,
                                                  loc.lo, loc.hi, loc.vlo, loc.vhi}));
                }
            }
        }
        else if (wsi->getCompoundElement().has_value())
        {
            auto loc = r[0];
            assert(r.size() == 1);
            auto src = shared::SampleDropSource::fromBrowserItem(wsi);
            if (src.isInstrumentWhichCanReplace() && savedReplace)
                sendToSerialization(cmsg::ClearPart(editor->selectedPart));
            sendToSerialization(cmsg::AddCompoundElementWithRange(
                {*wsi->getCompoundElement(), loc.root, loc.lo, loc.hi, loc.vlo, loc.vhi}));
        }
        else if (wsi->getDirEnt().has_value())
        {
            auto loc = r[0];
            assert(r.size() == 1);
            auto src = shared::SampleDropSource::fromBrowserItem(wsi);

            if (src.isPartFile())
            {
                sendToSerialization(
                    cmsg::LoadPartInto({src.pathStr, (int16_t)editor->selectedPart}));
            }
            else if (src.isMultiFile())
            {
                // SCM onto the mapping pane is not allowed
            }
            else if (src.isInstrumentWhichCanReplace())
            {
                if (savedReplace)
                    sendToSerialization(cmsg::ClearPart(editor->selectedPart));
                auto inst = browser::Browser::getMultiInstrumentElements(wsi->getDirEnt()->path());
                if (inst.empty())
                    sendToSerialization(cmsg::AddSampleWithRange(
                        {src.pathStr, loc.root, loc.lo, loc.hi, loc.vlo, loc.vhi}));
                else
                    promptForMultiInstrument(inst);
            }
            else if (src.isSingleSample())
            {
                sendToSerialization(cmsg::AddSampleWithRange(
                    {src.pathStr, loc.root, loc.lo, loc.hi, loc.vlo, loc.vhi}));
            }
        }
    }
    if (editor->editScreen->partSidebar)
    {
        editor->editScreen->partSidebar->setSelectedTab(2);
    }
    repaint();
}
void MappingDisplay::itemDragEnter(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    isUndertakingDrop = true;
    currentDragPoint = dragSourceDetails.localPosition;
    currentDragSource = {};

    auto wsi = browser_ui::asSampleInfo(dragSourceDetails.sourceComponent);
    if (wsi && !wsi->encompassesMultipleSampleInfos())
    {
        currentDragSource = shared::SampleDropSource::fromBrowserItem(wsi);
        if (currentDragSource.isInstrumentWhichCanReplace())
        {
            dragIsOnReplaceSide = dragSourceDetails.localPosition.x <
                                  (zoneLayoutViewport->getX() + zoneLayoutViewport->getWidth() / 2);
        }
    }
    repaint();
}

void MappingDisplay::itemDragExit(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    isUndertakingDrop = false;
    currentDragSource = {};
    repaint();
}

void MappingDisplay::itemDragMove(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    currentDragPoint = dragSourceDetails.localPosition;
    if (currentDragSource.isInstrumentWhichCanReplace())
    {
        dragIsOnReplaceSide = dragSourceDetails.localPosition.x <
                              (zoneLayoutViewport->getX() + zoneLayoutViewport->getWidth() / 2);
    }
    repaint();
}

bool MappingDisplay::isInterestedInFileDrag(const juce::StringArray &files)
{
    // SCP: replace this part. SCM: show "can't drop" warning (accepted for visual, rejected on
    // drop).
    if (files.size() == 1)
    {
        if (files[0].endsWithIgnoreCase(".scp") || files[0].endsWithIgnoreCase(".scm"))
        {
            dropElementCount = 1;
            return true;
        }
    }

    auto allLoadable = std::all_of(files.begin(), files.end(), [this](const auto &f) {
        try
        {
            auto pt = fs::path{(const char *)(f.toUTF8())};
            return editor->browser.isLoadableSample(pt);
        }
        catch (fs::filesystem_error &e)
        {
        }
        return false;
    });
    dropElementCount = 1;
    if (allLoadable)
        dropElementCount = files.size();
    return allLoadable;
}

void MappingDisplay::fileDragEnter(const juce::StringArray &files, int x, int y)
{
    isUndertakingDrop = true;
    currentDragPoint = {x, y};
    currentDragSource = {};

    if (files.size() == 1)
    {
        try
        {
            currentDragSource =
                shared::SampleDropSource::fromPath(fs::path{(const char *)files[0].toUTF8()});
            if (currentDragSource.isInstrumentWhichCanReplace())
                dragIsOnReplaceSide =
                    x < (zoneLayoutViewport->getX() + zoneLayoutViewport->getWidth() / 2);
        }
        catch (fs::filesystem_error &)
        {
        }
    }
    repaint();
}

void MappingDisplay::fileDragMove(const juce::StringArray &files, int x, int y)
{
    isUndertakingDrop = true;
    currentDragPoint = {x, y};
    if (currentDragSource.isInstrumentWhichCanReplace())
        dragIsOnReplaceSide = x < (zoneLayoutViewport->getX() + zoneLayoutViewport->getWidth() / 2);
    repaint();
}

void MappingDisplay::fileDragExit(const juce::StringArray &)
{
    isUndertakingDrop = false;
    currentDragSource = {};
    repaint();
}

void MappingDisplay::filesDropped(const juce::StringArray &files, int x, int y)
{
    // Do NOT rely on saved drag state — on macOS JUCE may call fileDragExit before filesDropped.
    // Recompute everything from files and drop position.
    isUndertakingDrop = false;
    currentDragSource = {};

    if (files.size() == 1)
    {
        shared::SampleDropSource src;
        try
        {
            src = shared::SampleDropSource::fromPath(fs::path{(const char *)files[0].toUTF8()});
        }
        catch (fs::filesystem_error &)
        {
            repaint();
            return;
        }

        if (src.isPartFile())
        {
            sendToSerialization(cmsg::LoadPartInto({src.pathStr, (int16_t)editor->selectedPart}));
            repaint();
            return;
        }
        if (src.isMultiFile())
        {
            // SCM onto the mapping pane is not allowed
            repaint();
            return;
        }
    }

    // Recompute replace vs add from drop position (left half = replace, right half = add)
    bool isReplace = x < (zoneLayoutViewport->getX() + zoneLayoutViewport->getWidth() / 2);

    // Clear part first if all dropped files are instrument containers and we're replacing
    bool allInstrument = !files.isEmpty();
    for (auto &f : files)
    {
        try
        {
            if (!shared::SampleDropSource::fromPath(fs::path{(const char *)f.toUTF8()})
                     .isInstrumentWhichCanReplace())
            {
                allInstrument = false;
                break;
            }
        }
        catch (fs::filesystem_error &)
        {
            allInstrument = false;
            break;
        }
    }
    if (allInstrument && isReplace)
        sendToSerialization(cmsg::ClearPart(editor->selectedPart));

    auto regions = mappingZones->rootAndRangeForPosition({x, y}, files.size(), false);
    int lidx{0};
    for (auto f : files)
    {
        auto &loc = regions[lidx++];
        auto p = fs::path{(const char *)(f.toUTF8())};
        auto inst = browser::Browser::getMultiInstrumentElements(p);
        if (inst.empty())
        {
            sendToSerialization(cmsg::AddSampleWithRange(
                {f.toStdString(), loc.root, loc.lo, loc.hi, loc.vlo, loc.vhi}));
        }
        else
        {
            promptForMultiInstrument(inst);
        }
    }
    if (editor->editScreen->partSidebar)
        editor->editScreen->partSidebar->setSelectedTab(2);
    repaint();
}

void MappingDisplay::promptForMultiInstrument(
    const std::vector<sample::compound::CompoundElement> &inst)
{
    shared::promptForMultiInstrument(editor, inst, [this](const auto &elem) {
        sendToSerialization(cmsg::AddCompoundElementWithRange({elem, 60, 0, 127, 0, 127}));
    });
}

void MappingDisplay::paintOverChildren(juce::Graphics &g)
{
    if (!isUndertakingDrop || currentDragSource.isNone())
        return;

    auto lb = zoneLayoutViewport->getBounds().toFloat();

    if (currentDragSource.isPartFile())
    {
        editor->occludeRegionWithText(
            g, lb, theme::ColorMap::accent_1b, theme::ColorMap::bg_1,
            theme::ColorMap::generic_content_high,
            {"Replace this part with", currentDragSource.displayName.toStdString()});
    }
    else if (currentDragSource.isMultiFile())
    {
        editor->occludeRegionWithText(g, lb, theme::ColorMap::warning_1a, theme::ColorMap::bg_1,
                                      theme::ColorMap::generic_content_high,
                                      {"Cannot drop multi onto mapping.", "Use the load function"});
    }
    else if (currentDragSource.isInstrumentWhichCanReplace())
    {
        auto halfX = lb.getX() + lb.getWidth() * 0.5f;
        auto replaceHalf = lb.withRight(halfX);
        auto addHalf = lb.withLeft(halfX);

        // Shared bg_1 base
        g.setColour(editor->themeColor(theme::ColorMap::bg_1));
        g.fillRect(lb);

        // Active half: frontColor@0.35; inactive half: frontColor@0.15
        auto frontCol = editor->themeColor(theme::ColorMap::accent_1b);
        g.setColour(frontCol.withAlpha(dragIsOnReplaceSide ? 0.35f : 0.15f));
        g.fillRect(replaceHalf);
        g.setColour(frontCol.withAlpha(dragIsOnReplaceSide ? 0.15f : 0.35f));
        g.fillRect(addHalf);

        // Text
        g.setColour(editor->themeColor(theme::ColorMap::generic_content_high));
        g.setFont(editor->themeApplier.interMediumFor(13));
        g.drawText("Drop here to replace part", replaceHalf.toNearestInt(),
                   juce::Justification::centred);
        g.drawText("Drop here to add to part", addHalf.toNearestInt(),
                   juce::Justification::centred);

        // Outer 2px border + divider
        g.setColour(frontCol);
        g.drawRect(lb, 2.f);
        g.drawVerticalLine((int)halfX, lb.getY(), lb.getBottom());
    }
}

void MappingDisplay::doZoneRename(const selection::SelectionManager::ZoneAddress &z)
{
    editor->makeComingSoon("Rename from Mapping Pane")();
}

bool MappingDisplay::keyPressed(const juce::KeyPress &key)
{
    if (
#if JUCE_MAC
        key.getModifiers().isCommandDown()
#else
        key.getModifiers().isCtrlDown()
#endif
        && (key.getKeyCode() == 'C' || key.getKeyCode() == 'V'))
    {
        if (key.getKeyCode() == 'C')
        {
            auto lz = editor->currentLeadZoneSelection;
            if (lz.has_value())
            {
                sendToSerialization(cmsg::CopyZone(*lz));
                return true;
            }
        }
        else
        {
            auto lg = editor->currentLeadGroupSelection;
            if (lg.has_value())
            {
                sendToSerialization(cmsg::PasteZone(*lg));
                return true;
            }
        }
    }
    return false;
}

MappingDisplay::DeltaResult
MappingDisplay::applyDeltaToSelectedZones(engine::Zone::ChangeDimension dim, int &deltaX,
                                          int &deltaY, bool updateLead)
{
    auto mv = (int)(DeltaResult::NO_CHANGE);

    for (const auto &z : summary)
    {
        if (editor->isSelected(z.address))
        {
            auto pdx = deltaX;
            adjustDeltasToMakeSureTheyWillFit(dim, deltaX, deltaY, z.kr, z.vr);
        }
    }

    // And once clamped we evaluate
    if (!evaluateCanApplyUISide(dim, deltaX, deltaY))
        return NO_CHANGE;

    if (deltaX == 0 && deltaY == 0)
    {
        return NO_CHANGE;
    }

    if (deltaX != 0)
    {
        mv |= (int)(DeltaResult::KEYRANGE_CHANGED);
    }

    if (deltaY != 0)
    {
        mv |= (int)(DeltaResult::VELOCITY_CHANGED);
    }

    if (updateLead)
    {
        // Update the lead zone so the sidebar updates
        engine::Zone::applyChange(dim, deltaX, deltaY, mappingView);
    }

    // it'll be a minute until we get this back from the server so keep local cache
    // ready for fast drags

    auto leadOnly = juce::ModifierKeys::getCurrentModifiers().isAltDown();
    if (!leadOnly)
    {
        for (auto &z : summary)
        {
            if (editor->isSelected(z.address))
            {
                auto zmd = engine::Zone::ZoneMappingData();
                zmd.rootKey = 0;
                zmd.keyboardRange = z.kr;
                zmd.velocityRange = z.vr;
                engine::Zone::applyChange(dim, deltaX, deltaY, zmd);
                z.kr = zmd.keyboardRange;
                z.vr = zmd.velocityRange;
            }
        }
    }

    if (mayBeAboutToMutate)
    {
        sendToSerialization(cmsg::BeginZoneMappingModification(true));
        mayBeAboutToMutate = false;
    }

    // this is absolute, lead-only, part, dim, dx, dy
    sendToSerialization(
        cmsg::ApplyZoneDelta({false, leadOnly, editor->selectedPart, dim, deltaX, deltaY}));

    return (DeltaResult)mv;
}

void MappingDisplay::adjustDeltasToMakeSureTheyWillFit(engine::Zone::ChangeDimension dim,
                                                       int &deltaX, int &deltaY,
                                                       const engine::KeyboardRange &kr,
                                                       const engine::VelocityRange &vr) const
{
    // You'd think we could avoid this by checking the constraint
    // but this true up helps us make sure fast mouse movements near
    // the edge get clamped appropriately
    auto iDim = (int)dim;
    auto isMove = dim == engine::Zone::MOVE_CTR;
    if (kr.keyStart + deltaX < 0 &&
        (isMove || (dim & engine::Zone::ChangeDimension::KEY_RANGE_START)))
        deltaX = -kr.keyStart;
    if (kr.keyEnd + deltaX > 127 &&
        (isMove || (dim & engine::Zone::ChangeDimension::KEY_RANGE_END)))
        deltaX = 127 - kr.keyEnd;
    if (vr.velStart + deltaY < 0 &&
        (isMove || (dim & engine::Zone::ChangeDimension::VEL_RANGE_START)))
        deltaY = -vr.velStart;
    if (vr.velEnd + deltaY >= 127 &&
        (isMove || (dim & engine::Zone::ChangeDimension::VEL_RANGE_END)))
        deltaY = 127 - vr.velEnd;
}

bool MappingDisplay::applyAbsoluteToSelectedZones(engine::Zone::ChangeDimension dim, int newX,
                                                  int newY, bool updateLead)
{
    assert(!updateLead);
    if (updateLead)
    {
        SCLOG_IF(warnings, "Warning: Update Lead on applyAbsolute not implemented (because we "
                           "didn't need it yet)");
    }

    bool doApply = true;
    for (const auto &z : summary)
    {
        if (editor->isSelected(z.address))
        {
            doApply =
                doApply && engine::Zone::canApplyAbsoluteBoundEdit(dim, newX, newY, z.kr, z.vr);
        }
    }

    if (!doApply)
        return false;

    if (mayBeAboutToMutate)
    {
        sendToSerialization(cmsg::BeginZoneMappingModification(true));
        mayBeAboutToMutate = false;
    }

    auto leadOnly = juce::ModifierKeys::getCurrentModifiers().isAltDown();
    sendToSerialization(
        cmsg::ApplyZoneDelta({true, leadOnly, editor->selectedPart, dim, newX, newY}));
    return true;
}

bool MappingDisplay::evaluateCanApplyUISide(engine::Zone::ChangeDimension dim, int deltaX,
                                            int deltaY) const
{
    bool doApply = true;

    for (const auto &z : summary)
    {
        if (editor->isSelected(z.address))
        {
            doApply = doApply && engine::Zone::canApplyChange(dim, deltaX, deltaY, z.kr, z.vr);
        }
    }

    return doApply;
}
} // namespace scxt::ui::app::edit_screen