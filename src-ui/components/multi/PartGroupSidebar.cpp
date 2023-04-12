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
#include "selection/selection_manager.h"

namespace scxt::ui::multi
{

struct PGZListBox : public juce::ListBox
{
};

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
        const auto &m = partGroupSidebar->pgzList;
        const auto &s = partGroupSidebar->pgzStructure;
        std::unordered_set<selection::SelectionManager::ZoneAddress,
                           selection::SelectionManager::ZoneAddress::Hash>
            newSelections;
        selection::SelectionManager::ZoneAddress lead;
        bool setLead{false};
        std::unordered_set<selection::SelectionManager::ZoneAddress,
                           selection::SelectionManager::ZoneAddress::Hash>
            removeSelections;
        if (m)
        {
            const auto &r = m->getSelectedRows();
            // Rather than iterating over the ranges, since we need to make
            // the both-way list we iterate over the entire structure
            for (const auto &[idx, dat] : sst::cpputils::enumerate(s))
            {
                bool selectedUI = r.contains(idx);
                bool selectedModel = partGroupSidebar->editor->isSelected(dat.first);

                if (selectedUI || selectedModel)
                {
                    SCDBGCOUT << SCD(dat.first) << SCD(idx) << SCD(selectedUI) << SCD(selectedModel)
                              << std::endl;
                }
                if (selectedUI && !selectedModel)
                {
                    if (idx == rowNumber)
                    {
                        lead = dat.first;
                        setLead = true;
                    }
                    else
                        newSelections.insert(dat.first);
                }
                if (!selectedUI && selectedModel)
                {
                    removeSelections.insert(dat.first);
                }
            }
        }

        auto dist = m->getSelectedRows().size() == 1;
        std::vector<selection::SelectionManager::SelectActionContents> res;
        for (auto &rm : newSelections)
        {
            res.emplace_back(rm, true, dist, false);
        }

        for (auto &rm : removeSelections)
        {
            res.emplace_back(rm, false, false, false);
        }

        if (setLead)
        {
            res.emplace_back(lead, true, false, true);
        }

        partGroupSidebar->editor->doMultiSelectionAction(res);
    }
};

PartGroupSidebar::PartGroupSidebar(SCXTEditor *e)
    : HasEditor(e), sst::jucegui::components::NamedPanel("Parts n Groups")
{
    isTabbed = true;
    hasHamburger = true;
    tabNames = {"PARTS", "GROUPS"};
    resetTabState();

    pgzList = std::make_unique<PGZListBox>();
    pgzListModel = std::make_unique<PGZListBoxModel>(this);
    pgzList->setModel(pgzListModel.get());
    pgzList->setMultipleSelectionEnabled(true);
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

    juce::SparseSet<int> rows;

    for (const auto &a : editor->allSelections)
    {
        int selR = -1;
        for (const auto &[i, r] : sst::cpputils::enumerate(pgzStructure))
        {
            if (r.first == a)
            {
                rows.addRange({(int)i, (int)(i + 1)});
            }
        }
    }

    pgzList->setSelectedRows(rows, juce::dontSendNotification);
    repaint();
}
} // namespace scxt::ui::multi