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

/*
 * This is just a stopgap until we finish up the clap wrapper standalone to get us into
 * full clap-first mode
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "utils.h"

#include "version.h"

#include "engine/engine.h"
#include "components/SCXTEditor.h"
#include "sst/voicemanager/midi1_to_voicemanager.h"

using namespace juce;

struct SCXTApplicationWindow : juce::DocumentWindow, juce::Button::Listener
{
    struct Intermediate : juce::Component
    {
        juce::Component &onto;
        Intermediate(juce::Component &o) : onto(o) { addAndMakeVisible(onto); }
        void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::red); }
        void resized() override
        {
            onto.setBounds(0, 0, scxt::ui::SCXTEditor::edWidth, scxt::ui::SCXTEditor::edHeight);
        }
    };
    SCXTApplicationWindow()
        : juce::DocumentWindow(
              juce::String("Shortcircuit XT "), /* + scxt::build::FullVersionStr, */
              juce::Colour(0x15, 0x15, 0x20),
              DocumentWindow::minimiseButton | DocumentWindow::closeButton),
          optionsButton("Options")
    {
        engine = std::make_unique<scxt::engine::Engine>();
        engine->runningEnvironment = "Temporary SCXT Standalone";

        editor = std::make_unique<scxt::ui::SCXTEditor>(
            *(engine->getMessageController()), *(engine->defaults), *(engine->getSampleManager()),
            *(engine->getBrowser()), engine->sharedUIMemoryState);
        editorHolder = std::make_unique<Intermediate>(*editor);
        editor->onZoomChanged = [this](auto f) {
            this->setSize(scxt::ui::SCXTEditor::edWidth * f,
                          scxt::ui::SCXTEditor::edHeight * f + getTitleBarHeight());
        };
        editor->setSize(scxt::ui::SCXTEditor::edWidth, scxt::ui::SCXTEditor::edHeight);
        Component::addAndMakeVisible(*editorHolder);

        setTitleBarButtonsRequired(DocumentWindow::minimiseButton | DocumentWindow::closeButton,
                                   false);

        Component::addAndMakeVisible(optionsButton);
        optionsButton.addListener(this);
        optionsButton.setTriggeredOnMouseDown(true);

        editor->onZoomChanged(1.f);

        juce::PropertiesFile::Options options;
        options.applicationName = "ShortcircuitXTStandalone";
#if JUCE_LINUX
        options.folderName = ".config/ShortcircuitXTStandalone";
#else
        options.folderName = "ShortcircuitXTStandalone";
#endif

        options.filenameSuffix = "settings";
        options.osxLibrarySubFolder = "Application Support";
        properties = std::make_unique<juce::PropertiesFile>(options);

        auto streamedState = properties->getValue("engineState");
        if (!streamedState.isEmpty())
        {
            scxt::messaging::client::clientSendToSerialization(
                scxt::messaging::client::UnstreamIntoEngine{streamedState.toStdString()},
                *engine->getMessageController());
        }

        setupAudio();
    }

    ~SCXTApplicationWindow()
    {
        // save on exit
        auto xml = deviceManager.createStateXml();
        properties->setValue("audioSetup", xml.get());

        engine->getSampleManager()->purgeUnreferencedSamples();
        try
        {
            auto engineXml = scxt::json::streamEngineState(*engine);
            SCLOG("Streaming State Information: " << engineXml.size() << " bytes");
            properties->setValue("engineState", juce::String(engineXml));
        }
        catch (const std::runtime_error &err)
        {
            SCLOG("Unable to stream [" << err.what() << "]");
        }
        properties->saveIfNeeded();

        // guarantee order
        deviceManager.removeMidiInputDeviceCallback({}, audioCB.get());
        deviceManager.removeAudioCallback(audioCB.get());

        editor.reset();
        engine.reset();
    }

    class SettingsComponent : public Component
    {
      public:
        SettingsComponent(SCXTApplicationWindow &pluginHolder,
                          AudioDeviceManager &deviceManagerToUse, int maxAudioInputChannels,
                          int maxAudioOutputChannels)
            : owner(pluginHolder), deviceSelector(deviceManagerToUse, 0, maxAudioInputChannels, 0,
                                                  maxAudioOutputChannels, true, false, true, false)
        {
            setOpaque(true);
            addAndMakeVisible(deviceSelector);
        }

        void paint(Graphics &g) override
        {
            g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
        }

        void resized() override
        {
            const ScopedValueSetter<bool> scope(isResizing, true);
            deviceSelector.setBounds(getLocalBounds());
        }

        void childBoundsChanged(Component *childComp) override
        {
            if (!isResizing && childComp == &deviceSelector)
                setToRecommendedSize();
        }

        void setToRecommendedSize()
        {
            const auto extraHeight = 0;
            setSize(getWidth(), deviceSelector.getHeight() + extraHeight);
        }

      private:
        //==============================================================================
        SCXTApplicationWindow &owner;
        AudioDeviceSelectorComponent deviceSelector;
        bool isResizing = false;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)
    };

    void buttonClicked(juce::Button *button) override
    {
        juce::DialogWindow::LaunchOptions o;

        int maxNumInputs = 0, maxNumOutputs = 2;

        auto content =
            std::make_unique<SettingsComponent>(*this, deviceManager, maxNumInputs, maxNumOutputs);
        content->setSize(500, 550);
        content->setToRecommendedSize();

        o.content.setOwned(content.release());

        o.dialogTitle = TRANS("Audio/MIDI Settings");
        o.dialogBackgroundColour =
            o.content->getLookAndFeel().findColour(ResizableWindow::backgroundColourId);
        o.escapeKeyTriggersCloseButton = true;
        o.useNativeTitleBar = true;
        o.resizable = false;

        o.launchAsync();
    }
    void resized() override
    {
        DocumentWindow::resized();
        optionsButton.setBounds(8, 6, 60, getTitleBarHeight() - 8);
        if (editorHolder)
            editorHolder->setBounds(getLocalBounds().withTrimmedTop(getTitleBarHeight()));
    }
    void closeButtonPressed() override
    {
        SCLOG("Shutdown please");
        juce::JUCEApplicationBase::quit();
    }

    struct AudioCallback : public AudioIODeviceCallback, MidiInputCallback
    {
        SCXTApplicationWindow &window;
        AudioCallback(SCXTApplicationWindow &w) : window(w) {}
        void audioDeviceAboutToStart(AudioIODevice *device) override
        {
            SCLOG("Starting engine at " << device->getCurrentSampleRate());
            window.engine->prepareToPlay(device->getCurrentSampleRate());
            window.engine->transport.tempo = 120;
        }
        void audioDeviceIOCallbackWithContext(const float *const *inputChannelData,
                                              int numInputChannels, float *const *outputChannelData,
                                              int numOutputChannels, int numSamples,
                                              const AudioIODeviceCallbackContext &context) override
        {
            auto &main = window.engine->getPatch()->busses.mainBus.output;

            for (auto s = 0U; s < numSamples; ++s)
            {
                if (blockPos == 0)
                {
                    bool tryRead{true};
                    while (tryRead)
                    {
                        auto scope = midiBufferFIFO.read(1);

                        if (scope.blockSize2 + scope.blockSize1 == 0)
                        {
                            tryRead = false;
                        }
                        else
                        {
                            auto msg = (scope.blockSize1 == 1) ? midiBuffer[scope.startIndex1]
                                                               : midiBuffer[scope.startIndex2];
                            sst::voicemanager::applyMidi1Message(window.engine->voiceManager, 0,
                                                                 msg.getRawData());
                        }
                    }
                    // Drain thread safe midi queue
                    window.engine->processAudio();
                    window.engine->transport.timeInBeats +=
                        (double)scxt::blockSize * window.engine->transport.tempo *
                        window.engine->getSampleRateInv() / 60.f;
                }

                // TODO: this can be way more efficient and block wise and stuff
                outputChannelData[0][s] = main[0][blockPos];
                outputChannelData[1][s] = main[1][blockPos];

                blockPos = (blockPos + 1) & (scxt::blockSize - 1);
            }
        }
        void audioDeviceStopped() override { SCLOG("Stopping engine"); }

        void handleIncomingMidiMessage(MidiInput *source, const MidiMessage &message) override
        {
            // Push to thread safe midi queue
            const auto scope = midiBufferFIFO.write(1);
            if (scope.blockSize1 == 1)
                midiBuffer[scope.startIndex1] = message;
            if (scope.blockSize2 == 1)
                midiBuffer[scope.startIndex2] = message;
        }

        static constexpr size_t midiBufferSize{4096};
        std::array<juce::MidiMessage, midiBufferSize> midiBuffer;
        juce::AbstractFifo midiBufferFIFO{midiBufferSize};
        // Add a thread safe midi queue
        uint32_t blockPos{0};
    };

    std::unique_ptr<AudioCallback> audioCB;
    void setupAudio()
    {
        auto xml = properties->getXmlValue("audioSetup");
        audioCB = std::make_unique<AudioCallback>(*this);
        deviceManager.addAudioCallback(audioCB.get());
        deviceManager.addMidiInputDeviceCallback({}, audioCB.get());
        deviceManager.initialise(0, 2, xml.get(), true, "", {});
    }

    juce::TextButton optionsButton;
    AudioDeviceManager deviceManager;

    std::unique_ptr<scxt::engine::Engine> engine;
    std::unique_ptr<scxt::ui::SCXTEditor> editor;
    std::unique_ptr<juce::PropertiesFile> properties;
    std::unique_ptr<Intermediate> editorHolder;
};

class SCXTStandaloneClapFirstJuceImpl : public juce::JUCEApplication
{
  public:
    SCXTStandaloneClapFirstJuceImpl() {}
    ~SCXTStandaloneClapFirstJuceImpl() {}

    void initialise(const juce::String &commandLine) override
    {
        myMainWindow.reset(new SCXTApplicationWindow());
        myMainWindow->setBounds(100, 100, 400, 500);
        myMainWindow->setVisible(true);
    }

    void shutdown() override { myMainWindow = nullptr; }

    const juce::String getApplicationName() override { return "Shortcircuit XT"; }

    const juce::String getApplicationVersion() override { return "1.0"; }
    std::unique_ptr<juce::LookAndFeel> appLF;

  private:
    std::unique_ptr<SCXTApplicationWindow> myMainWindow;
};

// this generates boilerplate code to launch our app class:
START_JUCE_APPLICATION(SCXTStandaloneClapFirstJuceImpl)