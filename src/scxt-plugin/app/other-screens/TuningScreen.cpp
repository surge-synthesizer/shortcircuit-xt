/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "TuningScreen.h"

#include "app/SCXTEditor.h"
#include "app/shared/UIHelpers.h"
#include "connectors/SCXTResources.h"
#include "messaging/client/enginestatus_messages.h"
#include "tuning/scl_kbm.h"

#include "sst/jucegui/components/TextPushButton.h"

namespace scxt::ui::app::other_screens
{
namespace jcmp = sst::jucegui::components;
namespace cmsg = scxt::messaging::client;

struct TuningScreenContents : juce::Component, HasEditor
{
    TuningScreen *parent{nullptr};

    std::unique_ptr<juce::TextEditor> sclEditor, kbmEditor;
    std::unique_ptr<jcmp::TextPushButton> loadButton, applyButton, revertButton, closeButton;

#if JUCE_VERSION >= 0x080000
    juce::Font displayFont{juce::FontOptions(1)};
#else
    juce::Font displayFont{};
#endif

    TuningScreenContents(TuningScreen *p, SCXTEditor *e) : parent(p), HasEditor(e)
    {
        auto anonPro =
            connectors::resources::loadTypeface("fonts/Anonymous_Pro/AnonymousPro-Regular.ttf");
        displayFont = editor->themeApplier.anonmyousProRegularFor(11);

        sclEditor = makeEditor();
        addAndMakeVisible(*sclEditor);
        kbmEditor = makeEditor();
        addChildComponent(*kbmEditor);

        loadButton = makeButton("Load...", [](auto *w) { w->doLoad(); });
        applyButton = makeButton("Apply", [](auto *w) { w->doApply(); });
        revertButton = makeButton("Revert", [](auto *w) { w->doRevert(); });
        closeButton = makeButton("Close", [](auto *w) { w->setVisible(false); });
    }

    std::unique_ptr<juce::TextEditor> makeEditor()
    {
        auto res = std::make_unique<juce::TextEditor>();
        res->setMultiLine(true);
        res->setReturnKeyStartsNewLine(true);
        res->setTabKeyUsedAsCharacter(true);
        res->setFont(displayFont);
        res->setColour(juce::TextEditor::backgroundColourId,
                       editor->themeColor(theme::ColorMap::bg_2));
        res->setColour(juce::TextEditor::textColourId,
                       editor->themeColor(theme::ColorMap::generic_content_high));
        res->setColour(juce::TextEditor::outlineColourId,
                       editor->themeColor(theme::ColorMap::grid_primary));
        res->setColour(juce::TextEditor::highlightColourId,
                       editor->themeColor(theme::ColorMap::accent_1a).withAlpha(0.4f));
        return res;
    }

    template <typename Fn> std::unique_ptr<jcmp::TextPushButton> makeButton(const char *l, Fn &&fn)
    {
        auto res = std::make_unique<jcmp::TextPushButton>();
        res->setLabel(l);
        res->setOnCallback([w = juce::Component::SafePointer(parent), fn]() {
            if (w)
                fn(w.getComponent());
        });
        addAndMakeVisible(*res);
        return res;
    }

    void setActiveTab(int t)
    {
        sclEditor->setVisible(t == 0);
        kbmEditor->setVisible(t == 1);
        resized();
    }

    void resized() override
    {
        auto bh = 26;
        auto b = getLocalBounds();
        auto tb = b.withTrimmedBottom(bh + 6);
        auto bb = b.withTop(b.getBottom() - bh);

        sclEditor->setBounds(tb);
        kbmEditor->setBounds(tb);

        auto bw = 90;
        loadButton->setBounds(bb.withWidth(bw));
        closeButton->setBounds(bb.withLeft(bb.getRight() - bw));
        applyButton->setBounds(bb.withLeft(bb.getRight() - 2 * bw - 4).withWidth(bw));
        revertButton->setBounds(bb.withLeft(bb.getRight() - 3 * bw - 8).withWidth(bw));
    }
};

TuningScreen::TuningScreen(SCXTEditor *e) : HasEditor(e)
{
    auto ct = std::make_unique<jcmp::NamedPanel>("SCL/KBM Tuning");
    ct->isTabbed = true;
    ct->tabNames = {"SCL", "KBM"};
    ct->selectTab(0);
    ct->onTabSelected = [w = juce::Component::SafePointer(this)](int i) {
        if (w)
            w->setActiveTab(i);
    };
    addAndMakeVisible(*ct);

    ct->setContentAreaComponent(std::make_unique<TuningScreenContents>(this, e));
    ct->resetTabState();
    contentsArea = std::move(ct);
}

TuningScreen::~TuningScreen() {}

TuningScreenContents *TuningScreen::contents()
{
    return dynamic_cast<TuningScreenContents *>(contentsArea->getContentAreaComponent().get());
}

void TuningScreen::setActiveTab(int t)
{
    activeTab = t;
    if (auto c = contents())
        c->setActiveTab(t);
}

void TuningScreen::setSclKbmFromEngine(const std::string &scl, const std::string &kbm)
{
    auto c = contents();
    if (!c)
        return;

    /*
     * Seed empty boxes with well formed examples rather than a blank page. Both
     * seeds are no-ops against what is showing, so Apply on an untouched box
     * cannot move the pitch.
     */
    auto sclShown = scl.empty() ? scxt::tuning::twelveTETSclText() : scl;
    auto kbmShown = kbm.empty() ? scxt::tuning::defaultKbmText() : kbm;
    c->sclEditor->setText(sclShown, juce::dontSendNotification);
    c->kbmEditor->setText(kbmShown, juce::dontSendNotification);
}

void TuningScreen::doApply()
{
    auto c = contents();
    if (!c)
        return;
    editor->applySclKbmText(c->sclEditor->getText().toStdString(),
                            c->kbmEditor->getText().toStdString());
}

void TuningScreen::doRevert() { setSclKbmFromEngine(editor->sclText, editor->kbmText); }

void TuningScreen::doLoad()
{
    auto isScl = activeTab == 0;
    fileChooser = std::make_unique<juce::FileChooser>(isScl ? "Load Scale (SCL)"
                                                            : "Load Keyboard Mapping (KBM)",
                                                      juce::File(), isScl ? "*.scl" : "*.kbm");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::openMode,
        [w = juce::Component::SafePointer(this), isScl](const juce::FileChooser &fc) {
            if (!w)
                return;
            auto result = fc.getResults();
            if (result.size() != 1)
                return;

            auto txt = shared::fileToString(shared::juceFileToFSPath(result[0]));
            if (!txt.has_value())
            {
                w->editor->displayError("Tuning Error", "Unable to open that file for reading.");
                return;
            }

            auto c = w->contents();
            if (!c)
                return;
            // Load only fills the box; the user presses Apply to make it live
            (isScl ? c->sclEditor : c->kbmEditor)->setText(*txt, juce::dontSendNotification);
        });
}

void TuningScreen::visibilityChanged()
{
    if (isVisible())
        doRevert();
}

void TuningScreen::resized() { contentsArea->setBounds(getLocalBounds().reduced(80, 40)); }

bool TuningScreen::keyPressed(const juce::KeyPress &key)
{
    if (key.getKeyCode() == juce::KeyPress::escapeKey)
    {
        setVisible(false);
        return true;
    }
    return false;
}

} // namespace scxt::ui::app::other_screens
