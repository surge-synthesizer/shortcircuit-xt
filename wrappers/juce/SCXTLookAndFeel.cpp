//
// Created by Paul Walker on 1/5/22.
//

#include "SCXTLookAndFeel.h"
#include "BinaryUIAssets.h"
#include "widgets/OutlinedTextButton.h"

SCXTLookAndFeel::SCXTLookAndFeel()
{
    setColour(SCXTColours::headerBackground, juce::Colour(0xFF303050));
    setColour(SCXTColours::headerButton, juce::Colour(0xFFAAAAAA));
    setColour(SCXTColours::headerButtonDown, juce::Colour(0xFFBB6666));
    setColour(SCXTColours::headerButtonText, juce::Colours::white);

    setColour(SCXTColours::vuBackground, juce::Colours::black);
    setColour(SCXTColours::vuOutline, juce::Colour(0xFFAAAAAA));
    setColour(SCXTColours::vuPlayHigh, juce::Colour(0xFF888800));
    setColour(SCXTColours::vuPlayLow, juce::Colour(0xFF3333EE));
    setColour(SCXTColours::vuClip, juce::Colours::red);

    setColour(SCXTColours::fxPanelHeaderText, juce::Colours::white);
    setColour(SCXTColours::fxPanelHeaderBackground, juce::Colour(0xFF336633));
    setColour(SCXTColours::fxPanelBackground, juce::Colour(0xFF446644));

    setColour(scxt::widgets::OutlinedTextButton::upColour, juce::Colour(0xFF151515));
    setColour(scxt::widgets::OutlinedTextButton::downColour, juce::Colour(0xFFAA1515));
    setColour(scxt::widgets::OutlinedTextButton::textColour, juce::Colours::white);
}

struct TypefaceHolder : public juce::DeletedAtShutdown
{
    TypefaceHolder()
    {
        monoTypeface = juce::Typeface::createSystemTypefaceFor(
            SCXTUIAssets::AnonymousProRegular_ttf, SCXTUIAssets::AnonymousProRegular_ttfSize);
    }
    juce::ReferenceCountedObjectPtr<juce::Typeface> monoTypeface;
};

juce::Font SCXTLookAndFeel::getMonoFontAt(int sz)
{
    static auto faces = new TypefaceHolder();
    return juce::Font(faces->monoTypeface).withHeight(sz);
}

void SCXTLookAndFeel::fillWithRaisedOutline(juce::Graphics &g, const juce::Rectangle<int> &r,
                                            juce::Colour base, bool down)
{
    auto up = base.brighter(0.4);
    auto dn = base.darker(0.4);

    auto drawThis = r.toFloat().reduced(0.5, 0.5);
    g.setColour(base);
    g.fillRoundedRectangle(drawThis, 2);
    g.setColour(down ? dn : up);
    g.drawRoundedRectangle(drawThis.toFloat(), 2, 1);
}

void SCXTLookAndFeel::fillWithGradientHeaderBand(juce::Graphics &g, const juce::Rectangle<int> &r,
                                                 juce::Colour base)
{
    auto rTop = r.withHeight(r.getHeight() * 0.7);
    auto rBottom = r.withTrimmedTop(rTop.getHeight());

    auto cgt = juce::ColourGradient::vertical(base.brighter(0.4), base, rTop);
    g.setGradientFill(cgt);
    g.fillRect(rTop);

    auto cgb = juce::ColourGradient::vertical(base, base.darker(0.4), rBottom);
    g.setGradientFill(cgb);
    g.fillRect(rBottom);

    g.setColour(juce::Colours::black);
    g.drawLine(r.getX(), r.getBottom(), r.getX() + r.getWidth(), r.getBottom(), 0.5);
}
void SCXTLookAndFeel::drawComboBox(juce::Graphics &g, int w, int h, bool isButtonDown, int buttonX,
                                   int buttonY, int buttonW, int buttonH, juce::ComboBox &box)
{
    auto c = juce::Colour(0xFF151515);
    if (!box.isEnabled())
        c = juce::Colour(0xFF777777);
    fillWithRaisedOutline(g, juce::Rectangle<int>(0, 0, w, h), c, !isButtonDown);
    auto r = juce::Rectangle<int>(buttonX, buttonY, buttonW, buttonH);
    auto cy = r.getCentreY();
    r = r.reduced(5, 5);
    r = r.withTrimmedLeft(r.getWidth() - r.getHeight());
    r = r.withHeight(8).withWidth(8).withCentre(juce::Point<int>{r.getCentreX(), cy});
    g.setColour(juce::Colours::white);
    g.drawLine(
        juce::Line<float>{r.getTopLeft().toFloat(), r.getCentre().withY(r.getBottom()).toFloat()},
        1);
    g.drawLine(
        juce::Line<float>{r.getTopRight().toFloat(), r.getCentre().withY(r.getBottom()).toFloat()},
        1);
}

void SCXTLookAndFeel::fillTextEditorBackground(juce::Graphics &g, int w, int h,
                                               juce::TextEditor &textEditor)
{
    auto c = juce::Colour(0xFF151515);
    fillWithRaisedOutline(g, juce::Rectangle<int>(0, 0, w, h), c, true);
}
