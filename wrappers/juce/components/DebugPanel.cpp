/*
** ShortCircuit3 is Free and Open Source Software
**
** ShortCircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** ShortCircuit was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made ShortCircuit
** open source in December 2020.
*/

#include "DebugPanel.h"
#include "SC3Editor.h"

void ActionRunner::buttonClicked(juce::Button *b)
{
    if (b == mSendActionBtn.get())
    {
        auto itemSelected = mActionList->getSelectedId();
        if (itemSelected > 0)
        {
            std::string error;
            actiondata ad;
            if (mItems[itemSelected - 1]->prepareAction(mEditor->audioProcessor.sc3.get(),
                                                        mParameters->getText().toStdString(), &ad,
                                                        &error))
            {
                mEditor->audioProcessor.sc3->postEventsFromWrapper(ad);
                mDescription->setText("Action was sent.", dontSendNotification);
            }
            else
            {
                mDescription->setText(juce::String(error), NotificationType::dontSendNotification);
            }
        }
    }
}
void ActionRunner::resized()
{
    auto r = getLocalBounds();
    auto t = r.removeFromTop(22);
    mSendActionBtn->setBounds(t.removeFromRight(40));
    t.removeFromRight(2);
    auto u = t.removeFromLeft(180);
    mActionList->setBounds(u);
    t.removeFromLeft(2);
    mParameters->setBounds(t);
    r.removeFromTop(2);
    mDescription->setBounds(r);
}
ActionRunner::ActionRunner()
{
    mSendActionBtn = std::make_unique<juce::TextButton>("send");
    addAndMakeVisible(mSendActionBtn.get());
    mSendActionBtn->addListener(this);
    mActionList = std::make_unique<juce::ComboBox>();
    mActionList->addListener(this);
    addAndMakeVisible(mActionList.get());
    mParameters = std::make_unique<juce::TextEditor>();
    addAndMakeVisible(mParameters.get());
    mDescription = std::make_unique<juce::TextEditor>();
    mDescription->setMultiLine(true, true);
    mDescription->setReadOnly(true);
    addAndMakeVisible(mDescription.get());

    mItems = registerScratchPadItems();
    int index = 0;
    for (auto it : mItems)
    {
        mActionList->addItem(it->mName, ++index); // must be 1 based
    }
    mDescription->setText("Select action to run", NotificationType::dontSendNotification);
}
void ActionRunner::comboBoxChanged(ComboBox *comboBoxThatHasChanged)
{
    auto id = mActionList->getSelectedId();
    if (id > 0)
    {
        mDescription->setText(mItems[id - 1]->mDescription, NotificationType::dontSendNotification);
    }
}
DebugPanel::DebugPanel() : Component("Debug Panel")
{
    // sampler state window
    samplerT = std::make_unique<juce::TextEditor>();
    samplerT->setMultiLine(true, false);
    addAndMakeVisible(samplerT.get());

    // log window
    logT = std::make_unique<juce::TextEditor>();
    logT->setMultiLine(true, false);
    addAndMakeVisible(logT.get());

    // action runner
    mActionRunner = std::make_unique<ActionRunner>();
    addAndMakeVisible(mActionRunner.get());
}
void DebugPanel::resized()
{
    auto r = getLocalBounds();
    r.reduce(5, 5);

    // action runner has fixed size
    auto t = r.removeFromTop(100);
    mActionRunner->setBounds(t);

    r.removeFromTop(5);
    auto h = r.getHeight() - 5; // 5 is space between

    // state occupies half rest of space
    t = r.removeFromTop(h / 2);
    samplerT->setBounds(t);

    // log occupies the other half
    r.removeFromTop(5);
    t = r.removeFromTop(h / 2);
    logT->setBounds(t);
}
DebugPanelWindow::DebugPanelWindow()
    : juce::DocumentWindow(juce::String("SC3 Debug - ") + SC3::Build::GitHash + "/" +
                               SC3::Build::GitBranch,
                           juce::Colour(0xFF000000), juce::DocumentWindow::allButtons)
{
    setResizable(true, true);
    setUsingNativeTitleBar(false);
    setSize(500, 640);
    setTopLeftPosition(100, 100);
    panel = new DebugPanel();
    setContentOwned(panel, false); // DebugPanelWindow takes ownership
}
