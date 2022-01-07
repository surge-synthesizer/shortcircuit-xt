//
// Created by Paul Walker on 1/4/22.
//

#ifndef SHORTCIRCUIT_ZONEEDITOR_H
#define SHORTCIRCUIT_ZONEEDITOR_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "DataInterfaces.h"

struct SC3Editor;

namespace SC3
{
namespace Components
{
struct ZoneEditor : public juce::Component,
                    UIStateProxy::Invalidatable,
                    juce::ComboBox::Listener,
                    juce::TextEditor::Listener
{
    ZoneEditor(SC3Editor *e);
    virtual ~ZoneEditor() = default;

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::orchid); }
    void resized() override;

    void comboBoxChanged(juce::ComboBox *comboBoxThatHasChanged) override;

    void textEditorReturnKeyPressed(juce::TextEditor &) override;

    void resetToZone(int z);
    void onProxyUpdate() override;

    std::unique_ptr<juce::ComboBox> zoneSelector;
    std::unique_ptr<juce::TextEditor> lowKey, hiKey;
    SC3Editor *editor{nullptr};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZoneEditor);
};
} // namespace Components
} // namespace SC3
#endif // SHORTCIRCUIT_ZONEEDITOR_H
