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

#include "MissingResolutionScreen.h"
#include "app/SCXTEditor.h"

#include <sst/jucegui/components/NamedPanel.h>
#include <sst/jucegui/components/TextPushButton.h>

#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/Viewport.h"
#include "sst/jucegui/component-adapters/DiscreteToReference.h"

#include "app/shared/UIHelpers.h"

namespace scxt::ui::app::missing_resolution
{
namespace jcmp = sst::jucegui::components;
namespace cmsg = scxt::messaging::client;

struct WorkItemLineItem : juce::Component, HasEditor
{
    static constexpr int height{40};
    MissingResolutionScreen *parent{nullptr};
    int index{0};
    std::unique_ptr<sst::jucegui::components::TextPushButton> okButton;

    WorkItemLineItem(MissingResolutionScreen *parent, int index)
        : parent(parent), HasEditor(parent->editor), index(index)
    {
        okButton = std::make_unique<jcmp::TextPushButton>();
        okButton->setLabel("Resolve");
        okButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            w->doCallback();
        });
        addAndMakeVisible(*okButton);
    }
    void doCallback() { parent->resolveItem(index); }
    void resized() override
    {
        auto b = getLocalBounds().reduced(2).withHeight(22).withTrimmedLeft(getWidth() - 100);
        okButton->setBounds(b);
    }
    void paint(juce::Graphics &g) override
    {
        const auto &wi = parent->workItems[index];
        g.setColour(editor->themeColor(theme::ColorMap::grid_primary));
        g.drawHorizontalLine(getHeight() - 1, 5, getWidth() - 5);

        g.setColour(editor->themeColor(theme::ColorMap::generic_content_high));
        g.setFont(editor->themeApplier.interMediumFor(12));

        auto bd = getLocalBounds().reduced(2);
        g.drawText(wi.path.parent_path().u8string(), bd, juce::Justification::topLeft);

        g.setColour(editor->themeColor(theme::ColorMap::generic_content_highest));
        g.setFont(editor->themeApplier.interMediumFor(18));
        g.drawText(wi.path.filename().u8string(), bd, juce::Justification::bottomLeft);
    }
};

struct Contents : juce::Component, HasEditor
{
    MissingResolutionScreen *parent{nullptr};
    std::unique_ptr<sst::jucegui::components::TextPushButton> okButton;

    std::unique_ptr<jcmp::Viewport> viewport;
    std::unique_ptr<juce::Component> viewportContents;

    std::unique_ptr<jcmp::Label> doAllLabel;
    std::unique_ptr<
        sst::jucegui::component_adapters::DiscreteToValueReference<jcmp::ToggleButton, bool>>
        doAllToggle;
    bool resolveSiblings{true};

    std::vector<std::unique_ptr<WorkItemLineItem>> workItemComps;

    Contents(MissingResolutionScreen *p, SCXTEditor *e) : parent(p), HasEditor(e)
    {
        okButton = std::make_unique<sst::jucegui::components::TextPushButton>();
        okButton->setLabel("OK");
        okButton->setOnCallback([w = juce::Component::SafePointer(parent)]() {
            if (!w)
                return;
            w->setVisible(false);
        });
        addAndMakeVisible(*okButton);

        doAllLabel = std::make_unique<jcmp::Label>();
        doAllLabel->setText("Auto-Resolve Siblings");
        addAndMakeVisible(*doAllLabel);

        doAllToggle = std::make_unique<
            sst::jucegui::component_adapters::DiscreteToValueReference<jcmp::ToggleButton, bool>>(
            resolveSiblings);
        doAllToggle->widget->setDrawMode(sst::jucegui::components::ToggleButton::DrawMode::FILLED);
        addAndMakeVisible(*(doAllToggle->widget));

        viewport = std::make_unique<jcmp::Viewport>("Parts");
        viewportContents = std::make_unique<juce::Component>();

        viewport->setViewedComponent(viewportContents.get(), false);
        addAndMakeVisible(*viewport);

        rebuildContents();
    }

    void rebuildContents()
    {
        viewportContents->removeAllChildren();
        workItemComps.clear();
        for (int i = 0; i < parent->workItems.size(); ++i)
        {
            auto w = std::make_unique<WorkItemLineItem>(parent, i);
            viewportContents->addAndMakeVisible(*w);
            workItemComps.push_back(std::move(w));
        }
        resized();
    }

    void resized() override
    {
        auto b = getLocalBounds();
        b = b.withTrimmedTop(b.getHeight() - 22);
        auto okb = b.withTrimmedLeft(b.getWidth() - 100);
        okButton->setBounds(okb);

        auto dal = b.withWidth(125);
        doAllLabel->setBounds(dal);
        dal = dal.translated(dal.getWidth() + 2, 0).withWidth(dal.getHeight()).reduced(2);
        doAllToggle->widget->setBounds(dal);

        viewport->setBounds(getLocalBounds().withTrimmedBottom(25));
        int vmargin{2};
        viewportContents->setBounds(0, 0, viewport->getWidth() - 5,
                                    workItemComps.size() * (WorkItemLineItem::height + vmargin));
        auto wib = viewportContents->getBounds().withHeight(WorkItemLineItem::height);
        for (auto &w : workItemComps)
        {
            w->setBounds(wib);
            wib = wib.translated(0, WorkItemLineItem::height + vmargin);
        }
    }
};

MissingResolutionScreen::MissingResolutionScreen(SCXTEditor *e) : HasEditor(e)
{
    auto ct = std::make_unique<sst::jucegui::components::NamedPanel>("Missing Sample Resolution");
    addAndMakeVisible(*ct);

    auto contents = std::make_unique<Contents>(this, e);
    ct->setContentAreaComponent(std::move(contents));
    contentsArea = std::move(ct);
}

MissingResolutionScreen::~MissingResolutionScreen() {}

void MissingResolutionScreen::setWorkItemList(
    const std::vector<engine::MissingResolutionWorkItem> &l)
{
    workItems = l;
    auto cp = dynamic_cast<Contents *>(contentsArea->getContentAreaComponent().get());
    if (cp)
        cp->rebuildContents();
    repaint();
}

void MissingResolutionScreen::resized()
{
    contentsArea->setBounds(getLocalBounds().reduced(100, 80));
}

void MissingResolutionScreen::resolveItem(int idx)
{
    const auto &wi = workItems[idx];

    fileChooser =
        std::make_unique<juce::FileChooser>("Resolve sample " + wi.path.filename().u8string());
    fileChooser->launchAsync(
        juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::openMode,
        [idx, w = juce::Component::SafePointer(this)](const juce::FileChooser &c) {
            auto result = c.getResults();
            if (result.isEmpty() || result.size() > 1)
            {
                return;
            }
            if (!w)
                return;
            auto p = shared::juceFileToFsPath(result[0]);
            w->applyResolution(idx, p);
        });
}

void MissingResolutionScreen::applyResolution(int idx, const fs::path &toThis)
{
    auto cp = dynamic_cast<Contents *>(contentsArea->getContentAreaComponent().get());

    auto pp = toThis.parent_path();
    auto op = workItems[idx].path.parent_path();

    std::vector<engine::MissingResolutionWorkItem> othersToMove{};
    for (auto wi : workItems)
    {
        if (wi.path != workItems[idx].path && wi.path.parent_path() == op)
        {
            auto np = pp / wi.path.filename();
            if (fs::exists(np))
            {
                othersToMove.push_back(wi);
            }
        }
    }

    sendToSerialization(
        scxt::messaging::client::ResolveSample({workItems[idx], toThis.u8string()}));
    if (!othersToMove.empty() && cp->resolveSiblings)
    {
        applyDirectoryResolution(othersToMove, pp);
    }
}

void MissingResolutionScreen::applyDirectoryResolution(
    std::vector<engine::MissingResolutionWorkItem> indexes, const fs::path &newParent)
{
    SCLOG("Resolving into " << newParent.u8string() << " with " << indexes.size() << " items");

    for (auto &wi : indexes)
    {
        auto toThis = newParent / wi.path.filename();

        SCLOG("Resolving " << wi.path << " to " << toThis.u8string());
        sendToSerialization(scxt::messaging::client::ResolveSample({wi, toThis.u8string()}));
    }
}

} // namespace scxt::ui::app::missing_resolution