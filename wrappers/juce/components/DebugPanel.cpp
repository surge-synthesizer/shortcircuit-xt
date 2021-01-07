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

void DebugPanel::buttonClicked(juce::Button *b)
{
    if (b == loadButton.get())
    {
        juce::FileChooser sampleChooser(
            "Please choose a sample file",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory));
        if (sampleChooser.browseForFileToOpen())
        {
            auto d = new DropList();

            auto f = sampleChooser.getResult();
            auto fd = DropList::File();
            fd.p = string_to_path(f.getFileName().toStdString().c_str());
            d->files.push_back(fd);

            actiondata ad;
            ad.actiontype = vga_load_dropfiles;
            ad.data.dropList = d;
            ed->audioProcessor.sc3->postEventsFromWrapper(ad);
        }
    }
}

void DebugPanel::buttonStateChanged(juce::Button *b)
{
    if (b == manualButton.get())
    {
        switch (manualButton->getState())
        {
        case juce::Button::buttonDown:
            playingNote = std::atoi(noteNumber->getText().toRawUTF8());
            ed->audioProcessor.sc3->PlayNote(0, playingNote, 127);
            manualPlaying = true;
            break;
        case juce::Button::buttonNormal:
            if (manualPlaying)
            {
                manualPlaying = false;
                ed->audioProcessor.sc3->ReleaseNote(0, playingNote, 127);
            }
            break;
        case juce::Button::buttonOver:
            break;
        }
    }
}