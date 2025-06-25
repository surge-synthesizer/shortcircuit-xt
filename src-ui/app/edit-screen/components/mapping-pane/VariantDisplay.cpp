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

#include "VariantDisplay.h"
#include "app/SCXTEditor.h"
#include "messaging/client/client_serial.h"
#include "SampleWaveform.h"
#include "app/browser-ui/BrowserPaneInterfaces.h"

namespace scxt::ui::app::edit_screen
{
namespace cmsg = scxt::messaging::client;
namespace jcmp = sst::jucegui::components;

struct NoSelection : juce::Component, HasEditor
{
    NoSelection(SCXTEditor *e) : HasEditor(e) {}
    void paint(juce::Graphics &g) override
    {
        g.fillAll(editor->themeColor(theme::ColorMap::bg_2));
        g.setColour(editor->themeColor(theme::ColorMap::generic_content_medium));
        g.setFont(editor->themeApplier.interMediumFor(11));
        g.drawText("Select a zone to use Sample tab", getLocalBounds(),
                   juce::Justification::centred);
    }
};

VariantDisplay::VariantDisplay(scxt::ui::app::edit_screen::MacroMappingVariantPane *p)
    : HasEditor(p->editor), variantView(p->sampleView), parentPane(p)
{
    noSelectionOverlay = std::make_unique<NoSelection>(editor);
    addChildComponent(*noSelectionOverlay);

    waveformsTabbedGroup = std::make_unique<MyTabbedComponent>(this);
    waveformsTabbedGroup->onTabPopupMenu = [w = juce::Component::SafePointer(this)](auto id) {
        if (w)
            w->showVariantTabMenu(id);
    };
    addAndMakeVisible(*waveformsTabbedGroup);
    for (auto i = 0; i < maxVariantsPerZone; ++i)
    {
        auto wf = std::make_unique<SampleWaveform>(this);
        wf->onPopupMenu = [i, w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showVariantTabMenu(i);
        };
        waveforms[i].waveformViewport = std::make_unique<jcmp::ZoomContainer>(std::move(wf));
        waveforms[i].waveformViewport->setVZoomFloor(1.0 / 16.0);

        waveforms[i].waveform = static_cast<SampleWaveform *>(
            waveforms[i].waveformViewport->contents->associatedComponent());
    }

    variantPlayModeLabel = std::make_unique<jcmp::Label>();
    variantPlayModeLabel->setText("Variants");
    addAndMakeVisible(*variantPlayModeLabel);

    variantPlaymodeButton = std::make_unique<jcmp::MenuButton>();
    variantPlaymodeButton->setOnCallback([this]() { showVariantPlaymodeMenu(); });
    addAndMakeVisible(*variantPlaymodeButton);

    editAllButton = std::make_unique<jcmp::TextPushButton>();
    editAllButton->setLabel("EDIT ALL");
    editAllButton->setOnCallback(editor->makeComingSoon("Edit All"));
    addAndMakeVisible(*editAllButton);

    fileLabel = std::make_unique<jcmp::Label>();
    fileLabel->setText("File");
    addAndMakeVisible(*fileLabel);

    fileButton = std::make_unique<jcmp::MenuButton>();
    fileButton->setOnCallback([this]() { showFileBrowser(); });
    addAndMakeVisible(*fileButton);

    nextFileButton = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::GlyphType::JOG_DOWN);
    nextFileButton->setOnCallback([this]() { selectNextFile(true); });
    addAndMakeVisible(*nextFileButton);

    prevFileButton = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::GlyphType::JOG_UP);
    prevFileButton->setOnCallback([this]() { selectNextFile(false); });
    addAndMakeVisible(*prevFileButton);

    auto ms = std::make_unique<jcmp::ToggleButton>();
    ms->setGlyph(jcmp::GlyphPainter::GlyphType::SHOW_INFO);
    ms->setDrawMode(jcmp::ToggleButton::DrawMode::GLYPH_WITH_BG);
    addAndMakeVisible(*ms);
    auto ds = std::make_unique<boolToggle_t>(ms, fileInfoShowing);
    ds->onValueChanged = [w = juce::Component::SafePointer(this)](auto v) {
        if (w)
            w->showFileInfos();
    };

    fileInfoButton = std::move(ds);

    fileInfos = std::make_unique<FileInfos>(this);
    addChildComponent(*fileInfos);

    rebuildForSelectedVariation(selectedVariation, true);
}

void VariantDisplay::rebuildForSelectedVariation(size_t sel, bool rebuildTabs)
{
    selectedVariation = sel;
    size_t emptySlot = 0;
    for (int i = 0; i < maxVariantsPerZone; ++i)
    {
        if (!variantView.variants[i].active)
        {
            emptySlot = i;
            break;
        }
    }
    selectedVariation = std::min(emptySlot, sel);

    // This wierd pattern is because for some reason we rebuild the components
    // when we select a new variant but not the header.
    // TODO - sort this out
    if (playModeLabel)
    {
        removeChildComponent(playModeLabel.get());
        playModeLabel.reset();
    }

    playModeLabel = std::make_unique<jcmp::Label>();
    playModeLabel->setText("Play");
    addAndMakeVisible(*playModeLabel);

    if (playModeButton)
    {
        removeChildComponent(playModeButton.get());
        playModeButton.reset();
    }
    playModeButton = std::make_unique<jcmp::MenuButton>();
    playModeButton->setOnCallback([this]() { showPlayModeMenu(); });
    addAndMakeVisible(*playModeButton);

    if (loopModeButton)
    {
        removeChildComponent(loopModeButton.get());
        loopModeButton.reset();
    }
    loopModeButton = std::make_unique<jcmp::MenuButton>();
    loopModeButton->setOnCallback([this]() { showLoopModeMenu(); });
    addAndMakeVisible(*loopModeButton);

    if (zoomButton)
    {
        removeChildComponent(zoomButton.get());
    }
    zoomButton = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::GlyphType::SEARCH);
    zoomButton->setOnCallback(editor->makeComingSoon("Sample Auto Zoom"));
    addAndMakeVisible(*zoomButton);

    auto attachSamplePoint = [this](Ctrl c, const std::string &aLabel, auto &v) {
        auto at = std::make_unique<connectors::SamplePointDataAttachment>(
            v, [this](const auto &) { onSamplePointChangedFromGUI(); });
        auto sl = std::make_unique<jcmp::DraggableTextEditableValue>();
        sl->setSource(at.get());
        if (sampleEditors[c])
        {
            removeChildComponent(sampleEditors[c].get());
            sampleEditors[c].reset();
            sampleAttachments[c].reset();
        }
        addAndMakeVisible(*sl);
        sampleEditors[c] = std::move(sl);
        sampleAttachments[c] = std::move(at);
    };

    auto attachFloatSamplePoint = [this](Ctrl c, const std::string &aLabel, auto &v) {
        auto pmd = scxt::datamodel::describeValue(variantView.variants[selectedVariation], v);

        auto at = std::make_unique<floatAttachment_t>(
            pmd, [this](const auto &) { onSamplePointChangedFromGUI(); }, v);
        auto sl = std::make_unique<jcmp::DraggableTextEditableValue>();
        sl->setSource(at.get());
        if (sampleEditors[c])
        {
            removeChildComponent(sampleEditors[c].get());
            sampleEditors[c].reset();
            sampleAttachments[c].reset();
        }
        addAndMakeVisible(*sl);
        sampleEditors[c] = std::move(sl);
        sampleFloatAttachments[c] = std::move(at);
    };

    auto attachToDummy = [this](Ctrl c, const std::string &aLabel, uint32_t idx) {
        sampleEditors[c] = connectors::makeConnectedToDummy<jcmp::DraggableTextEditableValue>(
            idx, aLabel, 0.f, false, editor->makeComingSoon(aLabel));
        addAndMakeVisible(*sampleEditors[c]);
    };

    auto addLabel = [this](Ctrl c, const std::string &label) {
        auto l = std::make_unique<jcmp::Label>();
        l->setJustification(juce::Justification::centredRight);
        l->setText(label);
        if (labels[c])
        {
            removeChildComponent(labels[c].get());
            labels[c].reset();
        }
        addAndMakeVisible(*l);
        labels[c] = std::move(l);
    };

    auto addGlyph = [this](Ctrl c, const jcmp::GlyphPainter::GlyphType g) {
        auto l = std::make_unique<jcmp::GlyphPainter>(g);
        if (glyphLabels[c])
        {
            removeChildComponent(glyphLabels[c].get());
        }
        addAndMakeVisible(*l);
        glyphLabels[c] = std::move(l);
    };

    attachSamplePoint(startP, "StartS", variantView.variants[selectedVariation].startSample);
    sampleAttachments[startP]->precheckGuiAdjust = [this](auto f) {
        return std::max(std::min(f, this->variantView.variants[this->selectedVariation].endSample),
                        (int64_t)0);
    };
    addLabel(startP, "Start");
    attachSamplePoint(endP, "EndS", variantView.variants[selectedVariation].endSample);
    sampleAttachments[endP]->precheckGuiAdjust = [this](auto f) {
        return std::max(f, this->variantView.variants[this->selectedVariation].startSample);
    };
    addLabel(endP, "End");
    attachSamplePoint(startL, "StartL", variantView.variants[selectedVariation].startLoop);
    sampleAttachments[startL]->precheckGuiAdjust = [this](auto f) {
        return std::min(f, this->variantView.variants[this->selectedVariation].endLoop);
    };

    editor->themeApplier.applyVariantLoopTheme(sampleEditors[startL].get());
    addLabel(startL, "Start");
    attachSamplePoint(endL, "EndL", variantView.variants[selectedVariation].endLoop);
    sampleAttachments[endL]->precheckGuiAdjust = [this](auto f) {
        return std::max(f, this->variantView.variants[this->selectedVariation].startLoop);
    };
    editor->themeApplier.applyVariantLoopTheme(sampleEditors[endL].get());
    addLabel(endL, "End");
    attachSamplePoint(fadeL, "fadeL", variantView.variants[selectedVariation].loopFade);
    editor->themeApplier.applyVariantLoopTheme(sampleEditors[fadeL].get());
    addLabel(fadeL, "XF");

    attachToDummy(curve, "Curve", 'vcrv');
    addGlyph(curve, jcmp::GlyphPainter::GlyphType::CURVE);
    editor->themeApplier.applyVariantLoopTheme(sampleEditors[curve].get());

    if (srcButton)
    {
        removeChildComponent(srcButton.get());
    }
    srcButton = std::make_unique<jcmp::MenuButton>();
    srcButton->centerTextAndExcludeArrow = true;
    srcButton->setLabel("SRC");
    srcButton->setOnCallback([this]() { showSRCMenu(); });
    addAndMakeVisible(*srcButton);
    addLabel(src, "SRC");

    attachFloatSamplePoint(volume, "Volume", variantView.variants[selectedVariation].amplitude);
    addGlyph(volume, jcmp::GlyphPainter::GlyphType::VOLUME);

    attachFloatSamplePoint(trak, "Tracking", variantView.variants[selectedVariation].tracking);
    addLabel(trak, "Trk");

    attachFloatSamplePoint(tune, "Tuning", variantView.variants[selectedVariation].pitchOffset);
    addGlyph(tune, jcmp::GlyphPainter::GlyphType::TUNING);

    if (loopActive)
    {
        removeChildComponent(loopActive.get());
        loopActive.reset();
        loopAttachment.reset();
    }

    loopAttachment =
        std::make_unique<connectors::BooleanPayloadDataAttachment<engine::Zone::Variants>>(
            "Loop",
            [w = juce::Component::SafePointer(this)](const auto &a) {
                if (w)
                {
                    w->onSamplePointChangedFromGUI();
                    w->rebuild();
                }
            },
            variantView.variants[selectedVariation].loopActive);
    loopActive = std::make_unique<jcmp::ToggleButton>();
    loopActive->setLabel(u8"\U0000221E");
    loopActive->setSource(loopAttachment.get());
    editor->themeApplier.applyVariantLoopTheme(loopActive.get());
    addAndMakeVisible(*loopActive);

    if (reverseActive)
    {
        removeChildComponent(reverseActive.get());
        reverseActive.reset();
        reverseAttachment.reset();
    }

    reverseAttachment =
        std::make_unique<connectors::BooleanPayloadDataAttachment<engine::Zone::Variants>>(
            "Reverse",
            [w = juce::Component::SafePointer(this)](const auto &a) {
                if (w)
                {
                    w->onSamplePointChangedFromGUI();
                }
            },
            variantView.variants[selectedVariation].playReverse);

    reverseActive = std::make_unique<jcmp::ToggleButton>();
    reverseActive->setDrawMode(jcmp::ToggleButton::DrawMode::GLYPH_WITH_BG);
    reverseActive->setGlyph(jcmp::GlyphPainter::GlyphType::REVERSE);
    reverseActive->setSource(reverseAttachment.get());
    addAndMakeVisible(*reverseActive);

    if (loopCnt)
    {
        removeChildComponent(loopCnt.get());
        loopCnt.reset();
        loopCntAttachment.reset();
    }

    loopCntAttachment = std::make_unique<sample_attachment_t>(
        datamodel::pmd()
            .withType(datamodel::pmd::INT)
            .withName("LoopCnt")
            .withRange(0, (float)UCHAR_MAX)
            .withDecimalPlaces(0)
            .withLinearScaleFormatting(""),
        [w = juce::Component::SafePointer(this)](const auto &a) {
            if (w)
            {
                w->onSamplePointChangedFromGUI();
            }
        },
        variantView.variants[selectedVariation].loopCountWhenCounted);
    loopCnt = std::make_unique<jcmp::DraggableTextEditableValue>();
    loopCnt->setSource(loopCntAttachment.get());
    addAndMakeVisible(*loopCnt);

    if (rebuildTabs)
    {
        auto selectedTab{selectedVariation};
        auto bg2 = editor->themeColor(theme::ColorMap::bg_2);
        waveformsTabbedGroup->clearTabs();
        for (auto i = 0; i < maxVariantsPerZone; ++i)
        {
            if (variantView.variants[i].active)
            {
                waveformsTabbedGroup->addTab(std::to_string(i + 1), bg2,
                                             waveforms[i].waveformViewport.get(), false, i);
            }
        }
        if (waveformsTabbedGroup->getNumTabs() < maxVariantsPerZone)
        {
            waveformsTabbedGroup->addTab("+", bg2,
                                         waveforms[maxVariantsPerZone - 1].waveformViewport.get(),
                                         false, maxVariantsPerZone - 1);
        }
        waveformsTabbedGroup->setCurrentTabIndex(selectedTab, true);
    }

    divider = std::make_unique<jcmp::NamedPanelDivider>(false);
    addAndMakeVisible(*divider);
    rebuild();

    auto hasSelZone = editor->currentLeadZoneSelection.has_value() &&
                      editor->currentLeadZoneSelection->zone >= 0 &&
                      editor->currentLeadZoneSelection->group >= 0 &&
                      editor->currentLeadZoneSelection->part >= 0;

    noSelectionOverlay->setVisible(!hasSelZone);
    if (noSelectionOverlay->isVisible())
        noSelectionOverlay->toFront(true);
    resized();
}

void VariantDisplay::resized()
{
    auto &v = variantView.variants[selectedVariation];

    auto viewArea = sampleDisplayRegion();
    waveformsTabbedGroup->setBounds(viewArea);
    auto p = getLocalBounds().withLeft(getLocalBounds().getWidth() - sidePanelWidth);

    auto npw = 3;
    auto npd = getLocalBounds()
                   .withLeft(getLocalBounds().getWidth() - sidePanelWidth - sidePanelDividerPad)
                   .withWidth(sidePanelDividerPad);
    npd = npd.reduced((npd.getWidth() - npw) * 0.5, 4).withTrimmedTop(20);
    divider->setBounds(npd);

    p = p.withHeight(18).translated(0, 24);
    auto row = p;

    int padding{4};
    auto ofRow = [&row, padding](const auto &wid, auto w) {
        wid->setBounds(row.withWidth(w));
        row = row.translated(w + padding, 0);
    };
    auto addPad = [&row](auto w) { row = row.translated(w, 0); };
    auto addGap = [&row, &p](auto g) {
        row = row.translated(0, g);
        p = p.translated(0, g);
    };
    auto newRow = [&row, &p, padding](auto &s) {
        if (row.getX() != p.getRight() + padding)
        {
            SCLOG(s << " Mismatched p layout " << row.getX() << " " << p.getRight());
        }
        p = p.translated(0, 23);
        row = p;
    };

    ofRow(playModeLabel, 27);
    ofRow(playModeButton, 87);
    ofRow(reverseActive, 14);
    newRow("playMode");

    for (const auto m : {startP, endP})
    {
        ofRow(labels[m], p.getWidth() - 72 - padding);
        ofRow(sampleEditors[m], 72);
        newRow("ctrl");
    }

    addGap(10);
    if (v.loopMode == engine::Zone::LOOP_COUNT)
    {
        ofRow(loopModeButton, 88);
        ofRow(loopCnt, 26);
    }
    else
    {
        ofRow(loopModeButton, 118);
    }
    ofRow(loopActive, 14);
    newRow("loop");

    ofRow(zoomButton, 16);
    ofRow(labels[startL], p.getWidth() - 72 - 16 - 2 * padding);
    ofRow(sampleEditors[startL], 72);
    newRow("endL");

    ofRow(labels[endL], p.getWidth() - 72 - padding);
    ofRow(sampleEditors[endL], 72);
    newRow("endL");

    // 25 34 18 40
    ofRow(labels[fadeL], 25);
    ofRow(sampleEditors[fadeL], 34);
    addPad(7);
    ofRow(glyphLabels[curve], 18);
    ofRow(sampleEditors[curve], 40);
    newRow("fadecurve");

    addGap(10);

    ofRow(labels[src], 25);
    ofRow(srcButton, 34);
    addPad(7);
    ofRow(glyphLabels[volume], 18);
    ofRow(sampleEditors[volume], 40);
    newRow("srcvol");

    ofRow(labels[trak], 25);
    ofRow(sampleEditors[trak], 34);
    addPad(7);
    ofRow(glyphLabels[tune], 18);
    ofRow(sampleEditors[tune], 40);
    newRow("traktune");

    /*
     * This is the section avovce the waveform
     */

    auto hl = getLocalBounds().withHeight(20).reduced(1).withTrimmedLeft(350);
    auto hP = [&hl, this](int w) {
        int margin{3};
        if (w < 0)
        {
            auto lbw = getLocalBounds().getWidth();
            auto spc = lbw - hl.getX() + w - margin;
            w = spc;
        }
        auto r = hl.withWidth(w);
        hl = hl.translated(w + margin, 0);
        return r;
    };

    variantPlayModeLabel->setBounds(hP(42));
    variantPlaymodeButton->setBounds(hP(100));
    editAllButton->setBounds(hP(55));
    fileInfoButton->widget->setBounds(hP(hl.getHeight()));
    fileLabel->setBounds(hP(20));
    fileButton->setBounds(hP(-14));

    auto jogs = hP(14);
    prevFileButton->setBounds(jogs.withTrimmedBottom(jogs.getHeight() / 2));
    nextFileButton->setBounds(jogs.withTrimmedTop(jogs.getHeight() / 2));

    // the fileInfos is self bounding
    fileInfos->setBounds({0, waveformsTabbedGroup->getY() + 20, getWidth(), 70});

    noSelectionOverlay->setBounds(getLocalBounds());
}

void VariantDisplay::rebuild()
{
    switch (variantView.variants[selectedVariation].playMode)
    {
    case engine::Zone::NORMAL:
        playModeButton->setLabel("NORMAL");
        break;
    case engine::Zone::ONE_SHOT:
        playModeButton->setLabel("ONE-SHOT");
        break;
    case engine::Zone::ON_RELEASE:
        playModeButton->setLabel("ON RELEASE");
        break;
    }

    std::string dirExtra = "";
    if (variantView.variants[selectedVariation].loopDirection == engine::Zone::ALTERNATE_DIRECTIONS)
    {
        dirExtra += " ";
        dirExtra += u8"\U00002194"; // left right arrow
    }
    switch (variantView.variants[selectedVariation].loopMode)
    {
    case engine::Zone::LOOP_DURING_VOICE:
        loopModeButton->setLabel("LOOP" + dirExtra);
        break;
    case engine::Zone::LOOP_WHILE_GATED:
        loopModeButton->setLabel("LOOP UNTIL REL" + dirExtra);
        break;
    case engine::Zone::LOOP_COUNT:
        loopModeButton->setLabel("COUNT");
        break;
    }

    switch (variantView.variantPlaybackMode)
    {
    case engine::Zone::FORWARD_RR:
        variantPlaymodeButton->setLabel("ROUND ROBIN");
        break;
    case engine::Zone::TRUE_RANDOM:
        variantPlaymodeButton->setLabel("RANDOM");
        break;
    case engine::Zone::RANDOM_CYCLE:
        variantPlaymodeButton->setLabel("EXCL RAND");
        break;
    case engine::Zone::UNISON:
        variantPlaymodeButton->setLabel("UNISON");
        break;
    }

    switch (variantView.variants[selectedVariation].interpolationType)
    {
    case dsp::InterpolationTypes::Sinc:
        srcButton->setLabel("SINC");
        break;
    case dsp::InterpolationTypes::Linear:
        srcButton->setLabel("LIN");
        break;
    case dsp::InterpolationTypes::ZOHAA:
        srcButton->setLabel("ZAA");
        break;
    case dsp::InterpolationTypes::ZeroOrderHold:
        srcButton->setLabel("ZOH");
        break;
    }

    auto samp = editor->sampleManager.getSample(variantView.variants[selectedVariation].sampleID);
    if (samp)
    {
        for (auto &[k, a] : sampleAttachments)
        {
            if (a)
            {
                a->sampleCount = samp->sample_length;
            }
        }

        fileButton->setLabel(samp->displayName);
        fileInfos->sampleRate = samp->sample_rate;
        fileInfos->bd = samp->getBitDepthText();
        fileInfos->sampleLength = samp->sample_length;
        fileInfos->channels = samp->channels;
    }

    bool hasLoop = variantView.variants[selectedVariation].loopActive;
    loopModeButton->setEnabled(hasLoop);
    loopCnt->setEnabled(hasLoop);
    sampleEditors[startL]->setVisible(hasLoop);
    sampleEditors[endL]->setVisible(hasLoop);

    sampleEditors[fadeL]->setVisible(hasLoop);
    sampleEditors[curve]->setVisible(hasLoop);
    zoomButton->setVisible(hasLoop);
    labels[startL]->setVisible(hasLoop);
    labels[endL]->setVisible(hasLoop);
    labels[fadeL]->setVisible(hasLoop);
    glyphLabels[curve]->setVisible(hasLoop);

    loopCnt->setVisible(variantView.variants[selectedVariation].loopMode ==
                        engine::Zone::LoopMode::LOOP_COUNT);

    waveforms[selectedVariation].waveform->rebuildHotZones();

    resized();
    repaint();
}

void VariantDisplay::selectNextFile(bool selectForward)
{
    auto samp{editor->sampleManager.getSample(variantView.variants[selectedVariation].sampleID)};
    if (!samp)
        return;

    std::vector<std::string> v; // todo maybe reserve space for the vector

    try
    {
        auto samp_path = samp->getPath();
        if (samp_path.empty())
            return;

        auto parent_path{samp_path.parent_path()};
        if (parent_path.empty())
            return;

        for (const auto &entry : fs::directory_iterator(parent_path))
        {
            if (entry.is_regular_file() &&
                scxt::browser::Browser::isLoadableSingleSample(entry.path()))
            {
                v.push_back(entry.path().u8string());
            }
        }
    }
    catch (const fs::filesystem_error &err)
    {
        SCLOG("Filesystem error: " << err.what());
        return;
    }

    // todo sort alphabetically
    std::sort(v.begin(), v.end());

    auto it{std::find(v.begin(), v.end(), samp->getPath().string())};
    auto index{std::distance(v.begin(), it)};
    if (selectForward)
    {
        index++;
        index %= v.size();
    }
    else
    {
        index--;
        if (index < 0)
        {
            index += v.size();
        }
    }

    auto f{v[index]};
    namespace cmsg = scxt::messaging::client;
    auto za{editor->currentLeadZoneSelection};
    auto sampleID{selectedVariation};
    sendToSerialization(cmsg::AddSampleInZone({f, za->part, za->group, za->zone, sampleID}));
}

void VariantDisplay::MyTabbedComponent::currentTabChanged(int newCurrentTabIndex,
                                                          const juce::String &newCurrentTabName)
{
    // called with -1 when clearing tabs
    if (newCurrentTabIndex >= 0)
    {
        display->rebuildForSelectedVariation(newCurrentTabIndex, false);
        display->repaint();
    }

    // The way tabs work has sytlesheet implications which we
    // deal with in the parent class
    jcmp::TabbedComponent::currentTabChanged(newCurrentTabIndex, newCurrentTabName);
}

void VariantDisplay::showFileBrowser()

{
    std::string filePattern;
    for (auto s : scxt::browser::Browser::LoadableFile::singleSample)
    {
        filePattern += "*" + s + ",";
    }
    filePattern.erase(filePattern.size() - 1);

    fileChooser.reset(new juce::FileChooser("Select an audio file...", {}, filePattern));

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser &fc) mutable {
            if (fc.getURLResults().size() > 0)
            {
                const auto u = fc.getResult().getFullPathName();

                namespace cmsg = scxt::messaging::client;
                auto za{editor->currentLeadZoneSelection};
                auto sampleID{selectedVariation};
                sendToSerialization(
                    cmsg::AddSampleInZone({std::string{(const char *)(u.toUTF8())}, za->part,
                                           za->group, za->zone, sampleID}));
            }

            fileChooser = nullptr;
        },
        nullptr);
}

void VariantDisplay::onSamplePointChangedFromGUI()
{
    sendToSerialization(cmsg::UpdateLeadZoneSingleVariant{
        {selectedVariation, variantView.variants[selectedVariation]}});
    waveforms[selectedVariation].waveform->rebuildHotZones();
    waveforms[selectedVariation].waveform->repaint();
    repaint();
}

void VariantDisplay::showPlayModeMenu()
{
    juce::PopupMenu p;
    p.addSectionHeader("PlayMode");
    p.addSeparator();

    auto add = [&p, this](auto e, auto n) {
        p.addItem(n, true, variantView.variants[selectedVariation].playMode == e, [this, e]() {
            variantView.variants[selectedVariation].playMode = e;
            onSamplePointChangedFromGUI();
            rebuild();
        });
    };
    add(engine::Zone::PlayMode::NORMAL, "Normal");
    add(engine::Zone::PlayMode::ONE_SHOT, "OneShot");
    add(engine::Zone::PlayMode::ON_RELEASE, "On Release (t/k)");

    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

void VariantDisplay::showLoopModeMenu()
{
    juce::PopupMenu p;
    p.addSectionHeader("Loop Mode");
    p.addSeparator();

    auto add = [&p, this](auto e, bool alt, auto n) {
        auto that = this;
        p.addItem(n, true,
                  variantView.variants[selectedVariation].loopMode == e &&
                      variantView.variants[selectedVariation].loopDirection ==
                          (alt ? engine::Zone::LoopDirection::ALTERNATE_DIRECTIONS
                               : engine::Zone::LoopDirection::FORWARD_ONLY),
                  [w = juce::Component::SafePointer(that), e, alt]() {
                      if (!w)
                          return;
                      w->variantView.variants[w->selectedVariation].loopMode = e;
                      w->variantView.variants[w->selectedVariation].loopDirection =
                          (alt ? engine::Zone::LoopDirection::ALTERNATE_DIRECTIONS
                               : engine::Zone::LoopDirection::FORWARD_ONLY);
                      w->onSamplePointChangedFromGUI();
                      w->rebuild();
                  });
    };
    add(engine::Zone::LoopMode::LOOP_DURING_VOICE, false, "Loop");
    add(engine::Zone::LoopMode::LOOP_DURING_VOICE, true, "Loop Alternate");
    add(engine::Zone::LoopMode::LOOP_WHILE_GATED, false, "Loop Until Release");
    add(engine::Zone::LoopMode::LOOP_WHILE_GATED, true, "Loop Alternate Until Release");
    add(engine::Zone::LoopMode::LOOP_COUNT, false, "Loop For Count");

    p.showMenuAsync(editor->defaultPopupMenuOptions(loopModeButton.get()));
}

void VariantDisplay::showSRCMenu()
{
    juce::PopupMenu p;
    p.addSectionHeader("SRC (Interpolation Type)");
    p.addSeparator();

    auto add = [&p, this](auto e, auto n) {
        p.addItem(
            n, true, variantView.variants[selectedVariation].interpolationType == e, [this, e]() {
                variantView.variants[selectedVariation].interpolationType = e;
                connectors::updateSingleValue<cmsg::UpdateZoneVariantsInt16TValue>(
                    variantView, variantView.variants[selectedVariation].interpolationType, this);
                rebuild();
            });
    };
    add(dsp::InterpolationTypes::Sinc, "Sinc");
    add(dsp::InterpolationTypes::Linear, "Linear");
    add(dsp::InterpolationTypes::ZOHAA, "Zero-Order Hold (Anti-Aliased)");
    add(dsp::InterpolationTypes::ZeroOrderHold, "Zero Order Hold");

    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

void VariantDisplay::showVariantPlaymodeMenu()
{
    juce::PopupMenu p;
    p.addSectionHeader("Variant Play Mode");
    p.addSeparator();

    auto add = [&p, this](auto e, auto n) {
        p.addItem(n, true, variantView.variantPlaybackMode == e, [this, e]() {
            variantView.variantPlaybackMode = e;
            connectors::updateSingleValue<cmsg::UpdateZoneVariantsInt16TValue>(
                variantView, variantView.variantPlaybackMode, this);
            rebuild();
        });
    };
    add(engine::Zone::VariantPlaybackMode::FORWARD_RR, "Round Robin");
    add(engine::Zone::VariantPlaybackMode::TRUE_RANDOM, "Random");
    add(engine::Zone::VariantPlaybackMode::RANDOM_CYCLE, "Random (exclusive)");
    add(engine::Zone::VariantPlaybackMode::UNISON, "Unison");

    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

bool VariantDisplay::MyTabbedComponent::isInterestedInDragSource(
    const SourceDetails &dragSourceDetails)
{
    auto wsi = browser_ui::asSampleInfo(dragSourceDetails.sourceComponent);
    if (!wsi)
        return false;
    if (wsi->getCompoundElement().has_value())
        return wsi->getCompoundElement()->type == sample::compound::CompoundElement::SAMPLE;
    return wsi->getDirEnt().has_value();
}

bool VariantDisplay::MyTabbedComponent::isInterestedInFileDrag(const juce::StringArray &files)
{
    auto loadable = std::all_of(files.begin(), files.end(), [this](const auto &f) {
        try
        {
            auto pt = fs::path{(const char *)(f.toUTF8())};
            return editor->browser.isLoadableSingleSample(pt);
        }
        catch (fs::filesystem_error &e)
        {
        }
        return false;
    });
    return loadable && getNumTabs() <= maxVariantsPerZone;
}

void VariantDisplay::MyTabbedComponent::filesDropped(const juce::StringArray &files, int x, int y)
{
    auto processFilesDropped = [this](const juce::StringArray &files, auto tabIndex) {
        for (const auto &fl : files)
        {
            if (tabIndex == maxVariantsPerZone)
            {
                break;
            }
            namespace cmsg = scxt::messaging::client;
            auto za{editor->currentLeadZoneSelection};
            auto sampleID{tabIndex};
            sendToSerialization(cmsg::AddSampleInZone({std::string{(const char *)(fl.toUTF8())},
                                                       za->part, za->group, za->zone, sampleID}));
            tabIndex++;
        }
    };

    auto tabIndex{getTabIndexFromPosition(x, y)};
    if (tabIndex != -1)
    {
        processFilesDropped(files, tabIndex);
    }
}

void VariantDisplay::MyTabbedComponent::itemDropped(
    const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)

{
    auto wsi = browser_ui::asSampleInfo(dragSourceDetails.sourceComponent);

    if (wsi)
    {
        namespace cmsg = scxt::messaging::client;
        auto za{editor->currentLeadZoneSelection};
        auto sampleID{getTabIndexFromPosition(dragSourceDetails.localPosition.x,
                                              dragSourceDetails.localPosition.y)};
        if (wsi->getCompoundElement().has_value())
        {
            auto ce = *wsi->getCompoundElement();
            sendToSerialization(cmsg::AddCompoundElementInZone(
                {*wsi->getCompoundElement(), za->part, za->group, za->zone, sampleID}));
        }
        else if (wsi->getDirEnt().has_value())
        {
            sendToSerialization(cmsg::AddSampleInZone(
                {wsi->getDirEnt()->path().u8string(), za->part, za->group, za->zone, sampleID}));
        }
    }
}

int VariantDisplay::MyTabbedComponent::getTabIndexFromPosition(int x, int y)
{
    auto tabIndex{-1};
    auto isOnWaveform{getCurrentContentComponent() &&
                      getCurrentContentComponent()->getBounds().contains(juce::Point<int>{x, y})};
    if (isOnWaveform)
    {
        tabIndex = display->selectedVariation;
    }
    else
    {
        auto &tabBar{getTabbedButtonBar()};
        for (auto index = 0; index < tabBar.getNumTabs(); ++index)
        {
            auto isOnTabBar{
                tabBar.getTabButton(index)->getBounds().contains(juce::Point<int>{x, y})};
            if (isOnTabBar)
            {
                tabIndex = index;
                break;
            }
        }
    }

    // We are neither on waveform neither on tab bar
    // we add at the end, but we check there is a free slot
    if (tabIndex == -1 &&
        std::any_of(display->variantView.variants.begin(), display->variantView.variants.end(),
                    [](const auto &s) { return s.active == false; }))
    {
        tabIndex = getNumTabs() - 1;
    }

    return tabIndex;
}

namespace detail
{
class comma_numpunct : public std::numpunct<char>
{
  public:
    comma_numpunct(char thousands_sep, const char *grouping)
        : m_thousands_sep(thousands_sep), m_grouping(grouping)
    {
    }

  protected:
    char do_thousands_sep() const { return m_thousands_sep; }
    std::string do_grouping() const { return m_grouping; }

  private:
    char m_thousands_sep;
    std::string m_grouping;
};
} // namespace detail
void VariantDisplay::FileInfos::paint(juce::Graphics &g)
{
    std::locale comma_locale(std::locale(), new detail::comma_numpunct(',', "\03"));

    std::ostringstream oss;
    oss.imbue(comma_locale);
    oss << std::fixed << sampleLength;

    auto msg = fmt::format("{:.1f}kHz {}-chan {}. {} samples ({:.3f}s)", sampleRate / 1000,
                           channels, bd, oss.str(), sampleLength / sampleRate);
    int margin{5};
    auto ft = editor->themeApplier.interMediumFor(12);

    auto w = SST_STRING_WIDTH_FLOAT(ft, msg);
    auto bx = getLocalBounds().withWidth(w + 2 * margin).withHeight(18);
    g.setColour(editor->themeColor(theme::ColorMap::bg_3).withAlpha(0.5f));
    g.fillRect(bx);
    g.setColour(editor->themeColor(theme::ColorMap::bg_3));
    g.drawHorizontalLine(bx.getBottom(), bx.getX(), bx.getRight());
    g.drawVerticalLine(bx.getRight(), bx.getY(), bx.getBottom());

    g.setFont(ft);
    g.setColour(editor->themeColor(theme::ColorMap::generic_content_medium));
    g.drawText(msg, bx.reduced(margin, 0), juce::Justification::centred);
}

void VariantDisplay::showVariantTabMenu(int variantIdx)
{
    auto numVariants{0};
    for (auto i = 0; i < maxVariantsPerZone; ++i)
    {
        if (variantView.variants[i].active)
        {
            numVariants++;
        }
    }
    bool isPlus = variantIdx >= numVariants;

    juce::PopupMenu p;
    if (isPlus)
    {
        p.addSectionHeader("Add Variant");
        p.addSeparator();
        ;
        p.addItem("Copy From", editor->makeComingSoon("Copy From"));
        p.addItem("Sample", editor->makeComingSoon("Sample"));
    }
    else
    {
        const auto &var = variantView.variants[variantIdx];
        p.addSectionHeader("Variant " + std::to_string(variantIdx + 1));
        p.addSeparator();

        p.addItem("Copy", editor->makeComingSoon("Copy Variant"));
        p.addItem("Delete", [variantIdx, w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            w->sendToSerialization(cmsg::DeleteVariant(variantIdx));
        });
        p.addSeparator();
        p.addItem("Normalize Variant " + std::to_string(variantIdx + 1),
                  [variantIdx, w = juce::Component::SafePointer(this)]() {
                      if (!w)
                          return;
                      w->sendToSerialization(cmsg::NormalizeVariantAmplitude({variantIdx, true}));
                  });
        p.addItem("Normalize All Variants", [variantIdx, w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            w->sendToSerialization(cmsg::NormalizeVariantAmplitude({-1, true}));
        });

        p.addItem("Clear Variant " + std::to_string(variantIdx + 1) + " Normalization",
                  var.normalizationAmplitude != 1.0, false,
                  [variantIdx, w = juce::Component::SafePointer(this)]() {
                      if (!w)
                          return;
                      w->sendToSerialization(cmsg::ClearVariantAmplitudeNormalization(variantIdx));
                  });
        p.addItem("Clear All Variant Normalization",
                  [variantIdx, w = juce::Component::SafePointer(this)]() {
                      if (!w)
                          return;
                      w->sendToSerialization(cmsg::ClearVariantAmplitudeNormalization(-1));
                  });
    }
    p.showMenuAsync(editor->defaultPopupMenuOptions());
}
} // namespace scxt::ui::app::edit_screen
