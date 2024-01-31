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

#include "riff_memfile.h"
#include <cassert>
#include <cmath>

#include "sample/sample.h"

namespace scxt::sample
{
#define UnsignedToFloat(u) (((double)((long)(u - 2147483647L - 1))) + 2147483648.0)

double ConvertFromIeeeExtended(unsigned char *bytes /* LCN */)
{
    double f;
    int expon;
    unsigned long hiMant, loMant;

    expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
    hiMant = ((unsigned long)(bytes[2] & 0xFF) << 24) | ((unsigned long)(bytes[3] & 0xFF) << 16) |
             ((unsigned long)(bytes[4] & 0xFF) << 8) | ((unsigned long)(bytes[5] & 0xFF));
    loMant = ((unsigned long)(bytes[6] & 0xFF) << 24) | ((unsigned long)(bytes[7] & 0xFF) << 16) |
             ((unsigned long)(bytes[8] & 0xFF) << 8) | ((unsigned long)(bytes[9] & 0xFF));

    if (expon == 0 && hiMant == 0 && loMant == 0)
    {
        f = 0;
    }
    else
    {
        if (expon == 0x7FFF)
        { /* Infinity or NaN */
            f = std::numeric_limits<float>::infinity();
        }
        else
        {
            expon -= 16383;
            f = std::ldexp(UnsignedToFloat(hiMant), expon -= 31);
            f += std::ldexp(UnsignedToFloat(loMant), expon -= 32);
        }
    }

    if (bytes[0] & 0x80)
        return -f;
    else
        return f;
}

#pragma pack(push, 1)

struct aiff_CommonChunk
{
    uint16_t numChannels;
    uint32_t numSampleFrames;
    uint16_t sampleSize;
    unsigned char sampleRate[10];
};

struct aiff_loop
{
    uint16_t playmode;
    uint16_t marker_start, marker_end;
};

struct aiff_marker
{
    uint16_t id;
    uint32_t pos;
    // pstring comes here
};
struct aiff_INST_chunk
{
    char baseNote;
    char detune;
    char lowNote;
    char highNote;
    char lowvelocity;
    char highvelocity;
    short gain;
    aiff_loop sustainLoop;
    aiff_loop releaseLoop;
};

#pragma pack(pop)

bool Sample::parse_aiff(void *data, size_t filesize)
{
    int datasize;
    scxt::sample::loaders::RIFFMemFile mf(data, filesize);
    mf.useWaveEndian = false;
    if (!mf.iff_descend_FORM('AIFF', &datasize))
        return false;
    off_t wr = mf.TellI();

    if (!mf.iff_descend('COMM', &datasize))
        return false;
    aiff_CommonChunk cc;
    // read header
    mf.Read(&cc, sizeof(cc));
    channels = sst::basic_blocks::mechanics::swap_endian_16(cc.numChannels);
    if (channels > 2)
        return false;
    int nsamples = sst::basic_blocks::mechanics::swap_endian_32(cc.numSampleFrames);
    int bitdepth = sst::basic_blocks::mechanics::swap_endian_16(cc.sampleSize);

    if (!SetMeta(channels, ConvertFromIeeeExtended(cc.sampleRate), nsamples))
        return false;

    // load sample data
    mf.SeekI(wr);
    if (!mf.iff_descend('SSND', &datasize))
        return false;
    uint32_t offset = sst::basic_blocks::mechanics::swap_endian_32(mf.ReadDWORD());
    uint32_t blocksize = sst::basic_blocks::mechanics::swap_endian_32(mf.ReadDWORD());
    mf.SeekI(offset, scxt::sample::loaders::mf_FromCurrent); // skip comment

    unsigned char *loaddata = (unsigned char *)mf.ReadPtr(datasize - 8);

    if (bitdepth == 32)
    {
        if (channels == 2)
        {
            load_data_i32BE(0, loaddata, nsamples, 8);
            load_data_i32BE(1, loaddata + 4, nsamples, 8);
        }
        else
            load_data_i32BE(0, loaddata, nsamples, 4);
    }
    else if (bitdepth == 24)
    {
        if (channels == 2)
        {
            load_data_i24BE(0, loaddata, nsamples, 6);
            load_data_i24BE(1, loaddata + 3, nsamples, 6);
        }
        else
            load_data_i24BE(0, loaddata, nsamples, 3);
    }
    else if (bitdepth == 16)
    {
        if (channels == 2)
        {
            load_data_i16BE(0, loaddata, nsamples, 4);
            load_data_i16BE(1, loaddata + 2, nsamples, 4);
        }
        else
            load_data_i16BE(0, loaddata, nsamples, 2);
    }
    else if (bitdepth == 8)
    {
        if (channels == 2)
        {
            load_data_i8(0, loaddata, nsamples, 2);
            load_data_i8(1, loaddata + 1, nsamples, 2);
        }
        else
            load_data_i8(0, loaddata, nsamples, 1);
    }

    this->sample_loaded = (sampleData[0] != 0);
    if (!sample_loaded)
    {
        clear_data();
        return false;
    }

    // read MARK chunk
    mf.SeekI(wr);
    if (mf.iff_descend('MARK', &datasize))
    {
        uint16_t markercount = sst::basic_blocks::mechanics::swap_endian_16(mf.ReadWORD());

        meta.slice_start = new int[markercount];
        meta.slice_end = new int[markercount];
        meta.n_slices = markercount;

        for (int i = 0; i < markercount; i++)
        {
            aiff_marker marker;
            mf.Read(&marker, sizeof(marker));
            mf.SkipPSTRING();
            uint16_t id = sst::basic_blocks::mechanics::swap_endian_16(marker.id);
            if (id < markercount)
            {
                meta.slice_start[id] = sst::basic_blocks::mechanics::swap_endian_32(marker.pos);
                meta.slice_end[id] = sst::basic_blocks::mechanics::swap_endian_32(marker.pos);
            }
        }
    }

    // read INST chunk
    mf.SeekI(wr);
    if (mf.iff_descend('INST', &datasize))
    {
        aiff_INST_chunk INST;
        if (mf.Read(&INST, sizeof(INST)))
        {
            meta.key_root = INST.baseNote;
            meta.detune = INST.detune;
            meta.key_low = INST.lowNote;
            meta.key_high = INST.highNote;
            meta.vel_low = INST.lowvelocity;
            meta.vel_high = INST.highvelocity;
            meta.key_present = true;
            meta.rootkey_present = true;
            meta.vel_present = true;

            INST.sustainLoop.marker_start =
                sst::basic_blocks::mechanics::swap_endian_16(INST.sustainLoop.marker_start);
            INST.sustainLoop.marker_end =
                sst::basic_blocks::mechanics::swap_endian_16(INST.sustainLoop.marker_end);
            INST.sustainLoop.playmode =
                sst::basic_blocks::mechanics::swap_endian_16(INST.sustainLoop.playmode);

            if (INST.sustainLoop.playmode)
            {
                if (INST.sustainLoop.playmode == 2)
                    meta.playmode = pm_forward_loop_bidirectional;
                else
                    meta.playmode = pm_forward_loop;
                meta.loop_present = true;
                meta.playmode_present = true;
                if (INST.sustainLoop.marker_start < meta.n_slices)
                    meta.loop_start = meta.slice_start[INST.sustainLoop.marker_start];
                if (INST.sustainLoop.marker_end < meta.n_slices)
                    meta.loop_end =
                        meta.slice_start[INST.sustainLoop.marker_end] +
                        1; // SC wants the loop end point to be the first sample AFTER the loop
            }
        }
    }

    return true;
}
} // namespace scxt::sample