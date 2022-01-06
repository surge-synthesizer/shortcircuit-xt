//
// Created by Paul Walker on 1/5/22.
//

#include "SCXTLookAndFeel.h"
#include "BinaryUIAssets.h"

SCXTLookAndFeel::SCXTLookAndFeel() {}

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
