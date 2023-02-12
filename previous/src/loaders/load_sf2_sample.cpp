/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#include <assert.h>
#if WINDOWS
#include <windows.h>
#include <mmreg.h>
#endif
#include "globals.h"
#include "infrastructure/logfile.h"
#include "riff_memfile.h"
#include "resampling.h"
#include "sample.h"
#include "sf2.h"

bool sample::parse_sf2_sample(void *data, size_t filesize, unsigned int sample_id)
{
    size_t datasize;
    scxt::Memfile::RIFFMemFile mf(data, filesize);
    if (!mf.riff_descend_RIFF_or_LIST('sfbk', &datasize))
        return false;

    off_t startpos = mf.TellI(); // store for later use
    if (!mf.riff_descend_RIFF_or_LIST('pdta', &datasize))
        return false;

    sf2_Sample sampledata;
    if (!mf.riff_descend('shdr', &datasize))
        return false;
    off_t shdrstartpos = mf.TellI(); // store for later use

    int sample_count = (datasize / 46) - 1;

    if (sample_id >= sample_count)
        return false;

    mf.SeekI(sample_id * 46, scxt::Memfile::mf_FromCurrent);
    mf.Read(&sampledata, 46);
    strncpy_0term(name, sampledata.achSampleName, 20);

    mf.SeekI(startpos);
    if (!mf.riff_descend_RIFF_or_LIST('sdta', &datasize))
        return false;

    off_t sdtastartpos = mf.TellI(); // store for later use
    mf.SeekI(sampledata.dwStart * 2 + 8, scxt::Memfile::mf_FromCurrent);
    int samplesize = sampledata.dwEnd - sampledata.dwStart;
    int16_t *loaddata = (int16_t *)mf.ReadPtr(samplesize * sizeof(int16_t));

    /* check for linked channel */
    sf2_Sample sampledataL;
    int16_t *loaddataL = 0;
    channels = 1;
    if ((sampledata.sfSampleType == rightSample) && (sampledata.wSampleLink < sample_count))
    {
        mf.SeekI(shdrstartpos + 46 * sampledata.wSampleLink);
        mf.Read(&sampledataL, 46);
        mf.SeekI(sdtastartpos + sampledataL.dwStart * 2 + 8);
        loaddataL = (int16_t *)mf.ReadPtr(samplesize * sizeof(int16_t));
        channels = 2;
    }

    if (channels == 2)
    {
        load_data_i16(0, loaddataL, samplesize, 2);
        load_data_i16(1, loaddata, samplesize, 2);
    }
    else
    {
        load_data_i16(0, loaddata, samplesize, 2);
    }

    this->sample_loaded = true;

    if (!SetMeta(channels, sampledata.dwSampleRate, samplesize))
        return false;

    meta.loop_present = true;
    meta.loop_start = sampledata.dwStartloop - sampledata.dwStart;
    meta.loop_end = sampledata.dwEndloop - sampledata.dwStart;
    return true;
}
