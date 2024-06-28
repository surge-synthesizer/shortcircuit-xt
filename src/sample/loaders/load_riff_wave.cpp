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

// #include "resampling.h"
#include "sample/sample.h"
// #include <windows.h>
// #include <mmreg.h>
#include "riff_memfile.h"
#include "riff_wave.h"
// #include "sampler_state.h"
#include <cassert>
#include <cstdint>

namespace scxt::sample
{
// TODO: What is this
size_t Sample::SaveWaveChunk(void *data)
{
#if 0
    // save the sample in a WAVE-compatible chunk
    // both used for persistence and .wav export
    size_t samplesize = channels * sample_length * (UseInt16 ? 2 : 4);
    size_t datasize = (8 + sizeof(wavheader)) + (8 + samplesize) +
                      (12 + scxt::Memfile::RIFFMemFile::RIFFTextChunkSize(name));

    if (!data)
        return datasize;

    scxt::Memfile::RIFFMemFile mf(data, datasize);

    wavheader header;
    header.wFormatTag = vt_write_int16LE(UseInt16 ? WAVE_FORMAT_PCM : WAVE_FORMAT_IEEE_FLOAT);
    header.wBitsPerSample = vt_write_int16LE(UseInt16 ? 16 : 32);
    header.nChannels = vt_write_int16LE(channels);
    header.nBlockAlign = vt_write_int16LE(channels * (UseInt16 ? 2 : 4));
    header.nSamplesPerSec = vt_write_int32LE(this->sample_rate);
    header.nAvgBytesPerSec = vt_write_int32LE(header.nBlockAlign * header.nSamplesPerSec);

    mf.RIFFCreateChunk('fmt ', &header, sizeof(wavheader));
    mf.RIFFCreateLISTHeader('INFO', scxt::Memfile::RIFFMemFile::RIFFTextChunkSize(name));
    mf.RIFFCreateTextChunk('INAM', name);
    mf.WriteDWORD(vt_write_int32BE('data'));
    mf.WriteDWORD(vt_write_int32LE(samplesize));

    if (UseInt16)
    {
        short *sp = (short *)mf.GetPtr();
        for (int c = 0; c < channels; c++)
        {
            short *rp = GetSamplePtrI16(c);
            for (int s = 0; s < sample_length; s++)
            {
                sp[s * channels + c] = vt_write_int16LE(rp[s]);
            }
        }
    }
    else
    {
        float *fp = (float *)mf.GetPtr();
        for (int c = 0; c < channels; c++)
        {
            float *rp = GetSamplePtrF32(c);
            for (int s = 0; s < sample_length; s++)
            {
                fp[s * channels + c] = vt_write_float32LE(rp[s]);
            }
        }
    }

    return datasize;
#endif
    return 0;
}

// TODO: What is this
bool Sample::save_wave_file(const fs::path &filename)
{
#if 0
    size_t datasize = SaveWaveChunk(0);
    void *data = malloc(datasize);
    SaveWaveChunk(data);
#if WINDOWS
    auto wide = filename.generic_wstring();
    FILE *f = _wfopen(wide.c_str(), L"wb");
#else
    FILE *f = fopen(path_to_string(filename).c_str(), "wb");
#endif
    int32_t d[3];
    d[0] = scxt::Memfile::swap_endian_32('RIFF');
    d[1] = datasize + 4;
    d[2] = scxt::Memfile::swap_endian_32('WAVE');
    fwrite(d, 4, 3, f);
    fwrite(data, datasize, 1, f);
    fclose(f);

    free(data);
#endif
    return true;
}

// TODO [prior] parse INAM etc etc metadata
bool Sample::parse_riff_wave(void *data, size_t filesize, bool skip_riffchunk)
{
    size_t datasize;
    scxt::sample::loaders::RIFFMemFile mf(data, filesize);
    if (!skip_riffchunk && !mf.riff_descend_RIFF_or_LIST('WAVE', &datasize))
    {
        SCLOG("Failed to load wave: not a WAVE LIST");
        return false;
    }

    loaders::wavheader wh;
    off_t wr = mf.TellI();
    if (!mf.riff_descend('fmt ', &datasize))
    {
        SCLOG("Failed to load wav: No 'fmt ' chunk found");
        return false;
    }

    // read header
    mf.Read(&wh, sizeof(loaders::wavheader));

    if (!wh.nSamplesPerSec)
    {
        SCLOG("Failed to load wav: header samples per second == 0");
        return false;
    }
    if (!wh.nChannels)
    {
        SCLOG("Failed to load wav: channels == 0");
        return false;
    }
    if (!wh.wBitsPerSample)
    {
        SCLOG("Failed to load wav: bits per sample == 0");
        return false;
    }

    mf.SeekI(wr);
    if (!mf.riff_descend('data', &datasize))
    {
        SCLOG("Failed to load wav: no 'data' chunk");
        return false;
    }

    int32_t WaveDataSize = datasize;
    if (!WaveDataSize)
    {
        SCLOG("Failed to load wav: datasize is zero");
        return false;
    }
    int32_t WaveDataSamples = 8 * WaveDataSize / (wh.wBitsPerSample * wh.nChannels);

    /*SCLOG("Loaded wav file " << SCD(wh.nChannels) << SCD(wh.nSamplesPerSec)
                             << SCD(wh.wBitsPerSample) << SCD(WaveDataSamples)); */

    /* get pointer to the sampledata */

    unsigned char *loaddata = (unsigned char *)mf.ReadPtr(WaveDataSize);
    if (!loaddata)
    {
        SCLOG("Failed to load wav: read of " << WaveDataSize << " returned a null");
        return false;
    }
    if (!SetMeta(wh.nChannels, wh.nSamplesPerSec, WaveDataSamples))
    {
        SCLOG("Failed to load wav: setMeta failed with "
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
            SCLOG("Failed to load: " << SCD(wh.wBitsPerSample)
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
            SCLOG("Failed to load wav: " << SCD(wh.wBitsPerSample)
                                         << " must be 32 or 64 for FLOAT wav");
            return false;
        }
    }
    else
    {
        SCLOG("Failed to load wav: Format Tag 0x"
              << std::hex << std::setw(4) << std::setfill('0') << wh.wFormatTag
              << " must be IEEE FLOAT (0x" << std::hex << std::setw(4) << std::setfill('0')
              << WAVE_FORMAT_IEEE_FLOAT << ") or PCM (0x" << std::hex << std::setw(4)
              << std::setfill('0') << WAVE_FORMAT_PCM << ")");
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
            meta.slice_end[cuepoints - 1] = sample_length;
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
                meta.slice_end[subchunks - 1] = sample_length;
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