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

#ifndef SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_SAMPLEWAVEFORM_H
#define SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_SAMPLEWAVEFORM_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/ZoomContainer.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/TabbedComponent.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "app/HasEditor.h"
#include "app/edit-screen/components/MacroMappingVariantPane.h"
#include "connectors/PayloadDataAttachment.h"

namespace scxt::ui::app::edit_screen
{
struct VariantDisplay;

struct SampleWaveform : juce::Component, HasEditor, sst::jucegui::components::ZoomContainerClient
{
    VariantDisplay *display{nullptr};
    SampleWaveform(VariantDisplay *d);
    void paint(juce::Graphics &g) override;
    void resized() override;
    void visibilityChanged() override
    {
        if (isVisible())
        {
            rebuildEnvelopePaths();
            repaint();
        }
    }

    juce::Rectangle<int> startSampleHZ, endSampleHZ, startLoopHZ, endLoopHZ, fadeLoopHz;
    void rebuildHotZones();

    // Anticipating future drag and so forth gestures
    enum struct MouseState
    {
        NONE,
        HZ_DRAG_SAMPSTART,
        HZ_DRAG_SAMPEND,
        HZ_DRAG_LOOPSTART,
        HZ_DRAG_LOOPEND,
    } mouseState{MouseState::NONE};

    struct PlaybackCursor : juce::Component
    {
        void paint(juce::Graphics &g) override
        { /*g.fillAll(juce::Colours::white);*/
        }
    } samplePlaybackPosition;
    void updateSamplePlaybackPosition(int64_t samplePos);

    void rebuildPaths();

    int usedChannels{1};
    juce::Path upperStroke[2], lowerStroke[2], upperFill[2], lowerFill[2];
    void rebuildEnvelopePaths();

    int64_t sampleForXPixel(float xpos);
    int xPixelForSample(int64_t samplePos, bool doClamp = true);
    int yPixelForAmplitude(float amp, float zeroPoint, float extraScale);
    int xPixelForSampleDistance(int64_t sampleDistance);
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseMove(const juce::MouseEvent &e) override;
    void mouseDoubleClick(const juce::MouseEvent &e) override;

    Component *associatedComponent() override { return this; }
    bool supportsVerticalZoom() const override { return true; }
    bool supportsHorizontalZoom() const override { return true; }

    juce::Rectangle<int> getInsetBounds() { return getLocalBounds().reduced(2); }

    float pctStart{0.f}, zoomFactor{1.f};
    void setHorizontalZoom(float ps, float zf) override
    {
        pctStart = ps;
        zoomFactor = zf;
        rebuildHotZones();
        repaint();
    }

    float vStart{0.f}, vZoom{1.f};
    void setVerticalZoom(float ps, float zf) override
    {
        vStart = ps;
        vZoom = zf;
        rebuildHotZones();
        repaint();
    }
};

} // namespace scxt::ui::app::edit_screen
#endif // SHORTCIRCUITXT_SAMPLEWAVEFORM_H
