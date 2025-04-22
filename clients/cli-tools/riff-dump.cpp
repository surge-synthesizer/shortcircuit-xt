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

#include <iostream>
#include <memory>
#include "sst/plugininfra/version_information.h"

#include "utils.h"

#include "sample/exs_support/exs_import.h"

#include "RIFF.h"

void dumpList(RIFF::List *list, const std::string &pfx = "+-- ")
{
    RIFF::Chunk *ck = list->GetFirstSubChunk();
    while (ck != NULL)
    {
        RIFF::Chunk *ckParent = ck;
        std::ostringstream oss;
        while (ckParent->GetParent() != NULL)
        {
            oss << "  "; // e.g. 'LIST(INFO)->'
            ckParent = ckParent->GetParent();
        }
        oss << ck->GetChunkIDString();
        switch (ck->GetChunkID())
        {
        case CHUNK_ID_LIST:
        case CHUNK_ID_RIFF:
        {
            RIFF::List *l = (RIFF::List *)ck;
            oss << "(" << l->GetListTypeString() << ")->";
            oss << " (" << l->GetSize() << " Bytes)";
            SCLOG(oss.str());
            dumpList(l);
            break;
        }
        default:
            oss << ";";
            oss << " (" << ck->GetSize() << " Bytes)";
            SCLOG(oss.str());
            break;
        }
        ck = list->GetNextSubChunk();
    }
};

int main(int argc, char **argv)
{
    SCLOG("RIFF-DUMP");
    SCLOG("    Version   = " << sst::plugininfra::VersionInformation::git_implied_display_version
                             << " / "
                             << sst::plugininfra::VersionInformation::project_version_and_hash);
    if (argc < 2)
    {
        SCLOG("Usage: " << argv[0] << " <exs-file>");
        return 1;
    }
    SCLOG("Opening " << argv[1]);

    try
    {
        auto rf = std::make_unique<RIFF::File>(argv[1]);

        SCLOG("RIFF : " << rf->GetChunkIDString() << " " << rf->GetListTypeString());
        dumpList(rf.get());
    }
    catch (const RIFF::Exception &e)
    {
        SCLOG("Exception: " << e.Message);
    }
    return 0;
}