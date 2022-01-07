//
// Created by Paul Walker on 1/5/22.
//

#include "SCXTLookAndFeel.h"
#include "BinaryUIAssets.h"

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
    g.setColour(base);
    g.fillRect(r);

    auto up = base.brighter(0.4);
    auto dn = base.darker(0.4);

    if (down)
    {
        std::swap(up, dn);
    }

    // This could obviously be cleaner
    auto rf = r.reduced(1, 1).toFloat();
    g.setColour(up);
    g.drawLine({rf.getTopLeft(), rf.getTopRight()}, 2.0);
    g.drawLine({rf.getTopLeft(), rf.getBottomLeft()}, 2.0);
    g.setColour(dn);
    g.drawLine({rf.getTopRight(), rf.getBottomRight()}, 2.0);
    g.drawLine({rf.getBottomLeft(), rf.getBottomRight()}, 2.0);
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
}