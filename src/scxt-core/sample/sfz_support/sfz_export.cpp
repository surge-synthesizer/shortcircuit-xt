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

#include "sfz_export.h"
#include <fstream>
#include "engine/engine.h"
#include "configuration.h"
#include "utils.h"
#include "patch_io/patch_io.h"
#include "messaging/messaging.h"

namespace scxt::sfz_support
{
bool exportSFZ(const fs::path &toFile, engine::Engine &e, int partNumber)
{
    auto dir =
        toFile.parent_path() / (toFile.filename().replace_extension("").u8string() + " Samples");
    try
    {
        fs::create_directories(dir);
    }
    catch (const fs::filesystem_error &fse)
    {
        RAISE_ERROR_ENGINE(e, "Unable to create directory", dir.u8string() + "\n" + fse.what());
        return false;
    }
    auto collectMap = patch_io::collectSamplesInto(dir, e, partNumber);

    std::ostringstream oss;
    oss << "// SFZ exported from ShortCircuit XT\n";
    auto &p = e.getPatch()->getPart(partNumber);
    for (auto &g : *p)
    {
        oss << "\n<group>\n";
        for (auto &z : *g)
        {
            int va{0};
            for (int i = 0; i < maxVariantsPerZone; ++i)
            {
                if (z->variantData.variants[i].active)
                    va++;
            }
            if (va == 0)
            {
                RAISE_ERROR_ENGINE(e, "SFZ Export Error",
                                   "Zones with no samples are not yet supported in SFZ export");
            }
            if (va > 1)
            {
                RAISE_ERROR_ENGINE(e, "SFZ Export Error",
                                   "Zones multiple variants are not yet supported in SFZ export");
            }
            oss << "\n<region>\n";
            oss << "// zone name: " << z->getName() << "\n";
            oss << "lokey=" << z->mapping.keyboardRange.keyStart << " ";
            oss << "hikey=" << z->mapping.keyboardRange.keyEnd << "\n";
            oss << "pitch_keycenter=" << z->mapping.rootKey << "\n";
            oss << "lovel=" << z->mapping.velocityRange.velStart << " ";
            oss << "hivel=" << z->mapping.velocityRange.velEnd << "\n";

            auto &ts = dsp::twentyFiveSecondExpTable;
            auto &aegs = z->egStorage[0];
            auto fm = [](auto flt) -> std::string {
                if (flt < 1e-6)
                    return "0.0";
                return fmt::format("{:.3f}", flt);
            };
            oss << "amp_delay=" << fm(ts.timeInSecondsFromParam(aegs.dly)) << " ";
            oss << "amp_attack=" << fm(ts.timeInSecondsFromParam(aegs.a)) << " ";
            oss << "amp_hold=" << fm(ts.timeInSecondsFromParam(aegs.h)) << " ";
            oss << "amp_decay=" << fm(ts.timeInSecondsFromParam(aegs.d)) << " ";
            oss << "amp_sustain=" << aegs.s * 100 << " ";
            oss << "amp_release=" << fm(ts.timeInSecondsFromParam(aegs.r)) << "\n";

            auto cmf = collectMap.find(z->variantData.variants[0].sampleID);
            if (cmf == collectMap.end())
            {
                RAISE_ERROR_ENGINE(e, "SFZ Export Error",
                                   "Can't remap " +
                                       z->variantData.variants[0].sampleID.to_string());
            }
            auto pt = cmf->second;
            auto rp = pt.lexically_relative(dir.parent_path());
            oss << "sample=" << rp.u8string() << "\n";
            oss << "start=" << z->variantData.variants[0].startSample << " ";
            oss << "end=" << z->variantData.variants[0].endSample << "\n";

            if (z->variantData.variants[0].loopActive)
            {
                switch (z->variantData.variants[0].loopMode)
                {
                case engine::Zone::LoopMode::LOOP_WHILE_GATED:
                    oss << "loop_mode=loop_sustain ";
                    break;
                default:
                    oss << "loop_mode=loop_continuous ";
                    break;
                }
                oss << "loop_start=" << z->variantData.variants[0].startLoop << " ";
                oss << "loop_end=" << z->variantData.variants[0].endLoop << "\n";
            }
        }
    }

    auto ofs = std::ofstream(toFile);
    if (!ofs.is_open())
    {
        RAISE_ERROR_ENGINE(e, "Unable to open file for writing", toFile.u8string());
        return false;
    }
    ofs << oss.str();
    return true;
}
} // namespace scxt::sfz_support
