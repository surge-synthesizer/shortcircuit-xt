//
// Created by Paul Walker on 10/25/23.
//

#include "scxt-plugin.h"
#include "version.h"

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
                                          "The Flagship Sampler from the Surge Synth Team",
                                          &features[0]};
    return &desc;
}

} // namespace scxt::clap_first::scxt_plugin