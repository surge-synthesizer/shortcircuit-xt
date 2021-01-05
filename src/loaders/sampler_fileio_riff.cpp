//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004-2006 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#include "sampler.h"
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <string>

#if WINDOWS
#include <windows.h>
#else
#include "windows_compat.h"
#include <limits.h>
#endif

#include <iostream>
#include <fstream>

using std::max;
using std::min;

#include "globals.h"
#include "riff_memfile.h"
#include "sample.h"

#include "configuration.h"
#include "loaders/shortcircuit2_RIFF_conversion.h"
#include "loaders/shortcircuit2_RIFF_format.h"
#include <vt_dsp/endian.h>

using std::vector;

// RIFF_StoreSample
// call with data=0 to query size of the required buffer
size_t RIFF_StoreSample(sample *s, void *data)
{
    fs::path filename;
    if (!s->Embedded && s->get_filename(&filename))
    {
        size_t ChunkSize = 12 + SC3::Memfile::RIFFMemFile::RIFFTextChunkSize(path_to_string(filename).c_str());
        if (!data)
            return ChunkSize;

        SC3::Memfile::RIFFMemFile mf(data, ChunkSize);
        mf.RIFFCreateLISTHeader('Smpl', ChunkSize - 12);
        mf.RIFFCreateTextChunk('SUrl', path_to_string(filename).c_str());

        return ChunkSize;
    }
    else
    {
        size_t WaveSize = s->SaveWaveChunk(0);
        size_t ChunkSize = 12 + WaveSize;
        if (!data)
            return ChunkSize;

        SC3::Memfile::RIFFMemFile mf(data, ChunkSize);
        mf.RIFFCreateLISTHeader('WAVE', WaveSize);
        s->SaveWaveChunk(mf.ReadPtr(WaveSize));

        return ChunkSize;
    }
}

// RIFF_StoreZone
// call with data=0 to query size of the required buffer
size_t RIFF_StoreZone(sample_zone *z, void *data, int SampleID)
{
    modmatrix mm;
    mm.assign(0, z, 0);

    size_t subsize = 2 * (12 + 8 + sizeof(RIFF_FILTER)) + mm_entries * (8 + sizeof(RIFF_MM_ENTRY)) +
                     nc_entries * (8 + sizeof(RIFF_NC_ENTRY)) + 2 * (8 + sizeof(RIFF_AHDSR)) +
                     3 * (8 + sizeof(RIFF_LFO)) + 3 * (8 + sizeof(RIFF_AUX_BUSS)) +
                     z->n_hitpoints * (8 + sizeof(RIFF_HITPOINT)) + (8 + sizeof(RIFF_ZONE)) +
                     SC3::Memfile::RIFFMemFile::RIFFTextChunkSize(z->name);

    size_t zonesize = 12 + subsize;

    if (!data)
        return zonesize;

    SC3::Memfile::RIFFMemFile mf(data, zonesize);

    mf.RIFFCreateLISTHeader('Zone', subsize);

    RIFF_ZONE rz;
    WriteChunkZonD(&rz, z, SampleID);

    mf.RIFFCreateChunk('ZonD', &rz, sizeof(RIFF_ZONE));

    mf.RIFFCreateTextChunk('Name', z->name);

    // Filt
    for (int i = 0; i < 2; i++)
    {
        mf.RIFFCreateLISTHeader('Fltr', sizeof(RIFF_FILTER) + 8);

        RIFF_FILTER f;
        WriteChunkFltD(&f, &z->Filter[i]);
        mf.RIFFCreateChunk('FltD', &f, sizeof(RIFF_FILTER));
    }

    // AuxB
    for (int i = 0; i < 3; i++)
    {
        RIFF_AUX_BUSS e;
        WriteChunkAuxB(&e, &z->aux[i]);
        mf.RIFFCreateChunk('AuxB', &e, sizeof(RIFF_AUX_BUSS));
    }

    // AHDS
    for (int i = 0; i < 2; i++)
    {
        RIFF_AHDSR e;
        envelope_AHDSR *eg = (i == 0) ? &z->AEG : &z->EG2;
        WriteChunkAHDS(&e, eg);
        mf.RIFFCreateChunk('AHDS', &e, sizeof(RIFF_AHDSR));
    }

    // SLFO
    for (int i = 0; i < 3; i++)
    {
        RIFF_LFO e;
        WriteChunkSLFO(&e, &z->LFO[i]);
        mf.RIFFCreateChunk('SLFO', &e, sizeof(RIFF_LFO));
    }

    // MMen
    for (int i = 0; i < mm_entries; i++)
    {
        RIFF_MM_ENTRY e;
        WriteChunkMMen(&e, &z->mm[i], &mm);
        mf.RIFFCreateChunk('MMen', &e, sizeof(RIFF_MM_ENTRY));
    }

    // NCen
    for (int i = 0; i < nc_entries; i++)
    {
        RIFF_NC_ENTRY e;
        WriteChunkNCen(&e, &z->nc[i], &mm);
        mf.RIFFCreateChunk('NCen', &e, sizeof(RIFF_NC_ENTRY));
    }

    // HtPt
    for (int i = 0; i < z->n_hitpoints; i++)
    {
        RIFF_HITPOINT e;
        WriteChunkHtPt(&e, &z->hp[i]);
        mf.RIFFCreateChunk('HtPt', &e, sizeof(RIFF_HITPOINT));
    }

    return zonesize;
}

size_t RIFF_StorePart(sample_part *p, void *data)
{
    modmatrix mm;
    mm.assign(0, 0, p);

    size_t ctrltxtsize = 0;
    for (int i = 0; i < n_custom_controllers; i++)
        ctrltxtsize += SC3::Memfile::RIFFMemFile::RIFFTextChunkSize(p->userparametername[i]);

    size_t subsize =
        2 * (12 + 8 + sizeof(RIFF_FILTER)) +
        mm_part_entries * (8 + sizeof(RIFF_MM_ENTRY)) + /*nc_entries*(8 + sizeof(RIFF_NC_ENTRY)) +*/
        num_layers * (12 + num_layer_ncs * (sizeof(RIFF_NC_ENTRY) + 8)) +
        n_custom_controllers * (12 + 8 + sizeof(RIFF_CONTROLLER)) + ctrltxtsize +
        3 * (8 + sizeof(RIFF_AUX_BUSS)) + (8 + sizeof(RIFF_PART)) +
        SC3::Memfile::RIFFMemFile::RIFFTextChunkSize(p->name);

    size_t partsize = 12 + subsize;

    if (!data)
        return partsize;

    SC3::Memfile::RIFFMemFile mf(data, partsize);

    mf.RIFFCreateLISTHeader('Part', subsize);

    RIFF_PART rp;
    WriteChunkParD(&rp, p);
    mf.RIFFCreateChunk('ParD', &rp, sizeof(RIFF_PART));

    mf.RIFFCreateTextChunk('Name', p->name);

    // Filt
    for (int i = 0; i < 2; i++)
    {
        mf.RIFFCreateLISTHeader('Fltr', sizeof(RIFF_FILTER) + 8);

        RIFF_FILTER f;
        WriteChunkFltD(&f, &p->Filter[i]);
        mf.RIFFCreateChunk('FltD', &f, sizeof(RIFF_FILTER));
    }

    // AuxB
    for (int i = 0; i < 3; i++)
    {
        RIFF_AUX_BUSS e;
        WriteChunkAuxB(&e, &p->aux[i]);
        mf.RIFFCreateChunk('AuxB', &e, sizeof(RIFF_AUX_BUSS));
    }

    // MMen
    for (int i = 0; i < mm_part_entries; i++)
    {
        RIFF_MM_ENTRY e;
        WriteChunkMMen(&e, &p->mm[i], &mm);
        mf.RIFFCreateChunk('MMen', &e, sizeof(RIFF_MM_ENTRY));
    }

    // Ctrl
    for (int i = 0; i < n_custom_controllers; i++)
    {
        mf.RIFFCreateLISTHeader('Ctrl', sizeof(RIFF_CONTROLLER) + 8 +
                                            mf.RIFFTextChunkSize(p->userparametername[i]));

        RIFF_CONTROLLER e;
        WriteChunkCtrD(&e, p, i);
        mf.RIFFCreateChunk('CtrD', &e, sizeof(RIFF_CONTROLLER));
        mf.RIFFCreateTextChunk('Name', p->userparametername[i]);
    }

    for (int j = 0; j < num_layers; j++)
    {
        mf.RIFFCreateLISTHeader('Layr', num_layer_ncs * (sizeof(RIFF_NC_ENTRY) + 8));

        // NCen
        for (int i = 0; i < num_layer_ncs; i++)
        {
            RIFF_NC_ENTRY e;
            WriteChunkNCen(&e, &p->nc[i + j * num_layer_ncs], &mm);
            mf.RIFFCreateChunk('NCen', &e, sizeof(RIFF_NC_ENTRY));
        }
    }

    return partsize;
}

size_t sampler::SaveAllAsRIFF(void **dataptr, const WCHAR *filename, int PartID)
{
    // Phase 1
    // Collect the size of all included zones/etc

    bool IsMulti = PartID < 0;
    size_t PartsSize = 0;
    size_t ZonesSize = 0;
    size_t SamplesSize = 0;
    size_t MultiSize = 0;
    vector<size_t> PartListSize;
    vector<size_t> SampleListSize;
    vector<unsigned int> SampleListID;
    vector<size_t> ZoneListSize;
    vector<unsigned int> ZoneListID;
    vector<unsigned int> ZoneListSampleID;

    if (IsMulti)
    {
        MultiSize = num_fxunits * (12 + 8 + sizeof(RIFF_FILTER) + 8 + sizeof(RIFF_FILTER_BUSSEX));
        for (unsigned int i = 0; i < 16; i++)
        {
            PartListSize.push_back(RIFF_StorePart(&parts[i], 0));
            PartsSize += PartListSize[i];
        }
    }
    else
    {
        PartsSize = RIFF_StorePart(&parts[PartID], 0);
    }

    for (unsigned int z = 0; z < max_zones; z++)
    {
        if (zone_exist(z) && ((zones[z].part == PartID) || (PartID < 0)))
        {
            size_t size = RIFF_StoreZone(&zones[z], 0, 0);
            ZoneListSize.push_back(size);
            ZoneListID.push_back(z);
            ZonesSize += size;
            unsigned int s = zones[z].sample_id;

            vector<unsigned int>::iterator result;
            result = find(SampleListID.begin(), SampleListID.end(), s);
            if (result == SampleListID.end())
            {
                size = RIFF_StoreSample(samples[s], 0);
                ZoneListSampleID.push_back(SampleListID.size());
                SampleListSize.push_back(size);
                SampleListID.push_back(s);
                SamplesSize += size;
            }
            else
            {
                ZoneListSampleID.push_back(result - SampleListID.begin());
            }
        }
    }

    // Phase 2
    // Allocate memory block

    size_t datasize = 12 + (12 + ZonesSize) + (12 + SamplesSize) + (12 + PartsSize);
    if (IsMulti)
        datasize += (12 + MultiSize);
    if (chunkDataPtr)
        free(chunkDataPtr);
    chunkDataPtr = malloc(datasize);
    SC3::Memfile::RIFFMemFile mf(chunkDataPtr, datasize);

    // TODO, ta h�nsyn till vsts chunk ptr issuesdryghet

    // Phase 3
    // Write data to RIFFMemFile

    mf.WriteDWORD(vt_write_int32BE('RIFF'));
    mf.WriteDWORD(datasize - 8);
    if (IsMulti)
        mf.WriteDWORD(vt_write_int32BE('SC2M')); // Multi
    else
        mf.WriteDWORD(vt_write_int32BE('SC2P')); // Part

    if (IsMulti)
    {
        mf.RIFFCreateLISTHeader('Mult', MultiSize);
        for (int i = 0; i < num_fxunits; i++)
        {
            mf.RIFFCreateLISTHeader('Fltr',
                                    sizeof(RIFF_FILTER) + 8 + sizeof(RIFF_FILTER_BUSSEX) + 8);

            RIFF_FILTER f;
            WriteChunkFltD(&f, &multi.Filter[i]);
            mf.RIFFCreateChunk('FltD', &f, sizeof(RIFF_FILTER));

            RIFF_FILTER_BUSSEX send;
            WriteChunkFltB(&send, &multi, i);
            mf.RIFFCreateChunk('FltB', &send, sizeof(RIFF_FILTER_BUSSEX));
        }
    }

    mf.RIFFCreateLISTHeader('PtLs', PartsSize);

    if (IsMulti)
    {
        for (unsigned int i = 0; i < 16; i++)
        {
            RIFF_StorePart(&parts[i], mf.ReadPtr(PartListSize[i]));
        }
    }
    else
    {
        RIFF_StorePart(&parts[PartID], mf.ReadPtr(PartsSize));
    }

    mf.RIFFCreateLISTHeader('ZnLs', ZonesSize);

    for (unsigned int i = 0; i < ZoneListID.size(); i++)
    {
        RIFF_StoreZone(&zones[ZoneListID[i]], mf.ReadPtr(ZoneListSize[i]), ZoneListSampleID[i]);
    }

    mf.RIFFCreateLISTHeader('SmLs', SamplesSize);

    for (unsigned int i = 0; i < SampleListID.size(); i++)
    {
        RIFF_StoreSample(samples[SampleListID[i]], mf.ReadPtr(SampleListSize[i]));
    }

    // Phase 4
    // Write file and/or output pointer

    if (dataptr)
    {
        *dataptr = chunkDataPtr;
        return datasize;
    }
    else if (filename)
    {
#if WINDOWS
        std::ofstream ofs(filename, std::ios::binary );
#else
        char fnu8[PATH_MAX];
        vtWStringToString(fnu8, filename, PATH_MAX );
        std::ofstream ofs(fnu8, std::ios::binary);
#endif
        if (!ofs.is_open())
            goto abort;

        ofs.write((const char*)chunkDataPtr, datasize );

        free(chunkDataPtr);
        chunkDataPtr = 0;
        return 1;
    }

    // Phase 5
    // Cleanup
abort:
    free(chunkDataPtr);
    chunkDataPtr = 0;
    return 0;
}

bool sampler::LoadAllFromRIFF(void *data, size_t datasize, bool Replace, int PartID)
{
    size_t chunksize;
    int tag, LISTtag;
    bool IsLIST;

    if (!data)
        return false;
    assert(datasize >= 4);
    SC3::Memfile::RIFFMemFile mf(data, datasize);

    bool IsMulti;
    // if((mf.RIFFGetFileType() == 'SC2P') && mf.riff_descend_RIFF_or_LIST('SC2P',0)) IsMulti =
    // false;
    if ((mf.RIFFGetFileType() == 'SC2P') && mf.RIFFDescendSearch('SC2P'))
        IsMulti = false;
    else if ((mf.RIFFGetFileType() == 'SC2M') && mf.RIFFDescendSearch('SC2M'))
        IsMulti = true;
    else if (*(int *)data == 'mx?<')
        return load_all_from_xml(data, datasize, fs::path(), Replace, PartID);
    else
        return false;

    off_t rootchunk = mf.TellI();

    if (Replace)
    {
        if (IsMulti)
            free_all();
        else
            part_init(PartID, true, true);
    }

    // load part(s)
    if (mf.RIFFDescendSearch('PtLs'))
    {
        size_t PartSize;
        int PartIDMulti = 0;
        while (mf.RIFFDescendSearch('Part', &PartSize))
        {
            size_t PartStart = mf.TellI();

            int FilterID = 0, MMID = 0, LayerID = 0, AuxBID = 0, CtrlID = 0;

            int id = IsMulti ? PartIDMulti : PartID;

            sample_part *p = &parts[id];
            modmatrix mmpart;
            mmpart.assign(0, 0, p);

            while (mf.RIFFPeekChunk(&tag, &chunksize, &IsLIST, &LISTtag))
            {
                if (IsLIST)
                {
                    switch (LISTtag)
                    {
                    case 'Fltr':
                        if (mf.RIFFDescend())
                        {
                            while (mf.RIFFPeekChunk(&tag, &chunksize) && (FilterID < 2))
                            {
                                if (tag == 'FltD')
                                {
                                    RIFF_FILTER *f = (RIFF_FILTER *)mf.RIFFReadChunk();
                                    if (FilterID < 2)
                                        ReadChunkFltD(f, &p->Filter[FilterID]);
                                }
                                else
                                    mf.RIFFSkipChunk();
                            }
                            FilterID++;
                            mf.RIFFAscend();
                        }
                        break;
                    case 'Layr':
                        if (mf.RIFFDescend())
                        {
                            int NCID = 0;
                            while (mf.RIFFPeekChunk(&tag, &chunksize) && (LayerID < num_layers))
                            {
                                if (tag == 'NCen')
                                {
                                    RIFF_NC_ENTRY *nc = (RIFF_NC_ENTRY *)mf.RIFFReadChunk();
                                    if (NCID < num_layer_ncs)
                                        ReadChunkNCen(nc, &p->nc[NCID + LayerID * num_layer_ncs],
                                                      &mmpart);
                                    NCID++;
                                }
                                else
                                    mf.RIFFSkipChunk();
                            }
                            LayerID++;
                            mf.RIFFAscend();
                        }
                        break;
                    case 'Ctrl':
                        if (mf.RIFFDescend())
                        {
                            while (mf.RIFFPeekChunk(&tag, &chunksize) &&
                                   (CtrlID < n_custom_controllers))
                            {
                                if (tag == 'CtrD')
                                {
                                    RIFF_CONTROLLER *Ctrl = (RIFF_CONTROLLER *)mf.RIFFReadChunk();
                                    if (CtrlID < n_custom_controllers)
                                        ReadChunkCtrD(Ctrl, p, CtrlID);
                                }
                                else if (tag == 'Name')
                                {
                                    vtCopyString(&(p->userparametername[CtrlID][0]),
                                                 (char *)mf.RIFFReadChunk(),
                                                 min(chunksize, (size_t)state_string_length));
                                    p->userparametername[CtrlID][state_string_length - 1] = 0;
                                }
                                else
                                    mf.RIFFSkipChunk();
                            }
                            CtrlID++;
                            mf.RIFFAscend();
                        }
                        break;
                    default:
                        mf.RIFFSkipChunk();
                        break;
                    }
                }
                else
                {
                    switch (tag)
                    {
                    case 'ParD':
                    {
                        RIFF_PART *ParD = (RIFF_PART *)mf.RIFFReadChunk();
                        ReadChunkParD(ParD, p);
                    }
                    break;
                    case 'Name':
                    {
                        vtCopyString(p->name, (char *)mf.RIFFReadChunk(),
                                     min(chunksize, (size_t)32));
                        p->name[31] = 0;
                    }
                    break;
                    case 'MMen':
                    {
                        RIFF_MM_ENTRY *mm = (RIFF_MM_ENTRY *)mf.RIFFReadChunk();
                        if (MMID < mm_part_entries)
                            ReadChunkMMen(mm, &p->mm[MMID], &mmpart);
                        MMID++;
                    }
                    break;
                    case 'AuxB':
                    {
                        RIFF_AUX_BUSS *e = (RIFF_AUX_BUSS *)mf.RIFFReadChunk();
                        if (AuxBID < 3)
                            ReadChunkAuxB(e, &p->aux[AuxBID]);
                        AuxBID++;
                    }
                    break;
                    default:
                        mf.RIFFSkipChunk();
                        break;
                    };
                }
            }
            mf.RIFFAscend();
            PartIDMulti++;
            if ((PartIDMulti > 15) || !IsMulti)
                break;
        }
        mf.RIFFAscend(true);
    }

    // load samples
    vector<unsigned int> SampleIDMap;
    if (mf.RIFFDescendSearch('SmLs'))
    {
        while (mf.RIFFPeekChunk(&tag, &chunksize, &IsLIST, &LISTtag))
        {
            if (IsLIST && (LISTtag == 'WAVE'))
            {
                size_t WAVEsize;
                if (mf.RIFFDescend(0, &WAVEsize))
                {
                    int s = this->GetFreeSampleId();
                    assert(s >= 0);
                    if (s < 0)
                        return false;
                    samples[s] = new sample(conf);
                    assert(samples[s]);
                    samples[s]->forget(); // to set refcounter=0
                    SampleIDMap.push_back(s);

                    if (!samples[s]->parse_riff_wave(mf.GetPtr(), WAVEsize, true))
                    {
                        delete samples[s];
                        samples[s] = 0;
                    }
                    mf.RIFFAscend();
                }
            }
            else
            {
                mf.RIFFSkipChunk();
                SampleIDMap.push_back(0); // even unrecoginzed blocks should increment the counter
            }
        }
        mf.RIFFAscend(true);
    }

    // load zones
    if (mf.RIFFDescendSearch('ZnLs'))
    {
        size_t ZoneSize;
        while (mf.RIFFDescendSearch('Zone', &ZoneSize))
        {
            size_t ZoneStart = mf.TellI();
            int ZoneID;
            if (add_zone(0, &ZoneID))
            {
                sample_zone *z = &zones[ZoneID];
                modmatrix mmzone;
                mmzone.assign(0, z, 0);

                // sample_zone *z = new sample_zone();
                int FilterID = 0, EGID = 0, LFOID = 0, MMID = 0, NCID = 0, HtPtID = 0, AuxBID = 0;

                while (mf.RIFFPeekChunk(&tag, &chunksize, &IsLIST, &LISTtag))
                {
                    if (IsLIST)
                    {
                        switch (LISTtag)
                        {
                        case 'Fltr':
                            if (mf.RIFFDescend())
                            {
                                while (mf.RIFFPeekChunk(&tag, &chunksize) && (FilterID < 2))
                                {
                                    if (tag == 'FltD')
                                    {
                                        RIFF_FILTER *f = (RIFF_FILTER *)mf.RIFFReadChunk();
                                        if (FilterID < 2)
                                            ReadChunkFltD(f, &z->Filter[FilterID]);
                                    }
                                    else
                                        mf.RIFFSkipChunk();
                                }
                                FilterID++;
                                mf.RIFFAscend();
                            }
                            break;
                        default:
                            mf.RIFFSkipChunk();
                            break;
                        }
                    }
                    else
                    {
                        switch (tag)
                        {
                        case 'ZonD':
                        {
                            RIFF_ZONE *ZonD = (RIFF_ZONE *)mf.RIFFReadChunk();
                            if (ZonD->SampleID < SampleIDMap.size())
                            {
                                z->sample_id = SampleIDMap[ZonD->SampleID];
                                samples[z->sample_id]->remember();
                            }
                            ReadChunkZonD(ZonD, z);
                        }
                        break;
                        case 'Name':
                        {
                            vtCopyString(z->name, (char *)mf.RIFFReadChunk(),
                                         min(chunksize, (size_t)32));
                            z->name[31] = 0;
                        }
                        break;
                        case 'MMen':
                        {
                            RIFF_MM_ENTRY *mm = (RIFF_MM_ENTRY *)mf.RIFFReadChunk();
                            if (MMID < mm_entries)
                                ReadChunkMMen(mm, &z->mm[MMID], &mmzone);
                            MMID++;
                        }
                        break;
                        case 'NCen':
                        {
                            RIFF_NC_ENTRY *nc = (RIFF_NC_ENTRY *)mf.RIFFReadChunk();
                            if (NCID < nc_entries)
                                ReadChunkNCen(nc, &z->nc[NCID], &mmzone);
                            NCID++;
                        }
                        break;
                        case 'AHDS':
                        {
                            RIFF_AHDSR *e = (RIFF_AHDSR *)mf.RIFFReadChunk();
                            if (EGID < 2)
                                ReadChunkAHDS(e, EGID ? &z->EG2 : &z->AEG);
                            EGID++;
                        }
                        break;
                        case 'SLFO':
                        {
                            RIFF_LFO *e = (RIFF_LFO *)mf.RIFFReadChunk();
                            if (LFOID < 3)
                                ReadChunkSLFO(e, &z->LFO[LFOID]);
                            LFOID++;
                        }
                        break;
                        case 'AuxB':
                        {
                            RIFF_AUX_BUSS *e = (RIFF_AUX_BUSS *)mf.RIFFReadChunk();
                            if (AuxBID < 3)
                                ReadChunkAuxB(e, &z->aux[AuxBID]);
                            AuxBID++;
                        }
                        break;
                        case 'HtPt':
                        {
                            RIFF_HITPOINT *e = (RIFF_HITPOINT *)mf.RIFFReadChunk();
                            if (HtPtID < max_hitpoints)
                                ReadChunkHtPt(e, &z->hp[HtPtID]);
                            HtPtID++;
                        }
                        break;
                        default:
                            mf.RIFFSkipChunk();
                            break;
                        }
                    }
                }
                if (!IsMulti)
                {
                    z->part = PartID;
                }
            }
            mf.RIFFAscend();
        }
        mf.RIFFAscend(true);
    }

    // load multi data
    if (IsMulti && mf.RIFFDescendSearch('Mult'))
    {
        int FXslot = 0;
        while (mf.RIFFPeekChunk(&tag, &chunksize, &IsLIST, &LISTtag))
        {
            if (IsLIST)
            {
                switch (LISTtag)
                {
                case 'Fltr':
                    if (mf.RIFFDescend())
                    {
                        while (mf.RIFFPeekChunk(&tag, &chunksize) && (FXslot < num_fxunits))
                        {
                            if (tag == 'FltD')
                            {
                                RIFF_FILTER *f = (RIFF_FILTER *)mf.RIFFReadChunk();
                                ReadChunkFltD(f, &multi.Filter[FXslot]);
                            }
                            else if (tag == 'FltB')
                            {
                                RIFF_FILTER_BUSSEX *f = (RIFF_FILTER_BUSSEX *)mf.RIFFReadChunk();
                                ReadChunkFltB(f, &multi, FXslot);
                            }
                            else
                                mf.RIFFSkipChunk();
                        }
                        FXslot++;
                        mf.RIFFAscend();
                    }
                    break;
                default:
                    mf.RIFFSkipChunk();
                    break;
                }
            }
            else
                mf.RIFFSkipChunk();
        }
        mf.RIFFAscend(true);
    }

    // TODO, release any orphan samples (& assert)

    // TODO, refresh editor
    post_initdata();

    return true;
}
