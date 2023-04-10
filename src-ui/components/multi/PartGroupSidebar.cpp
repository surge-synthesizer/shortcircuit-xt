/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#include "PartGroupSidebar.h"
#include "components/SCXTEditor.h"

namespace scxt::ui::multi
{

struct PGZListBoxModel : juce::ListBoxModel
{
    PartGroupSidebar *partGroupSidebar{nullptr};
    PGZListBoxModel(PartGroupSidebar *pgs) : partGroupSidebar(pgs) {}

    int getNumRows() override { return partGroupSidebar->pgzStructure.size(); }

    void paintListBoxItem(int rowNumber, juce::Graphics &g, int width, int height,
                          bool rowIsSelected) override
    {
        const auto &s = partGroupSidebar->pgzStructure;

        assert(rowNumber >= 0 && rowNumber < s.size());
        if (rowNumber < 0 || rowNumber >= s.size())
            return;

        const auto &[addr, name] = s[rowNumber];
        const auto &[pt, gr, zn] = addr;
        auto col = juce::Colours::black;
        auto off = 20;
        if (gr == -1)
        {
            off = 2;
            col = juce::Colours::darkred;
        }
        else if (zn == -1)
        {
            off = 11;
            col = juce::Colours::darkgreen;
        }

        if (rowIsSelected)
        {
            col = col.brighter();
        }

        g.setColour(col);
        g.fillRect(0, 0, width, height);

        g.setColour(col.contrasting());
        g.setFont(juce::Font("Comic Sans MS", 14, juce::Font::plain));
        g.drawText(name, off, 0, width - off, height, juce::Justification::centredLeft);
    }

    void selectedRowsChanged(int rowNumber) override
    {
        const auto &s = partGroupSidebar->pgzStructure;
        if (rowNumber < 0 || rowNumber >= s.size())
            return;
        SCDBGCOUT << "About to select " << SCD(std::get<0>(s[rowNumber])) << std::endl;

        partGroupSidebar->editor->doSelectionAction(std::get<0>(s[rowNumber]), true, true);
    }
};

PartGroupSidebar::PartGroupSidebar(SCXTEditor *e)
    : HasEditor(e), sst::jucegui::components::NamedPanel("Parts n Groups")
{
    isTabbed = true;
    hasHamburger = true;
    tabNames = {"PARTS", "GROUPS"};
    resetTabState();

    pgzList = std::make_unique<juce::ListBox>();
    pgzListModel = std::make_unique<PGZListBoxModel>(this);
    pgzList->setModel(pgzListModel.get());
    pgzList->setMultipleSelectionEnabled(false);
    addAndMakeVisible(*pgzList);
}

PartGroupSidebar::~PartGroupSidebar()
{
    // Just be sure it destroys in the right order
    if (pgzList)
        pgzList->setModel(nullptr);
}

void PartGroupSidebar::resized()
{
    if (pgzList)
        pgzList->setBounds(getContentArea());
}

void PartGroupSidebar::setPartGroupZoneStructure(const engine::Engine::pgzStructure_t &p)
{
    pgzStructure = p;
    pgzList->updateContent();
    editorSelectionChanged();
    repaint();
}

void PartGroupSidebar::editorSelectionChanged()
{
    if (!editor->currentLeadSelection.has_value())
        return;

    auto a = *(editor->currentLeadSelection);
    int selR = -1;
    for (const auto &[i, r] : sst::cpputils::enumerate(pgzStructure))
    {
        if ((r.first.part == a.part) && (r.first.group == a.group) && (r.first.zone == a.zone))
        {
            selR = i;
        }
    }
    if (selR > 0)
    {
        juce::SparseSet<int> rows;
        rows.addRange({selR, selR + 1});
        pgzList->setSelectedRows(rows, juce::dontSendNotification);
    }
    repaint();
}
} // namespace scxt::ui::multi