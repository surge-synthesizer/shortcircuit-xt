//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------
#include "sf2_import.h"
#include "globals.h"
#include "infrastructure/logfile.h"
#include "sampler.h"
#include "sf2.h"
#include "stdio.h"
#include "synthesis/modmatrix.h"
#include "synthesis/steplfo.h"
#include "unitconversion.h"
#include <cstring>
#include <vt_util/vt_string.h>

using std::max;
using std::min;

int get_sf2_patchlist(const wchar_t *filename, void **plist)
{
    HMMIO hmmio;

    /* Open the file for reading with buffered I/O. Let windows use its default internal buffer */
    hmmio = mmioOpenW((LPWSTR)filename, NULL, MMIO_READ | MMIO_ALLOCBUF);
    if (!hmmio)
    {
        char msg[256];
        sprintf(msg, "file io error: File [%ls] not found!", filename);
        write_log(msg);
        mmioClose(hmmio, 0);
        return false;
    }

    MMCKINFO mmckinfoParent; /* for the Group Header */

    /* Tell Windows to locate a sfbk Group header somewhere in the file, and read it in.
    This marks the start of any embedded sfbk format within the file */
    mmckinfoParent.fccType = mmioFOURCC('s', 'f', 'b', 'k');
    if (mmioDescend(hmmio, (LPMMCKINFO)&mmckinfoParent, 0, MMIO_FINDRIFF))
    {
        write_log("file io (sf2): This file doesn't contain a 'sfbk'!");
        mmioClose(hmmio, 0);
        return false;
    }

    MMCKINFO mmckinfoListchunk;
    MMCKINFO mmckinfoSubchunk;

    long startpos = mmioSeek(hmmio, 0, SEEK_CUR); // store for later use

    /*
    Uninteresting!
    mmckinfoListchunk.fccType = mmioFOURCC('I', 'N', 'F', 'O');
    if (mmioDescend(hmmio, &mmckinfoListchunk, &mmckinfoParent, MMIO_FINDLIST))
    {
            write_log("file io (sf2):	Required INFO LIST chunk was not found!");
            mmioClose(hmmio, 0);
            return false;
    }
    */

    mmckinfoListchunk.fccType = mmioFOURCC('p', 'd', 't', 'a');
    if (mmioDescend(hmmio, &mmckinfoListchunk, &mmckinfoParent, MMIO_FINDLIST))
    {
        /* Oops! The required fmt chunk was not found! */
        write_log("file io (sf2): Required pdta LIST chunk was not found!");
        mmioClose(hmmio, 0);
        return false;
    }

    // ok, we're in the presets chunk, put all the data in bufffers

    int i;
    int n_ph, n_pb, n_pg, n_ih, n_ib, n_ig, n_shdr;
    sf2_PresetHeader *preset_header = 0;
    sf2_PresetBag *preset_bag = 0;
    sf2_PresetGenList *preset_gen = 0;
    sf2_InstHeader *inst_header = 0;
    sf2_InstBag *inst_bag = 0;
    sf2_InstGenList *inst_gen = 0;
    sf2_Sample *shdr = 0;

    // preset header
    mmckinfoSubchunk.ckid = mmioFOURCC('p', 'h', 'd', 'r');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoListchunk, MMIO_FINDCHUNK))
    {
        write_log("file io (sf2): Required phdr chunk was not found!");
        return false; // was goto bailout;
    }
    n_ph = (mmckinfoSubchunk.cksize / 38);
    preset_header = new sf2_PresetHeader[n_ph];
    for (i = 0; i < n_ph; i++)
    {
        mmioRead(hmmio, (HPSTR)&preset_header[i], 38);
    }
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    // preset bag
    mmckinfoSubchunk.ckid = mmioFOURCC('p', 'b', 'a', 'g');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoListchunk, MMIO_FINDCHUNK))
    {
        write_log("file io (sf2): Required pbag chunk was not found!");
        return false; // was goto bailout;
    }
    n_pb = (mmckinfoSubchunk.cksize / 4);
    preset_bag = new sf2_PresetBag[n_pb];
    for (i = 0; i < n_pb; i++)
    {
        mmioRead(hmmio, (HPSTR)&preset_bag[i], 4);
    }
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    // preset generators
    mmckinfoSubchunk.ckid = mmioFOURCC('p', 'g', 'e', 'n');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoListchunk, MMIO_FINDCHUNK))
    {
        write_log("file io (sf2): Required pgen chunk was not found!");
        return false; // was goto bailout;
    }
    n_pg = (mmckinfoSubchunk.cksize / 4);
    preset_gen = new sf2_PresetGenList[n_pg];
    for (i = 0; i < n_pg; i++)
    {
        mmioRead(hmmio, (HPSTR)&preset_gen[i], 4);
    }
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    // instrument header
    mmckinfoSubchunk.ckid = mmioFOURCC('i', 'n', 's', 't');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoListchunk, MMIO_FINDCHUNK))
    {
        write_log("file io (sf2): Required inst chunk was not found!");
        return false; // was goto bailout;
    }
    n_ih = (mmckinfoSubchunk.cksize / 22);
    inst_header = new sf2_InstHeader[n_ih];
    for (i = 0; i < n_ih; i++)
    {
        mmioRead(hmmio, (HPSTR)&inst_header[i], 22);
    }
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    // instrument bag
    mmckinfoSubchunk.ckid = mmioFOURCC('i', 'b', 'a', 'g');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoListchunk, MMIO_FINDCHUNK))
    {
        write_log("file io (sf2): Required ibag chunk was not found!");
        return false; // was goto bailout;
    }
    n_ib = (mmckinfoSubchunk.cksize / 4);
    inst_bag = new sf2_InstBag[n_ib];
    for (i = 0; i < n_ib; i++)
    {
        mmioRead(hmmio, (HPSTR)&inst_bag[i], 4);
    }
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    // instrument generators
    mmckinfoSubchunk.ckid = mmioFOURCC('i', 'g', 'e', 'n');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoListchunk, MMIO_FINDCHUNK))
    {
        write_log("file io (sf2): Required igen chunk was not found!");
        return false; // was goto bailout;
    }
    n_ig = (mmckinfoSubchunk.cksize / 4);
    inst_gen = new sf2_InstGenList[n_ig];
    for (i = 0; i < n_ig; i++)
    {
        mmioRead(hmmio, (HPSTR)&inst_gen[i], 4);
    }
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    // samples
    mmckinfoSubchunk.ckid = mmioFOURCC('s', 'h', 'd', 'r');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoListchunk, MMIO_FINDCHUNK))
    {
        write_log("file io (sf2): Required shdr chunk was not found!");
        return false; // was goto bailout;
    }
    n_shdr = (mmckinfoSubchunk.cksize / 46);
    shdr = new sf2_Sample[n_shdr];
    for (i = 0; i < n_shdr; i++)
    {
        mmioRead(hmmio, (HPSTR)&shdr[i], 46);
    }
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    // OKOK, now the reading should be complete..
    mmioClose(hmmio, 0);

    // first, select a preset.. that will be converted to the "group" in the architecture
    int pre_id = 0;

    if (n_ph < 1)
        return 0;

    midipatch *mp = new midipatch[n_ph - 1];
    for (i = 0; i < (n_ph - 1); i++)
    {

        mp[i].bank = preset_header[i].wBank;
        mp[i].PC = preset_header[i].wPreset;
        vtCopyString(mp[i].name, preset_header[i].achPresetName, 32);
    }
    *plist = mp;
    mmioClose(hmmio, 0);
    delete preset_header;
    delete preset_bag;
    delete preset_gen;
    delete inst_header;
    delete inst_bag;
    delete inst_gen;
    delete shdr;
    return n_ph - 1;

bailout:
    mmioClose(hmmio, 0);
    delete preset_header;
    delete preset_bag;
    delete preset_gen;
    delete inst_header;
    delete inst_bag;
    delete inst_gen;
    delete shdr;
    return 0;
}

bool sampler::load_sf2_preset(const char *filename, int *new_group, char channel, int patch)
{
    HMMIO hmmio;

    /* Open the file for reading with buffered I/O. Let windows use its default internal buffer */
    hmmio = mmioOpen((LPSTR)filename, NULL, MMIO_READ | MMIO_ALLOCBUF);
    if (!hmmio)
    {
        char msg[256];
        sprintf(msg, "file io error: File [%s] not found!", filename);
        write_log(msg);
        mmioClose(hmmio, 0);
        return false;
    }

    MMCKINFO mmckinfoParent; /* for the Group Header */

    /* Tell Windows to locate a sfbk Group header somewhere in the file, and read it in.
    This marks the start of any embedded sfbk format within the file */
    mmckinfoParent.fccType = mmioFOURCC('s', 'f', 'b', 'k');
    if (mmioDescend(hmmio, (LPMMCKINFO)&mmckinfoParent, 0, MMIO_FINDRIFF))
    {
        write_log("file io (sf2): This file doesn't contain a 'sfbk'!");
        mmioClose(hmmio, 0);
        return false;
    }

    MMCKINFO mmckinfoListchunk;
    MMCKINFO mmckinfoSubchunk;

    long startpos = mmioSeek(hmmio, 0, SEEK_CUR); // store for later use

    /*
    Uninteresting!
    mmckinfoListchunk.fccType = mmioFOURCC('I', 'N', 'F', 'O');
    if (mmioDescend(hmmio, &mmckinfoListchunk, &mmckinfoParent, MMIO_FINDLIST))
    {
            write_log("file io (sf2):	Required INFO LIST chunk was not found!");
            mmioClose(hmmio, 0);
            return false;
    }
    */

    mmckinfoListchunk.fccType = mmioFOURCC('p', 'd', 't', 'a');
    if (mmioDescend(hmmio, &mmckinfoListchunk, &mmckinfoParent, MMIO_FINDLIST))
    {
        /* Oops! The required fmt chunk was not found! */
        write_log("file io (sf2): Required pdta LIST chunk was not found!");
        mmioClose(hmmio, 0);
        return false;
    }

    // ok, we're in the presets chunk, put all the data in bufffers

    int i;
    int n_ph, n_pb, n_pg, n_ih, n_ib, n_ig, n_shdr;
    sf2_PresetHeader *preset_header = 0;
    sf2_PresetBag *preset_bag = 0;
    sf2_PresetGenList *preset_gen = 0;
    sf2_InstHeader *inst_header = 0;
    sf2_InstBag *inst_bag = 0;
    sf2_InstGenList *inst_gen = 0;
    sf2_Sample *shdr = 0;

    // preset header
    mmckinfoSubchunk.ckid = mmioFOURCC('p', 'h', 'd', 'r');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoListchunk, MMIO_FINDCHUNK))
    {
        write_log("file io (sf2): Required phdr chunk was not found!");
        return false; // was goto bailout;
    }
    n_ph = (mmckinfoSubchunk.cksize / 38);
    preset_header = new sf2_PresetHeader[n_ph];
    for (i = 0; i < n_ph; i++)
    {
        mmioRead(hmmio, (HPSTR)&preset_header[i], 38);
    }
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    // preset bag
    mmckinfoSubchunk.ckid = mmioFOURCC('p', 'b', 'a', 'g');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoListchunk, MMIO_FINDCHUNK))
    {
        write_log("file io (sf2): Required pbag chunk was not found!");
        return false; // was goto bailout;
    }
    n_pb = (mmckinfoSubchunk.cksize / 4);
    preset_bag = new sf2_PresetBag[n_pb];
    for (i = 0; i < n_pb; i++)
    {
        mmioRead(hmmio, (HPSTR)&preset_bag[i], 4);
    }
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    // preset generators
    mmckinfoSubchunk.ckid = mmioFOURCC('p', 'g', 'e', 'n');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoListchunk, MMIO_FINDCHUNK))
    {
        write_log("file io (sf2): Required pgen chunk was not found!");
        return false; // was goto bailout;
    }
    n_pg = (mmckinfoSubchunk.cksize / 4);
    preset_gen = new sf2_PresetGenList[n_pg];
    for (i = 0; i < n_pg; i++)
    {
        mmioRead(hmmio, (HPSTR)&preset_gen[i], 4);
    }
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    // instrument header
    mmckinfoSubchunk.ckid = mmioFOURCC('i', 'n', 's', 't');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoListchunk, MMIO_FINDCHUNK))
    {
        write_log("file io (sf2): Required inst chunk was not found!");
        return false; // was goto bailout;
    }
    n_ih = (mmckinfoSubchunk.cksize / 22);
    inst_header = new sf2_InstHeader[n_ih];
    for (i = 0; i < n_ih; i++)
    {
        mmioRead(hmmio, (HPSTR)&inst_header[i], 22);
    }
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    // instrument bag
    mmckinfoSubchunk.ckid = mmioFOURCC('i', 'b', 'a', 'g');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoListchunk, MMIO_FINDCHUNK))
    {
        write_log("file io (sf2): Required ibag chunk was not found!");
        return false; // was goto bailout;
    }
    n_ib = (mmckinfoSubchunk.cksize / 4);
    inst_bag = new sf2_InstBag[n_ib];
    for (i = 0; i < n_ib; i++)
    {
        mmioRead(hmmio, (HPSTR)&inst_bag[i], 4);
    }
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    // instrument generators
    mmckinfoSubchunk.ckid = mmioFOURCC('i', 'g', 'e', 'n');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoListchunk, MMIO_FINDCHUNK))
    {
        write_log("file io (sf2): Required igen chunk was not found!");
        return false; // was goto bailout;
    }
    n_ig = (mmckinfoSubchunk.cksize / 4);
    inst_gen = new sf2_InstGenList[n_ig];
    for (i = 0; i < n_ig; i++)
    {
        mmioRead(hmmio, (HPSTR)&inst_gen[i], 4);
    }
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    // samples
    mmckinfoSubchunk.ckid = mmioFOURCC('s', 'h', 'd', 'r');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoListchunk, MMIO_FINDCHUNK))
    {
        write_log("file io (sf2): Required shdr chunk was not found!");
        return false; // was goto bailout;
    }
    n_shdr = (mmckinfoSubchunk.cksize / 46);
    shdr = new sf2_Sample[n_shdr];
    for (i = 0; i < n_shdr; i++)
    {
        mmioRead(hmmio, (HPSTR)&shdr[i], 46);
    }
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    // OKOK, now the reading should be complete..
    mmioClose(hmmio, 0);

    // first, select a preset.. that will be converted to the "group" in the architecture
    int pre_id = 0;
    if ((patch > -1) && (patch < (n_ph - 1)))
    {
        pre_id = patch;
    }
    else
    {
        /*
#ifndef SCPB
        if(n_ph>2)
        {
                midipatch *mp = new midipatch[n_ph-1];
                for(i=0; i<(n_ph-1); i++)
                {
                        mp[i].bank = preset_header[i].wBank;
                        mp[i].PC = preset_header[i].wPreset;
                        vtCopyString(mp[i].name,preset_header[i].achPresetName,20);
                }
                pre_id =
spawn_patch_dialog((HWND)((AEffGUIEditor*)editor)->getFrame()->getSystemWindow(), mp, n_ph-1);
                delete mp;
        }
#endif*/
    }
    if (pre_id < 0)
        return false; // was goto bailout;

    // work in progress v2
    // add_group(preset_header[pre_id].achPresetName,&new_g,channel);
    // if (new_group) *new_group = new_g;

    part_init(channel, true, true);
    vtCopyString(parts[channel].name, preset_header[pre_id].achPresetName, 32);

    int pb, pb_first = preset_header[pre_id].wPresetBagNdx,
            pb_end = preset_header[pre_id + 1].wPresetBagNdx;

    // global zone settings
    genAmountType p_globalzone_generators[sf2_numgenerators];
    memset(p_globalzone_generators, 0, sizeof(p_globalzone_generators));
    bool p_globalzone_generators_set[sf2_numgenerators];
    memset(p_globalzone_generators_set, 0, sizeof(bool[sf2_numgenerators]));

    for (pb = pb_first; pb < min(pb_end, n_pb); pb++)
    {
        // accumulate generators at the patch level
        int pg, pg_first = preset_bag[pb].wGenNdx, pg_end = preset_bag[pb + 1].wGenNdx;

        bool is_global_zone =
            ((pb == pb_first) && (preset_gen[min(pg_end, n_pg) - 1].sfGenOper != instrument));

        if (is_global_zone)
        {
            for (pg = pg_first; pg < min(pg_end, n_pg); pg++)
            {
                p_globalzone_generators[preset_gen[pg].sfGenOper] = preset_gen[pg].genAmount;
                p_globalzone_generators_set[preset_gen[pg].sfGenOper] = true;
            }
        }
        else
        {
            genAmountType p_generators[sf2_numgenerators];
            memcpy(p_generators, p_globalzone_generators, sizeof(p_generators));
            bool p_generators_set[sf2_numgenerators];
            memcpy(p_generators_set, p_globalzone_generators_set, sizeof(bool[sf2_numgenerators]));

            for (pg = pg_first; pg < min(pg_end, n_pg); pg++)
            {
                p_generators[preset_gen[pg].sfGenOper] = preset_gen[pg].genAmount;
                p_generators_set[preset_gen[pg].sfGenOper] = true;
            }

            // traverse trough the instrumentbags
            int inst_id = p_generators[instrument].wAmount;
            if (!p_generators_set[instrument])
                break;
            int ib, ib_first = inst_header[inst_id].wInstBagNdx,
                    ib_end = inst_header[inst_id + 1].wInstBagNdx;

            // global zone settings
            genAmountType i_globalzone_generators[sf2_numgenerators];
            memset(i_globalzone_generators, 0, sizeof(i_globalzone_generators));
            bool i_globalzone_generators_set[sf2_numgenerators];
            memset(i_globalzone_generators_set, 0, sizeof(bool[sf2_numgenerators]));

            // set default values
            i_globalzone_generators[initialFilterFc].shAmount = 13500;
            i_globalzone_generators[delayModLFO].shAmount = -12000;
            i_globalzone_generators[delayVibLFO].shAmount = -12000;
            i_globalzone_generators[delayModEnv].shAmount = -12000;
            i_globalzone_generators[attackModEnv].shAmount = -12000;
            i_globalzone_generators[holdModEnv].shAmount = -12000;
            i_globalzone_generators[decayModEnv].shAmount = -12000;
            i_globalzone_generators[releaseModEnv].shAmount = -12000;
            i_globalzone_generators[delayVolEnv].shAmount = -12000;
            i_globalzone_generators[attackVolEnv].shAmount = -12000;
            i_globalzone_generators[holdVolEnv].shAmount = -12000;
            i_globalzone_generators[decayVolEnv].shAmount = -12000;
            i_globalzone_generators[releaseVolEnv].shAmount = -12000;
            i_globalzone_generators[scaleTuning].shAmount = 100;
            i_globalzone_generators[overridingRootKey].shAmount = -1;
            i_globalzone_generators[keynum].shAmount = -1;
            i_globalzone_generators[velocity].shAmount = -1;
            i_globalzone_generators[keyRange].ranges.byLo = 0;
            i_globalzone_generators[keyRange].ranges.byHi = 127;
            i_globalzone_generators[velRange].ranges.byLo = 0;
            i_globalzone_generators[velRange].ranges.byHi = 127;

            for (ib = ib_first; ib < min(ib_end, n_ib); ib++)
            {
                int ig, ig_first = inst_bag[ib].wInstGenNdx, ig_end = inst_bag[ib + 1].wInstGenNdx;

                bool is_inst_global_zone =
                    ((ib == ib_first) && (inst_gen[min(ig_end, n_ig) - 1].sfGenOper != sampleID));

                if (is_inst_global_zone)
                {
                    for (ig = ig_first; ig < min(ig_end, n_ig); ig++)
                    {
                        i_globalzone_generators[inst_gen[ig].sfGenOper] = inst_gen[ig].genAmount;
                        i_globalzone_generators_set[inst_gen[ig].sfGenOper] = true;
                    }
                }
                else
                {
                    // accumulate generators at the instrument level
                    genAmountType i_generators[sf2_numgenerators];
                    memcpy(i_generators, i_globalzone_generators, sizeof(i_generators));
                    bool i_generators_set[sf2_numgenerators];
                    memcpy(i_generators_set, i_globalzone_generators_set,
                           sizeof(bool[sf2_numgenerators]));

                    for (ig = ig_first; ig < min(ig_end, n_ig); ig++)
                    {
                        int sfgo = inst_gen[ig].sfGenOper;
                        if (sfgo >= sf2_numgenerators)
                            break;
                        i_generators[sfgo] = inst_gen[ig].genAmount;
                        i_generators_set[sfgo] = true;
                    }

                    // add preset level offsets
                    int p;
                    for (p = 0; p < sf2_numgenerators; p++)
                    {
                        switch (p)
                        {
                        // these whouldn't exist at the patch level, ignore
                        case startAddrsOffset:
                        case endAddrsOffset:
                        case startloopAddrsOffset:
                        case endloopAddrsOffset:
                        case startAddrsCoarseOffset:
                        case endAddrsCoarseOffset:
                        case startloopAddrsCoarseOffset:
                        case keynum:
                        case velocity:
                        case endloopAddrsCoarseOffset:
                        case sampleModes:
                        case exclusiveClass:
                        case overridingRootKey:
                            break;
                        // and these need special treatment
                        case keyRange:
                        case velRange:
                            if (p_generators_set[p])
                            {
                                i_generators[p].ranges.byLo =
                                    max(i_generators[p].ranges.byLo, p_generators[p].ranges.byLo);
                                i_generators[p].ranges.byHi =
                                    min(i_generators[p].ranges.byHi, p_generators[p].ranges.byHi);
                                i_generators_set[p] = true;
                            }
                            break;
                        default:
                            if (p_generators_set[p])
                            {
                                i_generators[p].shAmount += p_generators[p].shAmount;
                                i_generators_set[p] = true;
                            }
                            break;
                        };
                    }

                    char fn[256];
                    int newzone;
                    int sample_id = i_generators[sampleID].wAmount;

                    sprintf(fn, "%s|%i", filename, sample_id);
                    if (i_generators_set[sampleID] &&
                        ((shdr[sample_id].sfSampleType == monoSample) ||
                         (shdr[sample_id].sfSampleType == rightSample)) &&
                        add_zone(fn, &newzone, channel))
                    {
                        // sample zone loaded ok..
                        // set all the proper parameters

                        sample_zone *z = &zones[newzone];

                        z->mute = false;
                        // z->layer = (pb-pb_first) & 7;		// layer by preset-bag test (no
                        // good)
                        /*char *comma = strrchr(fn,'|');
                        if (comma) strcpy(z->name, comma+1);*/
                        vtCopyString(z->name, shdr[sample_id].achSampleName, 32);

                        // get root key from sample
                        z->key_root = shdr[sample_id].byOriginalKey;
                        if (z->key_root > 127)
                            z->key_root = 60;

                        z->pitchcorrection = 0.01f * shdr[sample_id].chCorrection;
                        z->keytrack = 0.01f * i_generators[scaleTuning].shAmount;

                        // set instrument level generators

                        if (i_generators_set[overridingRootKey] &&
                            (i_generators[overridingRootKey].shAmount >= 0) &&
                            (i_generators[overridingRootKey].shAmount < 128))
                            z->key_root = (char)i_generators[overridingRootKey].shAmount;

                        if (i_generators[sampleModes].wAmount & 1)
                            z->playmode = pm_forward_loop;
                        else
                            z->playmode = pm_forward;

                        z->key_high = i_generators[keyRange].ranges.byHi;
                        z->key_low = i_generators[keyRange].ranges.byLo;

                        z->velocity_high = i_generators[velRange].ranges.byHi;
                        z->velocity_low = i_generators[velRange].ranges.byLo;

                        z->transpose = i_generators[coarseTune].shAmount;
                        z->finetune = 0.01f * i_generators[fineTune].shAmount;

                        z->aux[0].level =
                            min(0.f, 0.1f * i_generators[initialAttenuation].shAmount);

                        const float egmult = 1.f / 2.7778f;

                        z->AEG.attack = timecent_to_envtime(i_generators[attackVolEnv].shAmount);
                        z->AEG.hold = timecent_to_envtime(i_generators[holdVolEnv].shAmount);
                        z->AEG.decay =
                            timecent_to_envtime(i_generators[decayVolEnv].shAmount) * egmult;
                        z->AEG.release =
                            timecent_to_envtime(i_generators[releaseVolEnv].shAmount) * egmult;
                        z->AEG.sustain = dB_to_scamp(-0.1f * i_generators[sustainVolEnv].shAmount);
                        z->AEG.shape[0] = 0.f; // sf2 attack is linear
                        z->AEG.shape[1] = 3.f; // but decay and release is logarithmic
                        z->AEG.shape[2] = 3.f;

                        z->EG2.attack = timecent_to_envtime(i_generators[attackModEnv].shAmount);
                        z->EG2.hold = timecent_to_envtime(i_generators[holdModEnv].shAmount);
                        z->EG2.decay = timecent_to_envtime(i_generators[decayModEnv].shAmount);
                        z->EG2.release = timecent_to_envtime(i_generators[releaseModEnv].shAmount);
                        z->EG2.sustain = 0.001f * i_generators[sustainModEnv].shAmount;

                        // LFO's
                        load_lfo_preset(lp_tri, &z->LFO[0]);
                        load_lfo_preset(lp_tri, &z->LFO[1]);
                        z->LFO[0].rate = log2(8.176f * powf(2, i_generators[freqModLFO].shAmount /
                                                                   1200.f)); // mod lfo
                        z->LFO[0].triggermode = 0;
                        z->LFO[1].rate = log2(8.176f * powf(2, i_generators[freqVibLFO].shAmount /
                                                                   1200.f)); // vib lfo
                        z->LFO[1].triggermode = 0;

                        int mmslot = 0;

                        // modulations
                        if (i_generators[modLfoToPitch].shAmount)
                        {
                            z->mm[mmslot].source = get_mm_source_id("stepLFO1");
                            z->mm[mmslot].destination = get_mm_dest_id("pitch");
                            z->mm[mmslot++].strength = 0.01f * i_generators[modLfoToPitch].shAmount;
                        }
                        if (i_generators[vibLfoToPitch].shAmount)
                        {
                            z->mm[mmslot].source = get_mm_source_id("stepLFO2");
                            z->mm[mmslot].destination = get_mm_dest_id("pitch");
                            z->mm[mmslot++].strength = 0.01f * i_generators[vibLfoToPitch].shAmount;
                        }
                        if (i_generators[modEnvToPitch].shAmount)
                        {
                            z->mm[mmslot].source = get_mm_source_id("EG2");
                            z->mm[mmslot].destination = get_mm_dest_id("pitch");
                            z->mm[mmslot++].strength = 0.01f * i_generators[modEnvToPitch].shAmount;
                        }
                        if (i_generators[modLfoToFilterFc].shAmount)
                        {
                            z->mm[mmslot].source = get_mm_source_id("stepLFO1");
                            z->mm[mmslot].destination = get_mm_dest_id("f1p1");
                            z->mm[mmslot++].strength =
                                i_generators[modLfoToFilterFc].shAmount / 1200.f;
                        }
                        if (i_generators[modEnvToFilterFc].shAmount)
                        {
                            z->mm[mmslot].source = get_mm_source_id("EG2");
                            z->mm[mmslot].destination = get_mm_dest_id("f1p1");
                            z->mm[mmslot++].strength =
                                i_generators[modEnvToFilterFc].shAmount / 1200.f;
                        }
                        if (i_generators[modLfoToVolume].shAmount)
                        {
                            z->mm[mmslot].source = get_mm_source_id("stepLFO1");
                            z->mm[mmslot].destination = get_mm_dest_id("amplitude");
                            z->mm[mmslot++].strength = 0.1f * i_generators[modLfoToVolume].shAmount;
                        }
                        if (i_generators[keynumToVolEnvHold].shAmount)
                        {
                            z->mm[mmslot].source = get_mm_source_id("keytrack");
                            z->mm[mmslot].destination = get_mm_dest_id("eg1h");
                            z->mm[mmslot++].strength =
                                0.01f * i_generators[keynumToVolEnvHold].shAmount;
                        }
                        if (i_generators[keynumToVolEnvDecay].shAmount)
                        {
                            z->mm[mmslot].source = get_mm_source_id("keytrack");
                            z->mm[mmslot].destination = get_mm_dest_id("eg1d");
                            z->mm[mmslot++].strength =
                                0.01f * i_generators[keynumToVolEnvDecay].shAmount;
                        }
                        if (i_generators[keynumToModEnvHold].shAmount)
                        {
                            z->mm[mmslot].source = get_mm_source_id("keytrack");
                            z->mm[mmslot].destination = get_mm_dest_id("eg2h");
                            z->mm[mmslot++].strength =
                                0.01f * i_generators[keynumToModEnvHold].shAmount;
                        }
                        if (i_generators[keynumToModEnvDecay].shAmount)
                        {
                            z->mm[mmslot].source = get_mm_source_id("keytrack");
                            z->mm[mmslot].destination = get_mm_dest_id("eg2d");
                            z->mm[mmslot++].strength =
                                0.01f * i_generators[keynumToModEnvDecay].shAmount;
                        }

#ifdef SCPB
                        int mmi = 8;
                        z->mm[mmi].source = get_mm_source_id("c1");
                        z->mm[mmi].destination = get_mm_dest_id("f1p1");
                        z->mm[mmi++].strength = 6.f;
                        z->mm[mmi].source = get_mm_source_id("c2");
                        z->mm[mmi].destination = get_mm_dest_id("f1p2");
                        z->mm[mmi++].strength = 1.f;

#endif

                        if (shdr[sample_id].sfSampleType == monoSample)
                            z->aux[0].balance = 0.002f * i_generators[pan].shAmount;
                        else
                            z->aux[0].balance = 0.f;

                        if (i_generators_set[initialFilterFc] &&
                            (i_generators[initialFilterFc].wAmount > 1200))
                            z->Filter[0].type = ft_biquadSBQ;
                        else
                            z->Filter[0].type = ft_none;

                        z->Filter[0].p[0] =
                            -4.45943f + float(i_generators[initialFilterFc].wAmount - 1500) / 1200;
                        z->Filter[0].p[1] =
                            max(0.f, min(1.f, float(i_generators[initialFilterQ].wAmount / 960)));
                        update_zone_switches(newzone);
                    }
                }
            }
        }
    }

    delete preset_header;
    delete preset_bag;
    delete preset_gen;
    delete inst_header;
    delete inst_bag;
    delete inst_gen;
    delete shdr;
    return true;
bailout:
    mmioClose(hmmio, 0);
    delete preset_header;
    delete preset_bag;
    delete preset_gen;
    delete inst_header;
    delete inst_bag;
    delete inst_gen;
    delete shdr;
    return false;
}
