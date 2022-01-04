//
// Created by Paul Walker on 1/4/22.
//

#include "ZoneEditor.h"
#include "SC3Editor.h"

ZoneEditor::ZoneEditor(SC3Editor *e) : editor(e)
{
    zoneSelector = std::make_unique<juce::ComboBox>();
    zoneSelector->addListener(this);
    addAndMakeVisible(*zoneSelector);

    lowKey = std::make_unique<juce::TextEditor>();
    lowKey->addListener(this);
    addAndMakeVisible(*lowKey);

    hiKey = std::make_unique<juce::TextEditor>();
    hiKey->addListener(this);
    addAndMakeVisible(*hiKey);
}

void ZoneEditor::resized()
{
    auto b = getLocalBounds();
    zoneSelector->setBounds(2, 2, 200, 20);

    lowKey->setBounds(2, 25, 200, 20);
    hiKey->setBounds(2, 50, 200, 20);
}

void ZoneEditor::onProxyUpdate()
{
    zoneSelector->clear(juce::dontSendNotification);

    for (auto i = 0; i < max_zones; ++i)
    {
        if (editor->activeZones[i])
        {
            std::string n =
                std::string(editor->zonesCopy[i].name) + " (Zone " + std::to_string(i + 1) + ")";
            zoneSelector->addItem(n, i + 1);
        }
    }

    if (zoneSelector->getNumItems() > 0)
    {
        zoneSelector->setSelectedItemIndex(0, juce::dontSendNotification);
        resetToZone(0);
    }
}

void ZoneEditor::comboBoxChanged(juce::ComboBox *comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == zoneSelector.get())
    {
        auto idx = zoneSelector->getSelectedId();
        if (idx > 0)
            resetToZone(idx - 1);
    }
}

void ZoneEditor::resetToZone(int z)
{
    lowKey->setText(std::to_string(editor->zonesCopy[z].key_low), juce::dontSendNotification);
    hiKey->setText(std::to_string(editor->zonesCopy[z].key_high), juce::dontSendNotification);
}

void ZoneEditor::textEditorReturnKeyPressed(juce::TextEditor &e)
{
    actiondata ad;
    ad.actiontype = vga_set_zone_keyspan;
    ad.data.i[0] = zoneSelector->getSelectedId() - 1;
    ad.data.i[1] = lowKey->getText().getIntValue();
    ad.data.i[2] = lowKey->getText().getIntValue();
    ad.data.i[3] = hiKey->getText().getIntValue();
    editor->sendActionToEngine(ad);

    ad.actiontype = vga_request_refresh;
    editor->sendActionToEngine(ad);
}