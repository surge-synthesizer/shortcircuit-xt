#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "app.h"

#include "components/SCXTEditor.h"

struct SCXTAppComponent : public juce::Component
{
    std::unique_ptr<scxt::engine::Engine> engine;
    scxt::juce_app::PlaybackState playbackState;

    std::unique_ptr<scxt::ui::SCXTEditor> editor;

    SCXTAppComponent()
    {
        startEngine();
        editor = std::make_unique<scxt::ui::SCXTEditor>(*(engine->getMessageController()));
        addAndMakeVisible(*editor);
    }
    ~SCXTAppComponent()
    {
        removeChildComponent(editor.get());
        editor.reset();
        stopEngine();
    }

    void resized() override { editor->setBounds(getLocalBounds()); }

    void startEngine()
    {
        assert(!engine);
        // MOve all this to the main window component
        engine = std::make_unique<scxt::engine::Engine>();
        playbackState.engine = engine.get();

        auto samplePath = fs::current_path();

        auto rm = fs::path{"resources/test_samples/next/README.md"};

        while (samplePath.has_parent_path() && !(fs::exists(samplePath / rm)))
        {
            samplePath = samplePath.parent_path();
        }
        if (fs::exists(samplePath / rm))
        {
            auto sid = engine->getSampleManager()->loadSampleByPath(
                samplePath / "resources/test_samples/next/PulseSaw.wav");
            auto zptr = std::make_unique<scxt::engine::Zone>(*sid);
            zptr->keyboardRange = {48, 72};
            zptr->rootKey = 60;
            zptr->attachToSample(*(engine->getSampleManager()));

            zptr->processorStorage[0].type = scxt::dsp::processor::proct_SuperSVF;
            zptr->processorStorage[0].mix = 1.0;

            zptr->aegStorage.a = 0.7;

            engine->getPatch()->getPart(0)->getGroup(0)->addZone(zptr);
        }
        scxt::juce_app::startMIDI(&playbackState, "LPK");
        scxt::juce_app::startAudioThread(&playbackState, true);
    }

    void stopEngine()
    {
        // TODO: Probably an order thing with the editor
        scxt::juce_app::stopMIDI(&playbackState);
        scxt::juce_app::stopAudioThread(&playbackState);

        engine.reset();

        scxt::showLeakLog();
    }

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::red); }
};

class SCXTAppWindow : public juce::DocumentWindow
{
  public:
    SCXTAppWindow(const juce::String &name)
        : DocumentWindow(name,
                         juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                             ResizableWindow::backgroundColourId),
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, false);
        setResizeLimits(400, 400, 10000, 10000);

        setBounds((int)(0.1f * (float)getParentWidth()), (int)(0.1f * (float)getParentHeight()),
                  1186, 810);

        auto ac = new SCXTAppComponent();
        setContentOwned(ac, false);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SCXTAppWindow)
};

//==============================================================================
class SCXTAppClient : public juce::JUCEApplication
{
  public:
    //==============================================================================
    SCXTAppClient() = default;
    ~SCXTAppClient() override = default;

    const juce::String getApplicationName() override { return "scxt juce app"; }
    const juce::String getApplicationVersion() override { return "0.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    //==============================================================================
    void initialise(const juce::String &commandLine) override
    {
        mainWindow = std::make_unique<SCXTAppWindow>(getApplicationName());
    }

    void shutdown() override { mainWindow.reset(); }

    //==============================================================================
    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted(const juce::String &) override {}

  private:
    std::unique_ptr<SCXTAppWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(SCXTAppClient)
