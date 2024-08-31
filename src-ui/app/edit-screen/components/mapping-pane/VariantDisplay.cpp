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

namespace scxt::ui::app::edit_screen
{
namespace cmsg = scxt::messaging::client;

VariantDisplay::VariantDisplay(scxt::ui::app::edit_screen::MacroMappingVariantPane *p)
    : HasEditor(p->editor), variantView(p->sampleView), parentPane(p)
{
    waveformsTabbedGroup = std::make_unique<MyTabbedComponent>(this);
    addAndMakeVisible(*waveformsTabbedGroup);
    for (auto i = 0; i < maxVariantsPerZone; ++i)
    {
        waveforms[i].waveformViewport = std::make_unique<sst::jucegui::components::ZoomContainer>(
            std::make_unique<SampleWaveform>(this));
        waveforms[i].waveform = static_cast<SampleWaveform *>(
            waveforms[i].waveformViewport->contents->associatedComponent());
    }

    variantPlayModeLabel = std::make_unique<sst::jucegui::components::Label>();
    variantPlayModeLabel->setText("Variant");
    addAndMakeVisible(*variantPlayModeLabel);

    variantPlaymodeButton = std::make_unique<juce::TextButton>("Variant");
    variantPlaymodeButton->onClick = [this]() { showVariantPlaymodeMenu(); };
    addAndMakeVisible(*variantPlaymodeButton);

    fileLabel = std::make_unique<sst::jucegui::components::Label>();
    fileLabel->setText("File");
    addAndMakeVisible(*fileLabel);

    fileButton = std::make_unique<juce::TextButton>("file");
    fileButton->onClick = [this]() { showFileBrowser(); };
    addAndMakeVisible(*fileButton);

    nextFileButton = std::make_unique<juce::TextButton>(">");
    nextFileButton->onClick = [this]() { selectNextFile(true); };
    addAndMakeVisible(*nextFileButton);

    prevFileButton = std::make_unique<juce::TextButton>("<");
    prevFileButton->onClick = [this]() { selectNextFile(false); };
    addAndMakeVisible(*prevFileButton);

    fileInfoButton = std::make_unique<juce::TextButton>("i");
    fileInfoButton->setClickingTogglesState(true);
    fileInfoButton->onClick = [this]() { showFileInfos(); };
    addAndMakeVisible(*fileInfoButton);

    fileInfos = std::make_unique<FileInfos>();
    addChildComponent(*fileInfos);

    rebuildForSelectedVariation(selectedVariation, true);
}

void VariantDisplay::rebuildForSelectedVariation(size_t sel, bool rebuildTabs)
{
    selectedVariation = sel;

    if (playModeLabel)
    {
        removeChildComponent(playModeLabel.get());
        playModeLabel.reset();
    }

    playModeLabel = std::make_unique<sst::jucegui::components::Label>();
    playModeLabel->setText("Play");
    addAndMakeVisible(*playModeLabel);

    if (playModeButton)
    {
        removeChildComponent(playModeButton.get());
        playModeButton.reset();
    }
    playModeButton = std::make_unique<juce::TextButton>("mode");
    playModeButton->onClick = [this]() { showPlayModeMenu(); };
    addAndMakeVisible(*playModeButton);

    if (loopModeButton)
    {
        removeChildComponent(loopModeButton.get());
        loopModeButton.reset();
    }
    loopModeButton = std::make_unique<juce::TextButton>("loopmode");
    loopModeButton->onClick = [this]() { showLoopModeMenu(); };
    addAndMakeVisible(*loopModeButton);

    if (loopDirectionButton)
    {
        removeChildComponent(loopDirectionButton.get());
        loopDirectionButton.reset();
    }
    loopDirectionButton = std::make_unique<juce::TextButton>("loopdir");
    loopDirectionButton->onClick = [this]() { showLoopDirectionMenu(); };
    addAndMakeVisible(*loopDirectionButton);

    auto attachSamplePoint = [this](Ctrl c, const std::string &aLabel, auto &v) {
        auto at = std::make_unique<connectors::SamplePointDataAttachment>(
            v, [this](const auto &) { onSamplePointChangedFromGUI(); });
        auto sl = std::make_unique<sst::jucegui::components::DraggableTextEditableValue>();
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

    auto addLabel = [this](Ctrl c, const std::string &label) {
        auto l = std::make_unique<sst::jucegui::components::Label>();
        l->setText(label);
        if (sampleEditors[c])
        {
            removeChildComponent(labels[c].get());
            labels[c].reset();
        }
        addAndMakeVisible(*l);
        labels[c] = std::move(l);
    };

    attachSamplePoint(startP, "StartS", variantView.variants[selectedVariation].startSample);
    addLabel(startP, "Start");
    attachSamplePoint(endP, "EndS", variantView.variants[selectedVariation].endSample);
    addLabel(endP, "End");
    attachSamplePoint(startL, "StartL", variantView.variants[selectedVariation].startLoop);
    addLabel(startL, "Loop Start");
    attachSamplePoint(endL, "EndL", variantView.variants[selectedVariation].endLoop);
    addLabel(endL, "Loop End");
    attachSamplePoint(fadeL, "fadeL", variantView.variants[selectedVariation].loopFade);
    addLabel(fadeL, "Loop Fade");

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
    loopActive = std::make_unique<sst::jucegui::components::ToggleButton>();
    loopActive->setLabel("On");
    loopActive->setSource(loopAttachment.get());
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

    reverseActive = std::make_unique<sst::jucegui::components::ToggleButton>();
    reverseActive->setLabel("<-");
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
    loopCnt = std::make_unique<sst::jucegui::components::DraggableTextEditableValue>();
    loopCnt->setSource(loopCntAttachment.get());
    addAndMakeVisible(*loopCnt);

    if (rebuildTabs)
    {
        auto selectedTab{selectedVariation};
        waveformsTabbedGroup->clearTabs();
        for (auto i = 0; i < maxVariantsPerZone; ++i)
        {
            if (variantView.variants[i].active)
            {
                waveformsTabbedGroup->addTab(std::to_string(i + 1), juce::Colours::darkgrey,
                                             waveforms[i].waveformViewport.get(), false, i);
            }
        }
        if (waveformsTabbedGroup->getNumTabs() < maxVariantsPerZone)
        {
            waveformsTabbedGroup->addTab("+", juce::Colours::darkgrey,
                                         waveforms[maxVariantsPerZone - 1].waveformViewport.get(),
                                         false, maxVariantsPerZone - 1);
        }
        waveformsTabbedGroup->setCurrentTabIndex(selectedTab, true);
    }
    // needed to place all the children
    resized();

    rebuild();
}

void VariantDisplay::resized()
{
    auto viewArea = sampleDisplayRegion();
    waveformsTabbedGroup->setBounds(viewArea);
    auto p = getLocalBounds().withLeft(getLocalBounds().getWidth() - sidePanelWidth).reduced(2);

    p = p.withHeight(18).translated(0, 20);

    playModeLabel->setBounds(p.withWidth(40));

    playModeButton->setBounds(p.withX(playModeLabel->getRight() + 1).withWidth(p.getWidth() - 60));
    reverseActive->setBounds(p.withX(playModeButton->getRight() + 1).withWidth(18));
    p = p.translated(0, 20);

    auto w = p.getWidth() / 2;
    for (const auto m : {startP, endP})
    {
        sampleEditors[m]->setBounds(p.withLeft(p.getX() + w));
        labels[m]->setBounds(p.withWidth(w));
        p = p.translated(0, 20);
    }

    p = p.translated(0, 20);
    p = p.translated(0, 20);
    loopActive->setBounds(p.withWidth(18));
    loopModeButton->setBounds(p.withX(loopActive->getRight() + 1).withWidth(p.getWidth() - 50));
    loopCnt->setBounds(p.withX(loopModeButton->getRight() + 1).withWidth(28));
    p = p.translated(0, 20);

    for (const auto m : {startL, endL, fadeL})
    {
        sampleEditors[m]->setBounds(p.withLeft(p.getX() + w));
        labels[m]->setBounds(p.withWidth(w));
        p = p.translated(0, 20);
    }

    loopDirectionButton->setBounds(p);

    variantPlayModeLabel->setBounds({getWidth() / 2, 0, 40, p.getHeight()});
    variantPlaymodeButton->setBounds({variantPlayModeLabel->getRight(), 0, 60, p.getHeight()});

    fileInfoButton->setBounds(
        {variantPlaymodeButton->getRight() + 5, 0, p.getHeight(), p.getHeight()});
    fileLabel->setBounds({fileInfoButton->getRight(), 0, 30, p.getHeight()});
    fileButton->setBounds({fileLabel->getRight(), 0, 100, p.getHeight()});
    prevFileButton->setBounds({fileButton->getRight(), 0, p.getHeight(), p.getHeight()});
    nextFileButton->setBounds({prevFileButton->getRight(), 0, p.getHeight(), p.getHeight()});

    fileInfos->setBounds({getWidth() / 4, 20, getWidth() / 2, 20});
}

void VariantDisplay::rebuild()

{
    switch (variantView.variants[selectedVariation].playMode)
    {
    case engine::Zone::NORMAL:
        playModeButton->setButtonText("Normal");
        break;
    case engine::Zone::ONE_SHOT:
        playModeButton->setButtonText("Oneshot");
        break;
    case engine::Zone::ON_RELEASE:
        playModeButton->setButtonText("On Release (t/k)");
        break;
    }

    switch (variantView.variants[selectedVariation].loopMode)
    {
    case engine::Zone::LOOP_DURING_VOICE:
        loopModeButton->setButtonText("Loop");
        break;
    case engine::Zone::LOOP_WHILE_GATED:
        loopModeButton->setButtonText("Loop While Gated");
        break;
    case engine::Zone::LOOP_FOR_COUNT:
        loopModeButton->setButtonText("For Count (t/k)");
        break;
    }

    switch (variantView.variants[selectedVariation].loopDirection)
    {
    case engine::Zone::FORWARD_ONLY:
        loopDirectionButton->setButtonText("Loop Forward");
        break;
    case engine::Zone::ALTERNATE_DIRECTIONS:
        loopDirectionButton->setButtonText("Loop Alternate");
        break;
    }

    switch (variantView.variantPlaybackMode)
    {
    case engine::Zone::FORWARD_RR:
        variantPlaymodeButton->setButtonText("Round robin");
        break;
    case engine::Zone::TRUE_RANDOM:
        variantPlaymodeButton->setButtonText("Random");
        break;
    case engine::Zone::RANDOM_CYCLE:
        variantPlaymodeButton->setButtonText("Random per Cycle");
        break;
    case engine::Zone::UNISON:
        variantPlaymodeButton->setButtonText("Unison");
        break;
    }

    auto samp = editor->sampleManager.getSample(variantView.variants[selectedVariation].sampleID);
    if (samp)
    {
        for (auto &[k, a] : sampleAttachments)
        {
            a->sampleCount = samp->sample_length;
        }

        fileButton->setButtonText(samp->displayName);
        fileInfos->srVal->setText(std::to_string(samp->sample_rate));
        fileInfos->bdVal->setText(samp->getBitDepthText());
        auto duration = ((float)samp->sample_length) / samp->sample_rate;
        auto duration_s{fmt::format("{:.3f} s", duration)};
        fileInfos->lengthVal->setText(duration_s);
        fileInfos->sizeVal->setText(std::to_string(samp->getDataSize()));
    }

    loopModeButton->setEnabled(variantView.variants[selectedVariation].loopActive);
    loopDirectionButton->setEnabled(variantView.variants[selectedVariation].loopActive);
    loopCnt->setEnabled(variantView.variants[selectedVariation].loopActive);
    sampleEditors[startL]->setVisible(variantView.variants[selectedVariation].loopActive);
    sampleEditors[endL]->setVisible(variantView.variants[selectedVariation].loopActive);
    sampleEditors[fadeL]->setVisible(variantView.variants[selectedVariation].loopActive);

    loopCnt->setVisible(variantView.variants[selectedVariation].loopMode ==
                        engine::Zone::LoopMode::LOOP_FOR_COUNT);

    waveforms[selectedVariation].waveform->rebuildHotZones();

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
    sst::jucegui::components::TabbedComponent::currentTabChanged(newCurrentTabIndex,
                                                                 newCurrentTabName);
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

    auto add = [&p, this](auto e, auto n) {
        p.addItem(n, true, variantView.variants[selectedVariation].loopMode == e, [this, e]() {
            variantView.variants[selectedVariation].loopMode = e;
            onSamplePointChangedFromGUI();
            rebuild();
        });
    };
    add(engine::Zone::LoopMode::LOOP_DURING_VOICE, "Loop");
    add(engine::Zone::LoopMode::LOOP_WHILE_GATED, "Loop While Gated");
    add(engine::Zone::LoopMode::LOOP_FOR_COUNT, "For Count (t/k)");

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
    add(engine::Zone::VariantPlaybackMode::RANDOM_CYCLE, "Random per Cycle");
    add(engine::Zone::VariantPlaybackMode::UNISON, "Unison");

    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

void VariantDisplay::showLoopDirectionMenu()
{
    juce::PopupMenu p;
    p.addSectionHeader("Loop Direction");
    p.addSeparator();

    auto add = [&p, this](auto e, auto n) {
        p.addItem(n, true, variantView.variants[selectedVariation].loopDirection == e, [this, e]() {
            variantView.variants[selectedVariation].loopDirection = e;
            onSamplePointChangedFromGUI();
            rebuild();
        });
    };
    add(engine::Zone::LoopDirection::FORWARD_ONLY, "Loop Forward");
    add(engine::Zone::LoopDirection::ALTERNATE_DIRECTIONS, "Loop Alternate");

    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

std::optional<std::string> VariantDisplay::MyTabbedComponent::sourceDetailsDragAndDropSample(
    const SourceDetails &dragSourceDetails)
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

bool VariantDisplay::MyTabbedComponent::isInterestedInDragSource(
    const SourceDetails &dragSourceDetails)
{
    auto os = sourceDetailsDragAndDropSample(dragSourceDetails);
    return os.has_value();
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
    auto os = sourceDetailsDragAndDropSample(dragSourceDetails);
    if (os.has_value())
    {
        namespace cmsg = scxt::messaging::client;
        auto za{editor->currentLeadZoneSelection};
        auto sampleID{getTabIndexFromPosition(dragSourceDetails.localPosition.x,
                                              dragSourceDetails.localPosition.y)};
        sendToSerialization(cmsg::AddSampleInZone({*os, za->part, za->group, za->zone, sampleID}));
    }
}

int VariantDisplay::MyTabbedComponent::getTabIndexFromPosition(int x, int y)
{
    auto tabIndex{-1};
    auto isOnWaveform{getCurrentContentComponent()->getBounds().contains(juce::Point<int>{x, y})};
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

} // namespace scxt::ui::app::edit_screen