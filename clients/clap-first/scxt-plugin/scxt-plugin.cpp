//
// Created by Paul Walker on 10/25/23.
//

#include "scxt-plugin.h"
#include "version.h"
#include "components/SCXTEditor.h"

namespace scxt::clap_first::scxt_plugin
{
const clap_plugin_descriptor *getDescription()
{
    static const char *features[] = {CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_SAMPLER,
                                     CLAP_PLUGIN_FEATURE_SYNTHESIZER, nullptr};
    static clap_plugin_descriptor desc = {CLAP_VERSION,
                                          "org.surge-synth-team.scxt.clap-first.scxt-plugin",
                                          "Shortcircuit XT (Clap First Version)",
                                          "Surge Synth Team",
                                          "https://surge-synth-team.org",
                                          "",
                                          "",
                                          scxt::build::FullVersionStr,
                                          "The Flagship Creative Sampler from the Surge Synth Team",
                                          &features[0]};
    return &desc;
}

SCXTPlugin::SCXTPlugin(const clap_host *h) : plugHelper_t(getDescription(), h)
{
    engine = std::make_unique<scxt::engine::Engine>();

    clapJuceShim = std::make_unique<sst::clap_juce_shim::ClapJuceShim>(this);
    clapJuceShim->setResizable(true);
}

std::unique_ptr<juce::Component> SCXTPlugin::createEditor()
{
    auto ed = std::make_unique<scxt::ui::SCXTEditor>(
        *(engine->getMessageController()), *(engine->defaults), *(engine->getSampleManager()),
        *(engine->getBrowser()), engine->sharedUIMemoryState);
    ed->onZoomChanged = [this](auto f) {
        if (_host.canUseGui() && clapJuceShim->isEditorAttached())
        {
            _host.guiRequestResize(scxt::ui::SCXTEditor::edWidth * f,
                                   scxt::ui::SCXTEditor::edHeight * f);
        }
    };
    ed->setSize(scxt::ui::SCXTEditor::edWidth, scxt::ui::SCXTEditor::edHeight);
    return ed;
}
bool SCXTPlugin::stateSave(const clap_ostream *ostream) noexcept
{
    engine->getSampleManager()->purgeUnreferencedSamples();
    auto xml = scxt::json::streamEngineState(*engine);

    auto c = xml.c_str();
    auto s = xml.length(); // write the null terminator
    while (s > 0)
    {
        auto r = ostream->write(ostream, c, s);
        if (r < 0)
            return false;
        s -= r;
        c += r;
    }
    return true;
}
bool SCXTPlugin::stateLoad(const clap_istream *istream) noexcept
{
    static constexpr uint32_t initSize = 1 << 16, chunkSize = 1 << 8;
    std::vector<char> buffer;
    buffer.resize(initSize);

    int64_t rd{0};
    int64_t totalRd{0};
    auto bp = buffer.data();

    while ((rd = istream->read(istream, bp, chunkSize)) > 0)
    {
        bp += rd;
        totalRd += rd;
        if (totalRd >= buffer.size() - chunkSize - 1)
        {
            buffer.resize(buffer.size() * 2);
            bp = buffer.data() + totalRd;
        }
    }
    buffer[totalRd] = 0;

    auto xml = std::string(buffer.data());
    try
    {
        engine->getMessageController()->threadingChecker.bypassThreadChecks = true;
        std::lock_guard<std::mutex> g(engine->modifyStructureMutex);
        scxt::json::unstreamEngineState(*engine, xml);
    }
    catch (std::exception &e)
    {
        std::cerr << "Unstream exception " << e.what() << std::endl;
        return false;
    }
    engine->getMessageController()->threadingChecker.bypassThreadChecks = false;
    return true;
}

} // namespace scxt::clap_first::scxt_plugin