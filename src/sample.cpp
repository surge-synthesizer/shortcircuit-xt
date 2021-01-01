//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#include "sample.h"
#include "configuration.h"
#include "resampling.h"
#include <assert.h>
#include <vt_dsp/endian.h>
#if WINDOWS
#include <windows.h>
#include <mmreg.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "vt_util/vt_string.h"
#endif

#include "vt_util/vt_string.h"
#include "infrastructure/logfile.h"
#include "infrastructure/file_map_view.h"

sample::sample(configuration *conf)
{
    refcount = 1;
    this->conf = conf;

    // zero pointers
    SampleData[0] = 0;
    SampleData[1] = 0;
    meta.slice_start = 0;
    meta.slice_end = 0;
    graintable = 0;
    Embedded = true;
    UseInt16 = false;

    clear_data();
}

short *sample::GetSamplePtrI16(int Channel)
{
    if (!UseInt16)
        return 0;
    return &((short *)SampleData[Channel])[FIRoffset];
}
float *sample::GetSamplePtrF32(int Channel)
{
    if (UseInt16)
        return 0;
    return &((float *)SampleData[Channel])[FIRoffset];
}

bool sample::AllocateI16(int Channel, int Samples)
{
    // int samplesizewithmargin = Samples + 2*FIRipol_N + block_size + FIRoffset;
    int samplesizewithmargin = Samples + FIRipol_N;
    if (SampleData[Channel])
        delete SampleData[Channel];
    SampleData[Channel] = new short[samplesizewithmargin];
    if (!SampleData[Channel])
        return false;
    UseInt16 = true;

    // clear pre/post zero area
    memset(SampleData[Channel], 0, FIRoffset * sizeof(short));
    memset((char *)SampleData[Channel] + (Samples + FIRoffset) * sizeof(short), 0,
           FIRoffset * sizeof(short));

    return true;
}
bool sample::AllocateF32(int Channel, int Samples)
{
    int samplesizewithmargin = Samples + FIRipol_N;
    if (SampleData[Channel])
        delete SampleData[Channel];
    SampleData[Channel] = new float[samplesizewithmargin];
    if (!SampleData[Channel])
        return false;
    UseInt16 = false;

    // clear pre/post zero area
    memset(SampleData[Channel], 0, FIRoffset * sizeof(float));
    memset((char *)SampleData[Channel] + (Samples + FIRoffset) * sizeof(float), 0,
           FIRoffset * sizeof(float));

    return true;
}

int sample::GetRefCount() { return refcount; }
size_t sample::GetDataSize() { return sample_length * (UseInt16 ? 2 : 4) * channels; }
char *sample::GetName()
{
    return name;
    // TODO samplingar beh�ver numera namn!
    // TODO som �ven ska sparas i RIFFdata (anv�nd wav's metastruktur)
}

void sample::clear_data()
{
    sample_loaded = false;
    grains_initialized = false;

    // free any allocated data
    if (SampleData[0])
        delete SampleData[0];
    if (SampleData[1])
        delete SampleData[1];
    if (meta.slice_start)
        delete meta.slice_start;
    if (meta.slice_end)
        delete meta.slice_end;
    if (graintable)
        delete graintable;

    // and zero their pointers
    SampleData[0] = 0;
    SampleData[1] = 0;
    meta.slice_start = 0;
    meta.slice_end = 0;
    graintable = 0;

    memset(name, 0, 64);
    memset(&meta, 0, sizeof(meta));
    filename[0] = 0;
}

sample::~sample()
{
    clear_data(); // this should free everything
}

bool sample::load(const char *filename)
{
    assert(filename);
    wchar_t wfilename[pathlength];
    vtStringToWString(wfilename, filename, pathlength);
    bool r = load(wfilename);
    return r;
}

bool sample::SetMeta(unsigned int Channels, unsigned int SampleRate, unsigned int SampleLength)
{
    if (Channels > 2)
        return false; // not supported

    channels = Channels;
    sample_length = SampleLength;
    sample_rate = SampleRate;
    InvSampleRate = 1.f / (float)SampleRate;

    return true;
}

bool sample::load(const wchar_t *filename)
{
    assert(conf);
    assert(filename);
    wchar_t filename_decoded[pathlength], extension[64];
    int sample_id, program_id;
    conf->decode_pathW(filename, filename_decoded, extension, &program_id, &sample_id);

    auto mapper = std::make_unique<SC3::FileMapView>(filename_decoded);
    if (!mapper->isMapped())
    {
        SC3::Log::logos() << "Unable to map view of file '" << filename_decoded << "'";
        return false;
    }
    auto data = mapper->data();
    auto datasize = mapper->dataSize();

    clear_data(); // clear to a more predictable state

    const wchar_t *sname = wcsrchr(filename, L'\\');
    if (sname)
    {
        sname++;
        int length = wcsrchr(sname, '.') - sname;
#if WINDOWS
        if (length > 0)
            WideCharToMultiByte(CP_UTF8, 0, sname, length, name, 64, 0, 0);
#else
#warning Compiling un-ported WideChar code
#endif
    }

    bool r = false;
    if (wcsicmp(extension, L"wav") == 0)
    {
        r = parse_riff_wave(data, datasize);
    }
    else if (wcsicmp(extension, L"sf2") == 0)
    {
        r = parse_sf2_sample(data, datasize, sample_id);
    }
    else if ((wcsicmp(extension, L"dls") == 0) || (wcsicmp(extension, L"gig") == 0))
    {
        r = parse_dls_sample(data, datasize, sample_id);
    }
    else if ((wcsicmp(extension, L"aif") == 0) || (wcsicmp(extension, L"aiff") == 0))
    {
        r = parse_aiff(data, datasize);
    }

    if (r)
    {
        assert(SampleData[0]);
        if (channels == 2)
            assert(SampleData[1]);
    }

    if (r)
        wcsncpy(this->filename, filename, pathlength);

    if (!r)
    {
        wchar_t tmp[512];
        swprintf(tmp, 512, L"HERE Could not read file %s", filename);
#if WINDOWS
        MessageBoxW(::GetActiveWindow(), tmp, L"File I/O Error", MB_OK | MB_ICONERROR);
#else
#warning Implement user feedeback
        SC3::Log::logos() << "File IO error" << std::endl;
#endif
    }

    return r;
}

void sample::init_grains()
{
    return; // disabled atm

    // TODO inkompatible med short
    /*

    if (!grains_initialized)
    {
            const int max_grains = 65536;
            const int grainsize = 200;
            uint32 temp_grains[max_grains];

            num_grains = 0;
            uint32 samplepos = 1;
            while(num_grains<max_grains)
            {
                    if(samplepos > sample_length)
                            break;

                    // check if upwards zc has occured
                    if ((sample_data[samplepos+FIRoffset]>0) &&
    (sample_data[samplepos+FIRoffset-1]<0))
                    {
                            temp_grains[num_grains++] = samplepos;
                            samplepos += grainsize;
                    }
                    else
                    {
                            samplepos++;
                    }
            }
            num_grains -= 1;
            if (num_grains<1) return;
            int i;
            graintable = new uint32[num_grains];
            for(i=0; i<num_grains; i++)
            {
                    graintable[i] = temp_grains[i];
            }
            num_grains -= 1;	// number of total loops instead of sample locations
            grains_initialized = true;
    }*/
}

bool sample::get_filename(char *utf8name)
{
    vtStringToWString(filename, utf8name, 256 );
    return true;
}

bool sample::compare_filename(const char *utf8name)
{
    assert(filename);
    wchar_t wfilename[pathlength];
    vtStringToWString(wfilename, utf8name, 1024 );
    return (wcscmp(filename, wfilename) == 0);
}

bool sample::load_data_ui8(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    AllocateI16(channel, samplesize);
    short *sampledata = GetSamplePtrI16(channel);

    for (int i = 0; i < samplesize; i++)
    {
        sampledata[i] = (((short)*((unsigned char *)data + i * stride)) - 128) << 8;
    }
    return true;
}

bool sample::load_data_i8(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    AllocateI16(channel, samplesize);
    short *sampledata = GetSamplePtrI16(channel);

    for (int i = 0; i < samplesize; i++)
    {
        sampledata[i] = ((short)*((char *)data + i * stride)) << 8;
    }
    return true;
}

bool sample::load_data_i16(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    AllocateI16(channel, samplesize);
    short *sampledata = GetSamplePtrI16(channel);

    for (int i = 0; i < samplesize; i++)
    {
        sampledata[i] = vt_read_int16LE(*(short *)((char *)data + i * stride));
    }
    return true;
}

bool sample::load_data_i16BE(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    AllocateI16(channel, samplesize);
    short *sampledata = GetSamplePtrI16(channel);

    for (int i = 0; i < samplesize; i++)
    {
        sampledata[i] = vt_read_int16BE(*(short *)((char *)data + i * stride));
    }
    return true;
}
bool sample::load_data_i32(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    AllocateF32(channel, samplesize);
    float *sampledata = GetSamplePtrF32(channel);

    for (int i = 0; i < samplesize; i++)
    {
        int x = vt_read_int32LE(*(int *)((char *)data + i * stride));
        sampledata[i] = (4.6566128730772E-10f) * (float)x;
    }
    return true;
}

bool sample::load_data_i32BE(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    AllocateF32(channel, samplesize);
    float *sampledata = GetSamplePtrF32(channel);

    for (int i = 0; i < samplesize; i++)
    {
        int x = vt_read_int32BE(*(int *)((char *)data + i * stride));
        sampledata[i] = (4.6566128730772E-10f) * (float)x;
    }
    return true;
}

bool sample::load_data_i24(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    AllocateF32(channel, samplesize);
    float *sampledata = GetSamplePtrF32(channel);

    for (int i = 0; i < samplesize; i++)
    {
        unsigned char *cval = (unsigned char *)data + i * stride;
        int value = (cval[2] << 16) | (cval[1] << 8) | cval[0];
        value -= (value & 0x800000) << 1;
        sampledata[i] = 0.00000011920928955078f * float(value);
        ;
    }
    return true;
}

bool sample::load_data_i24BE(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    AllocateF32(channel, samplesize);
    float *sampledata = GetSamplePtrF32(channel);

    for (int i = 0; i < samplesize; i++)
    {
        unsigned char *cval = (unsigned char *)data + i * stride;
        int value = (cval[0] << 16) | (cval[1] << 8) | cval[2];
        value -= (value & 0x800000) << 1;
        sampledata[i] = 0.00000011920928955078f * float(value);
        ;
    }
    return true;
}

bool sample::load_data_f32(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    AllocateF32(channel, samplesize);
    float *sampledata = GetSamplePtrF32(channel);

    for (int i = 0; i < samplesize; i++)
    {
        sampledata[i] = (*(float *)((char *)data + i * stride));
    }
    return true;
}

bool sample::load_data_f64(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    AllocateF32(channel, samplesize);
    float *sampledata = GetSamplePtrF32(channel);

    for (int i = 0; i < samplesize; i++)
    {
        sampledata[i] = (float)(*(double *)((char *)data + i * stride));
    }
    return true;
}
