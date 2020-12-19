//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004-2006 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#include "globals.h"
#include "infrastructure/logfile.h"
#include "resampling.h"
#include "sample.h"
//#include <windows.h>
//#include <mmreg.h>
#include "memfile.h"
#include "riff_wave.h"
#include "sampler_state.h"
#include <assert.h>
#include <cstdint>

size_t sample::SaveWaveChunk(void *data)
{
    // save the sample in a WAVE-compatible chunk
    // both used for persistance and .wav export
    size_t samplesize = channels * sample_length * (UseInt16 ? 2 : 4);
    size_t datasize =
        (8 + sizeof(wavheader)) + (8 + samplesize) + (12 + memfile::RIFFTextChunkSize(name));

    if (!data)
        return datasize;

    memfile mf(data, datasize);

    wavheader header;
    header.wFormatTag = vt_write_int16LE(UseInt16 ? WAVE_FORMAT_PCM : WAVE_FORMAT_IEEE_FLOAT);
    header.wBitsPerSample = vt_write_int16LE(UseInt16 ? 16 : 32);
    header.nChannels = vt_write_int16LE(channels);
    header.nBlockAlign = vt_write_int16LE(channels * (UseInt16 ? 2 : 4));
    header.nSamplesPerSec = vt_write_int32LE(this->sample_rate);
    header.nAvgBytesPerSec = vt_write_int32LE(header.nBlockAlign * header.nSamplesPerSec);

    mf.RIFFCreateChunk('fmt ', &header, sizeof(wavheader));
    mf.RIFFCreateLISTHeader('INFO', memfile::RIFFTextChunkSize(name));
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
}

bool sample::save_wave_file(const char *filename)
{
    assert(filename);
    /*wchar_t wfilename[pathlength];
    int result = MultiByteToWideChar(CP_UTF8,0,filename,-1,wfilename,1024);
    if(!result) return false;*/
    // TODO change to use unicode

    size_t datasize = SaveWaveChunk(0);
    void *data = malloc(datasize);
    SaveWaveChunk(data);

    FILE *f = fopen(filename, "wb");
    int32_t d[3];
    d[0] = swap_endianDW('RIFF');
    d[1] = datasize + 4;
    d[2] = swap_endianDW('WAVE');
    fwrite(d, 4, 3, f);
    fwrite(data, datasize, 1, f);
    fclose(f);

    free(data);

    return true;
}

// TODO parse INAM etc etc metadata
bool sample::parse_riff_wave(void *data, size_t filesize, bool skip_riffchunk)
{
    size_t datasize;
    memfile mf(data, filesize);
    if (!skip_riffchunk && !mf.riff_descend_RIFF_or_LIST('WAVE', &datasize))
        return false;

    wavheader wh;
    off_t wr = mf.TellI();
    if (!mf.riff_descend('fmt ', &datasize))
        return false;

    // read header
    mf.Read(&wh, sizeof(wavheader));

    if (!wh.nSamplesPerSec)
        return false;
    if (!wh.nChannels)
        return false;
    if (!wh.wBitsPerSample)
        return false;

    mf.SeekI(wr);
    if (!mf.riff_descend('data', &datasize))
        return false;

    int32_t WaveDataSize = datasize;
    if (!WaveDataSize)
        return false;
    int32_t WaveDataSamples = 8 * WaveDataSize / (wh.wBitsPerSample * wh.nChannels);

    /* get pointer to the sampledata */

    unsigned char *loaddata = (unsigned char *)mf.ReadPtr(WaveDataSize);
    if (!loaddata)
        return false;

    if (!SetMeta(wh.nChannels, wh.nSamplesPerSec, WaveDataSamples))
        return false;

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
            return false;
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
            return false;
    }
    else
        return false;

    this->sample_loaded = true;

    // read smpl chunk
    mf.SeekI(wr, mf_FromStart);
    if (mf.riff_descend('smpl', &datasize))
    {
        SamplerChunk smpl_chunk;
        SampleLoop smpl_loop;

        mf.Read(&smpl_chunk, sizeof(SamplerChunk));
        meta.key_root = smpl_chunk.dwMIDIUnityNote & 0xFF;
        meta.rootkey_present = true;

        if (smpl_chunk.cSampleLoops > 0)
        {
            meta.loop_present = true;
            mf.Read(&smpl_loop, sizeof(SampleLoop));
            // smpl_loop.dwEnd++;	// SC wants the loop end point to be the first sample AFTER the
            // loop
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
        wave_inst_chunk INST;
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
        wave_acid_chunk ACID;
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
                vtCopyString(name, (char *)mf.RIFFReadChunk(), 64);
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
                CuePoint cp;
                mf.Read(&cp, sizeof(CuePoint));
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
            wave_strc_header header;
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

                mf.SeekI(sizeof(wave_strc_entry), mf_FromCurrent); // skip first entry

                for (int i = 0; i < subchunks; i++)
                {
                    wave_strc_entry entry;
                    mf.Read(&entry, sizeof(wave_strc_entry));
                    meta.slice_start[i] = entry.spos1;
                    if (i)
                        meta.slice_end[i - 1] = entry.spos1;
                }
            }
        }
    }

    return true;
}