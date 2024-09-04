/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#include "MappingDisplay.h"
#include "app/edit-screen/components/MacroMappingVariantPane.h"

#include "ZoneLayoutDisplay.h"
#include "ZoneLayoutKeyboard.h"

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

    // Start here tomorrow
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
    };
    auto iAddConstrained = [this](auto &v, auto &a, auto &w, std::function<bool(int)> accept) {
        ifac::attachAndAdd(mappingView, v, this, a, w);
        connectors::addGuiStepBeforeSend(*a, [&aWrite = a, accept](const auto &a) {
            if (!accept(aWrite->value))
                aWrite->value = a.prevValue;
        });
    };

    auto fAdd = [this](auto &v, auto &a, auto &w) {
        ffac::attachAndAdd(mappingView, v, this, a, w);
    };

    auto iAddFadeConstraint = [this](auto &a, auto &r, bool isStart) {
        if constexpr (std::is_same_v<std::remove_reference_t<decltype(r)>,
                                     scxt::engine::KeyboardRange>)
        {
            connectors::addGuiStepBeforeSend(*a, [&aWrite = a, &r, isStart](const auto &a) {
                auto span = r.keyEnd - r.keyStart;
                auto fadeSpan = a.value + (isStart ? r.fadeEnd : r.fadeStart) + 1;
                if (fadeSpan > span + 1)
                {
                    aWrite->value = a.prevValue;
                }
            });
        }
        if constexpr (std::is_same_v<std::remove_reference_t<decltype(r)>,
                                     scxt::engine::VelocityRange>)
        {
            connectors::addGuiStepBeforeSend(*a, [&aWrite = a, &r, isStart](const auto &a) {
                auto span = r.velEnd - r.velStart;
                auto fadeSpan = a.value + (isStart ? r.fadeEnd : r.fadeStart) + 1;
                if (fadeSpan > span + 1)
                {
                    aWrite->value = a.prevValue;
                }
            });
        }
    };

    iAdd(mappingView.rootKey, intAttachments.RootKey, textEds.RootKey);
    makeLabel(labels.RootKey, "Root Key");

    iAddConstrained(mappingView.keyboardRange.keyStart, intAttachments.KeyStart, textEds.KeyStart,
                    [w = juce::Component::SafePointer(this)](auto v) {
                        if (!w)
                            return true;
                        return v <= w->mappingView.keyboardRange.keyEnd;
                    });
    iAddConstrained(mappingView.keyboardRange.keyEnd, intAttachments.KeyEnd, textEds.KeyEnd,
                    [w = juce::Component::SafePointer(this)](auto v) {
                        if (!w)
                            return true;
                        return v >= w->mappingView.keyboardRange.keyStart;
                    });

    makeLabel(labels.KeyStart, "Key Range");
    iAdd(mappingView.keyboardRange.fadeStart, intAttachments.FadeStart, textEds.FadeStart);
    iAddFadeConstraint(intAttachments.FadeStart, mappingView.keyboardRange, true);
    iAdd(mappingView.keyboardRange.fadeEnd, intAttachments.FadeEnd, textEds.FadeEnd);
    iAddFadeConstraint(intAttachments.FadeEnd, mappingView.keyboardRange, false);
    makeLabel(labels.FadeStart, "Crossfade");

    iAddConstrained(mappingView.velocityRange.velStart, intAttachments.VelStart, textEds.VelStart,
                    [w = juce::Component::SafePointer(this)](auto v) {
                        if (!w)
                            return true;
                        return v <= w->mappingView.velocityRange.velEnd;
                    });
    iAddConstrained(mappingView.velocityRange.velEnd, intAttachments.VelEnd, textEds.VelEnd,
                    [w = juce::Component::SafePointer(this)](auto v) {
                        if (!w)
                            return true;
                        return v >= w->mappingView.velocityRange.velStart;
                    });
    makeLabel(labels.VelStart, "Vel Range");

    iAdd(mappingView.velocityRange.fadeStart, intAttachments.VelFadeStart, textEds.VelFadeStart);
    iAddFadeConstraint(intAttachments.VelFadeStart, mappingView.velocityRange, true);
    iAdd(mappingView.velocityRange.fadeEnd, intAttachments.VelFadeEnd, textEds.VelFadeEnd);
    iAddFadeConstraint(intAttachments.VelFadeEnd, mappingView.velocityRange, false);

    makeLabel(labels.VelFadeStart, "CrossFade");

    iAdd(mappingView.pbDown, intAttachments.PBDown, textEds.PBDown);
    iAdd(mappingView.pbUp, intAttachments.PBUp, textEds.PBUp);
    makeLabel(labels.PBDown, "Pitch Bend");

    fAdd(mappingView.amplitude, floatAttachments.Level, textEds.Level);
    makeGlyph(glyphs.Level, sst::jucegui::components::GlyphPainter::VOLUME);

    fAdd(mappingView.pan, floatAttachments.Pan, textEds.Pan);
    makeGlyph(glyphs.Pan, sst::jucegui::components::GlyphPainter::PAN);

    fAdd(mappingView.pitchOffset, floatAttachments.Pitch, textEds.Pitch);
    makeGlyph(glyphs.Pitch, sst::jucegui::components::GlyphPainter::TUNING);
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
    zoneHeader->setBounds(b.withHeight(headerSize));

    // Mapping Display
    auto z = b.withTrimmedTop(headerSize);
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
        mappingZones->repaint();
    repaint();
}

void MappingDisplay::setLeadSelection(const selection::SelectionManager::ZoneAddress &za)
{
    // but we need to call setLeadZoneBounds to make the hotspots so rename that too
    for (const auto &s : summary)
    {
        if (s.first == za)
        {
            if (mappingZones)
                mappingZones->setLeadZoneBounds(s.second);
        }
    }
}

int MappingDisplay::voiceCountFor(const selection::SelectionManager::ZoneAddress &z)
{
    int res{0};
    for (const auto &v : editor->sharedUiMemoryState.voiceDisplayItems)
    {
        if (v.active && v.part == z.part && v.group == z.group && v.zone == z.zone)
        {
            res++;
        }
    }
    return res;
}

std::optional<std::string> MappingDisplay::sourceDetailsDragAndDropSample(
    const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    auto w = dragSourceDetails.sourceComponent;
    if (w)
    {
        auto p = w->getProperties().getVarPointer("DragAndDropSample");
        if (p && p->isString())
        {
            return p->toString().toStdString();
        }
    }
    return std::nullopt;
}

bool MappingDisplay::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    auto os = sourceDetailsDragAndDropSample(dragSourceDetails);
    return os.has_value();
}
void MappingDisplay::itemDropped(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    auto os = sourceDetailsDragAndDropSample(dragSourceDetails);
    if (os.has_value())
    {
        namespace cmsg = scxt::messaging::client;
        auto r = mappingZones->rootAndRangeForPosition(dragSourceDetails.localPosition);
        sendToSerialization(cmsg::AddSampleWithRange({*os, r[0], r[1], r[2], 0, 127}));
    }
    isUndertakingDrop = false;
    repaint();
}
void MappingDisplay::itemDragEnter(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    // TODO Compound types are special here
    isUndertakingDrop = true;
    currentDragPoint = dragSourceDetails.localPosition;
    repaint();
}

void MappingDisplay::itemDragExit(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    isUndertakingDrop = false;
    repaint();
}

void MappingDisplay::itemDragMove(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    currentDragPoint = dragSourceDetails.localPosition;
    repaint();
}

bool MappingDisplay::isInterestedInFileDrag(const juce::StringArray &files)
{
    return std::all_of(files.begin(), files.end(), [this](const auto &f) {
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
}

void MappingDisplay::fileDragEnter(const juce::StringArray &files, int x, int y)
{
    isUndertakingDrop = true;
    currentDragPoint = {x, y};
    repaint();
}
void MappingDisplay::fileDragMove(const juce::StringArray &files, int x, int y)
{
    isUndertakingDrop = true;
    currentDragPoint = {x, y};
    repaint();
}
void MappingDisplay::fileDragExit(const juce::StringArray &files)
{
    isUndertakingDrop = false;
    repaint();
}
void MappingDisplay::filesDropped(const juce::StringArray &files, int x, int y)
{
    namespace cmsg = scxt::messaging::client;
    auto r = mappingZones->rootAndRangeForPosition({x, y});
    for (auto f : files)
    {
        sendToSerialization(cmsg::AddSampleWithRange({f.toStdString(), r[0], r[1], r[2], 0, 127}));
    }
    isUndertakingDrop = false;
    repaint();
}

} // namespace scxt::ui::app::edit_screen