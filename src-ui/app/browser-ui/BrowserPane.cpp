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

#include "BrowserPane.h"
#include "app/SCXTEditor.h"
#include "browser/browser.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/NamedPanelDivider.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/plugininfra/strnatcmp.h"

namespace scxt::ui::app::browser_ui
{
namespace jcmp = sst::jucegui::components;

struct TempPane : HasEditor, juce::Component
{
    std::string label;
    TempPane(SCXTEditor *e, const std::string &s) : HasEditor(e), label(s) {}
    void paint(juce::Graphics &g) override
    {
        auto ft = editor->style()->getFont(jcmp::Label::Styles::styleClass,
                                           jcmp::Label::Styles::labelfont);
        g.setFont(ft.withHeight(20));
        g.setColour(juce::Colours::white);
        g.drawText(label, getLocalBounds().withTrimmedBottom(20), juce::Justification::centred);

        g.setFont(ft.withHeight(14));
        g.setColour(juce::Colours::white);
        g.drawText("Coming Soon", getLocalBounds().withTrimmedTop(20),
                   juce::Justification::centred);
    }
};

struct DriveListBoxModel : juce::ListBoxModel
{
    BrowserPane *browserPane{nullptr};
    DriveListBoxModel(BrowserPane *b) : browserPane(b) {}
    int getNumRows() override { return browserPane->roots.size(); }
    void paintListBoxItem(int rowNumber, juce::Graphics &g, int width, int height,
                          bool rowIsSelected) override
    {
        if (rowNumber >= 0 && rowNumber < browserPane->roots.size())
        {
            g.setFont(browserPane->editor->themeApplier.interMediumFor(12));

            // TODO: Style all of these
            auto textColor =
                browserPane->editor->themeColor(theme::ColorMap::generic_content_medium);
            auto fillColor = juce::Colour(0, 0, 0).withAlpha(0.f);

            if (rowIsSelected)
            {
                fillColor = browserPane->editor->themeColor(theme::ColorMap::bg_3);
                textColor = browserPane->editor->themeColor(theme::ColorMap::generic_content_high);
            }
            g.setColour(fillColor);
            g.fillRect(0, 0, width, height);
            g.setColour(textColor);
            g.drawText(browserPane->roots[rowNumber].second, 1, 1, width - 2, height - 2,
                       juce::Justification::centredLeft);
        }
    }
    void selectedRowsChanged(int lastRowSelected) override;
};

struct DriveArea : juce::Component, HasEditor
{
    BrowserPane *browserPane{nullptr};
    std::unique_ptr<DriveListBoxModel> model;
    std::unique_ptr<juce::ListBox> lbox;
    DriveArea(BrowserPane *b, SCXTEditor *e) : browserPane(b), HasEditor(e)
    {
        model = std::make_unique<DriveListBoxModel>(browserPane);
        lbox = std::make_unique<juce::ListBox>("Root Devices", model.get());
        lbox->setClickingTogglesRowSelection(true);
        lbox->setMultipleSelectionEnabled(false);
        lbox->setRowHeight(18);
        lbox->setColour(juce::ListBox::ColourIds::backgroundColourId,
                        juce::Colour(0.f, 0.f, 0.f, 0.f));

        addAndMakeVisible(*lbox);
    };

    ~DriveArea() { lbox->setModel(nullptr); }

    void paint(juce::Graphics &g) override
    {
        auto c = browserPane->style()->getColour(jcmp::NamedPanel::Styles::styleClass,
                                                 jcmp::NamedPanel::Styles::outline);
        g.setColour(c);
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 2, 1);
    }

    void resized() override { lbox->setBounds(getLocalBounds().reduced(1)); }
};

struct DriveFSListBoxModel : juce::ListBoxModel
{
    BrowserPane *browserPane{nullptr};
    DriveFSListBoxModel(BrowserPane *b) : browserPane(b) {}
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics &g, int width, int height,
                          bool rowIsSelected) override
    {
    }
    juce::Component *refreshComponentForRow(int rowNumber, bool isRowSelected,
                                            juce::Component *existingComponentToUpdate) override;

    void selectedRowsChanged(int lastRowSelected) override {}
};

struct DriveFSArea : juce::Component, HasEditor
{
    BrowserPane *browserPane{nullptr};
    std::unique_ptr<DriveFSListBoxModel> model;
    std::unique_ptr<juce::ListBox> lbox;
    std::unique_ptr<jcmp::TextPushButton> dotDot;

    DriveFSArea(BrowserPane *b, SCXTEditor *e) : browserPane(b), HasEditor(e)
    {
        model = std::make_unique<DriveFSListBoxModel>(browserPane);
        lbox = std::make_unique<juce::ListBox>("Current Data", model.get());
        lbox->setClickingTogglesRowSelection(true);
        lbox->setMultipleSelectionEnabled(false);
        lbox->setRowHeight(18);
        lbox->setColour(juce::ListBox::ColourIds::backgroundColourId,
                        juce::Colour(0.f, 0.f, 0.f, 0.f));

        addAndMakeVisible(*lbox);

        dotDot = std::make_unique<jcmp::TextPushButton>();
        dotDot->setLabel("up one dir");
        dotDot->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->upOneLevel();
        });
        dotDot->setEnabled(false);
        addAndMakeVisible(*dotDot);
    }

    void paint(juce::Graphics &g) override
    {
        auto c = browserPane->style()->getColour(jcmp::NamedPanel::Styles::styleClass,
                                                 jcmp::NamedPanel::Styles::brightoutline);
        g.setColour(c);
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 2, 1);
    }

    fs::path rootPath, currentPath;
    void setRootRow(const fs::path &p)
    {
        rootPath = p;
        currentPath = p;
        dotDot->setEnabled(false);
        recalcContents();
    }

    void setCurrentPath(const fs::path &p)
    {
        currentPath = p;
        dotDot->setEnabled(currentPath > rootPath);
        recalcContents();
    }

    void upOneLevel()
    {
        if (currentPath > rootPath)
        {
            setCurrentPath(currentPath.parent_path());
        }
    }

    std::vector<fs::directory_entry> contents;
    void recalcContents()
    {
        // Make a copy for now. this sucks and we should be smarter
        contents.clear();
        try
        {
            for (auto const &dir_entry : fs::directory_iterator{currentPath})
            {
                // Who to skip? Well
                bool include = false;
                // Include directories
                include = include || dir_entry.is_directory();
                // and loadable files
                include = include || editor->browser.isLoadableFile(dir_entry.path());
                // but skip files starting with a '.'
                auto fn = dir_entry.path().filename().u8string();
                include = include && (fn.empty() || fn[0] != '.');
                // TODO: And consider skipping hidden directories (but that's OS dependent)

                if (include)
                {
                    contents.push_back(dir_entry);
                }
            }
        }
        catch (const fs::filesystem_error &e)
        {
            SCLOG(e.what());
        }
        std::sort(contents.begin(), contents.end(), [](auto &a, auto &b) {
            return strnatcasecmp(a.path().u8string().c_str(), b.path().u8string().c_str()) < 0;
        });
        lbox->updateContent();
    }

    void resized() override
    {
        auto bd = getLocalBounds().reduced(1);
        dotDot->setBounds(bd.withHeight(20));
        lbox->setBounds(bd.withTrimmedTop(22));
    }
};

struct DevicesPane : HasEditor, juce::Component
{
    BrowserPane *browserPane{nullptr};
    std::unique_ptr<DriveArea> driveArea;
    std::unique_ptr<DriveFSArea> driveFSArea;
    std::unique_ptr<jcmp::NamedPanelDivider> divider;
    DevicesPane(BrowserPane *p) : browserPane(p), HasEditor(p->editor)
    {
        driveArea = std::make_unique<DriveArea>(p, editor);
        addAndMakeVisible(*driveArea);
        driveFSArea = std::make_unique<DriveFSArea>(p, editor);
        addAndMakeVisible(*driveFSArea);
        divider = std::make_unique<jcmp::NamedPanelDivider>();
        addAndMakeVisible(*divider);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        auto dh = 170;
        auto drives = b.withHeight(dh);
        auto div = b.withTrimmedTop(dh).withHeight(8);
        auto rest = b.withTrimmedTop(dh + 8);

        driveArea->setBounds(drives);
        divider->setBounds(div);
        driveFSArea->setBounds(rest);
    }
};

void DriveListBoxModel::selectedRowsChanged(int lastRowSelected)
{
    browserPane->devicesPane->driveFSArea->setRootRow(browserPane->roots[lastRowSelected].first);
}

int DriveFSListBoxModel::getNumRows()
{
    if (!browserPane || !browserPane->devicesPane || !browserPane->devicesPane->driveFSArea)
        return 0;

    return browserPane->devicesPane->driveFSArea->contents.size();
}

struct DriveFSListBoxRow : public juce::Component
{
    bool isSelected;
    int rowNumber;
    BrowserPane *browserPane{nullptr};
    DriveFSListBoxModel *model{nullptr};

    juce::ListBox *enclosingBox() { return browserPane->devicesPane->driveFSArea->lbox.get(); }
    bool isDragging;
    void mouseDown(const juce::MouseEvent &event) override
    {
        enclosingBox()->selectRowsBasedOnModifierKeys(rowNumber, event.mods, false);

        if (event.mods.isPopupMenu())
        {
            if (!isFile())
            {
                showDirectoryPopup();
            }
        }
    }

    void mouseDrag(const juce::MouseEvent &e) override
    {
        if (!isFile())
            return;

        if (!isDragging && e.getDistanceFromDragStart() > 1.5f)
        {
            if (auto *container = juce::DragAndDropContainer::findParentDragContainerFor(this))
            {
                const auto &data = browserPane->devicesPane->driveFSArea->contents;
                getProperties().set(
                    "DragAndDropSample",
                    juce::String::fromUTF8(data[rowNumber].path().u8string().c_str()));
                container->startDragging("FileSystem Row", this);
                isDragging = true;
            }
        }
    }

    void mouseUp(const juce::MouseEvent &event) override
    {
        if (isDragging)
        {
            isDragging = false;
        }
        else
        {
            enclosingBox()->selectRowsBasedOnModifierKeys(rowNumber, event.mods, true);
        }
    }
    void mouseDoubleClick(const juce::MouseEvent &event) override
    {
        const auto &data = browserPane->devicesPane->driveFSArea->contents;
        if (rowNumber >= 0 && rowNumber < data.size())
        {
            if (data[rowNumber].is_directory())
            {
                browserPane->devicesPane->driveFSArea->setCurrentPath(data[rowNumber].path());
            }
            else
            {
                // This is a hack and should be a drag and drop gesture really I guess
                namespace cmsg = scxt::messaging::client;
                scxt::messaging::client::clientSendToSerialization(
                    cmsg::AddSample(data[rowNumber].path().u8string()),
                    browserPane->editor->msgCont);
            }
        }
    }

    void showDirectoryPopup()
    {
        const auto &data = browserPane->devicesPane->driveFSArea->contents;
        if (rowNumber >= 0 && rowNumber < data.size())
        {
            auto p = juce::PopupMenu();

            p.addSectionHeader("Directory");
            p.addSeparator();
            p.addItem("Add to Locations", [w = juce::Component::SafePointer(browserPane),
                                           d = data[rowNumber]]() {
                if (!w)
                    return;
                namespace cmsg = scxt::messaging::client;
                w->sendToSerialization(cmsg::AddBrowserDeviceLocation(d.path().u8string()));
            });
            p.showMenuAsync(browserPane->editor->defaultPopupMenuOptions());
        }
    }

    bool isFile()
    {
        const auto &data = browserPane->devicesPane->driveFSArea->contents;

        if (rowNumber >= 0 && rowNumber < data.size())
        {
            const auto &entry = data[rowNumber];
            return !entry.is_directory();
        }
        return false;
    }
    void paint(juce::Graphics &g) override
    {
        const auto &data = browserPane->devicesPane->driveFSArea->contents;
        if (rowNumber >= 0 && rowNumber < data.size())
        {
            const auto &entry = data[rowNumber];
            auto width = getWidth();
            auto height = getHeight();

            g.setFont(browserPane->editor->themeApplier.interMediumFor(12));

            // TODO: Style all of these
            auto textColor =
                browserPane->editor->themeColor(theme::ColorMap::generic_content_medium);
            auto fillColor = juce::Colour(0, 0, 0).withAlpha(0.f);

            if (isSelected)
            {
                fillColor = browserPane->editor->themeColor(theme::ColorMap::bg_3);
                textColor = browserPane->editor->themeColor(theme::ColorMap::generic_content_high);
            }
            g.setColour(fillColor);
            g.fillRect(0, 0, width, height);
            // TODO: Replace with glyph painter fo course
            auto r = juce::Rectangle<int>(1, (height - 10) / 2, 10, 10);
            if (entry.is_directory())
            {
                g.setColour(textColor.withAlpha(0.5f));
                g.fillRect(r);
                g.setColour(textColor.brighter(0.4f));
                g.drawRect(r);
            }
            else
            {
                jcmp::GlyphPainter::paintGlyph(g, r, jcmp::GlyphPainter::TUNING,
                                               textColor.withAlpha(0.5f));
            }
            g.setColour(textColor);
            g.drawText(entry.path().filename().u8string(), 14, 1, width - 16, height - 2,
                       juce::Justification::centredLeft);
        }
    }
};

juce::Component *
DriveFSListBoxModel::refreshComponentForRow(int rowNumber, bool isRowSelected,
                                            juce::Component *existingComponentToUpdate)
{
    if (!browserPane || !browserPane->devicesPane || !browserPane->devicesPane->driveFSArea)
        return nullptr;

    const auto &data = browserPane->devicesPane->driveFSArea->contents;
    if (rowNumber >= 0 && rowNumber < data.size())
    {
        auto rc = dynamic_cast<DriveFSListBoxRow *>(existingComponentToUpdate);

        if (!rc)
        {
            // Should never happen but
            if (existingComponentToUpdate)
            {
                delete existingComponentToUpdate;
                existingComponentToUpdate = nullptr;
            }

            rc = new DriveFSListBoxRow();
        }

        rc->isSelected = isRowSelected;
        rc->rowNumber = rowNumber;
        rc->browserPane = browserPane;
        rc->model = this;
        rc->repaint();
        return rc;
    }

    if (existingComponentToUpdate)
        delete existingComponentToUpdate;
    return nullptr;
}

struct FavoritesPane : TempPane
{
    FavoritesPane(SCXTEditor *e) : TempPane(e, "Favorites") {}
};

struct SearchPane : TempPane
{
    SearchPane(SCXTEditor *e) : TempPane(e, "Search") {}
};

struct sfData : sst::jucegui::data::Discrete
{
    BrowserPane *browserPane{nullptr};
    sfData(BrowserPane *hr) : browserPane(hr) {}
    std::string getLabel() const override { return "Function"; }
    int getValue() const override { return browserPane->selectedPane; }

    void setValueFromGUI(const int &f) override
    {
        if (browserPane)
        {
            browserPane->selectPane(f);
        }
    }

    void setValueFromModel(const int &f) override {}

    std::string getValueAsStringFor(int i) const override
    {
        if (browserPane)
        {
            switch (i)
            {
            case 0:
                return "DEVICES";
            case 1:
                return "FAVORITES";
            case 2:
                return "SEARCH";
            }
        }
        SCLOG("getValueAsStringFor with invalid value " << i);
        assert(false);
        return "-error-";
    }

    int getMax() const override { return 2; }
};

BrowserPane::BrowserPane(SCXTEditor *e)
    : HasEditor(e), sst::jucegui::components::NamedPanel("Browser")
{
    hasHamburger = true;

    selectedFunction = std::make_unique<sst::jucegui::components::ToggleButtonRadioGroup>();
    selectedFunctionData = std::make_unique<sfData>(this);
    selectedFunction->setSource(selectedFunctionData.get());

    addAndMakeVisible(*selectedFunction);

    devicesPane = std::make_unique<DevicesPane>(this);
    addChildComponent(*devicesPane);
    favoritesPane = std::make_unique<FavoritesPane>(e);
    addChildComponent(*favoritesPane);
    searchPane = std::make_unique<SearchPane>(e);
    addChildComponent(*searchPane);

    selectPane(selectedPane);

    resetRoots();
}

BrowserPane::~BrowserPane() { selectedFunction->setSource(nullptr); }

void BrowserPane::resized()
{
    auto r = getContentArea();
    auto sel = r.withHeight(22);
    auto ct = r.withTrimmedTop(25);
    selectedFunction->setBounds(sel);

    devicesPane->setBounds(ct);
    favoritesPane->setBounds(ct);
    searchPane->setBounds(ct);
}

void BrowserPane::resetRoots()
{
    roots = editor->browser.getRootPathsForDeviceView();
    if (devicesPane && devicesPane->driveArea && devicesPane->driveArea->lbox)
        devicesPane->driveArea->lbox->updateContent();
    repaint();
}

void BrowserPane::selectPane(int i)
{
    selectedPane = i;

    devicesPane->setVisible(i == 0);
    favoritesPane->setVisible(i == 1);
    searchPane->setVisible(i == 2);
    repaint();
}
} // namespace scxt::ui::app::browser_ui
