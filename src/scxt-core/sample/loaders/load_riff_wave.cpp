/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

// #include "resampling.h"
#include "sample/sample.h"
// #include <windows.h>
// #include <mmreg.h>
#include "riff_memfile.h"
#include "riff_wave.h"
// #include "sampler_state.h"
#include <cassert>
#include <cstdint>

#define ADD_ERROR_MESSAGE(...)                                                                     \
    {                                                                                              \
        std::ostringstream oss;                                                                    \
        oss << __VA_ARGS__;                                                                        \
        addError(oss.str());                                                                       \
    }

namespace scxt::sample
{
// TODO [prior] parse INAM etc etc metadata
bool Sample::parse_riff_wave(void *data, size_t filesize, bool skip_riffchunk)
{
    size_t datasize;
    scxt::sample::loaders::RIFFMemFile mf(data, filesize);
    if (!skip_riffchunk && !mf.riff_descend_RIFF_or_LIST('WAVE', &datasize))
    {
        ADD_ERROR_MESSAGE("Failed to load wave: not a WAVE LIST");
        return false;
    }

    loaders::wavheader wh;
    off_t wr = mf.TellI();
    if (!mf.riff_descend('fmt ', &datasize))
    {
        ADD_ERROR_MESSAGE("Failed to load wav: No 'fmt ' chunk found");
        return false;
    }

    // read header
    mf.Read(&wh, sizeof(loaders::wavheader));

    if (!wh.nSamplesPerSec)
    {
        ADD_ERROR_MESSAGE("Failed to load wav: header samples per second == 0");
        return false;
    }
    if (!wh.nChannels)
    {
        ADD_ERROR_MESSAGE("Failed to load wav: channels == 0");
        return false;
    }
    if (!wh.wBitsPerSample)
    {
        ADD_ERROR_MESSAGE("Failed to load wav: bits per sample == 0");
        return false;
    }

    mf.SeekI(wr);
    if (!mf.riff_descend('data', &datasize))
    {
        ADD_ERROR_MESSAGE("Failed to load wav: no 'data' chunk");
        return false;
    }

    int32_t WaveDataSize = datasize;
    if (!WaveDataSize)
    {
        ADD_ERROR_MESSAGE("Failed to load wav: datasize is zero");
        return false;
    }
    int32_t WaveDataSamples = 8 * WaveDataSize / (wh.wBitsPerSample * wh.nChannels);

    /* get pointer to the sampledata */

    unsigned char *loaddata = (unsigned char *)mf.ReadPtr(WaveDataSize);
    if (!loaddata)
    {
        ADD_ERROR_MESSAGE("Failed to load wav: read of " << WaveDataSize << " returned a null");
        return false;
    }
    if (!SetMeta(wh.nChannels, wh.nSamplesPerSec, WaveDataSamples))
    {
        ADD_ERROR_MESSAGE("Failed to load wav: setMeta failed with "
                          << SCD(wh.nChannels) << SCD(wh.nSamplesPerSec) << SCD(WaveDataSamples));
        return false;
    }

    if (wh.wFormatTag == WAVE_FORMAT_PCM)
    {
        if (wh.wBitsPerSample == 8)
        {
            if (channels == 2)
            {
                load_data_ui8(0, loaddata, WaveDataSamples, 2);
                load_data_ui8(1, loaddata + 1, WaveDataSamples, 2);
            }
            else
                load_data_ui8(0, loaddata, WaveDataSamples, 1);
        }
        else if (wh.wBitsPerSample == 16)
        {
            if (channels == 2)
            {
                load_data_i16(0, loaddata, WaveDataSamples, 4);
                load_data_i16(1, loaddata + 2, WaveDataSamples, 4);
            }
            else
                load_data_i16(0, loaddata, WaveDataSamples, 2);
        }
        else if (wh.wBitsPerSample == 24)
        {
            if (channels == 2)
            {
                load_data_i24(0, loaddata, WaveDataSamples, 6);
                load_data_i24(1, loaddata + 3, WaveDataSamples, 6);
            }
            else
                load_data_i24(0, loaddata, WaveDataSamples, 3);
        }
        else if (wh.wBitsPerSample == 32)
        {
            if (channels == 2)
            {
                load_data_i32(0, loaddata, WaveDataSamples, 8);
                load_data_i32(1, loaddata + 4, WaveDataSamples, 8);
            }
            else
                load_data_i32(0, loaddata, WaveDataSamples, 4);
        }
        else
        {
            ADD_ERROR_MESSAGE("Failed to load: " << SCD(wh.wBitsPerSample)
                                                 << " must be 8, 16, 24 or 32 for PCM");
            return false;
        }
    }
    else if (wh.wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
    {
        if (wh.wBitsPerSample == 32)
        {
            if (channels == 2)
            {
                load_data_f32(0, loaddata, WaveDataSamples, 8);
                load_data_f32(1, loaddata + 4, WaveDataSamples, 8);
            }
            else
                load_data_f32(0, loaddata, WaveDataSamples, 4);
        }
        else if (wh.wBitsPerSample == 64)
        {
            if (channels == 2)
            {
                load_data_f64(0, loaddata, WaveDataSamples, 16);
                load_data_f64(1, loaddata + 8, WaveDataSamples, 16);
            }
            else
                load_data_f64(0, loaddata, WaveDataSamples, 8);
        }
        else
        {
            ADD_ERROR_MESSAGE("Failed to load wav: " << SCD(wh.wBitsPerSample)
                                                     << " must be 32 or 64 for FLOAT wav");
            return false;
        }
    }
    else
    {
        ADD_ERROR_MESSAGE("Failed to load wav: Format Tag 0x"
                          << std::hex << std::setw(4) << std::setfill('0') << wh.wFormatTag
                          << " must be IEEE FLOAT (0x" << std::hex << std::setw(4)
                          << std::setfill('0') << WAVE_FORMAT_IEEE_FLOAT << ") or PCM (0x"
                          << std::hex << std::setw(4) << std::setfill('0') << WAVE_FORMAT_PCM
                          << ")");
        return false;
    }
    this->sample_loaded = true;

    // read smpl chunk
    mf.SeekI(wr, scxt::sample::loaders::mf_FromStart);
    if (mf.riff_descend('smpl', &datasize))
    {
        loaders::SamplerChunk smpl_chunk;
        loaders::SampleLoop smpl_loop;

        mf.Read(&smpl_chunk, sizeof(loaders::SamplerChunk));
        meta.key_root = smpl_chunk.dwMIDIUnityNote & 0xFF;
        meta.rootkey_present = true;

        if (smpl_chunk.cSampleLoops > 0)
        {
            meta.loop_present = true;
            mf.Read(&smpl_loop, sizeof(loaders::SampleLoop));
            // smpl_loop.dwEnd++;	// SC wants the loop end point to be the first sample AFTER
            // the loop
            meta.loop_start = smpl_loop.dwStart;
            meta.loop_end = smpl_loop.dwEnd + 1;
            if (smpl_loop.dwType == 1)
                meta.playmode = pm_forward_loop_bidirectional;
            else
                meta.playmode = pm_forward_loop;
        }
    }

    // read inst chunk
    mf.SeekI(wr);
    if (mf.riff_descend('inst', &datasize))
    {
        loaders::wave_inst_chunk INST;
        mf.Read(&INST, sizeof(INST));
        meta.key_root = INST.key_root;
        meta.key_low = INST.key_low;
        meta.key_high = INST.key_high;
        meta.vel_low = INST.vel_low;
        meta.vel_high = INST.vel_high;
        meta.rootkey_present = true;
        meta.key_present = true;
        meta.vel_present = true;
        meta.detune = INST.detune_cents * 0.01f;
    }

    // read acid chunk
    mf.SeekI(wr);
    if (mf.riff_descend('acid', &datasize))
    {
        loaders::wave_acid_chunk ACID;
        mf.Read(&ACID, sizeof(ACID));
        if (!(ACID.type & 0x01))
            meta.n_beats = ACID.n_beats;
    }

    // read LIST:INFO chunk & subchunks
    mf.SeekI(wr);
    if (mf.RIFFDescendSearch('INFO'))
    {
        size_t chunksize;
        int tag;
        while (mf.RIFFPeekChunk(&tag, &chunksize))
        {
            switch (tag)
            {
            case 'INAM':
                strncpy(name, (char *)mf.RIFFReadChunk(), 64);
                break;
            default:
                mf.RIFFSkipChunk();
                break;
            };
        }
        mf.RIFFAscend(true);
    }

    // read cuepoints (as slices)
    mf.SeekI(wr);
    if (mf.riff_descend('cue ', &datasize))
    {
        int32_t cuepoints = mf.ReadDWORD();

        if (cuepoints > 1)
        {
            meta.slice_start = new int[cuepoints];
            meta.slice_end = new int[cuepoints];
            meta.slice_end[cuepoints - 1] = sampleLengthPerChannel;
            meta.n_slices = cuepoints;
            meta.playmode = pm_forward_hitpoints;
            meta.playmode_present = true;

            for (int i = 0; i < cuepoints; i++)
            {
                loaders::CuePoint cp;
                mf.Read(&cp, sizeof(loaders::CuePoint));
                meta.slice_start[i] = cp.dwSampleOffset;
                if (i)
                    meta.slice_end[i - 1] = cp.dwSampleOffset;
            }
        }
    }
    else
    {
        // read acidpoints (as slices)
        mf.SeekI(wr);
        if (mf.riff_descend('strc', &datasize))
        {
            loaders::wave_strc_header header;
            mf.Read(&header, sizeof(header));
            int32_t subchunks = header.subchunks - 1; // first entry seem different

            if (subchunks > 1)
            {
                meta.slice_start = new int[subchunks];
                meta.slice_end = new int[subchunks];
                meta.slice_end[subchunks - 1] = sampleLengthPerChannel;
                meta.n_slices = subchunks;
                meta.playmode = pm_forward_hitpoints;
                meta.playmode_present = true;

                mf.SeekI(sizeof(loaders::wave_strc_entry),
                         scxt::sample::loaders::mf_FromCurrent); // skip first entry

                for (int i = 0; i < subchunks; i++)
                {
                    loaders::wave_strc_entry entry;
                    mf.Read(&entry, sizeof(loaders::wave_strc_entry));
                    meta.slice_start[i] = entry.spos1;
                    if (i)
                        meta.slice_end[i - 1] = entry.spos1;
                }
            }
        }
    }

    return true;
}
} // namespace scxt::sample