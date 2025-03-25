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
#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/ListView.h"
#include "sst/plugininfra/strnatcmp.h"

#include <infrastructure/user_defaults.h>
#include "BrowserPaneInterfaces.h"

#define SHOW_INDEX_MENUS 0

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

struct DriveArea : juce::Component, HasEditor
{
    struct DriveAreaRow : juce::Component
    {
        DriveArea *driveArea{nullptr};
        uint32_t rowNumber{0};
        bool selected{false};
        bool hovered{false};

        DriveAreaRow(DriveArea *p) : driveArea(p) {}
        void paint(juce::Graphics &g) override
        {
            auto *ed = driveArea->browserPane->editor;
            auto *bp = driveArea->browserPane;

            if (rowNumber >= 0 && rowNumber < bp->roots.size())
            {
                g.setFont(ed->themeApplier.interMediumFor(12));

                // TODO: Style all of these
                auto textColor = ed->themeColor(theme::ColorMap::generic_content_medium);
                auto fillColor = juce::Colour(0, 0, 0).withAlpha(0.f);

                if (selected)
                {
                    fillColor = ed->themeColor(theme::ColorMap::bg_3);
                    textColor = ed->themeColor(theme::ColorMap::generic_content_high);
                }
                else if (hovered)
                {
                    fillColor = ed->themeColor(theme::ColorMap::bg_3);
                }
                g.setColour(fillColor);
                g.fillRect(0, 0, getWidth(), getHeight());
                g.setColour(textColor);
                g.drawText(std::get<1>(bp->roots[rowNumber]), 1, 1, getWidth() - 2, getHeight() - 2,
                           juce::Justification::centredLeft);
            }
        }
        void mouseEnter(const juce::MouseEvent &event) override
        {
            hovered = true;
            repaint();
        }
        void mouseExit(const juce::MouseEvent &event) override
        {
            hovered = false;
            repaint();
        }

        void mouseDown(const juce::MouseEvent &e) override;

        void showPopup()
        {
            auto *bp = driveArea->browserPane;
            auto pth = std::get<0>(bp->roots[rowNumber]);
            auto nm = std::get<1>(bp->roots[rowNumber]);

            auto p = juce::PopupMenu();
            p.addSectionHeader(nm);
            p.addSeparator();

            p.addItem("Remove '" + nm + "' from Locations", [w = juce::Component::SafePointer(
                                                                 driveArea->browserPane),
                                                             removeThis = pth]() {
                if (!w)
                    return;
                namespace cmsg = scxt::messaging::client;
                w->sendToSerialization(cmsg::RemoveBrowserDeviceLocation(removeThis.u8string()));
            });
#if SHOW_INDEX_MENUS
            // TODO: If Indexed
            p.addItem("Add '" + nm + "' to Search Index", false, false, [](){});

            p.addItem("Reindex '" + nm + "'", true, false,
                      [pth, w = juce::Component::SafePointer(this)]() {
                          namespace cmsg = scxt::messaging::client;
                          w->driveArea->browserPane->sendToSerialization(
                              cmsg::ReindexBrowserLocation{pth.u8string()});
                      });
#endif
            if (!fs::is_directory(pth))
            {
                pth = pth.parent_path();
            }
            p.addItem("Reveal '" + nm + "'...",
                      [pth]() { juce::File(pth.u8string()).revealToUser(); });
            p.addSeparator();
            for (auto &idx :
#if SHOW_INDEX_MENUS
                 {false, true}
#else
                 {false}
#endif
            )
            {
                std::string lab = "Add Location...";
                if (idx)
                    lab = "Add and Index Location...";
                p.addItem(lab, [idxx = idx, w = juce::Component::SafePointer(this)]() {
                    if (!w)
                        return;
                    auto ed = w->driveArea->browserPane->editor;
                    ed->fileChooser = std::make_unique<juce::FileChooser>("Add Location");
                    ed->fileChooser->launchAsync(
                        juce::FileBrowserComponent::canSelectDirectories,
                        [idxxx = idxx, w, ed](const auto &c) {
                            auto result = c.getResults();
                            namespace cmsg = scxt::messaging::client;
                            w->driveArea->browserPane->sendToSerialization(
                                cmsg::AddBrowserDeviceLocation(
                                    {result[0].getFullPathName().toStdString(), idxxx}));
                        });
                });
            }
            p.showMenuAsync(driveArea->browserPane->editor->defaultPopupMenuOptions());
        }
    };

    BrowserPane *browserPane{nullptr};
    std::unique_ptr<sst::jucegui::components::ListView> listView;
    DriveArea(BrowserPane *b, SCXTEditor *e) : browserPane(b), HasEditor(e)
    {
        listView = std::make_unique<sst::jucegui::components::ListView>();
        listView->getRowCount = [this]() { return browserPane->roots.size(); };
        listView->getRowHeight = [this]() { return 18; };
        listView->makeRowComponent = [this]() { return makeComponent(); };
        listView->assignComponentToRow = [this](const auto &c, auto r) {
            return assignComponentToRow(c, r);
        };
        listView->setRowSelection = [this](const auto &c, auto r) { return setRowSelection(c, r); };
        listView->refresh();
        addAndMakeVisible(*listView);
    };

    ~DriveArea() {}

    void paint(juce::Graphics &g) override
    {
        auto c = browserPane->style()->getColour(jcmp::NamedPanel::Styles::styleClass,
                                                 jcmp::NamedPanel::Styles::outline);
        g.setColour(c);
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 2, 1);
    }

    void resized() override { listView->setBounds(getLocalBounds().reduced(1)); }
    std::unique_ptr<juce::Component> makeComponent()
    {
        return std::make_unique<DriveAreaRow>(this);
    }
    void assignComponentToRow(const std::unique_ptr<juce::Component> &c, int r)
    {
        auto *dar = dynamic_cast<DriveAreaRow *>(c.get());
        if (dar)
        {
            dar->rowNumber = r;
            dar->repaint();
        }
    }
    void setRowSelection(const std::unique_ptr<juce::Component> &c, bool r)
    {
        auto *dar = dynamic_cast<DriveAreaRow *>(c.get());
        if (dar)
        {
            dar->selected = r;
            dar->repaint();
        }
    }
};

struct DriveFSArea : juce::Component, HasEditor
{
    BrowserPane *browserPane{nullptr};
    std::unique_ptr<jcmp::ListView> listView;

    DriveFSArea(BrowserPane *b, SCXTEditor *e) : browserPane(b), HasEditor(e)
    {
        listView = std::make_unique<jcmp::ListView>();
        setupListView();
        addAndMakeVisible(*listView);
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
        recalcContents();
    }

    void setCurrentPath(const fs::path &p)
    {
        currentPath = p;
        recalcContents();
    }

    void upOneLevel()
    {
        if (currentPath > rootPath)
        {
            setCurrentPath(currentPath.parent_path());
        }
    }

    struct RowContents
    {
        fs::directory_entry dirent;
        bool isExpanded{false};
        std::optional<sample::compound::CompoundElement> expandableAddress{std::nullopt};
    };
    std::vector<RowContents> contents;
    void recalcContents()
    {
        // Make a copy for now. this sucks and we should be smarter
        contents.clear();
        auto lcontents = contents;
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
                    lcontents.push_back({dir_entry});
                }
            }
        }
        catch (const fs::filesystem_error &e)
        {
            SCLOG(e.what());
        }
        std::sort(lcontents.begin(), lcontents.end(), [](auto &a, auto &b) {
            return strnatcasecmp(a.dirent.path().u8string().c_str(),
                                 b.dirent.path().u8string().c_str()) < 0;
        });
        if (currentPath > rootPath)
            contents.push_back({fs::directory_entry("..")});
        for (auto &c : lcontents)
            contents.emplace_back(c);
        listView->refresh();
    }

    void resized() override
    {
        auto bd = getLocalBounds().reduced(1);
        listView->setBounds(bd);
    }

    void setupListView();

    void expandMultifile(int row)
    {
        auto &ent = contents[row];
        assert(!ent.isExpanded);
        auto ct = browser::Browser::expandForBrowser(ent.dirent.path());
        if (!ct.empty())
        {
            auto after = contents.begin() + row + 1;
            for (auto c : ct)
            {
                after = contents.insert(after, {contents[row].dirent, false, c});
                after++;
            }

            listView->rowSelected(row + 1, true);
        }
        ent.isExpanded = true;
        listView->refresh();
    }
    void collapsMultifile(int row)
    {
        auto &ent = contents[row];
        assert(ent.isExpanded);
        auto after = contents.begin() + row + 1;
        while (after != contents.end() && after->expandableAddress.has_value() &&
               after->dirent.path() == ent.dirent.path())
        {
            after = contents.erase(after);
        }
        ent.isExpanded = false;
        listView->rowSelected(row, true);
        listView->refresh();
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

void DriveArea::DriveAreaRow::mouseDown(const juce::MouseEvent &e)
{
    if (rowNumber >= 0 && rowNumber < driveArea->browserPane->roots.size())
    {
        if (e.mods.isPopupMenu())
        {
            showPopup();
        }
        else
        {
            driveArea->browserPane->devicesPane->driveFSArea->setRootRow(
                std::get<0>(driveArea->browserPane->roots[rowNumber]));
            driveArea->listView->rowSelected(rowNumber, true);
        }
    }
}

struct DriveFSRowComponent : public juce::Component, WithSampleInfo
{
    bool isSelected;
    int rowNumber;
    BrowserPane *browserPane{nullptr};
    jcmp::ListView *listView{nullptr};
    static constexpr int32_t glyphSize{14};

    DriveFSRowComponent(BrowserPane *p, jcmp::ListView *v) : browserPane(p), listView(v) {}

    bool isDragging{false};
    bool isMouseDownWithoutDrag{false};
    bool hasStartedPreview{false};

    std::optional<fs::directory_entry> getDirEnt() const override
    {
        auto &data = browserPane->devicesPane->driveFSArea->contents;
        auto &entry = data[rowNumber];
        return entry.dirent;
        ;
    }
    std::optional<sample::compound::CompoundElement> getCompoundElement() const override
    {
        auto &data = browserPane->devicesPane->driveFSArea->contents;
        auto &entry = data[rowNumber];
        return entry.expandableAddress;
        ;
    }

    void mouseDown(const juce::MouseEvent &event) override
    {
        auto &data = browserPane->devicesPane->driveFSArea->contents;
        auto &entry = data[rowNumber];

        isMouseDownWithoutDrag = true;
        if (browser::Browser::isExpandableInBrowser(entry.dirent.path()))
        {
            if (event.x < glyphSize + 2)
            {
                if (entry.isExpanded)
                {
                    browserPane->devicesPane->driveFSArea->collapsMultifile(rowNumber);
                }
                else
                {
                    browserPane->devicesPane->driveFSArea->expandMultifile(rowNumber);
                }
                repaint();
                return;
            }
        }
        if (browserPane->autoPreviewEnabled && isFile())
        {
            if (browser::Browser::isLoadableSingleSample(entry.dirent.path()))
            {
                hasStartedPreview = true;
                namespace cmsg = scxt::messaging::client;
                scxt::messaging::client::clientSendToSerialization(
                    cmsg::PreviewBrowserSample({true, data[rowNumber].dirent.path().u8string()}),
                    browserPane->editor->msgCont);
                repaint();
            }
        }

        listView->rowSelected(rowNumber, true);

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

        isMouseDownWithoutDrag = false;

        if (!isDragging && e.getDistanceFromDragStart() > 2)
        {
            stopPreview();
            if (auto *container = juce::DragAndDropContainer::findParentDragContainerFor(this))
            {
                const auto &data = browserPane->devicesPane->driveFSArea->contents;
                getProperties().set(
                    "DragAndDropSample",
                    juce::String::fromUTF8(data[rowNumber].dirent.path().u8string().c_str()));
                container->startDragging("FileSystem Row", this);
                isDragging = true;
            }
        }
    }

    void mouseUp(const juce::MouseEvent &event) override
    {
        isMouseDownWithoutDrag = false;
        if (isDragging)
        {
            isDragging = false;
        }
    }

    bool isHovered;
    void mouseEnter(const juce::MouseEvent &) override
    {
        isHovered = true;
        repaint();
    }
    void mouseExit(const juce::MouseEvent &) override
    {
        isHovered = false;
        repaint();
    }

    void stopPreview()
    {
        if (hasStartedPreview)
        {
            hasStartedPreview = false;
            repaint();
            namespace cmsg = scxt::messaging::client;

            scxt::messaging::client::clientSendToSerialization(
                cmsg::PreviewBrowserSample({false, ""}), browserPane->editor->msgCont);
        }
    }

    void mouseDoubleClick(const juce::MouseEvent &event) override
    {
        isMouseDownWithoutDrag = false;
        stopPreview();
        const auto &data = browserPane->devicesPane->driveFSArea->contents;
        if (rowNumber >= 0 && rowNumber < data.size())
        {
            if (data[rowNumber].dirent == fs::path(".."))
            {
                browserPane->devicesPane->driveFSArea->upOneLevel();
            }
            else if (data[rowNumber].dirent.is_directory())
            {
                browserPane->devicesPane->driveFSArea->setCurrentPath(
                    data[rowNumber].dirent.path());
            }
            else
            {
                // This is a hack and should be a drag and drop gesture really I guess
                namespace cmsg = scxt::messaging::client;
                scxt::messaging::client::clientSendToSerialization(
                    cmsg::AddSample(data[rowNumber].dirent.path().u8string()),
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
            p.addItem("Add to Locations",
                      [w = juce::Component::SafePointer(browserPane), d = data[rowNumber]]() {
                          if (!w)
                              return;
                          namespace cmsg = scxt::messaging::client;
                          w->sendToSerialization(
                              cmsg::AddBrowserDeviceLocation({d.dirent.path().u8string(), false}));
                      });
#if SHOW_INDEX_MENUS
            p.addItem("Add to Index and Locations",
                      [w = juce::Component::SafePointer(browserPane), d = data[rowNumber]]() {
                          if (!w)
                              return;
                          namespace cmsg = scxt::messaging::client;
                          w->sendToSerialization(
                              cmsg::AddBrowserDeviceLocation({d.dirent.path().u8string(), true}));
                      });
#endif
            p.showMenuAsync(browserPane->editor->defaultPopupMenuOptions());
        }
    }

    bool isFile()
    {
        const auto &data = browserPane->devicesPane->driveFSArea->contents;

        if (rowNumber >= 0 && rowNumber < data.size())
        {
            const auto &entry = data[rowNumber];
            return !entry.dirent.is_directory();
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
            else if (isHovered)
            {
                fillColor = browserPane->editor->themeColor(theme::ColorMap::bg_3);
            }
            g.setColour(fillColor);
            g.fillRect(0, 0, width, height);
            auto r = juce::Rectangle<int>(1, (height - glyphSize) / 2, glyphSize, glyphSize);
            if (entry.expandableAddress.has_value())
            {
                auto b = getLocalBounds().withTrimmedLeft(2 * glyphSize + 2);
                auto gr = r.translated(glyphSize + 2, 0);
                if (entry.expandableAddress->type == sample::compound::CompoundElement::SAMPLE)
                {
                    jcmp::GlyphPainter::paintGlyph(g, gr, jcmp::GlyphPainter::SPEAKER,
                                                   textColor.withAlpha(isSelected ? 1.f : 0.5f));
                }
                else
                {
                    jcmp::GlyphPainter::paintGlyph(g, gr, jcmp::GlyphPainter::FILE_MUSIC,
                                                   textColor.withAlpha(isSelected ? 1.f : 0.5f));
                }
                g.setColour(textColor);
                g.drawText(entry.expandableAddress->name, b.reduced(2, 0),
                           juce::Justification::centredLeft);
            }
            else if (entry.dirent.is_directory())
            {
                jcmp::GlyphPainter::paintGlyph(g, r, jcmp::GlyphPainter::FOLDER,
                                               textColor.withAlpha(isSelected ? 1.f : 0.5f));
                g.setColour(textColor);
                g.drawText(entry.dirent.path().filename().u8string(), 16, 1, width - 16, height - 2,
                           juce::Justification::centredLeft);
            }
            else
            {
                if (browser::Browser::isExpandableInBrowser(entry.dirent.path()))
                {
                    jcmp::GlyphPainter::paintGlyph(g, r,
                                                   entry.isExpanded ? jcmp::GlyphPainter::JOG_DOWN
                                                                    : jcmp::GlyphPainter::PLUS,
                                                   textColor.withAlpha(isSelected ? 1.f : 0.5f));
                }
                else
                {
                    jcmp::GlyphPainter::paintGlyph(g, r, jcmp::GlyphPainter::FILE_MUSIC,
                                                   textColor.withAlpha(isSelected ? 1.f : 0.5f));
                }
                g.setColour(textColor);
                g.drawText(entry.dirent.path().filename().u8string(), 16, 1, width - 16, height - 2,
                           juce::Justification::centredLeft);
            }

            if (hasStartedPreview)
            {
                // auto q = r.translated(getWidth() - r.getHeight() - 2, 0);
                // jcmp::GlyphPainter::paintGlyph(g, q, jcmp::GlyphPainter::SPEAKER,
                //                                textColor.withAlpha(0.5f));
            }
        }
    }
};

void DriveFSArea::setupListView()
{
    listView->getRowCount = [this]() -> uint32_t {
        if (!browserPane || !browserPane->devicesPane || !browserPane->devicesPane->driveFSArea)
            return 0;

        return browserPane->devicesPane->driveFSArea->contents.size();
    };
    listView->getRowHeight = [this]() { return 18; };
    listView->makeRowComponent = [this]() {
        return std::make_unique<DriveFSRowComponent>(browserPane, listView.get());
    };
    listView->assignComponentToRow = [this](const auto &c, auto r) {
        auto dfs = dynamic_cast<DriveFSRowComponent *>(c.get());
        if (dfs)
        {
            dfs->rowNumber = r;
            dfs->repaint();
        }
    };
    listView->setRowSelection = [this](const auto &c, auto r) {
        auto dfs = dynamic_cast<DriveFSRowComponent *>(c.get());
        if (dfs)
        {
            dfs->isSelected = r;
            dfs->repaint();
        }
    };
}

struct FavoritesPane : TempPane
{
    FavoritesPane(SCXTEditor *e) : TempPane(e, "Favorites") {}
};

struct SearchPane : TempPane
{
    SearchPane(SCXTEditor *e) : TempPane(e, "Search") {}
};

struct BrowserPaneFooter : HasEditor, juce::Component
{
    BrowserPane *parent{nullptr};
    std::unique_ptr<jcmp::ToggleButton> autoPreview;
    std::unique_ptr<jcmp::GlyphButton> preview;
    std::unique_ptr<jcmp::HSlider> previewLevel;
    std::unique_ptr<connectors::DirectBooleanPayloadDataAttachment> autoPreviewAtt;

    BrowserPaneFooter(SCXTEditor *e, BrowserPane *p) : HasEditor(e), parent(p)
    {
        autoPreview = std::make_unique<jcmp::ToggleButton>();
        autoPreview->setDrawMode(sst::jucegui::components::ToggleButton::DrawMode::GLYPH_WITH_BG);
        autoPreview->setGlyph(sst::jucegui::components::GlyphPainter::SPEAKER);
        autoPreviewAtt = std::make_unique<connectors::DirectBooleanPayloadDataAttachment>(
            [w = juce::Component::SafePointer(this)](auto v) {
                if (!w)
                    return;
                auto *ed = w->editor;

                ed->defaultsProvider.updateUserDefaultValue(
                    infrastructure::DefaultKeys::browserAutoPreviewEnabled, v);
            },
            parent->autoPreviewEnabled);
        autoPreview->setSource(autoPreviewAtt.get());
        addAndMakeVisible(*autoPreview);
    }
    void resized() override
    {
        auto r = getLocalBounds();
        autoPreview->setBounds(r.withWidth(r.getHeight()));
        r = r.translated(r.getHeight() + 2, 0);
    }
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
                return "LOCATIONS";
                //            case 1:
                //                return "FAVORITES";
            case 1:
                return "SEARCH";
            }
        }
        SCLOG("getValueAsStringFor with invalid value " << i);
        assert(false);
        return "-error-";
    }

    int getMax() const override { return 1; }
};

BrowserPane::BrowserPane(SCXTEditor *e)
    : HasEditor(e), sst::jucegui::components::NamedPanel("Browser")
{
    hasHamburger = true;
    autoPreviewEnabled = editor->defaultsProvider.getUserDefaultValue(
        infrastructure::DefaultKeys::browserAutoPreviewEnabled, true);

    selectedFunction = std::make_unique<sst::jucegui::components::ToggleButtonRadioGroup>();
    selectedFunctionData = std::make_unique<sfData>(this);
    selectedFunction->setSource(selectedFunctionData.get());

    addAndMakeVisible(*selectedFunction);

    devicesPane = std::make_unique<DevicesPane>(this);
    addChildComponent(*devicesPane);
    // favoritesPane = std::make_unique<FavoritesPane>(e);
    // addChildComponent(*favoritesPane);
    searchPane = std::make_unique<SearchPane>(e);
    addChildComponent(*searchPane);

    footerArea = std::make_unique<BrowserPaneFooter>(e, this);
    addAndMakeVisible(*footerArea);

    selectPane(selectedPane);

    resetRoots();
}

BrowserPane::~BrowserPane() { selectedFunction->setSource(nullptr); }

void BrowserPane::resized()
{
    auto r = getContentArea();
    auto sel = r.withHeight(18);
    auto ct = r.withTrimmedTop(21).withTrimmedBottom(23);
    auto ft = r.withTop(r.getBottom() - 21).withTrimmedBottom(2);
    selectedFunction->setBounds(sel);

    devicesPane->setBounds(ct);
    // favoritesPane->setBounds(ct);
    searchPane->setBounds(ct);
    footerArea->setBounds(ft);
}

void BrowserPane::resetRoots()
{
    roots = editor->browser.getRootPathsForDeviceView();
    if (devicesPane && devicesPane->driveArea && devicesPane->driveArea->listView)
        devicesPane->driveArea->listView->refresh();
    repaint();
}

void BrowserPane::selectPane(int i)
{
    selectedPane = i;
    if (selectedPane < 0 || selectedPane > 1)
        selectedPane = 0;

    devicesPane->setVisible(selectedPane == 0);
    // favoritesPane->setVisible(i == 1);
    searchPane->setVisible(selectedPane == 1);
    repaint();
}

void BrowserPane::setIndexWorkload(std::pair<int32_t, int32_t> v)
{
    if (v.first != -1)
        fJobs = v.first;
    ;
    if (v.second != -1)
        dbJobs = v.second;

    if (fJobs == 0 && dbJobs == 0)
    {
        setName("Browser");
    }
    else
    {
        setName("Browser (" + std::to_string(fJobs) + "; " + std::to_string(dbJobs) + ")");
    }
    repaint();
}

} // namespace scxt::ui::app::browser_ui
