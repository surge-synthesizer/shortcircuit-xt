/***************************************************************************
 *                                                                         *
 *   libgig - C++ cross-platform Gigasampler format file access library    *
 *                                                                         *
 *   Copyright (C) 2003-2020 by Christian Schoenebeck                      *
 *                              <cuse@users.sourceforge.net>               *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this library; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#include "gig.h"

#include "helper.h"
#include "Serialization.h"

#include <algorithm>
#include <math.h>
#include <iostream>
#include <assert.h>

/// libgig's current file format version (for extending the original Giga file
/// format with libgig's own custom data / custom features).
#define GIG_FILE_EXT_VERSION    2

/// Initial size of the sample buffer which is used for decompression of
/// compressed sample wave streams - this value should always be bigger than
/// the biggest sample piece expected to be read by the sampler engine,
/// otherwise the buffer size will be raised at runtime and thus the buffer
/// reallocated which is time consuming and unefficient.
#define INITIAL_SAMPLE_BUFFER_SIZE              512000 // 512 kB

/** (so far) every exponential paramater in the gig format has a basis of 1.000000008813822 */
#define GIG_EXP_DECODE(x)                       (pow(1.000000008813822, x))
#define GIG_EXP_ENCODE(x)                       (log(x) / log(1.000000008813822))
#define GIG_PITCH_TRACK_EXTRACT(x)              (!(x & 0x01))
#define GIG_PITCH_TRACK_ENCODE(x)               ((x) ? 0x00 : 0x01)
#define GIG_VCF_RESONANCE_CTRL_EXTRACT(x)       ((x >> 4) & 0x03)
#define GIG_VCF_RESONANCE_CTRL_ENCODE(x)        ((x & 0x03) << 4)
#define GIG_EG_CTR_ATTACK_INFLUENCE_EXTRACT(x)  ((x >> 1) & 0x03)
#define GIG_EG_CTR_DECAY_INFLUENCE_EXTRACT(x)   ((x >> 3) & 0x03)
#define GIG_EG_CTR_RELEASE_INFLUENCE_EXTRACT(x) ((x >> 5) & 0x03)
#define GIG_EG_CTR_ATTACK_INFLUENCE_ENCODE(x)   ((x & 0x03) << 1)
#define GIG_EG_CTR_DECAY_INFLUENCE_ENCODE(x)    ((x & 0x03) << 3)
#define GIG_EG_CTR_RELEASE_INFLUENCE_ENCODE(x)  ((x & 0x03) << 5)

#define SRLZ(member) \
    archive->serializeMember(*this, member, #member);

namespace gig {

// *************** Internal functions for sample decompression ***************
// *

namespace {

    inline int get12lo(const unsigned char* pSrc)
    {
        const int x = pSrc[0] | (pSrc[1] & 0x0f) << 8;
        return x & 0x800 ? x - 0x1000 : x;
    }

    inline int get12hi(const unsigned char* pSrc)
    {
        const int x = pSrc[1] >> 4 | pSrc[2] << 4;
        return x & 0x800 ? x - 0x1000 : x;
    }

    inline int16_t get16(const unsigned char* pSrc)
    {
        return int16_t(pSrc[0] | pSrc[1] << 8);
    }

    inline int get24(const unsigned char* pSrc)
    {
        const int x = pSrc[0] | pSrc[1] << 8 | pSrc[2] << 16;
        return x & 0x800000 ? x - 0x1000000 : x;
    }

    inline void store24(unsigned char* pDst, int x)
    {
        pDst[0] = x;
        pDst[1] = x >> 8;
        pDst[2] = x >> 16;
    }

    void Decompress16(int compressionmode, const unsigned char* params,
                      int srcStep, int dstStep,
                      const unsigned char* pSrc, int16_t* pDst,
                      file_offset_t currentframeoffset,
                      file_offset_t copysamples)
    {
        switch (compressionmode) {
            case 0: // 16 bit uncompressed
                pSrc += currentframeoffset * srcStep;
                while (copysamples) {
                    *pDst = get16(pSrc);
                    pDst += dstStep;
                    pSrc += srcStep;
                    copysamples--;
                }
                break;

            case 1: // 16 bit compressed to 8 bit
                int y  = get16(params);
                int dy = get16(params + 2);
                while (currentframeoffset) {
                    dy -= int8_t(*pSrc);
                    y  -= dy;
                    pSrc += srcStep;
                    currentframeoffset--;
                }
                while (copysamples) {
                    dy -= int8_t(*pSrc);
                    y  -= dy;
                    *pDst = y;
                    pDst += dstStep;
                    pSrc += srcStep;
                    copysamples--;
                }
                break;
        }
    }

    void Decompress24(int compressionmode, const unsigned char* params,
                      int dstStep, const unsigned char* pSrc, uint8_t* pDst,
                      file_offset_t currentframeoffset,
                      file_offset_t copysamples, int truncatedBits)
    {
        int y, dy, ddy, dddy;

#define GET_PARAMS(params)                      \
        y    = get24(params);                   \
        dy   = y - get24((params) + 3);         \
        ddy  = get24((params) + 6);             \
        dddy = get24((params) + 9)

#define SKIP_ONE(x)                             \
        dddy -= (x);                            \
        ddy  -= dddy;                           \
        dy   =  -dy - ddy;                      \
        y    += dy

#define COPY_ONE(x)                             \
        SKIP_ONE(x);                            \
        store24(pDst, y << truncatedBits);      \
        pDst += dstStep

        switch (compressionmode) {
            case 2: // 24 bit uncompressed
                pSrc += currentframeoffset * 3;
                while (copysamples) {
                    store24(pDst, get24(pSrc) << truncatedBits);
                    pDst += dstStep;
                    pSrc += 3;
                    copysamples--;
                }
                break;

            case 3: // 24 bit compressed to 16 bit
                GET_PARAMS(params);
                while (currentframeoffset) {
                    SKIP_ONE(get16(pSrc));
                    pSrc += 2;
                    currentframeoffset--;
                }
                while (copysamples) {
                    COPY_ONE(get16(pSrc));
                    pSrc += 2;
                    copysamples--;
                }
                break;

            case 4: // 24 bit compressed to 12 bit
                GET_PARAMS(params);
                while (currentframeoffset > 1) {
                    SKIP_ONE(get12lo(pSrc));
                    SKIP_ONE(get12hi(pSrc));
                    pSrc += 3;
                    currentframeoffset -= 2;
                }
                if (currentframeoffset) {
                    SKIP_ONE(get12lo(pSrc));
                    currentframeoffset--;
                    if (copysamples) {
                        COPY_ONE(get12hi(pSrc));
                        pSrc += 3;
                        copysamples--;
                    }
                }
                while (copysamples > 1) {
                    COPY_ONE(get12lo(pSrc));
                    COPY_ONE(get12hi(pSrc));
                    pSrc += 3;
                    copysamples -= 2;
                }
                if (copysamples) {
                    COPY_ONE(get12lo(pSrc));
                }
                break;

            case 5: // 24 bit compressed to 8 bit
                GET_PARAMS(params);
                while (currentframeoffset) {
                    SKIP_ONE(int8_t(*pSrc++));
                    currentframeoffset--;
                }
                while (copysamples) {
                    COPY_ONE(int8_t(*pSrc++));
                    copysamples--;
                }
                break;
        }
    }

    const int bytesPerFrame[] =      { 4096, 2052, 768, 524, 396, 268 };
    const int bytesPerFrameNoHdr[] = { 4096, 2048, 768, 512, 384, 256 };
    const int headerSize[] =         { 0, 4, 0, 12, 12, 12 };
    const int bitsPerSample[] =      { 16, 8, 24, 16, 12, 8 };
}



// *************** Internal CRC-32 (Cyclic Redundancy Check) functions  ***************
// *

    static uint32_t* __initCRCTable() {
        static uint32_t res[256];

        for (int i = 0 ; i < 256 ; i++) {
            uint32_t c = i;
            for (int j = 0 ; j < 8 ; j++) {
                c = (c & 1) ? 0xedb88320 ^ (c >> 1) : c >> 1;
            }
            res[i] = c;
        }
        return res;
    }

    static const uint32_t* __CRCTable = __initCRCTable();

    /**
     * Initialize a CRC variable.
     *
     * @param crc - variable to be initialized
     */
    inline static void __resetCRC(uint32_t& crc) {
        crc = 0xffffffff;
    }

    /**
     * Used to calculate checksums of the sample data in a gig file. The
     * checksums are stored in the 3crc chunk of the gig file and
     * automatically updated when a sample is written with Sample::Write().
     *
     * One should call __resetCRC() to initialize the CRC variable to be
     * used before calling this function the first time.
     *
     * After initializing the CRC variable one can call this function
     * arbitrary times, i.e. to split the overall CRC calculation into
     * steps.
     *
     * Once the whole data was processed by __calculateCRC(), one should
     * call __finalizeCRC() to get the final CRC result.
     *
     * @param buf     - pointer to data the CRC shall be calculated of
     * @param bufSize - size of the data to be processed
     * @param crc     - variable the CRC sum shall be stored to
     */
    static void __calculateCRC(unsigned char* buf, size_t bufSize, uint32_t& crc) {
        for (size_t i = 0 ; i < bufSize ; i++) {
            crc = __CRCTable[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
        }
    }

    /**
     * Returns the final CRC result.
     *
     * @param crc - variable previously passed to __calculateCRC()
     */
    inline static void __finalizeCRC(uint32_t& crc) {
        crc ^= 0xffffffff;
    }



// *************** Other Internal functions  ***************
// *

    static split_type_t __resolveSplitType(dimension_t dimension) {
        return (
            dimension == dimension_layer ||
            dimension == dimension_samplechannel ||
            dimension == dimension_releasetrigger ||
            dimension == dimension_keyboard ||
            dimension == dimension_roundrobin ||
            dimension == dimension_random ||
            dimension == dimension_smartmidi ||
            dimension == dimension_roundrobinkeyboard
        ) ? split_type_bit : split_type_normal;
    }

    static int __resolveZoneSize(dimension_def_t& dimension_definition) {
        return (dimension_definition.split_type == split_type_normal)
        ? int(128.0 / dimension_definition.zones) : 0;
    }



// *************** leverage_ctrl_t ***************
// *

    void leverage_ctrl_t::serialize(Serialization::Archive* archive) {
        SRLZ(type);
        SRLZ(controller_number);
    }



// *************** crossfade_t ***************
// *

    void crossfade_t::serialize(Serialization::Archive* archive) {
        SRLZ(in_start);
        SRLZ(in_end);
        SRLZ(out_start);
        SRLZ(out_end);
    }



// *************** eg_opt_t ***************
// *

    eg_opt_t::eg_opt_t() {
        AttackCancel     = true;
        AttackHoldCancel = true;
        Decay1Cancel     = true;
        Decay2Cancel     = true;
        ReleaseCancel    = true;
    }

    void eg_opt_t::serialize(Serialization::Archive* archive) {
        SRLZ(AttackCancel);
        SRLZ(AttackHoldCancel);
        SRLZ(Decay1Cancel);
        SRLZ(Decay2Cancel);
        SRLZ(ReleaseCancel);
    }



// *************** Sample ***************
// *

    size_t       Sample::Instances = 0;
    buffer_t     Sample::InternalDecompressionBuffer;

    /** @brief Constructor.
     *
     * Load an existing sample or create a new one. A 'wave' list chunk must
     * be given to this constructor. In case the given 'wave' list chunk
     * contains a 'fmt', 'data' (and optionally a '3gix', 'smpl') chunk, the
     * format and sample data will be loaded from there, otherwise default
     * values will be used and those chunks will be created when
     * File::Save() will be called later on.
     *
     * @param pFile          - pointer to gig::File where this sample is
     *                         located (or will be located)
     * @param waveList       - pointer to 'wave' list chunk which is (or
     *                         will be) associated with this sample
     * @param WavePoolOffset - offset of this sample data from wave pool
     *                         ('wvpl') list chunk
     * @param fileNo         - number of an extension file where this sample
     *                         is located, 0 otherwise
     * @param index          - wave pool index of sample (may be -1 on new sample)
     */
    Sample::Sample(File* pFile, RIFF::List* waveList, file_offset_t WavePoolOffset, unsigned long fileNo, int index)
        : DLS::Sample((DLS::File*) pFile, waveList, WavePoolOffset)
    {
        static const DLS::Info::string_length_t fixedStringLengths[] = {
            { CHUNK_ID_INAM, 64 },
            { 0, 0 }
        };
        pInfo->SetFixedStringLengths(fixedStringLengths);
        Instances++;
        FileNo = fileNo;

        __resetCRC(crc);
        // if this is not a new sample, try to get the sample's already existing
        // CRC32 checksum from disk, this checksum will reflect the sample's CRC32
        // checksum of the time when the sample was consciously modified by the
        // user for the last time (by calling Sample::Write() that is).
        if (index >= 0) { // not a new file ...
            try {
                uint32_t crc = pFile->GetSampleChecksumByIndex(index);
                this->crc = crc;
            } catch (...) {}
        }

        pCk3gix = waveList->GetSubChunk(CHUNK_ID_3GIX);
        if (pCk3gix) {
            pCk3gix->SetPos(0);

            uint16_t iSampleGroup = pCk3gix->ReadInt16();
            pGroup = pFile->GetGroup(iSampleGroup);
        } else { // '3gix' chunk missing
            // by default assigned to that mandatory "Default Group"
            pGroup = pFile->GetGroup(0);
        }

        pCkSmpl = waveList->GetSubChunk(CHUNK_ID_SMPL);
        if (pCkSmpl) {
            pCkSmpl->SetPos(0);

            Manufacturer  = pCkSmpl->ReadInt32();
            Product       = pCkSmpl->ReadInt32();
            SamplePeriod  = pCkSmpl->ReadInt32();
            MIDIUnityNote = pCkSmpl->ReadInt32();
            FineTune      = pCkSmpl->ReadInt32();
            pCkSmpl->Read(&SMPTEFormat, 1, 4);
            SMPTEOffset   = pCkSmpl->ReadInt32();
            Loops         = pCkSmpl->ReadInt32();
            pCkSmpl->ReadInt32(); // manufByt
            LoopID        = pCkSmpl->ReadInt32();
            pCkSmpl->Read(&LoopType, 1, 4);
            LoopStart     = pCkSmpl->ReadInt32();
            LoopEnd       = pCkSmpl->ReadInt32();
            LoopFraction  = pCkSmpl->ReadInt32();
            LoopPlayCount = pCkSmpl->ReadInt32();
        } else { // 'smpl' chunk missing
            // use default values
            Manufacturer  = 0;
            Product       = 0;
            SamplePeriod  = uint32_t(1000000000.0 / SamplesPerSecond + 0.5);
            MIDIUnityNote = 60;
            FineTune      = 0;
            SMPTEFormat   = smpte_format_no_offset;
            SMPTEOffset   = 0;
            Loops         = 0;
            LoopID        = 0;
            LoopType      = loop_type_normal;
            LoopStart     = 0;
            LoopEnd       = 0;
            LoopFraction  = 0;
            LoopPlayCount = 0;
        }

        FrameTable                 = NULL;
        SamplePos                  = 0;
        RAMCache.Size              = 0;
        RAMCache.pStart            = NULL;
        RAMCache.NullExtensionSize = 0;

        if (BitDepth > 24) throw gig::Exception("Only samples up to 24 bit supported");

        RIFF::Chunk* ewav = waveList->GetSubChunk(CHUNK_ID_EWAV);
        Compressed        = ewav;
        Dithered          = false;
        TruncatedBits     = 0;
        if (Compressed) {
            ewav->SetPos(0);

            uint32_t version = ewav->ReadInt32();
            if (version > 2 && BitDepth == 24) {
                Dithered = ewav->ReadInt32();
                ewav->SetPos(Channels == 2 ? 84 : 64);
                TruncatedBits = ewav->ReadInt32();
            }
            ScanCompressedSample();
        }

        // we use a buffer for decompression and for truncating 24 bit samples to 16 bit
        if ((Compressed || BitDepth == 24) && !InternalDecompressionBuffer.Size) {
            InternalDecompressionBuffer.pStart = new unsigned char[INITIAL_SAMPLE_BUFFER_SIZE];
            InternalDecompressionBuffer.Size   = INITIAL_SAMPLE_BUFFER_SIZE;
        }
        FrameOffset = 0; // just for streaming compressed samples

        LoopSize = LoopEnd - LoopStart + 1;
    }

    /**
     * Make a (semi) deep copy of the Sample object given by @a orig (without
     * the actual waveform data) and assign it to this object.
     *
     * Discussion: copying .gig samples is a bit tricky. It requires three
     * steps:
     * 1. Copy sample's meta informations (done by CopyAssignMeta()) including
     *    its new sample waveform data size.
     * 2. Saving the file (done by File::Save()) so that it gains correct size
     *    and layout for writing the actual wave form data directly to disc
     *    in next step.
     * 3. Copy the waveform data with disk streaming (done by CopyAssignWave()).
     *
     * @param orig - original Sample object to be copied from
     */
    void Sample::CopyAssignMeta(const Sample* orig) {
        // handle base classes
        DLS::Sample::CopyAssignCore(orig);
        
        // handle actual own attributes of this class
        Manufacturer = orig->Manufacturer;
        Product = orig->Product;
        SamplePeriod = orig->SamplePeriod;
        MIDIUnityNote = orig->MIDIUnityNote;
        FineTune = orig->FineTune;
        SMPTEFormat = orig->SMPTEFormat;
        SMPTEOffset = orig->SMPTEOffset;
        Loops = orig->Loops;
        LoopID = orig->LoopID;
        LoopType = orig->LoopType;
        LoopStart = orig->LoopStart;
        LoopEnd = orig->LoopEnd;
        LoopSize = orig->LoopSize;
        LoopFraction = orig->LoopFraction;
        LoopPlayCount = orig->LoopPlayCount;
        
        // schedule resizing this sample to the given sample's size
        Resize(orig->GetSize());
    }

    /**
     * Should be called after CopyAssignMeta() and File::Save() sequence.
     * Read more about it in the discussion of CopyAssignMeta(). This method
     * copies the actual waveform data by disk streaming.
     *
     * @e CAUTION: this method is currently not thread safe! During this
     * operation the sample must not be used for other purposes by other
     * threads!
     *
     * @param orig - original Sample object to be copied from
     */
    void Sample::CopyAssignWave(const Sample* orig) {
        const int iReadAtOnce = 32*1024;
        char* buf = new char[iReadAtOnce * orig->FrameSize];
        Sample* pOrig = (Sample*) orig; //HACK: remove constness for now
        file_offset_t restorePos = pOrig->GetPos();
        pOrig->SetPos(0);
        SetPos(0);
        for (file_offset_t n = pOrig->Read(buf, iReadAtOnce); n;
                           n = pOrig->Read(buf, iReadAtOnce))
        {
            Write(buf, n);
        }
        pOrig->SetPos(restorePos);
        delete [] buf;
    }

    /**
     * Apply sample and its settings to the respective RIFF chunks. You have
     * to call File::Save() to make changes persistent.
     *
     * Usually there is absolutely no need to call this method explicitly.
     * It will be called automatically when File::Save() was called.
     *
     * @param pProgress - callback function for progress notification
     * @throws DLS::Exception if FormatTag != DLS_WAVE_FORMAT_PCM or no sample data
     *                        was provided yet
     * @throws gig::Exception if there is any invalid sample setting
     */
    void Sample::UpdateChunks(progress_t* pProgress) {
        // first update base class's chunks
        DLS::Sample::UpdateChunks(pProgress);

        // make sure 'smpl' chunk exists
        pCkSmpl = pWaveList->GetSubChunk(CHUNK_ID_SMPL);
        if (!pCkSmpl) {
            pCkSmpl = pWaveList->AddSubChunk(CHUNK_ID_SMPL, 60);
            memset(pCkSmpl->LoadChunkData(), 0, 60);
        }
        // update 'smpl' chunk
        uint8_t* pData = (uint8_t*) pCkSmpl->LoadChunkData();
        SamplePeriod = uint32_t(1000000000.0 / SamplesPerSecond + 0.5);
        store32(&pData[0], Manufacturer);
        store32(&pData[4], Product);
        store32(&pData[8], SamplePeriod);
        store32(&pData[12], MIDIUnityNote);
        store32(&pData[16], FineTune);
        store32(&pData[20], SMPTEFormat);
        store32(&pData[24], SMPTEOffset);
        store32(&pData[28], Loops);

        // we skip 'manufByt' for now (4 bytes)

        store32(&pData[36], LoopID);
        store32(&pData[40], LoopType);
        store32(&pData[44], LoopStart);
        store32(&pData[48], LoopEnd);
        store32(&pData[52], LoopFraction);
        store32(&pData[56], LoopPlayCount);

        // make sure '3gix' chunk exists
        pCk3gix = pWaveList->GetSubChunk(CHUNK_ID_3GIX);
        if (!pCk3gix) pCk3gix = pWaveList->AddSubChunk(CHUNK_ID_3GIX, 4);
        // determine appropriate sample group index (to be stored in chunk)
        uint16_t iSampleGroup = 0; // 0 refers to default sample group
        File* pFile = static_cast<File*>(pParent);
        if (pFile->pGroups) {
            std::list<Group*>::iterator iter = pFile->pGroups->begin();
            std::list<Group*>::iterator end  = pFile->pGroups->end();
            for (int i = 0; iter != end; i++, iter++) {
                if (*iter == pGroup) {
                    iSampleGroup = i;
                    break; // found
                }
            }
        }
        // update '3gix' chunk
        pData = (uint8_t*) pCk3gix->LoadChunkData();
        store16(&pData[0], iSampleGroup);

        // if the library user toggled the "Compressed" attribute from true to
        // false, then the EWAV chunk associated with compressed samples needs
        // to be deleted
        RIFF::Chunk* ewav = pWaveList->GetSubChunk(CHUNK_ID_EWAV);
        if (ewav && !Compressed) {
            pWaveList->DeleteSubChunk(ewav);
        }
    }

    /// Scans compressed samples for mandatory informations (e.g. actual number of total sample points).
    void Sample::ScanCompressedSample() {
        //TODO: we have to add some more scans here (e.g. determine compression rate)
        this->SamplesTotal = 0;
        std::list<file_offset_t> frameOffsets;

        SamplesPerFrame = BitDepth == 24 ? 256 : 2048;
        WorstCaseFrameSize = SamplesPerFrame * FrameSize + Channels; // +Channels for compression flag

        // Scanning
        pCkData->SetPos(0);
        if (Channels == 2) { // Stereo
            for (int i = 0 ; ; i++) {
                // for 24 bit samples every 8:th frame offset is
                // stored, to save some memory
                if (BitDepth != 24 || (i & 7) == 0) frameOffsets.push_back(pCkData->GetPos());

                const int mode_l = pCkData->ReadUint8();
                const int mode_r = pCkData->ReadUint8();
                if (mode_l > 5 || mode_r > 5) throw gig::Exception("Unknown compression mode");
                const file_offset_t frameSize = bytesPerFrame[mode_l] + bytesPerFrame[mode_r];

                if (pCkData->RemainingBytes() <= frameSize) {
                    SamplesInLastFrame =
                        ((pCkData->RemainingBytes() - headerSize[mode_l] - headerSize[mode_r]) << 3) /
                        (bitsPerSample[mode_l] + bitsPerSample[mode_r]);
                    SamplesTotal += SamplesInLastFrame;
                    break;
                }
                SamplesTotal += SamplesPerFrame;
                pCkData->SetPos(frameSize, RIFF::stream_curpos);
            }
        }
        else { // Mono
            for (int i = 0 ; ; i++) {
                if (BitDepth != 24 || (i & 7) == 0) frameOffsets.push_back(pCkData->GetPos());

                const int mode = pCkData->ReadUint8();
                if (mode > 5) throw gig::Exception("Unknown compression mode");
                const file_offset_t frameSize = bytesPerFrame[mode];

                if (pCkData->RemainingBytes() <= frameSize) {
                    SamplesInLastFrame =
                        ((pCkData->RemainingBytes() - headerSize[mode]) << 3) / bitsPerSample[mode];
                    SamplesTotal += SamplesInLastFrame;
                    break;
                }
                SamplesTotal += SamplesPerFrame;
                pCkData->SetPos(frameSize, RIFF::stream_curpos);
            }
        }
        pCkData->SetPos(0);

        // Build the frames table (which is used for fast resolving of a frame's chunk offset)
        if (FrameTable) delete[] FrameTable;
        FrameTable = new file_offset_t[frameOffsets.size()];
        std::list<file_offset_t>::iterator end  = frameOffsets.end();
        std::list<file_offset_t>::iterator iter = frameOffsets.begin();
        for (int i = 0; iter != end; i++, iter++) {
            FrameTable[i] = *iter;
        }
    }

    /**
     * Loads (and uncompresses if needed) the whole sample wave into RAM. Use
     * ReleaseSampleData() to free the memory if you don't need the cached
     * sample data anymore.
     *
     * @returns  buffer_t structure with start address and size of the buffer
     *           in bytes
     * @see      ReleaseSampleData(), Read(), SetPos()
     */
    buffer_t Sample::LoadSampleData() {
        return LoadSampleDataWithNullSamplesExtension(this->SamplesTotal, 0); // 0 amount of NullSamples
    }

    /**
     * Reads (uncompresses if needed) and caches the first \a SampleCount
     * numbers of SamplePoints in RAM. Use ReleaseSampleData() to free the
     * memory space if you don't need the cached samples anymore. There is no
     * guarantee that exactly \a SampleCount samples will be cached; this is
     * not an error. The size will be eventually truncated e.g. to the
     * beginning of a frame of a compressed sample. This is done for
     * efficiency reasons while streaming the wave by your sampler engine
     * later. Read the <i>Size</i> member of the <i>buffer_t</i> structure
     * that will be returned to determine the actual cached samples, but note
     * that the size is given in bytes! You get the number of actually cached
     * samples by dividing it by the frame size of the sample:
     * @code
     * 	buffer_t buf       = pSample->LoadSampleData(acquired_samples);
     * 	long cachedsamples = buf.Size / pSample->FrameSize;
     * @endcode
     *
     * @param SampleCount - number of sample points to load into RAM
     * @returns             buffer_t structure with start address and size of
     *                      the cached sample data in bytes
     * @see                 ReleaseSampleData(), Read(), SetPos()
     */
    buffer_t Sample::LoadSampleData(file_offset_t SampleCount) {
        return LoadSampleDataWithNullSamplesExtension(SampleCount, 0); // 0 amount of NullSamples
    }

    /**
     * Loads (and uncompresses if needed) the whole sample wave into RAM. Use
     * ReleaseSampleData() to free the memory if you don't need the cached
     * sample data anymore.
     * The method will add \a NullSamplesCount silence samples past the
     * official buffer end (this won't affect the 'Size' member of the
     * buffer_t structure, that means 'Size' always reflects the size of the
     * actual sample data, the buffer might be bigger though). Silence
     * samples past the official buffer are needed for differential
     * algorithms that always have to take subsequent samples into account
     * (resampling/interpolation would be an important example) and avoids
     * memory access faults in such cases.
     *
     * @param NullSamplesCount - number of silence samples the buffer should
     *                           be extended past it's data end
     * @returns                  buffer_t structure with start address and
     *                           size of the buffer in bytes
     * @see                      ReleaseSampleData(), Read(), SetPos()
     */
    buffer_t Sample::LoadSampleDataWithNullSamplesExtension(uint NullSamplesCount) {
        return LoadSampleDataWithNullSamplesExtension(this->SamplesTotal, NullSamplesCount);
    }

    /**
     * Reads (uncompresses if needed) and caches the first \a SampleCount
     * numbers of SamplePoints in RAM. Use ReleaseSampleData() to free the
     * memory space if you don't need the cached samples anymore. There is no
     * guarantee that exactly \a SampleCount samples will be cached; this is
     * not an error. The size will be eventually truncated e.g. to the
     * beginning of a frame of a compressed sample. This is done for
     * efficiency reasons while streaming the wave by your sampler engine
     * later. Read the <i>Size</i> member of the <i>buffer_t</i> structure
     * that will be returned to determine the actual cached samples, but note
     * that the size is given in bytes! You get the number of actually cached
     * samples by dividing it by the frame size of the sample:
     * @code
     * 	buffer_t buf       = pSample->LoadSampleDataWithNullSamplesExtension(acquired_samples, null_samples);
     * 	long cachedsamples = buf.Size / pSample->FrameSize;
     * @endcode
     * The method will add \a NullSamplesCount silence samples past the
     * official buffer end (this won't affect the 'Size' member of the
     * buffer_t structure, that means 'Size' always reflects the size of the
     * actual sample data, the buffer might be bigger though). Silence
     * samples past the official buffer are needed for differential
     * algorithms that always have to take subsequent samples into account
     * (resampling/interpolation would be an important example) and avoids
     * memory access faults in such cases.
     *
     * @param SampleCount      - number of sample points to load into RAM
     * @param NullSamplesCount - number of silence samples the buffer should
     *                           be extended past it's data end
     * @returns                  buffer_t structure with start address and
     *                           size of the cached sample data in bytes
     * @see                      ReleaseSampleData(), Read(), SetPos()
     */
    buffer_t Sample::LoadSampleDataWithNullSamplesExtension(file_offset_t SampleCount, uint NullSamplesCount) {
        if (SampleCount > this->SamplesTotal) SampleCount = this->SamplesTotal;
        if (RAMCache.pStart) delete[] (int8_t*) RAMCache.pStart;
        file_offset_t allocationsize = (SampleCount + NullSamplesCount) * this->FrameSize;
        SetPos(0); // reset read position to begin of sample
        RAMCache.pStart            = new int8_t[allocationsize];
        RAMCache.Size              = Read(RAMCache.pStart, SampleCount) * this->FrameSize;
        RAMCache.NullExtensionSize = allocationsize - RAMCache.Size;
        // fill the remaining buffer space with silence samples
        memset((int8_t*)RAMCache.pStart + RAMCache.Size, 0, RAMCache.NullExtensionSize);
        return GetCache();
    }

    /**
     * Returns current cached sample points. A buffer_t structure will be
     * returned which contains address pointer to the begin of the cache and
     * the size of the cached sample data in bytes. Use
     * <i>LoadSampleData()</i> to cache a specific amount of sample points in
     * RAM.
     *
     * @returns  buffer_t structure with current cached sample points
     * @see      LoadSampleData();
     */
    buffer_t Sample::GetCache() {
        // return a copy of the buffer_t structure
        buffer_t result;
        result.Size              = this->RAMCache.Size;
        result.pStart            = this->RAMCache.pStart;
        result.NullExtensionSize = this->RAMCache.NullExtensionSize;
        return result;
    }

    /**
     * Frees the cached sample from RAM if loaded with
     * <i>LoadSampleData()</i> previously.
     *
     * @see  LoadSampleData();
     */
    void Sample::ReleaseSampleData() {
        if (RAMCache.pStart) delete[] (int8_t*) RAMCache.pStart;
        RAMCache.pStart = NULL;
        RAMCache.Size   = 0;
        RAMCache.NullExtensionSize = 0;
    }

    /** @brief Resize sample.
     *
     * Resizes the sample's wave form data, that is the actual size of
     * sample wave data possible to be written for this sample. This call
     * will return immediately and just schedule the resize operation. You
     * should call File::Save() to actually perform the resize operation(s)
     * "physically" to the file. As this can take a while on large files, it
     * is recommended to call Resize() first on all samples which have to be
     * resized and finally to call File::Save() to perform all those resize
     * operations in one rush.
     *
     * The actual size (in bytes) is dependant to the current FrameSize
     * value. You may want to set FrameSize before calling Resize().
     *
     * <b>Caution:</b> You cannot directly write (i.e. with Write()) to
     * enlarged samples before calling File::Save() as this might exceed the
     * current sample's boundary!
     *
     * Also note: only DLS_WAVE_FORMAT_PCM is currently supported, that is
     * FormatTag must be DLS_WAVE_FORMAT_PCM. Trying to resize samples with
     * other formats will fail!
     *
     * @param NewSize - new sample wave data size in sample points (must be
     *                  greater than zero)
     * @throws DLS::Excecption if FormatTag != DLS_WAVE_FORMAT_PCM
     * @throws DLS::Exception if \a NewSize is less than 1 or unrealistic large
     * @throws gig::Exception if existing sample is compressed
     * @see DLS::Sample::GetSize(), DLS::Sample::FrameSize,
     *      DLS::Sample::FormatTag, File::Save()
     */
    void Sample::Resize(file_offset_t NewSize) {
        if (Compressed) throw gig::Exception("There is no support for modifying compressed samples (yet)");
        DLS::Sample::Resize(NewSize);
    }

    /**
     * Sets the position within the sample (in sample points, not in
     * bytes). Use this method and <i>Read()</i> if you don't want to load
     * the sample into RAM, thus for disk streaming.
     *
     * Although the original Gigasampler engine doesn't allow positioning
     * within compressed samples, I decided to implement it. Even though
     * the Gigasampler format doesn't allow to define loops for compressed
     * samples at the moment, positioning within compressed samples might be
     * interesting for some sampler engines though. The only drawback about
     * my decision is that it takes longer to load compressed gig Files on
     * startup, because it's neccessary to scan the samples for some
     * mandatory informations. But I think as it doesn't affect the runtime
     * efficiency, nobody will have a problem with that.
     *
     * @param SampleCount  number of sample points to jump
     * @param Whence       optional: to which relation \a SampleCount refers
     *                     to, if omited <i>RIFF::stream_start</i> is assumed
     * @returns            the new sample position
     * @see                Read()
     */
    file_offset_t Sample::SetPos(file_offset_t SampleCount, RIFF::stream_whence_t Whence) {
        if (Compressed) {
            switch (Whence) {
                case RIFF::stream_curpos:
                    this->SamplePos += SampleCount;
                    break;
                case RIFF::stream_end:
                    this->SamplePos = this->SamplesTotal - 1 - SampleCount;
                    break;
                case RIFF::stream_backward:
                    this->SamplePos -= SampleCount;
                    break;
                case RIFF::stream_start: default:
                    this->SamplePos = SampleCount;
                    break;
            }
            if (this->SamplePos > this->SamplesTotal) this->SamplePos = this->SamplesTotal;

            file_offset_t frame = this->SamplePos / 2048; // to which frame to jump
            this->FrameOffset   = this->SamplePos % 2048; // offset (in sample points) within that frame
            pCkData->SetPos(FrameTable[frame]);           // set chunk pointer to the start of sought frame
            return this->SamplePos;
        }
        else { // not compressed
            file_offset_t orderedBytes = SampleCount * this->FrameSize;
            file_offset_t result = pCkData->SetPos(orderedBytes, Whence);
            return (result == orderedBytes) ? SampleCount
                                            : result / this->FrameSize;
        }
    }

    /**
     * Returns the current position in the sample (in sample points).
     */
    file_offset_t Sample::GetPos() const {
        if (Compressed) return SamplePos;
        else            return pCkData->GetPos() / FrameSize;
    }

    /**
     * Reads \a SampleCount number of sample points from the position stored
     * in \a pPlaybackState into the buffer pointed by \a pBuffer and moves
     * the position within the sample respectively, this method honors the
     * looping informations of the sample (if any). The sample wave stream
     * will be decompressed on the fly if using a compressed sample. Use this
     * method if you don't want to load the sample into RAM, thus for disk
     * streaming. All this methods needs to know to proceed with streaming
     * for the next time you call this method is stored in \a pPlaybackState.
     * You have to allocate and initialize the playback_state_t structure by
     * yourself before you use it to stream a sample:
     * @code
     * gig::playback_state_t playbackstate;
     * playbackstate.position         = 0;
     * playbackstate.reverse          = false;
     * playbackstate.loop_cycles_left = pSample->LoopPlayCount;
     * @endcode
     * You don't have to take care of things like if there is actually a loop
     * defined or if the current read position is located within a loop area.
     * The method already handles such cases by itself.
     *
     * <b>Caution:</b> If you are using more than one streaming thread, you
     * have to use an external decompression buffer for <b>EACH</b>
     * streaming thread to avoid race conditions and crashes!
     *
     * @param pBuffer          destination buffer
     * @param SampleCount      number of sample points to read
     * @param pPlaybackState   will be used to store and reload the playback
     *                         state for the next ReadAndLoop() call
     * @param pDimRgn          dimension region with looping information
     * @param pExternalDecompressionBuffer  (optional) external buffer to use for decompression
     * @returns                number of successfully read sample points
     * @see                    CreateDecompressionBuffer()
     */
    file_offset_t Sample::ReadAndLoop(void* pBuffer, file_offset_t SampleCount, playback_state_t* pPlaybackState,
                                      DimensionRegion* pDimRgn, buffer_t* pExternalDecompressionBuffer) {
        file_offset_t samplestoread = SampleCount, totalreadsamples = 0, readsamples, samplestoloopend;
        uint8_t* pDst = (uint8_t*) pBuffer;

        SetPos(pPlaybackState->position); // recover position from the last time

        if (pDimRgn->SampleLoops) { // honor looping if there are loop points defined

            const DLS::sample_loop_t& loop = pDimRgn->pSampleLoops[0];
            const uint32_t loopEnd = loop.LoopStart + loop.LoopLength;

            if (GetPos() <= loopEnd) {
                switch (loop.LoopType) {

                    case loop_type_bidirectional: { //TODO: not tested yet!
                        do {
                            // if not endless loop check if max. number of loop cycles have been passed
                            if (this->LoopPlayCount && !pPlaybackState->loop_cycles_left) break;

                            if (!pPlaybackState->reverse) { // forward playback
                                do {
                                    samplestoloopend  = loopEnd - GetPos();
                                    readsamples       = Read(&pDst[totalreadsamples * this->FrameSize], Min(samplestoread, samplestoloopend), pExternalDecompressionBuffer);
                                    samplestoread    -= readsamples;
                                    totalreadsamples += readsamples;
                                    if (readsamples == samplestoloopend) {
                                        pPlaybackState->reverse = true;
                                        break;
                                    }
                                } while (samplestoread && readsamples);
                            }
                            else { // backward playback

                                // as we can only read forward from disk, we have to
                                // determine the end position within the loop first,
                                // read forward from that 'end' and finally after
                                // reading, swap all sample frames so it reflects
                                // backward playback

                                file_offset_t swapareastart       = totalreadsamples;
                                file_offset_t loopoffset          = GetPos() - loop.LoopStart;
                                file_offset_t samplestoreadinloop = Min(samplestoread, loopoffset);
                                file_offset_t reverseplaybackend  = GetPos() - samplestoreadinloop;

                                SetPos(reverseplaybackend);

                                // read samples for backward playback
                                do {
                                    readsamples          = Read(&pDst[totalreadsamples * this->FrameSize], samplestoreadinloop, pExternalDecompressionBuffer);
                                    samplestoreadinloop -= readsamples;
                                    samplestoread       -= readsamples;
                                    totalreadsamples    += readsamples;
                                } while (samplestoreadinloop && readsamples);

                                SetPos(reverseplaybackend); // pretend we really read backwards

                                if (reverseplaybackend == loop.LoopStart) {
                                    pPlaybackState->loop_cycles_left--;
                                    pPlaybackState->reverse = false;
                                }

                                // reverse the sample frames for backward playback
                                if (totalreadsamples > swapareastart) //FIXME: this if() is just a crash workaround for now (#102), but totalreadsamples <= swapareastart should never be the case, so there's probably still a bug above!
                                    SwapMemoryArea(&pDst[swapareastart * this->FrameSize], (totalreadsamples - swapareastart) * this->FrameSize, this->FrameSize);
                            }
                        } while (samplestoread && readsamples);
                        break;
                    }

                    case loop_type_backward: { // TODO: not tested yet!
                        // forward playback (not entered the loop yet)
                        if (!pPlaybackState->reverse) do {
                            samplestoloopend  = loopEnd - GetPos();
                            readsamples       = Read(&pDst[totalreadsamples * this->FrameSize], Min(samplestoread, samplestoloopend), pExternalDecompressionBuffer);
                            samplestoread    -= readsamples;
                            totalreadsamples += readsamples;
                            if (readsamples == samplestoloopend) {
                                pPlaybackState->reverse = true;
                                break;
                            }
                        } while (samplestoread && readsamples);

                        if (!samplestoread) break;

                        // as we can only read forward from disk, we have to
                        // determine the end position within the loop first,
                        // read forward from that 'end' and finally after
                        // reading, swap all sample frames so it reflects
                        // backward playback

                        file_offset_t swapareastart       = totalreadsamples;
                        file_offset_t loopoffset          = GetPos() - loop.LoopStart;
                        file_offset_t samplestoreadinloop = (this->LoopPlayCount) ? Min(samplestoread, pPlaybackState->loop_cycles_left * loop.LoopLength - loopoffset)
                                                                                  : samplestoread;
                        file_offset_t reverseplaybackend  = loop.LoopStart + Abs((loopoffset - samplestoreadinloop) % loop.LoopLength);

                        SetPos(reverseplaybackend);

                        // read samples for backward playback
                        do {
                            // if not endless loop check if max. number of loop cycles have been passed
                            if (this->LoopPlayCount && !pPlaybackState->loop_cycles_left) break;
                            samplestoloopend     = loopEnd - GetPos();
                            readsamples          = Read(&pDst[totalreadsamples * this->FrameSize], Min(samplestoreadinloop, samplestoloopend), pExternalDecompressionBuffer);
                            samplestoreadinloop -= readsamples;
                            samplestoread       -= readsamples;
                            totalreadsamples    += readsamples;
                            if (readsamples == samplestoloopend) {
                                pPlaybackState->loop_cycles_left--;
                                SetPos(loop.LoopStart);
                            }
                        } while (samplestoreadinloop && readsamples);

                        SetPos(reverseplaybackend); // pretend we really read backwards

                        // reverse the sample frames for backward playback
                        SwapMemoryArea(&pDst[swapareastart * this->FrameSize], (totalreadsamples - swapareastart) * this->FrameSize, this->FrameSize);
                        break;
                    }

                    default: case loop_type_normal: {
                        do {
                            // if not endless loop check if max. number of loop cycles have been passed
                            if (this->LoopPlayCount && !pPlaybackState->loop_cycles_left) break;
                            samplestoloopend  = loopEnd - GetPos();
                            readsamples       = Read(&pDst[totalreadsamples * this->FrameSize], Min(samplestoread, samplestoloopend), pExternalDecompressionBuffer);
                            samplestoread    -= readsamples;
                            totalreadsamples += readsamples;
                            if (readsamples == samplestoloopend) {
                                pPlaybackState->loop_cycles_left--;
                                SetPos(loop.LoopStart);
                            }
                        } while (samplestoread && readsamples);
                        break;
                    }
                }
            }
        }

        // read on without looping
        if (samplestoread) do {
            readsamples = Read(&pDst[totalreadsamples * this->FrameSize], samplestoread, pExternalDecompressionBuffer);
            samplestoread    -= readsamples;
            totalreadsamples += readsamples;
        } while (readsamples && samplestoread);

        // store current position
        pPlaybackState->position = GetPos();

        return totalreadsamples;
    }

    /**
     * Reads \a SampleCount number of sample points from the current
     * position into the buffer pointed by \a pBuffer and increments the
     * position within the sample. The sample wave stream will be
     * decompressed on the fly if using a compressed sample. Use this method
     * and <i>SetPos()</i> if you don't want to load the sample into RAM,
     * thus for disk streaming.
     *
     * <b>Caution:</b> If you are using more than one streaming thread, you
     * have to use an external decompression buffer for <b>EACH</b>
     * streaming thread to avoid race conditions and crashes!
     *
     * For 16 bit samples, the data in the buffer will be int16_t
     * (using native endianness). For 24 bit, the buffer will
     * contain three bytes per sample, little-endian.
     *
     * @param pBuffer      destination buffer
     * @param SampleCount  number of sample points to read
     * @param pExternalDecompressionBuffer  (optional) external buffer to use for decompression
     * @returns            number of successfully read sample points
     * @see                SetPos(), CreateDecompressionBuffer()
     */
    file_offset_t Sample::Read(void* pBuffer, file_offset_t SampleCount, buffer_t* pExternalDecompressionBuffer) {
        if (SampleCount == 0) return 0;
        if (!Compressed) {
            if (BitDepth == 24) {
                return pCkData->Read(pBuffer, SampleCount * FrameSize, 1) / FrameSize;
            }
            else { // 16 bit
                // (pCkData->Read does endian correction)
                return Channels == 2 ? pCkData->Read(pBuffer, SampleCount << 1, 2) >> 1
                                     : pCkData->Read(pBuffer, SampleCount, 2);
            }
        }
        else {
            if (this->SamplePos >= this->SamplesTotal) return 0;
            //TODO: efficiency: maybe we should test for an average compression rate
            file_offset_t assumedsize      = GuessSize(SampleCount),
                          remainingbytes   = 0,           // remaining bytes in the local buffer
                          remainingsamples = SampleCount,
                          copysamples, skipsamples,
                          currentframeoffset = this->FrameOffset;  // offset in current sample frame since last Read()
            this->FrameOffset = 0;

            buffer_t* pDecompressionBuffer = (pExternalDecompressionBuffer) ? pExternalDecompressionBuffer : &InternalDecompressionBuffer;

            // if decompression buffer too small, then reduce amount of samples to read
            if (pDecompressionBuffer->Size < assumedsize) {
                std::cerr << "gig::Read(): WARNING - decompression buffer size too small!" << std::endl;
                SampleCount      = WorstCaseMaxSamples(pDecompressionBuffer);
                remainingsamples = SampleCount;
                assumedsize      = GuessSize(SampleCount);
            }

            unsigned char* pSrc = (unsigned char*) pDecompressionBuffer->pStart;
            int16_t* pDst = static_cast<int16_t*>(pBuffer);
            uint8_t* pDst24 = static_cast<uint8_t*>(pBuffer);
            remainingbytes = pCkData->Read(pSrc, assumedsize, 1);

            while (remainingsamples && remainingbytes) {
                file_offset_t framesamples = SamplesPerFrame;
                file_offset_t framebytes, rightChannelOffset = 0, nextFrameOffset;

                int mode_l = *pSrc++, mode_r = 0;

                if (Channels == 2) {
                    mode_r = *pSrc++;
                    framebytes = bytesPerFrame[mode_l] + bytesPerFrame[mode_r] + 2;
                    rightChannelOffset = bytesPerFrameNoHdr[mode_l];
                    nextFrameOffset = rightChannelOffset + bytesPerFrameNoHdr[mode_r];
                    if (remainingbytes < framebytes) { // last frame in sample
                        framesamples = SamplesInLastFrame;
                        if (mode_l == 4 && (framesamples & 1)) {
                            rightChannelOffset = ((framesamples + 1) * bitsPerSample[mode_l]) >> 3;
                        }
                        else {
                            rightChannelOffset = (framesamples * bitsPerSample[mode_l]) >> 3;
                        }
                    }
                }
                else {
                    framebytes = bytesPerFrame[mode_l] + 1;
                    nextFrameOffset = bytesPerFrameNoHdr[mode_l];
                    if (remainingbytes < framebytes) {
                        framesamples = SamplesInLastFrame;
                    }
                }

                // determine how many samples in this frame to skip and read
                if (currentframeoffset + remainingsamples >= framesamples) {
                    if (currentframeoffset <= framesamples) {
                        copysamples = framesamples - currentframeoffset;
                        skipsamples = currentframeoffset;
                    }
                    else {
                        copysamples = 0;
                        skipsamples = framesamples;
                    }
                }
                else {
                    // This frame has enough data for pBuffer, but not
                    // all of the frame is needed. Set file position
                    // to start of this frame for next call to Read.
                    copysamples = remainingsamples;
                    skipsamples = currentframeoffset;
                    pCkData->SetPos(remainingbytes, RIFF::stream_backward);
                    this->FrameOffset = currentframeoffset + copysamples;
                }
                remainingsamples -= copysamples;

                if (remainingbytes > framebytes) {
                    remainingbytes -= framebytes;
                    if (remainingsamples == 0 &&
                        currentframeoffset + copysamples == framesamples) {
                        // This frame has enough data for pBuffer, and
                        // all of the frame is needed. Set file
                        // position to start of next frame for next
                        // call to Read. FrameOffset is 0.
                        pCkData->SetPos(remainingbytes, RIFF::stream_backward);
                    }
                }
                else remainingbytes = 0;

                currentframeoffset -= skipsamples;

                if (copysamples == 0) {
                    // skip this frame
                    pSrc += framebytes - Channels;
                }
                else {
                    const unsigned char* const param_l = pSrc;
                    if (BitDepth == 24) {
                        if (mode_l != 2) pSrc += 12;

                        if (Channels == 2) { // Stereo
                            const unsigned char* const param_r = pSrc;
                            if (mode_r != 2) pSrc += 12;

                            Decompress24(mode_l, param_l, 6, pSrc, pDst24,
                                         skipsamples, copysamples, TruncatedBits);
                            Decompress24(mode_r, param_r, 6, pSrc + rightChannelOffset, pDst24 + 3,
                                         skipsamples, copysamples, TruncatedBits);
                            pDst24 += copysamples * 6;
                        }
                        else { // Mono
                            Decompress24(mode_l, param_l, 3, pSrc, pDst24,
                                         skipsamples, copysamples, TruncatedBits);
                            pDst24 += copysamples * 3;
                        }
                    }
                    else { // 16 bit
                        if (mode_l) pSrc += 4;

                        int step;
                        if (Channels == 2) { // Stereo
                            const unsigned char* const param_r = pSrc;
                            if (mode_r) pSrc += 4;

                            step = (2 - mode_l) + (2 - mode_r);
                            Decompress16(mode_l, param_l, step, 2, pSrc, pDst, skipsamples, copysamples);
                            Decompress16(mode_r, param_r, step, 2, pSrc + (2 - mode_l), pDst + 1,
                                         skipsamples, copysamples);
                            pDst += copysamples << 1;
                        }
                        else { // Mono
                            step = 2 - mode_l;
                            Decompress16(mode_l, param_l, step, 1, pSrc, pDst, skipsamples, copysamples);
                            pDst += copysamples;
                        }
                    }
                    pSrc += nextFrameOffset;
                }

                // reload from disk to local buffer if needed
                if (remainingsamples && remainingbytes < WorstCaseFrameSize && pCkData->GetState() == RIFF::stream_ready) {
                    assumedsize    = GuessSize(remainingsamples);
                    pCkData->SetPos(remainingbytes, RIFF::stream_backward);
                    if (pCkData->RemainingBytes() < assumedsize) assumedsize = pCkData->RemainingBytes();
                    remainingbytes = pCkData->Read(pDecompressionBuffer->pStart, assumedsize, 1);
                    pSrc = (unsigned char*) pDecompressionBuffer->pStart;
                }
            } // while

            this->SamplePos += (SampleCount - remainingsamples);
            if (this->SamplePos > this->SamplesTotal) this->SamplePos = this->SamplesTotal;
            return (SampleCount - remainingsamples);
        }
    }

    /** @brief Write sample wave data.
     *
     * Writes \a SampleCount number of sample points from the buffer pointed
     * by \a pBuffer and increments the position within the sample. Use this
     * method to directly write the sample data to disk, i.e. if you don't
     * want or cannot load the whole sample data into RAM.
     *
     * You have to Resize() the sample to the desired size and call
     * File::Save() <b>before</b> using Write().
     *
     * Note: there is currently no support for writing compressed samples.
     *
     * For 16 bit samples, the data in the source buffer should be
     * int16_t (using native endianness). For 24 bit, the buffer
     * should contain three bytes per sample, little-endian.
     *
     * @param pBuffer     - source buffer
     * @param SampleCount - number of sample points to write
     * @throws DLS::Exception if current sample size is too small
     * @throws gig::Exception if sample is compressed
     * @see DLS::LoadSampleData()
     */
    file_offset_t Sample::Write(void* pBuffer, file_offset_t SampleCount) {
        if (Compressed) throw gig::Exception("There is no support for writing compressed gig samples (yet)");

        // if this is the first write in this sample, reset the
        // checksum calculator
        if (pCkData->GetPos() == 0) {
            __resetCRC(crc);
        }
        if (GetSize() < SampleCount) throw Exception("Could not write sample data, current sample size to small");
        file_offset_t res;
        if (BitDepth == 24) {
            res = pCkData->Write(pBuffer, SampleCount * FrameSize, 1) / FrameSize;
        } else { // 16 bit
            res = Channels == 2 ? pCkData->Write(pBuffer, SampleCount << 1, 2) >> 1
                                : pCkData->Write(pBuffer, SampleCount, 2);
        }
        __calculateCRC((unsigned char *)pBuffer, SampleCount * FrameSize, crc);

        // if this is the last write, update the checksum chunk in the
        // file
        if (pCkData->GetPos() == pCkData->GetSize()) {
            __finalizeCRC(crc);
            File* pFile = static_cast<File*>(GetParent());
            pFile->SetSampleChecksum(this, crc);
        }
        return res;
    }

    /**
     * Allocates a decompression buffer for streaming (compressed) samples
     * with Sample::Read(). If you are using more than one streaming thread
     * in your application you <b>HAVE</b> to create a decompression buffer
     * for <b>EACH</b> of your streaming threads and provide it with the
     * Sample::Read() call in order to avoid race conditions and crashes.
     *
     * You should free the memory occupied by the allocated buffer(s) once
     * you don't need one of your streaming threads anymore by calling
     * DestroyDecompressionBuffer().
     *
     * @param MaxReadSize - the maximum size (in sample points) you ever
     *                      expect to read with one Read() call
     * @returns allocated decompression buffer
     * @see DestroyDecompressionBuffer()
     */
    buffer_t Sample::CreateDecompressionBuffer(file_offset_t MaxReadSize) {
        buffer_t result;
        const double worstCaseHeaderOverhead =
                (256.0 /*frame size*/ + 12.0 /*header*/ + 2.0 /*compression type flag (stereo)*/) / 256.0;
        result.Size              = (file_offset_t) (double(MaxReadSize) * 3.0 /*(24 Bit)*/ * 2.0 /*stereo*/ * worstCaseHeaderOverhead);
        result.pStart            = new int8_t[result.Size];
        result.NullExtensionSize = 0;
        return result;
    }

    /**
     * Free decompression buffer, previously created with
     * CreateDecompressionBuffer().
     *
     * @param DecompressionBuffer - previously allocated decompression
     *                              buffer to free
     */
    void Sample::DestroyDecompressionBuffer(buffer_t& DecompressionBuffer) {
        if (DecompressionBuffer.Size && DecompressionBuffer.pStart) {
            delete[] (int8_t*) DecompressionBuffer.pStart;
            DecompressionBuffer.pStart = NULL;
            DecompressionBuffer.Size   = 0;
            DecompressionBuffer.NullExtensionSize = 0;
        }
    }

    /**
     * Returns pointer to the Group this Sample belongs to. In the .gig
     * format a sample always belongs to one group. If it wasn't explicitly
     * assigned to a certain group, it will be automatically assigned to a
     * default group.
     *
     * @returns Sample's Group (never NULL)
     */
    Group* Sample::GetGroup() const {
        return pGroup;
    }

    /**
     * Returns the CRC-32 checksum of the sample's raw wave form data at the
     * time when this sample's wave form data was modified for the last time
     * by calling Write(). This checksum only covers the raw wave form data,
     * not any meta informations like i.e. bit depth or loop points. Since
     * this method just returns the checksum stored for this sample i.e. when
     * the gig file was loaded, this method returns immediately. So it does no
     * recalcuation of the checksum with the currently available sample wave
     * form data.
     *
     * @see VerifyWaveData()
     */
    uint32_t Sample::GetWaveDataCRC32Checksum() {
        return crc;
    }

    /**
     * Checks the integrity of this sample's raw audio wave data. Whenever a
     * Sample's raw wave data is intentionally modified (i.e. by calling
     * Write() and supplying the new raw audio wave form data) a CRC32 checksum
     * is calculated and stored/updated for this sample, along to the sample's
     * meta informations.
     *
     * Now by calling this method the current raw audio wave data is checked
     * against the already stored CRC32 check sum in order to check whether the
     * sample data had been damaged unintentionally for some reason. Since by
     * calling this method always the entire raw audio wave data has to be
     * read, verifying all samples this way may take a long time accordingly.
     * And that's also the reason why the sample integrity is not checked by
     * default whenever a gig file is loaded. So this method must be called
     * explicitly to fulfill this task.
     *
     * @param pActually - (optional) if provided, will be set to the actually
     *                    calculated checksum of the current raw wave form data,
     *                    you can get the expected checksum instead by calling
     *                    GetWaveDataCRC32Checksum()
     * @returns true if sample is OK or false if the sample is damaged
     * @throws Exception if no checksum had been stored to disk for this
     *         sample yet, or on I/O issues
     * @see GetWaveDataCRC32Checksum()
     */
    bool Sample::VerifyWaveData(uint32_t* pActually) {
        //File* pFile = static_cast<File*>(GetParent());
        uint32_t crc = CalculateWaveDataChecksum();
        if (pActually) *pActually = crc;
        return crc == this->crc;
    }

    uint32_t Sample::CalculateWaveDataChecksum() {
        const size_t sz = 20*1024; // 20kB buffer size
        std::vector<uint8_t> buffer(sz);
        buffer.resize(sz);

        const size_t n = sz / FrameSize;
        SetPos(0);
        uint32_t crc = 0;
        __resetCRC(crc);
        while (true) {
            file_offset_t nRead = Read(&buffer[0], n);
            if (nRead <= 0) break;
            __calculateCRC(&buffer[0], nRead * FrameSize, crc);
        }
        __finalizeCRC(crc);
        return crc;
    }

    Sample::~Sample() {
        Instances--;
        if (!Instances && InternalDecompressionBuffer.Size) {
            delete[] (unsigned char*) InternalDecompressionBuffer.pStart;
            InternalDecompressionBuffer.pStart = NULL;
            InternalDecompressionBuffer.Size   = 0;
        }
        if (FrameTable) delete[] FrameTable;
        if (RAMCache.pStart) delete[] (int8_t*) RAMCache.pStart;
    }



// *************** DimensionRegion ***************
// *

    size_t                             DimensionRegion::Instances       = 0;
    DimensionRegion::VelocityTableMap* DimensionRegion::pVelocityTables = NULL;

    DimensionRegion::DimensionRegion(Region* pParent, RIFF::List* _3ewl) : DLS::Sampler(_3ewl) {
        Instances++;

        pSample = NULL;
        pRegion = pParent;

        if (_3ewl->GetSubChunk(CHUNK_ID_WSMP)) memcpy(&Crossfade, &SamplerOptions, 4);
        else memset(&Crossfade, 0, 4);

        if (!pVelocityTables) pVelocityTables = new VelocityTableMap;

        RIFF::Chunk* _3ewa = _3ewl->GetSubChunk(CHUNK_ID_3EWA);
        if (_3ewa) { // if '3ewa' chunk exists
            _3ewa->SetPos(0);

            _3ewa->ReadInt32(); // unknown, always == chunk size ?
            LFO3Frequency = (double) GIG_EXP_DECODE(_3ewa->ReadInt32());
            EG3Attack     = (double) GIG_EXP_DECODE(_3ewa->ReadInt32());
            _3ewa->ReadInt16(); // unknown
            LFO1InternalDepth = _3ewa->ReadUint16();
            _3ewa->ReadInt16(); // unknown
            LFO3InternalDepth = _3ewa->ReadInt16();
            _3ewa->ReadInt16(); // unknown
            LFO1ControlDepth = _3ewa->ReadUint16();
            _3ewa->ReadInt16(); // unknown
            LFO3ControlDepth = _3ewa->ReadInt16();
            EG1Attack           = (double) GIG_EXP_DECODE(_3ewa->ReadInt32());
            EG1Decay1           = (double) GIG_EXP_DECODE(_3ewa->ReadInt32());
            _3ewa->ReadInt16(); // unknown
            EG1Sustain          = _3ewa->ReadUint16();
            EG1Release          = (double) GIG_EXP_DECODE(_3ewa->ReadInt32());
            EG1Controller       = DecodeLeverageController(static_cast<_lev_ctrl_t>(_3ewa->ReadUint8()));
            uint8_t eg1ctrloptions        = _3ewa->ReadUint8();
            EG1ControllerInvert           = eg1ctrloptions & 0x01;
            EG1ControllerAttackInfluence  = GIG_EG_CTR_ATTACK_INFLUENCE_EXTRACT(eg1ctrloptions);
            EG1ControllerDecayInfluence   = GIG_EG_CTR_DECAY_INFLUENCE_EXTRACT(eg1ctrloptions);
            EG1ControllerReleaseInfluence = GIG_EG_CTR_RELEASE_INFLUENCE_EXTRACT(eg1ctrloptions);
            EG2Controller       = DecodeLeverageController(static_cast<_lev_ctrl_t>(_3ewa->ReadUint8()));
            uint8_t eg2ctrloptions        = _3ewa->ReadUint8();
            EG2ControllerInvert           = eg2ctrloptions & 0x01;
            EG2ControllerAttackInfluence  = GIG_EG_CTR_ATTACK_INFLUENCE_EXTRACT(eg2ctrloptions);
            EG2ControllerDecayInfluence   = GIG_EG_CTR_DECAY_INFLUENCE_EXTRACT(eg2ctrloptions);
            EG2ControllerReleaseInfluence = GIG_EG_CTR_RELEASE_INFLUENCE_EXTRACT(eg2ctrloptions);
            LFO1Frequency    = (double) GIG_EXP_DECODE(_3ewa->ReadInt32());
            EG2Attack        = (double) GIG_EXP_DECODE(_3ewa->ReadInt32());
            EG2Decay1        = (double) GIG_EXP_DECODE(_3ewa->ReadInt32());
            _3ewa->ReadInt16(); // unknown
            EG2Sustain       = _3ewa->ReadUint16();
            EG2Release       = (double) GIG_EXP_DECODE(_3ewa->ReadInt32());
            _3ewa->ReadInt16(); // unknown
            LFO2ControlDepth = _3ewa->ReadUint16();
            LFO2Frequency    = (double) GIG_EXP_DECODE(_3ewa->ReadInt32());
            _3ewa->ReadInt16(); // unknown
            LFO2InternalDepth = _3ewa->ReadUint16();
            int32_t eg1decay2 = _3ewa->ReadInt32();
            EG1Decay2          = (double) GIG_EXP_DECODE(eg1decay2);
            EG1InfiniteSustain = (eg1decay2 == 0x7fffffff);
            _3ewa->ReadInt16(); // unknown
            EG1PreAttack      = _3ewa->ReadUint16();
            int32_t eg2decay2 = _3ewa->ReadInt32();
            EG2Decay2         = (double) GIG_EXP_DECODE(eg2decay2);
            EG2InfiniteSustain = (eg2decay2 == 0x7fffffff);
            _3ewa->ReadInt16(); // unknown
            EG2PreAttack      = _3ewa->ReadUint16();
            uint8_t velocityresponse = _3ewa->ReadUint8();
            if (velocityresponse < 5) {
                VelocityResponseCurve = curve_type_nonlinear;
                VelocityResponseDepth = velocityresponse;
            } else if (velocityresponse < 10) {
                VelocityResponseCurve = curve_type_linear;
                VelocityResponseDepth = velocityresponse - 5;
            } else if (velocityresponse < 15) {
                VelocityResponseCurve = curve_type_special;
                VelocityResponseDepth = velocityresponse - 10;
            } else {
                VelocityResponseCurve = curve_type_unknown;
                VelocityResponseDepth = 0;
            }
            uint8_t releasevelocityresponse = _3ewa->ReadUint8();
            if (releasevelocityresponse < 5) {
                ReleaseVelocityResponseCurve = curve_type_nonlinear;
                ReleaseVelocityResponseDepth = releasevelocityresponse;
            } else if (releasevelocityresponse < 10) {
                ReleaseVelocityResponseCurve = curve_type_linear;
                ReleaseVelocityResponseDepth = releasevelocityresponse - 5;
            } else if (releasevelocityresponse < 15) {
                ReleaseVelocityResponseCurve = curve_type_special;
                ReleaseVelocityResponseDepth = releasevelocityresponse - 10;
            } else {
                ReleaseVelocityResponseCurve = curve_type_unknown;
                ReleaseVelocityResponseDepth = 0;
            }
            VelocityResponseCurveScaling = _3ewa->ReadUint8();
            AttenuationControllerThreshold = _3ewa->ReadInt8();
            _3ewa->ReadInt32(); // unknown
            SampleStartOffset = (uint16_t) _3ewa->ReadInt16();
            _3ewa->ReadInt16(); // unknown
            uint8_t pitchTrackDimensionBypass = _3ewa->ReadInt8();
            PitchTrack = GIG_PITCH_TRACK_EXTRACT(pitchTrackDimensionBypass);
            if      (pitchTrackDimensionBypass & 0x10) DimensionBypass = dim_bypass_ctrl_94;
            else if (pitchTrackDimensionBypass & 0x20) DimensionBypass = dim_bypass_ctrl_95;
            else                                       DimensionBypass = dim_bypass_ctrl_none;
            uint8_t pan = _3ewa->ReadUint8();
            Pan         = (pan < 64) ? pan : -((int)pan - 63); // signed 7 bit -> signed 8 bit
            SelfMask = _3ewa->ReadInt8() & 0x01;
            _3ewa->ReadInt8(); // unknown
            uint8_t lfo3ctrl = _3ewa->ReadUint8();
            LFO3Controller           = static_cast<lfo3_ctrl_t>(lfo3ctrl & 0x07); // lower 3 bits
            LFO3Sync                 = lfo3ctrl & 0x20; // bit 5
            InvertAttenuationController = lfo3ctrl & 0x80; // bit 7
            AttenuationController  = DecodeLeverageController(static_cast<_lev_ctrl_t>(_3ewa->ReadUint8()));
            uint8_t lfo2ctrl       = _3ewa->ReadUint8();
            LFO2Controller         = static_cast<lfo2_ctrl_t>(lfo2ctrl & 0x07); // lower 3 bits
            LFO2FlipPhase          = lfo2ctrl & 0x80; // bit 7
            LFO2Sync               = lfo2ctrl & 0x20; // bit 5
            bool extResonanceCtrl  = lfo2ctrl & 0x40; // bit 6
            uint8_t lfo1ctrl       = _3ewa->ReadUint8();
            LFO1Controller         = static_cast<lfo1_ctrl_t>(lfo1ctrl & 0x07); // lower 3 bits
            LFO1FlipPhase          = lfo1ctrl & 0x80; // bit 7
            LFO1Sync               = lfo1ctrl & 0x40; // bit 6
            VCFResonanceController = (extResonanceCtrl) ? static_cast<vcf_res_ctrl_t>(GIG_VCF_RESONANCE_CTRL_EXTRACT(lfo1ctrl))
                                                        : vcf_res_ctrl_none;
            uint16_t eg3depth = _3ewa->ReadUint16();
            EG3Depth = (eg3depth <= 1200) ? eg3depth /* positives */
                                        : (-1) * (int16_t) ((eg3depth ^ 0xfff) + 1); /* binary complementary for negatives */
            _3ewa->ReadInt16(); // unknown
            ChannelOffset = _3ewa->ReadUint8() / 4;
            uint8_t regoptions = _3ewa->ReadUint8();
            MSDecode           = regoptions & 0x01; // bit 0
            SustainDefeat      = regoptions & 0x02; // bit 1
            _3ewa->ReadInt16(); // unknown
            VelocityUpperLimit = _3ewa->ReadInt8();
            _3ewa->ReadInt8(); // unknown
            _3ewa->ReadInt16(); // unknown
            ReleaseTriggerDecay = _3ewa->ReadUint8(); // release trigger decay
            _3ewa->ReadInt8(); // unknown
            _3ewa->ReadInt8(); // unknown
            EG1Hold = _3ewa->ReadUint8() & 0x80; // bit 7
            uint8_t vcfcutoff = _3ewa->ReadUint8();
            VCFEnabled = vcfcutoff & 0x80; // bit 7
            VCFCutoff  = vcfcutoff & 0x7f; // lower 7 bits
            VCFCutoffController = static_cast<vcf_cutoff_ctrl_t>(_3ewa->ReadUint8());
            uint8_t vcfvelscale = _3ewa->ReadUint8();
            VCFCutoffControllerInvert = vcfvelscale & 0x80; // bit 7
            VCFVelocityScale = vcfvelscale & 0x7f; // lower 7 bits
            _3ewa->ReadInt8(); // unknown
            uint8_t vcfresonance = _3ewa->ReadUint8();
            VCFResonance = vcfresonance & 0x7f; // lower 7 bits
            VCFResonanceDynamic = !(vcfresonance & 0x80); // bit 7
            uint8_t vcfbreakpoint         = _3ewa->ReadUint8();
            VCFKeyboardTracking           = vcfbreakpoint & 0x80; // bit 7
            VCFKeyboardTrackingBreakpoint = vcfbreakpoint & 0x7f; // lower 7 bits
            uint8_t vcfvelocity = _3ewa->ReadUint8();
            VCFVelocityDynamicRange = vcfvelocity % 5;
            VCFVelocityCurve        = static_cast<curve_type_t>(vcfvelocity / 5);
            VCFType = static_cast<vcf_type_t>(_3ewa->ReadUint8());
            if (VCFType == vcf_type_lowpass) {
                if (lfo3ctrl & 0x40) // bit 6
                    VCFType = vcf_type_lowpassturbo;
            }
            if (_3ewa->RemainingBytes() >= 8) {
                _3ewa->Read(DimensionUpperLimits, 1, 8);
            } else {
                memset(DimensionUpperLimits, 0, 8);
            }
        } else { // '3ewa' chunk does not exist yet
            // use default values
            LFO3Frequency                   = 1.0;
            EG3Attack                       = 0.0;
            LFO1InternalDepth               = 0;
            LFO3InternalDepth               = 0;
            LFO1ControlDepth                = 0;
            LFO3ControlDepth                = 0;
            EG1Attack                       = 0.0;
            EG1Decay1                       = 0.005;
            EG1Sustain                      = 1000;
            EG1Release                      = 0.3;
            EG1Controller.type              = eg1_ctrl_t::type_none;
            EG1Controller.controller_number = 0;
            EG1ControllerInvert             = false;
            EG1ControllerAttackInfluence    = 0;
            EG1ControllerDecayInfluence     = 0;
            EG1ControllerReleaseInfluence   = 0;
            EG2Controller.type              = eg2_ctrl_t::type_none;
            EG2Controller.controller_number = 0;
            EG2ControllerInvert             = false;
            EG2ControllerAttackInfluence    = 0;
            EG2ControllerDecayInfluence     = 0;
            EG2ControllerReleaseInfluence   = 0;
            LFO1Frequency                   = 1.0;
            EG2Attack                       = 0.0;
            EG2Decay1                       = 0.005;
            EG2Sustain                      = 1000;
            EG2Release                      = 60;
            LFO2ControlDepth                = 0;
            LFO2Frequency                   = 1.0;
            LFO2InternalDepth               = 0;
            EG1Decay2                       = 0.0;
            EG1InfiniteSustain              = true;
            EG1PreAttack                    = 0;
            EG2Decay2                       = 0.0;
            EG2InfiniteSustain              = true;
            EG2PreAttack                    = 0;
            VelocityResponseCurve           = curve_type_nonlinear;
            VelocityResponseDepth           = 3;
            ReleaseVelocityResponseCurve    = curve_type_nonlinear;
            ReleaseVelocityResponseDepth    = 3;
            VelocityResponseCurveScaling    = 32;
            AttenuationControllerThreshold  = 0;
            SampleStartOffset               = 0;
            PitchTrack                      = true;
            DimensionBypass                 = dim_bypass_ctrl_none;
            Pan                             = 0;
            SelfMask                        = true;
            LFO3Controller                  = lfo3_ctrl_modwheel;
            LFO3Sync                        = false;
            InvertAttenuationController     = false;
            AttenuationController.type      = attenuation_ctrl_t::type_none;
            AttenuationController.controller_number = 0;
            LFO2Controller                  = lfo2_ctrl_internal;
            LFO2FlipPhase                   = false;
            LFO2Sync                        = false;
            LFO1Controller                  = lfo1_ctrl_internal;
            LFO1FlipPhase                   = false;
            LFO1Sync                        = false;
            VCFResonanceController          = vcf_res_ctrl_none;
            EG3Depth                        = 0;
            ChannelOffset                   = 0;
            MSDecode                        = false;
            SustainDefeat                   = false;
            VelocityUpperLimit              = 0;
            ReleaseTriggerDecay             = 0;
            EG1Hold                         = false;
            VCFEnabled                      = false;
            VCFCutoff                       = 0;
            VCFCutoffController             = vcf_cutoff_ctrl_none;
            VCFCutoffControllerInvert       = false;
            VCFVelocityScale                = 0;
            VCFResonance                    = 0;
            VCFResonanceDynamic             = false;
            VCFKeyboardTracking             = false;
            VCFKeyboardTrackingBreakpoint   = 0;
            VCFVelocityDynamicRange         = 0x04;
            VCFVelocityCurve                = curve_type_linear;
            VCFType                         = vcf_type_lowpass;
            memset(DimensionUpperLimits, 127, 8);
        }

        // chunk for own format extensions, these will *NOT* work with Gigasampler/GigaStudio !
        RIFF::Chunk* lsde = _3ewl->GetSubChunk(CHUNK_ID_LSDE);
        if (lsde) { // format extension for EG behavior options
            lsde->SetPos(0);

            eg_opt_t* pEGOpts[2] = { &EG1Options, &EG2Options };
            for (int i = 0; i < 2; ++i) { // NOTE: we reserved a 3rd byte for a potential future EG3 option
                unsigned char byte = lsde->ReadUint8();
                pEGOpts[i]->AttackCancel     = byte & 1;
                pEGOpts[i]->AttackHoldCancel = byte & (1 << 1);
                pEGOpts[i]->Decay1Cancel     = byte & (1 << 2);
                pEGOpts[i]->Decay2Cancel     = byte & (1 << 3);
                pEGOpts[i]->ReleaseCancel    = byte & (1 << 4);
            }
        }
        // format extension for sustain pedal up effect on release trigger samples
        if (lsde && lsde->GetSize() > 3) { // NOTE: we reserved the 3rd byte for a potential future EG3 option
            lsde->SetPos(3);
            uint8_t byte = lsde->ReadUint8();
            SustainReleaseTrigger   = static_cast<sust_rel_trg_t>(byte & 0x03);
            NoNoteOffReleaseTrigger = byte >> 7;
        } else {
            SustainReleaseTrigger   = sust_rel_trg_none;
            NoNoteOffReleaseTrigger = false;
        }
        // format extension for LFOs' wave form, phase displacement and for
        // LFO3's flip phase
        if (lsde && lsde->GetSize() > 4) {
            lsde->SetPos(4);
            LFO1WaveForm = static_cast<lfo_wave_t>( lsde->ReadUint16() );
            LFO2WaveForm = static_cast<lfo_wave_t>( lsde->ReadUint16() );
            LFO3WaveForm = static_cast<lfo_wave_t>( lsde->ReadUint16() );
            lsde->ReadUint16(); // unused 16 bits, reserved for potential future use
            LFO1Phase = (double) GIG_EXP_DECODE( lsde->ReadInt32() );
            LFO2Phase = (double) GIG_EXP_DECODE( lsde->ReadInt32() );
            LFO3Phase = (double) GIG_EXP_DECODE( lsde->ReadInt32() );
            const uint32_t flags = lsde->ReadInt32();
            LFO3FlipPhase = flags & 1;
        } else {
            LFO1WaveForm = lfo_wave_sine;
            LFO2WaveForm = lfo_wave_sine;
            LFO3WaveForm = lfo_wave_sine;
            LFO1Phase = 0.0;
            LFO2Phase = 0.0;
            LFO3Phase = 0.0;
            LFO3FlipPhase = false;
        }

        pVelocityAttenuationTable = GetVelocityTable(VelocityResponseCurve,
                                                     VelocityResponseDepth,
                                                     VelocityResponseCurveScaling);

        pVelocityReleaseTable = GetReleaseVelocityTable(
                                    ReleaseVelocityResponseCurve,
                                    ReleaseVelocityResponseDepth
                                );

        pVelocityCutoffTable = GetCutoffVelocityTable(VCFVelocityCurve,
                                                      VCFVelocityDynamicRange,
                                                      VCFVelocityScale,
                                                      VCFCutoffController);

        SampleAttenuation = pow(10.0, -Gain / (20.0 * 655360));
        VelocityTable = 0;
    }

    /*
     * Constructs a DimensionRegion by copying all parameters from
     * another DimensionRegion
     */
    DimensionRegion::DimensionRegion(RIFF::List* _3ewl, const DimensionRegion& src) : DLS::Sampler(_3ewl) {
        Instances++;
        //NOTE: I think we cannot call CopyAssign() here (in a constructor) as long as its a virtual method
        *this = src; // default memberwise shallow copy of all parameters
        pParentList = _3ewl; // restore the chunk pointer

        // deep copy of owned structures
        if (src.VelocityTable) {
            VelocityTable = new uint8_t[128];
            for (int k = 0 ; k < 128 ; k++)
                VelocityTable[k] = src.VelocityTable[k];
        }
        if (src.pSampleLoops) {
            pSampleLoops = new DLS::sample_loop_t[src.SampleLoops];
            for (int k = 0 ; k < src.SampleLoops ; k++)
                pSampleLoops[k] = src.pSampleLoops[k];
        }
    }
    
    /**
     * Make a (semi) deep copy of the DimensionRegion object given by @a orig
     * and assign it to this object.
     *
     * Note that all sample pointers referenced by @a orig are simply copied as
     * memory address. Thus the respective samples are shared, not duplicated!
     *
     * @param orig - original DimensionRegion object to be copied from
     */
    void DimensionRegion::CopyAssign(const DimensionRegion* orig) {
        CopyAssign(orig, NULL);
    }

    /**
     * Make a (semi) deep copy of the DimensionRegion object given by @a orig
     * and assign it to this object.
     *
     * @param orig - original DimensionRegion object to be copied from
     * @param mSamples - crosslink map between the foreign file's samples and
     *                   this file's samples
     */
    void DimensionRegion::CopyAssign(const DimensionRegion* orig, const std::map<Sample*,Sample*>* mSamples) {
        // delete all allocated data first
        if (VelocityTable) delete [] VelocityTable;
        if (pSampleLoops) delete [] pSampleLoops;
        
        // backup parent list pointer
        RIFF::List* p = pParentList;
        
        gig::Sample* pOriginalSample = pSample;
        gig::Region* pOriginalRegion = pRegion;
        
        //NOTE: copy code copied from assignment constructor above, see comment there as well
        
        *this = *orig; // default memberwise shallow copy of all parameters
        
        // restore members that shall not be altered
        pParentList = p; // restore the chunk pointer
        pRegion = pOriginalRegion;
        
        // only take the raw sample reference reference if the
        // two DimensionRegion objects are part of the same file
        if (pOriginalRegion->GetParent()->GetParent() != orig->pRegion->GetParent()->GetParent()) {
            pSample = pOriginalSample;
        }
        
        if (mSamples && mSamples->count(orig->pSample)) {
            pSample = mSamples->find(orig->pSample)->second;
        }

        // deep copy of owned structures
        if (orig->VelocityTable) {
            VelocityTable = new uint8_t[128];
            for (int k = 0 ; k < 128 ; k++)
                VelocityTable[k] = orig->VelocityTable[k];
        }
        if (orig->pSampleLoops) {
            pSampleLoops = new DLS::sample_loop_t[orig->SampleLoops];
            for (int k = 0 ; k < orig->SampleLoops ; k++)
                pSampleLoops[k] = orig->pSampleLoops[k];
        }
    }

    void DimensionRegion::serialize(Serialization::Archive* archive) {
        // in case this class will become backward incompatible one day,
        // then set a version and minimum version for this class like:
        //archive->setVersion(*this, 2);
        //archive->setMinVersion(*this, 1);

        SRLZ(VelocityUpperLimit);
        SRLZ(EG1PreAttack);
        SRLZ(EG1Attack);
        SRLZ(EG1Decay1);
        SRLZ(EG1Decay2);
        SRLZ(EG1InfiniteSustain);
        SRLZ(EG1Sustain);
        SRLZ(EG1Release);
        SRLZ(EG1Hold);
        SRLZ(EG1Controller);
        SRLZ(EG1ControllerInvert);
        SRLZ(EG1ControllerAttackInfluence);
        SRLZ(EG1ControllerDecayInfluence);
        SRLZ(EG1ControllerReleaseInfluence);
        SRLZ(LFO1WaveForm);
        SRLZ(LFO1Frequency);
        SRLZ(LFO1Phase);
        SRLZ(LFO1InternalDepth);
        SRLZ(LFO1ControlDepth);
        SRLZ(LFO1Controller);
        SRLZ(LFO1FlipPhase);
        SRLZ(LFO1Sync);
        SRLZ(EG2PreAttack);
        SRLZ(EG2Attack);
        SRLZ(EG2Decay1);
        SRLZ(EG2Decay2);
        SRLZ(EG2InfiniteSustain);
        SRLZ(EG2Sustain);
        SRLZ(EG2Release);
        SRLZ(EG2Controller);
        SRLZ(EG2ControllerInvert);
        SRLZ(EG2ControllerAttackInfluence);
        SRLZ(EG2ControllerDecayInfluence);
        SRLZ(EG2ControllerReleaseInfluence);
        SRLZ(LFO2WaveForm);
        SRLZ(LFO2Frequency);
        SRLZ(LFO2Phase);
        SRLZ(LFO2InternalDepth);
        SRLZ(LFO2ControlDepth);
        SRLZ(LFO2Controller);
        SRLZ(LFO2FlipPhase);
        SRLZ(LFO2Sync);
        SRLZ(EG3Attack);
        SRLZ(EG3Depth);
        SRLZ(LFO3WaveForm);
        SRLZ(LFO3Frequency);
        SRLZ(LFO3Phase);
        SRLZ(LFO3InternalDepth);
        SRLZ(LFO3ControlDepth);
        SRLZ(LFO3Controller);
        SRLZ(LFO3FlipPhase);
        SRLZ(LFO3Sync);
        SRLZ(VCFEnabled);
        SRLZ(VCFType);
        SRLZ(VCFCutoffController);
        SRLZ(VCFCutoffControllerInvert);
        SRLZ(VCFCutoff);
        SRLZ(VCFVelocityCurve);
        SRLZ(VCFVelocityScale);
        SRLZ(VCFVelocityDynamicRange);
        SRLZ(VCFResonance);
        SRLZ(VCFResonanceDynamic);
        SRLZ(VCFResonanceController);
        SRLZ(VCFKeyboardTracking);
        SRLZ(VCFKeyboardTrackingBreakpoint);
        SRLZ(VelocityResponseCurve);
        SRLZ(VelocityResponseDepth);
        SRLZ(VelocityResponseCurveScaling);
        SRLZ(ReleaseVelocityResponseCurve);
        SRLZ(ReleaseVelocityResponseDepth);
        SRLZ(ReleaseTriggerDecay);
        SRLZ(Crossfade);
        SRLZ(PitchTrack);
        SRLZ(DimensionBypass);
        SRLZ(Pan);
        SRLZ(SelfMask);
        SRLZ(AttenuationController);
        SRLZ(InvertAttenuationController);
        SRLZ(AttenuationControllerThreshold);
        SRLZ(ChannelOffset);
        SRLZ(SustainDefeat);
        SRLZ(MSDecode);
        //SRLZ(SampleStartOffset);
        SRLZ(SampleAttenuation);
        SRLZ(EG1Options);
        SRLZ(EG2Options);
        SRLZ(SustainReleaseTrigger);
        SRLZ(NoNoteOffReleaseTrigger);

        // derived attributes from DLS::Sampler
        SRLZ(FineTune);
        SRLZ(Gain);
    }

    /**
     * Updates the respective member variable and updates @c SampleAttenuation
     * which depends on this value.
     */
    void DimensionRegion::SetGain(int32_t gain) {
        DLS::Sampler::SetGain(gain);
        SampleAttenuation = pow(10.0, -Gain / (20.0 * 655360));
    }

    /**
     * Apply dimension region settings to the respective RIFF chunks. You
     * have to call File::Save() to make changes persistent.
     *
     * Usually there is absolutely no need to call this method explicitly.
     * It will be called automatically when File::Save() was called.
     *
     * @param pProgress - callback function for progress notification
     */
    void DimensionRegion::UpdateChunks(progress_t* pProgress) {
        // first update base class's chunk
        DLS::Sampler::UpdateChunks(pProgress);

        RIFF::Chunk* wsmp = pParentList->GetSubChunk(CHUNK_ID_WSMP);
        uint8_t* pData = (uint8_t*) wsmp->LoadChunkData();
        pData[12] = Crossfade.in_start;
        pData[13] = Crossfade.in_end;
        pData[14] = Crossfade.out_start;
        pData[15] = Crossfade.out_end;

        // make sure '3ewa' chunk exists
        RIFF::Chunk* _3ewa = pParentList->GetSubChunk(CHUNK_ID_3EWA);
        if (!_3ewa) {
            File* pFile = (File*) GetParent()->GetParent()->GetParent();
            bool versiongt2 = pFile->pVersion && pFile->pVersion->major > 2;
            _3ewa = pParentList->AddSubChunk(CHUNK_ID_3EWA, versiongt2 ? 148 : 140);
        }
        pData = (uint8_t*) _3ewa->LoadChunkData();

        // update '3ewa' chunk with DimensionRegion's current settings

        const uint32_t chunksize = (uint32_t) _3ewa->GetNewSize();
        store32(&pData[0], chunksize); // unknown, always chunk size?

        const int32_t lfo3freq = (int32_t) GIG_EXP_ENCODE(LFO3Frequency);
        store32(&pData[4], lfo3freq);

        const int32_t eg3attack = (int32_t) GIG_EXP_ENCODE(EG3Attack);
        store32(&pData[8], eg3attack);

        // next 2 bytes unknown

        store16(&pData[14], LFO1InternalDepth);

        // next 2 bytes unknown

        store16(&pData[18], LFO3InternalDepth);

        // next 2 bytes unknown

        store16(&pData[22], LFO1ControlDepth);

        // next 2 bytes unknown

        store16(&pData[26], LFO3ControlDepth);

        const int32_t eg1attack = (int32_t) GIG_EXP_ENCODE(EG1Attack);
        store32(&pData[28], eg1attack);

        const int32_t eg1decay1 = (int32_t) GIG_EXP_ENCODE(EG1Decay1);
        store32(&pData[32], eg1decay1);

        // next 2 bytes unknown

        store16(&pData[38], EG1Sustain);

        const int32_t eg1release = (int32_t) GIG_EXP_ENCODE(EG1Release);
        store32(&pData[40], eg1release);

        const uint8_t eg1ctl = (uint8_t) EncodeLeverageController(EG1Controller);
        pData[44] = eg1ctl;

        const uint8_t eg1ctrloptions =
            (EG1ControllerInvert ? 0x01 : 0x00) |
            GIG_EG_CTR_ATTACK_INFLUENCE_ENCODE(EG1ControllerAttackInfluence) |
            GIG_EG_CTR_DECAY_INFLUENCE_ENCODE(EG1ControllerDecayInfluence) |
            GIG_EG_CTR_RELEASE_INFLUENCE_ENCODE(EG1ControllerReleaseInfluence);
        pData[45] = eg1ctrloptions;

        const uint8_t eg2ctl = (uint8_t) EncodeLeverageController(EG2Controller);
        pData[46] = eg2ctl;

        const uint8_t eg2ctrloptions =
            (EG2ControllerInvert ? 0x01 : 0x00) |
            GIG_EG_CTR_ATTACK_INFLUENCE_ENCODE(EG2ControllerAttackInfluence) |
            GIG_EG_CTR_DECAY_INFLUENCE_ENCODE(EG2ControllerDecayInfluence) |
            GIG_EG_CTR_RELEASE_INFLUENCE_ENCODE(EG2ControllerReleaseInfluence);
        pData[47] = eg2ctrloptions;

        const int32_t lfo1freq = (int32_t) GIG_EXP_ENCODE(LFO1Frequency);
        store32(&pData[48], lfo1freq);

        const int32_t eg2attack = (int32_t) GIG_EXP_ENCODE(EG2Attack);
        store32(&pData[52], eg2attack);

        const int32_t eg2decay1 = (int32_t) GIG_EXP_ENCODE(EG2Decay1);
        store32(&pData[56], eg2decay1);

        // next 2 bytes unknown

        store16(&pData[62], EG2Sustain);

        const int32_t eg2release = (int32_t) GIG_EXP_ENCODE(EG2Release);
        store32(&pData[64], eg2release);

        // next 2 bytes unknown

        store16(&pData[70], LFO2ControlDepth);

        const int32_t lfo2freq = (int32_t) GIG_EXP_ENCODE(LFO2Frequency);
        store32(&pData[72], lfo2freq);

        // next 2 bytes unknown

        store16(&pData[78], LFO2InternalDepth);

        const int32_t eg1decay2 = (int32_t) (EG1InfiniteSustain) ? 0x7fffffff : (int32_t) GIG_EXP_ENCODE(EG1Decay2);
        store32(&pData[80], eg1decay2);

        // next 2 bytes unknown

        store16(&pData[86], EG1PreAttack);

        const int32_t eg2decay2 = (int32_t) (EG2InfiniteSustain) ? 0x7fffffff : (int32_t) GIG_EXP_ENCODE(EG2Decay2);
        store32(&pData[88], eg2decay2);

        // next 2 bytes unknown

        store16(&pData[94], EG2PreAttack);

        {
            if (VelocityResponseDepth > 4) throw Exception("VelocityResponseDepth must be between 0 and 4");
            uint8_t velocityresponse = VelocityResponseDepth;
            switch (VelocityResponseCurve) {
                case curve_type_nonlinear:
                    break;
                case curve_type_linear:
                    velocityresponse += 5;
                    break;
                case curve_type_special:
                    velocityresponse += 10;
                    break;
                case curve_type_unknown:
                default:
                    throw Exception("Could not update DimensionRegion's chunk, unknown VelocityResponseCurve selected");
            }
            pData[96] = velocityresponse;
        }

        {
            if (ReleaseVelocityResponseDepth > 4) throw Exception("ReleaseVelocityResponseDepth must be between 0 and 4");
            uint8_t releasevelocityresponse = ReleaseVelocityResponseDepth;
            switch (ReleaseVelocityResponseCurve) {
                case curve_type_nonlinear:
                    break;
                case curve_type_linear:
                    releasevelocityresponse += 5;
                    break;
                case curve_type_special:
                    releasevelocityresponse += 10;
                    break;
                case curve_type_unknown:
                default:
                    throw Exception("Could not update DimensionRegion's chunk, unknown ReleaseVelocityResponseCurve selected");
            }
            pData[97] = releasevelocityresponse;
        }

        pData[98] = VelocityResponseCurveScaling;

        pData[99] = AttenuationControllerThreshold;

        // next 4 bytes unknown

        store16(&pData[104], SampleStartOffset);

        // next 2 bytes unknown

        {
            uint8_t pitchTrackDimensionBypass = GIG_PITCH_TRACK_ENCODE(PitchTrack);
            switch (DimensionBypass) {
                case dim_bypass_ctrl_94:
                    pitchTrackDimensionBypass |= 0x10;
                    break;
                case dim_bypass_ctrl_95:
                    pitchTrackDimensionBypass |= 0x20;
                    break;
                case dim_bypass_ctrl_none:
                    //FIXME: should we set anything here?
                    break;
                default:
                    throw Exception("Could not update DimensionRegion's chunk, unknown DimensionBypass selected");
            }
            pData[108] = pitchTrackDimensionBypass;
        }

        const uint8_t pan = (Pan >= 0) ? Pan : ((-Pan) + 63); // signed 8 bit -> signed 7 bit
        pData[109] = pan;

        const uint8_t selfmask = (SelfMask) ? 0x01 : 0x00;
        pData[110] = selfmask;

        // next byte unknown

        {
            uint8_t lfo3ctrl = LFO3Controller & 0x07; // lower 3 bits
            if (LFO3Sync) lfo3ctrl |= 0x20; // bit 5
            if (InvertAttenuationController) lfo3ctrl |= 0x80; // bit 7
            if (VCFType == vcf_type_lowpassturbo) lfo3ctrl |= 0x40; // bit 6
            pData[112] = lfo3ctrl;
        }

        const uint8_t attenctl = EncodeLeverageController(AttenuationController);
        pData[113] = attenctl;

        {
            uint8_t lfo2ctrl = LFO2Controller & 0x07; // lower 3 bits
            if (LFO2FlipPhase) lfo2ctrl |= 0x80; // bit 7
            if (LFO2Sync)      lfo2ctrl |= 0x20; // bit 5
            if (VCFResonanceController != vcf_res_ctrl_none) lfo2ctrl |= 0x40; // bit 6
            pData[114] = lfo2ctrl;
        }

        {
            uint8_t lfo1ctrl = LFO1Controller & 0x07; // lower 3 bits
            if (LFO1FlipPhase) lfo1ctrl |= 0x80; // bit 7
            if (LFO1Sync)      lfo1ctrl |= 0x40; // bit 6
            if (VCFResonanceController != vcf_res_ctrl_none)
                lfo1ctrl |= GIG_VCF_RESONANCE_CTRL_ENCODE(VCFResonanceController);
            pData[115] = lfo1ctrl;
        }

        const uint16_t eg3depth = (EG3Depth >= 0) ? EG3Depth
                                                  : uint16_t(((-EG3Depth) - 1) ^ 0xfff); /* binary complementary for negatives */
        store16(&pData[116], eg3depth);

        // next 2 bytes unknown

        const uint8_t channeloffset = ChannelOffset * 4;
        pData[120] = channeloffset;

        {
            uint8_t regoptions = 0;
            if (MSDecode)      regoptions |= 0x01; // bit 0
            if (SustainDefeat) regoptions |= 0x02; // bit 1
            pData[121] = regoptions;
        }

        // next 2 bytes unknown

        pData[124] = VelocityUpperLimit;

        // next 3 bytes unknown

        pData[128] = ReleaseTriggerDecay;

        // next 2 bytes unknown

        const uint8_t eg1hold = (EG1Hold) ? 0x80 : 0x00; // bit 7
        pData[131] = eg1hold;

        const uint8_t vcfcutoff = (VCFEnabled ? 0x80 : 0x00) |  /* bit 7 */
                                  (VCFCutoff & 0x7f);   /* lower 7 bits */
        pData[132] = vcfcutoff;

        pData[133] = VCFCutoffController;

        const uint8_t vcfvelscale = (VCFCutoffControllerInvert ? 0x80 : 0x00) | /* bit 7 */
                                    (VCFVelocityScale & 0x7f); /* lower 7 bits */
        pData[134] = vcfvelscale;

        // next byte unknown

        const uint8_t vcfresonance = (VCFResonanceDynamic ? 0x00 : 0x80) | /* bit 7 */
                                     (VCFResonance & 0x7f); /* lower 7 bits */
        pData[136] = vcfresonance;

        const uint8_t vcfbreakpoint = (VCFKeyboardTracking ? 0x80 : 0x00) | /* bit 7 */
                                      (VCFKeyboardTrackingBreakpoint & 0x7f); /* lower 7 bits */
        pData[137] = vcfbreakpoint;

        const uint8_t vcfvelocity = VCFVelocityDynamicRange % 5 +
                                    VCFVelocityCurve * 5;
        pData[138] = vcfvelocity;

        const uint8_t vcftype = (VCFType == vcf_type_lowpassturbo) ? vcf_type_lowpass : VCFType;
        pData[139] = vcftype;

        if (chunksize >= 148) {
            memcpy(&pData[140], DimensionUpperLimits, 8);
        }

        // chunk for own format extensions, these will *NOT* work with
        // Gigasampler/GigaStudio !
        RIFF::Chunk* lsde = pParentList->GetSubChunk(CHUNK_ID_LSDE);
        const int lsdeSize =
            3 /* EG cancel options */ +
            1 /* sustain pedal up on release trigger option */ +
            8 /* LFOs' wave forms */ + 12 /* LFOs' phase */ + 4 /* flags (LFO3FlipPhase) */;
        if (!lsde && UsesAnyGigFormatExtension()) {
            // only add this "LSDE" chunk if there is some (format extension)
            // setting effective that would require our "LSDE" format extension
            // chunk to be stored
            lsde = pParentList->AddSubChunk(CHUNK_ID_LSDE, lsdeSize);
            // move LSDE chunk to the end of parent list
            pParentList->MoveSubChunk(lsde, (RIFF::Chunk*)NULL);
        }
        if (lsde) {
            if (lsde->GetNewSize() < lsdeSize)
                lsde->Resize(lsdeSize);
            // format extension for EG behavior options
            unsigned char* pData = (unsigned char*) lsde->LoadChunkData();
            eg_opt_t* pEGOpts[2] = { &EG1Options, &EG2Options };
            for (int i = 0; i < 2; ++i) { // NOTE: we reserved the 3rd byte for a potential future EG3 option
                pData[i] =
                    (pEGOpts[i]->AttackCancel     ? 1 : 0) |
                    (pEGOpts[i]->AttackHoldCancel ? (1<<1) : 0) |
                    (pEGOpts[i]->Decay1Cancel     ? (1<<2) : 0) |
                    (pEGOpts[i]->Decay2Cancel     ? (1<<3) : 0) |
                    (pEGOpts[i]->ReleaseCancel    ? (1<<4) : 0);
            }
            // format extension for release trigger options
            pData[3] = static_cast<uint8_t>(SustainReleaseTrigger) | (NoNoteOffReleaseTrigger ? (1<<7) : 0);
            // format extension for LFOs' wave form, phase displacement and for
            // LFO3's flip phase
            store16(&pData[4], LFO1WaveForm);
            store16(&pData[6], LFO2WaveForm);
            store16(&pData[8], LFO3WaveForm);
            //NOTE: 16 bits reserved here for potential future use !
            const int32_t lfo1Phase = (int32_t) GIG_EXP_ENCODE(LFO1Phase);
            const int32_t lfo2Phase = (int32_t) GIG_EXP_ENCODE(LFO2Phase);
            const int32_t lfo3Phase = (int32_t) GIG_EXP_ENCODE(LFO3Phase);
            store32(&pData[12], lfo1Phase);
            store32(&pData[16], lfo2Phase);
            store32(&pData[20], lfo3Phase);
            const int32_t flags = LFO3FlipPhase ? 1 : 0;
            store32(&pData[24], flags);

            // compile time sanity check: is our last store access here
            // consistent with the initial lsdeSize value assignment?
            static_assert(lsdeSize == 28, "Inconsistency in assumed 'LSDE' RIFF chunk size");
        }
    }

    /**
     * Returns @c true in case this DimensionRegion object uses any gig format
     * extension, that is whether this DimensionRegion object currently has any
     * setting effective that would require our "LSDE" RIFF chunk to be stored
     * to the gig file.
     *
     * Right now this is a private method. It is considerable though this method
     * to become (in slightly modified form) a public API method in future, i.e.
     * to allow instrument editors to visualize and/or warn the user of any
     * format extension being used. Right now this method really just serves to
     * answer the question whether an LSDE chunk is required, for the public API
     * purpose this method would also need to check whether any other setting
     * stored to the regular value '3ewa' chunk, is actually a format extension
     * as well.
     */
    bool DimensionRegion::UsesAnyGigFormatExtension() const {
        eg_opt_t defaultOpt;
        return memcmp(&EG1Options, &defaultOpt, sizeof(eg_opt_t)) ||
               memcmp(&EG2Options, &defaultOpt, sizeof(eg_opt_t)) ||
               SustainReleaseTrigger || NoNoteOffReleaseTrigger ||
               LFO1WaveForm || LFO2WaveForm || LFO3WaveForm ||
               LFO1Phase || LFO2Phase || LFO3Phase ||
               LFO3FlipPhase;
    }

    double* DimensionRegion::GetReleaseVelocityTable(curve_type_t releaseVelocityResponseCurve, uint8_t releaseVelocityResponseDepth) {
        curve_type_t curveType = releaseVelocityResponseCurve;
        uint8_t depth = releaseVelocityResponseDepth;
        // this models a strange behaviour or bug in GSt: two of the
        // velocity response curves for release time are not used even
        // if specified, instead another curve is chosen.
        if ((curveType == curve_type_nonlinear && depth == 0) ||
            (curveType == curve_type_special   && depth == 4)) {
            curveType = curve_type_nonlinear;
            depth = 3;
        }
        return GetVelocityTable(curveType, depth, 0);
    }

    double* DimensionRegion::GetCutoffVelocityTable(curve_type_t vcfVelocityCurve,
                                                    uint8_t vcfVelocityDynamicRange,
                                                    uint8_t vcfVelocityScale,
                                                    vcf_cutoff_ctrl_t vcfCutoffController)
    {
        curve_type_t curveType = vcfVelocityCurve;
        uint8_t depth = vcfVelocityDynamicRange;
        // even stranger GSt: two of the velocity response curves for
        // filter cutoff are not used, instead another special curve
        // is chosen. This curve is not used anywhere else.
        if ((curveType == curve_type_nonlinear && depth == 0) ||
            (curveType == curve_type_special   && depth == 4)) {
            curveType = curve_type_special;
            depth = 5;
        }
        return GetVelocityTable(curveType, depth,
                                (vcfCutoffController <= vcf_cutoff_ctrl_none2)
                                    ? vcfVelocityScale : 0);
    }

    // get the corresponding velocity table from the table map or create & calculate that table if it doesn't exist yet
    double* DimensionRegion::GetVelocityTable(curve_type_t curveType, uint8_t depth, uint8_t scaling)
    {
        // sanity check input parameters
        // (fallback to some default parameters on ill input)
        switch (curveType) {
            case curve_type_nonlinear:
            case curve_type_linear:
                if (depth > 4) {
                    printf("Warning: Invalid depth (0x%x) for velocity curve type (0x%x).\n", depth, curveType);
                    depth   = 0;
                    scaling = 0;
                }
                break;
            case curve_type_special:
                if (depth > 5) {
                    printf("Warning: Invalid depth (0x%x) for velocity curve type 'special'.\n", depth);
                    depth   = 0;
                    scaling = 0;
                }
                break;
            case curve_type_unknown:
            default:
                printf("Warning: Unknown velocity curve type (0x%x).\n", curveType);
                curveType = curve_type_linear;
                depth     = 0;
                scaling   = 0;
                break;
        }

        double* table;
        uint32_t tableKey = (curveType<<16) | (depth<<8) | scaling;
        if (pVelocityTables->count(tableKey)) { // if key exists
            table = (*pVelocityTables)[tableKey];
        }
        else {
            table = CreateVelocityTable(curveType, depth, scaling);
            (*pVelocityTables)[tableKey] = table; // put the new table into the tables map
        }
        return table;
    }

    Region* DimensionRegion::GetParent() const {
        return pRegion;
    }

// show error if some _lev_ctrl_* enum entry is not listed in the following function
// (commented out for now, because "diagnostic push" not supported prior GCC 4.6)
// TODO: uncomment and add a GCC version check (see also commented "#pragma GCC diagnostic pop" below)
//#pragma GCC diagnostic push
//#pragma GCC diagnostic error "-Wswitch"

    leverage_ctrl_t DimensionRegion::DecodeLeverageController(_lev_ctrl_t EncodedController) {
        leverage_ctrl_t decodedcontroller;
        switch (EncodedController) {
            // special controller
            case _lev_ctrl_none:
                decodedcontroller.type = leverage_ctrl_t::type_none;
                decodedcontroller.controller_number = 0;
                break;
            case _lev_ctrl_velocity:
                decodedcontroller.type = leverage_ctrl_t::type_velocity;
                decodedcontroller.controller_number = 0;
                break;
            case _lev_ctrl_channelaftertouch:
                decodedcontroller.type = leverage_ctrl_t::type_channelaftertouch;
                decodedcontroller.controller_number = 0;
                break;

            // ordinary MIDI control change controller
            case _lev_ctrl_modwheel:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 1;
                break;
            case _lev_ctrl_breath:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 2;
                break;
            case _lev_ctrl_foot:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 4;
                break;
            case _lev_ctrl_effect1:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 12;
                break;
            case _lev_ctrl_effect2:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 13;
                break;
            case _lev_ctrl_genpurpose1:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 16;
                break;
            case _lev_ctrl_genpurpose2:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 17;
                break;
            case _lev_ctrl_genpurpose3:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 18;
                break;
            case _lev_ctrl_genpurpose4:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 19;
                break;
            case _lev_ctrl_portamentotime:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 5;
                break;
            case _lev_ctrl_sustainpedal:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 64;
                break;
            case _lev_ctrl_portamento:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 65;
                break;
            case _lev_ctrl_sostenutopedal:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 66;
                break;
            case _lev_ctrl_softpedal:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 67;
                break;
            case _lev_ctrl_genpurpose5:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 80;
                break;
            case _lev_ctrl_genpurpose6:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 81;
                break;
            case _lev_ctrl_genpurpose7:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 82;
                break;
            case _lev_ctrl_genpurpose8:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 83;
                break;
            case _lev_ctrl_effect1depth:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 91;
                break;
            case _lev_ctrl_effect2depth:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 92;
                break;
            case _lev_ctrl_effect3depth:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 93;
                break;
            case _lev_ctrl_effect4depth:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 94;
                break;
            case _lev_ctrl_effect5depth:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 95;
                break;

            // format extension (these controllers are so far only supported by
            // LinuxSampler & gigedit) they will *NOT* work with
            // Gigasampler/GigaStudio !
            case _lev_ctrl_CC3_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 3;
                break;
            case _lev_ctrl_CC6_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 6;
                break;
            case _lev_ctrl_CC7_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 7;
                break;
            case _lev_ctrl_CC8_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 8;
                break;
            case _lev_ctrl_CC9_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 9;
                break;
            case _lev_ctrl_CC10_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 10;
                break;
            case _lev_ctrl_CC11_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 11;
                break;
            case _lev_ctrl_CC14_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 14;
                break;
            case _lev_ctrl_CC15_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 15;
                break;
            case _lev_ctrl_CC20_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 20;
                break;
            case _lev_ctrl_CC21_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 21;
                break;
            case _lev_ctrl_CC22_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 22;
                break;
            case _lev_ctrl_CC23_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 23;
                break;
            case _lev_ctrl_CC24_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 24;
                break;
            case _lev_ctrl_CC25_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 25;
                break;
            case _lev_ctrl_CC26_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 26;
                break;
            case _lev_ctrl_CC27_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 27;
                break;
            case _lev_ctrl_CC28_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 28;
                break;
            case _lev_ctrl_CC29_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 29;
                break;
            case _lev_ctrl_CC30_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 30;
                break;
            case _lev_ctrl_CC31_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 31;
                break;
            case _lev_ctrl_CC68_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 68;
                break;
            case _lev_ctrl_CC69_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 69;
                break;
            case _lev_ctrl_CC70_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 70;
                break;
            case _lev_ctrl_CC71_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 71;
                break;
            case _lev_ctrl_CC72_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 72;
                break;
            case _lev_ctrl_CC73_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 73;
                break;
            case _lev_ctrl_CC74_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 74;
                break;
            case _lev_ctrl_CC75_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 75;
                break;
            case _lev_ctrl_CC76_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 76;
                break;
            case _lev_ctrl_CC77_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 77;
                break;
            case _lev_ctrl_CC78_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 78;
                break;
            case _lev_ctrl_CC79_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 79;
                break;
            case _lev_ctrl_CC84_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 84;
                break;
            case _lev_ctrl_CC85_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 85;
                break;
            case _lev_ctrl_CC86_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 86;
                break;
            case _lev_ctrl_CC87_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 87;
                break;
            case _lev_ctrl_CC89_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 89;
                break;
            case _lev_ctrl_CC90_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 90;
                break;
            case _lev_ctrl_CC96_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 96;
                break;
            case _lev_ctrl_CC97_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 97;
                break;
            case _lev_ctrl_CC102_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 102;
                break;
            case _lev_ctrl_CC103_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 103;
                break;
            case _lev_ctrl_CC104_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 104;
                break;
            case _lev_ctrl_CC105_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 105;
                break;
            case _lev_ctrl_CC106_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 106;
                break;
            case _lev_ctrl_CC107_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 107;
                break;
            case _lev_ctrl_CC108_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 108;
                break;
            case _lev_ctrl_CC109_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 109;
                break;
            case _lev_ctrl_CC110_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 110;
                break;
            case _lev_ctrl_CC111_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 111;
                break;
            case _lev_ctrl_CC112_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 112;
                break;
            case _lev_ctrl_CC113_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 113;
                break;
            case _lev_ctrl_CC114_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 114;
                break;
            case _lev_ctrl_CC115_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 115;
                break;
            case _lev_ctrl_CC116_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 116;
                break;
            case _lev_ctrl_CC117_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 117;
                break;
            case _lev_ctrl_CC118_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 118;
                break;
            case _lev_ctrl_CC119_EXT:
                decodedcontroller.type = leverage_ctrl_t::type_controlchange;
                decodedcontroller.controller_number = 119;
                break;

            // unknown controller type
            default:
                decodedcontroller.type = leverage_ctrl_t::type_none;
                decodedcontroller.controller_number = 0;
                printf("Warning: Unknown leverage controller type (0x%x).\n", EncodedController);
                break;
        }
        return decodedcontroller;
    }
    
// see above (diagnostic push not supported prior GCC 4.6)
//#pragma GCC diagnostic pop

    DimensionRegion::_lev_ctrl_t DimensionRegion::EncodeLeverageController(leverage_ctrl_t DecodedController) {
        _lev_ctrl_t encodedcontroller;
        switch (DecodedController.type) {
            // special controller
            case leverage_ctrl_t::type_none:
                encodedcontroller = _lev_ctrl_none;
                break;
            case leverage_ctrl_t::type_velocity:
                encodedcontroller = _lev_ctrl_velocity;
                break;
            case leverage_ctrl_t::type_channelaftertouch:
                encodedcontroller = _lev_ctrl_channelaftertouch;
                break;

            // ordinary MIDI control change controller
            case leverage_ctrl_t::type_controlchange:
                switch (DecodedController.controller_number) {
                    case 1:
                        encodedcontroller = _lev_ctrl_modwheel;
                        break;
                    case 2:
                        encodedcontroller = _lev_ctrl_breath;
                        break;
                    case 4:
                        encodedcontroller = _lev_ctrl_foot;
                        break;
                    case 12:
                        encodedcontroller = _lev_ctrl_effect1;
                        break;
                    case 13:
                        encodedcontroller = _lev_ctrl_effect2;
                        break;
                    case 16:
                        encodedcontroller = _lev_ctrl_genpurpose1;
                        break;
                    case 17:
                        encodedcontroller = _lev_ctrl_genpurpose2;
                        break;
                    case 18:
                        encodedcontroller = _lev_ctrl_genpurpose3;
                        break;
                    case 19:
                        encodedcontroller = _lev_ctrl_genpurpose4;
                        break;
                    case 5:
                        encodedcontroller = _lev_ctrl_portamentotime;
                        break;
                    case 64:
                        encodedcontroller = _lev_ctrl_sustainpedal;
                        break;
                    case 65:
                        encodedcontroller = _lev_ctrl_portamento;
                        break;
                    case 66:
                        encodedcontroller = _lev_ctrl_sostenutopedal;
                        break;
                    case 67:
                        encodedcontroller = _lev_ctrl_softpedal;
                        break;
                    case 80:
                        encodedcontroller = _lev_ctrl_genpurpose5;
                        break;
                    case 81:
                        encodedcontroller = _lev_ctrl_genpurpose6;
                        break;
                    case 82:
                        encodedcontroller = _lev_ctrl_genpurpose7;
                        break;
                    case 83:
                        encodedcontroller = _lev_ctrl_genpurpose8;
                        break;
                    case 91:
                        encodedcontroller = _lev_ctrl_effect1depth;
                        break;
                    case 92:
                        encodedcontroller = _lev_ctrl_effect2depth;
                        break;
                    case 93:
                        encodedcontroller = _lev_ctrl_effect3depth;
                        break;
                    case 94:
                        encodedcontroller = _lev_ctrl_effect4depth;
                        break;
                    case 95:
                        encodedcontroller = _lev_ctrl_effect5depth;
                        break;

                    // format extension (these controllers are so far only
                    // supported by LinuxSampler & gigedit) they will *NOT*
                    // work with Gigasampler/GigaStudio !
                    case 3:
                        encodedcontroller = _lev_ctrl_CC3_EXT;
                        break;
                    case 6:
                        encodedcontroller = _lev_ctrl_CC6_EXT;
                        break;
                    case 7:
                        encodedcontroller = _lev_ctrl_CC7_EXT;
                        break;
                    case 8:
                        encodedcontroller = _lev_ctrl_CC8_EXT;
                        break;
                    case 9:
                        encodedcontroller = _lev_ctrl_CC9_EXT;
                        break;
                    case 10:
                        encodedcontroller = _lev_ctrl_CC10_EXT;
                        break;
                    case 11:
                        encodedcontroller = _lev_ctrl_CC11_EXT;
                        break;
                    case 14:
                        encodedcontroller = _lev_ctrl_CC14_EXT;
                        break;
                    case 15:
                        encodedcontroller = _lev_ctrl_CC15_EXT;
                        break;
                    case 20:
                        encodedcontroller = _lev_ctrl_CC20_EXT;
                        break;
                    case 21:
                        encodedcontroller = _lev_ctrl_CC21_EXT;
                        break;
                    case 22:
                        encodedcontroller = _lev_ctrl_CC22_EXT;
                        break;
                    case 23:
                        encodedcontroller = _lev_ctrl_CC23_EXT;
                        break;
                    case 24:
                        encodedcontroller = _lev_ctrl_CC24_EXT;
                        break;
                    case 25:
                        encodedcontroller = _lev_ctrl_CC25_EXT;
                        break;
                    case 26:
                        encodedcontroller = _lev_ctrl_CC26_EXT;
                        break;
                    case 27:
                        encodedcontroller = _lev_ctrl_CC27_EXT;
                        break;
                    case 28:
                        encodedcontroller = _lev_ctrl_CC28_EXT;
                        break;
                    case 29:
                        encodedcontroller = _lev_ctrl_CC29_EXT;
                        break;
                    case 30:
                        encodedcontroller = _lev_ctrl_CC30_EXT;
                        break;
                    case 31:
                        encodedcontroller = _lev_ctrl_CC31_EXT;
                        break;
                    case 68:
                        encodedcontroller = _lev_ctrl_CC68_EXT;
                        break;
                    case 69:
                        encodedcontroller = _lev_ctrl_CC69_EXT;
                        break;
                    case 70:
                        encodedcontroller = _lev_ctrl_CC70_EXT;
                        break;
                    case 71:
                        encodedcontroller = _lev_ctrl_CC71_EXT;
                        break;
                    case 72:
                        encodedcontroller = _lev_ctrl_CC72_EXT;
                        break;
                    case 73:
                        encodedcontroller = _lev_ctrl_CC73_EXT;
                        break;
                    case 74:
                        encodedcontroller = _lev_ctrl_CC74_EXT;
                        break;
                    case 75:
                        encodedcontroller = _lev_ctrl_CC75_EXT;
                        break;
                    case 76:
                        encodedcontroller = _lev_ctrl_CC76_EXT;
                        break;
                    case 77:
                        encodedcontroller = _lev_ctrl_CC77_EXT;
                        break;
                    case 78:
                        encodedcontroller = _lev_ctrl_CC78_EXT;
                        break;
                    case 79:
                        encodedcontroller = _lev_ctrl_CC79_EXT;
                        break;
                    case 84:
                        encodedcontroller = _lev_ctrl_CC84_EXT;
                        break;
                    case 85:
                        encodedcontroller = _lev_ctrl_CC85_EXT;
                        break;
                    case 86:
                        encodedcontroller = _lev_ctrl_CC86_EXT;
                        break;
                    case 87:
                        encodedcontroller = _lev_ctrl_CC87_EXT;
                        break;
                    case 89:
                        encodedcontroller = _lev_ctrl_CC89_EXT;
                        break;
                    case 90:
                        encodedcontroller = _lev_ctrl_CC90_EXT;
                        break;
                    case 96:
                        encodedcontroller = _lev_ctrl_CC96_EXT;
                        break;
                    case 97:
                        encodedcontroller = _lev_ctrl_CC97_EXT;
                        break;
                    case 102:
                        encodedcontroller = _lev_ctrl_CC102_EXT;
                        break;
                    case 103:
                        encodedcontroller = _lev_ctrl_CC103_EXT;
                        break;
                    case 104:
                        encodedcontroller = _lev_ctrl_CC104_EXT;
                        break;
                    case 105:
                        encodedcontroller = _lev_ctrl_CC105_EXT;
                        break;
                    case 106:
                        encodedcontroller = _lev_ctrl_CC106_EXT;
                        break;
                    case 107:
                        encodedcontroller = _lev_ctrl_CC107_EXT;
                        break;
                    case 108:
                        encodedcontroller = _lev_ctrl_CC108_EXT;
                        break;
                    case 109:
                        encodedcontroller = _lev_ctrl_CC109_EXT;
                        break;
                    case 110:
                        encodedcontroller = _lev_ctrl_CC110_EXT;
                        break;
                    case 111:
                        encodedcontroller = _lev_ctrl_CC111_EXT;
                        break;
                    case 112:
                        encodedcontroller = _lev_ctrl_CC112_EXT;
                        break;
                    case 113:
                        encodedcontroller = _lev_ctrl_CC113_EXT;
                        break;
                    case 114:
                        encodedcontroller = _lev_ctrl_CC114_EXT;
                        break;
                    case 115:
                        encodedcontroller = _lev_ctrl_CC115_EXT;
                        break;
                    case 116:
                        encodedcontroller = _lev_ctrl_CC116_EXT;
                        break;
                    case 117:
                        encodedcontroller = _lev_ctrl_CC117_EXT;
                        break;
                    case 118:
                        encodedcontroller = _lev_ctrl_CC118_EXT;
                        break;
                    case 119:
                        encodedcontroller = _lev_ctrl_CC119_EXT;
                        break;

                    default:
                        throw gig::Exception("leverage controller number is not supported by the gig format");
                }
                break;
            default:
                throw gig::Exception("Unknown leverage controller type.");
        }
        return encodedcontroller;
    }

    DimensionRegion::~DimensionRegion() {
        Instances--;
        if (!Instances) {
            // delete the velocity->volume tables
            VelocityTableMap::iterator iter;
            for (iter = pVelocityTables->begin(); iter != pVelocityTables->end(); iter++) {
                double* pTable = iter->second;
                if (pTable) delete[] pTable;
            }
            pVelocityTables->clear();
            delete pVelocityTables;
            pVelocityTables = NULL;
        }
        if (VelocityTable) delete[] VelocityTable;
    }

    /**
     * Returns the correct amplitude factor for the given \a MIDIKeyVelocity.
     * All involved parameters (VelocityResponseCurve, VelocityResponseDepth
     * and VelocityResponseCurveScaling) involved are taken into account to
     * calculate the amplitude factor. Use this method when a key was
     * triggered to get the volume with which the sample should be played
     * back.
     *
     * @param MIDIKeyVelocity  MIDI velocity value of the triggered key (between 0 and 127)
     * @returns                amplitude factor (between 0.0 and 1.0)
     */
    double DimensionRegion::GetVelocityAttenuation(uint8_t MIDIKeyVelocity) {
        return pVelocityAttenuationTable[MIDIKeyVelocity];
    }

    double DimensionRegion::GetVelocityRelease(uint8_t MIDIKeyVelocity) {
        return pVelocityReleaseTable[MIDIKeyVelocity];
    }

    double DimensionRegion::GetVelocityCutoff(uint8_t MIDIKeyVelocity) {
        return pVelocityCutoffTable[MIDIKeyVelocity];
    }

    /**
     * Updates the respective member variable and the lookup table / cache
     * that depends on this value.
     */
    void DimensionRegion::SetVelocityResponseCurve(curve_type_t curve) {
        pVelocityAttenuationTable =
            GetVelocityTable(
                curve, VelocityResponseDepth, VelocityResponseCurveScaling
            );
        VelocityResponseCurve = curve;
    }

    /**
     * Updates the respective member variable and the lookup table / cache
     * that depends on this value.
     */
    void DimensionRegion::SetVelocityResponseDepth(uint8_t depth) {
        pVelocityAttenuationTable =
            GetVelocityTable(
                VelocityResponseCurve, depth, VelocityResponseCurveScaling
            );
        VelocityResponseDepth = depth;
    }

    /**
     * Updates the respective member variable and the lookup table / cache
     * that depends on this value.
     */
    void DimensionRegion::SetVelocityResponseCurveScaling(uint8_t scaling) {
        pVelocityAttenuationTable =
            GetVelocityTable(
                VelocityResponseCurve, VelocityResponseDepth, scaling
            );
        VelocityResponseCurveScaling = scaling;
    }

    /**
     * Updates the respective member variable and the lookup table / cache
     * that depends on this value.
     */
    void DimensionRegion::SetReleaseVelocityResponseCurve(curve_type_t curve) {
        pVelocityReleaseTable = GetReleaseVelocityTable(curve, ReleaseVelocityResponseDepth);
        ReleaseVelocityResponseCurve = curve;
    }

    /**
     * Updates the respective member variable and the lookup table / cache
     * that depends on this value.
     */
    void DimensionRegion::SetReleaseVelocityResponseDepth(uint8_t depth) {
        pVelocityReleaseTable = GetReleaseVelocityTable(ReleaseVelocityResponseCurve, depth);
        ReleaseVelocityResponseDepth = depth;
    }

    /**
     * Updates the respective member variable and the lookup table / cache
     * that depends on this value.
     */
    void DimensionRegion::SetVCFCutoffController(vcf_cutoff_ctrl_t controller) {
        pVelocityCutoffTable = GetCutoffVelocityTable(VCFVelocityCurve, VCFVelocityDynamicRange, VCFVelocityScale, controller);
        VCFCutoffController = controller;
    }

    /**
     * Updates the respective member variable and the lookup table / cache
     * that depends on this value.
     */
    void DimensionRegion::SetVCFVelocityCurve(curve_type_t curve) {
        pVelocityCutoffTable = GetCutoffVelocityTable(curve, VCFVelocityDynamicRange, VCFVelocityScale, VCFCutoffController);
        VCFVelocityCurve = curve;
    }

    /**
     * Updates the respective member variable and the lookup table / cache
     * that depends on this value.
     */
    void DimensionRegion::SetVCFVelocityDynamicRange(uint8_t range) {
        pVelocityCutoffTable = GetCutoffVelocityTable(VCFVelocityCurve, range, VCFVelocityScale, VCFCutoffController);
        VCFVelocityDynamicRange = range;
    }

    /**
     * Updates the respective member variable and the lookup table / cache
     * that depends on this value.
     */
    void DimensionRegion::SetVCFVelocityScale(uint8_t scaling) {
        pVelocityCutoffTable = GetCutoffVelocityTable(VCFVelocityCurve, VCFVelocityDynamicRange, scaling, VCFCutoffController);
        VCFVelocityScale = scaling;
    }

    double* DimensionRegion::CreateVelocityTable(curve_type_t curveType, uint8_t depth, uint8_t scaling) {

        // line-segment approximations of the 15 velocity curves

        // linear
        const int lin0[] = { 1, 1, 127, 127 };
        const int lin1[] = { 1, 21, 127, 127 };
        const int lin2[] = { 1, 45, 127, 127 };
        const int lin3[] = { 1, 74, 127, 127 };
        const int lin4[] = { 1, 127, 127, 127 };

        // non-linear
        const int non0[] = { 1, 4, 24, 5, 57, 17, 92, 57, 122, 127, 127, 127 };
        const int non1[] = { 1, 4, 46, 9, 93, 56, 118, 106, 123, 127,
                             127, 127 };
        const int non2[] = { 1, 4, 46, 9, 57, 20, 102, 107, 107, 127,
                             127, 127 };
        const int non3[] = { 1, 15, 10, 19, 67, 73, 80, 80, 90, 98, 98, 127,
                             127, 127 };
        const int non4[] = { 1, 25, 33, 57, 82, 81, 92, 127, 127, 127 };

        // special
        const int spe0[] = { 1, 2, 76, 10, 90, 15, 95, 20, 99, 28, 103, 44,
                             113, 127, 127, 127 };
        const int spe1[] = { 1, 2, 27, 5, 67, 18, 89, 29, 95, 35, 107, 67,
                             118, 127, 127, 127 };
        const int spe2[] = { 1, 1, 33, 1, 53, 5, 61, 13, 69, 32, 79, 74,
                             85, 90, 91, 127, 127, 127 };
        const int spe3[] = { 1, 32, 28, 35, 66, 48, 89, 59, 95, 65, 99, 73,
                             117, 127, 127, 127 };
        const int spe4[] = { 1, 4, 23, 5, 49, 13, 57, 17, 92, 57, 122, 127,
                             127, 127 };

        // this is only used by the VCF velocity curve
        const int spe5[] = { 1, 2, 30, 5, 60, 19, 77, 70, 83, 85, 88, 106,
                             91, 127, 127, 127 };

        const int* const curves[] = { non0, non1, non2, non3, non4,
                                      lin0, lin1, lin2, lin3, lin4,
                                      spe0, spe1, spe2, spe3, spe4, spe5 };

        double* const table = new double[128];

        const int* curve = curves[curveType * 5 + depth];
        const int s = scaling == 0 ? 20 : scaling; // 0 or 20 means no scaling

        table[0] = 0;
        for (int x = 1 ; x < 128 ; x++) {

            if (x > curve[2]) curve += 2;
            double y = curve[1] + (x - curve[0]) *
                (double(curve[3] - curve[1]) / (curve[2] - curve[0]));
            y = y / 127;

            // Scale up for s > 20, down for s < 20. When
            // down-scaling, the curve still ends at 1.0.
            if (s < 20 && y >= 0.5)
                y = y / ((2 - 40.0 / s) * y + 40.0 / s - 1);
            else
                y = y * (s / 20.0);
            if (y > 1) y = 1;

            table[x] = y;
        }
        return table;
    }


// *************** Region ***************
// *

    Region::Region(Instrument* pInstrument, RIFF::List* rgnList) : DLS::Region((DLS::Instrument*) pInstrument, rgnList) {
        // Initialization
        Dimensions = 0;
        for (int i = 0; i < 256; i++) {
            pDimensionRegions[i] = NULL;
        }
        Layers = 1;
        File* file = (File*) GetParent()->GetParent();
        int dimensionBits = (file->pVersion && file->pVersion->major > 2) ? 8 : 5;

        // Actual Loading

        if (!file->GetAutoLoad()) return;

        LoadDimensionRegions(rgnList);

        RIFF::Chunk* _3lnk = rgnList->GetSubChunk(CHUNK_ID_3LNK);
        if (_3lnk) {
            _3lnk->SetPos(0);

            DimensionRegions = _3lnk->ReadUint32();
            for (int i = 0; i < dimensionBits; i++) {
                dimension_t dimension = static_cast<dimension_t>(_3lnk->ReadUint8());
                uint8_t     bits      = _3lnk->ReadUint8();
                _3lnk->ReadUint8(); // bit position of the dimension (bits[0] + bits[1] + ... + bits[i-1])
                _3lnk->ReadUint8(); // (1 << bit position of next dimension) - (1 << bit position of this dimension)
                uint8_t     zones     = _3lnk->ReadUint8(); // new for v3: number of zones doesn't have to be == pow(2,bits)
                if (dimension == dimension_none) { // inactive dimension
                    pDimensionDefinitions[i].dimension  = dimension_none;
                    pDimensionDefinitions[i].bits       = 0;
                    pDimensionDefinitions[i].zones      = 0;
                    pDimensionDefinitions[i].split_type = split_type_bit;
                    pDimensionDefinitions[i].zone_size  = 0;
                }
                else { // active dimension
                    pDimensionDefinitions[i].dimension = dimension;
                    pDimensionDefinitions[i].bits      = bits;
                    pDimensionDefinitions[i].zones     = zones ? zones : 0x01 << bits; // = pow(2,bits)
                    pDimensionDefinitions[i].split_type = __resolveSplitType(dimension);
                    pDimensionDefinitions[i].zone_size  = __resolveZoneSize(pDimensionDefinitions[i]);
                    Dimensions++;

                    // if this is a layer dimension, remember the amount of layers
                    if (dimension == dimension_layer) Layers = pDimensionDefinitions[i].zones;
                }
                _3lnk->SetPos(3, RIFF::stream_curpos); // jump forward to next dimension definition
            }
            for (int i = dimensionBits ; i < 8 ; i++) pDimensionDefinitions[i].bits = 0;

            // if there's a velocity dimension and custom velocity zone splits are used,
            // update the VelocityTables in the dimension regions
            UpdateVelocityTable();

            // jump to start of the wave pool indices (if not already there)
            if (file->pVersion && file->pVersion->major > 2)
                _3lnk->SetPos(68); // version 3 has a different 3lnk structure
            else
                _3lnk->SetPos(44);

            // load sample references (if auto loading is enabled)
            if (file->GetAutoLoad()) {
                for (uint i = 0; i < DimensionRegions; i++) {
                    uint32_t wavepoolindex = _3lnk->ReadUint32();
                    if (file->pWavePoolTable && pDimensionRegions[i])
                        pDimensionRegions[i]->pSample = GetSampleFromWavePool(wavepoolindex);
                }
                GetSample(); // load global region sample reference
            }
        } else {
            DimensionRegions = 0;
            for (int i = 0 ; i < 8 ; i++) {
                pDimensionDefinitions[i].dimension  = dimension_none;
                pDimensionDefinitions[i].bits       = 0;
                pDimensionDefinitions[i].zones      = 0;
            }
        }

        // make sure there is at least one dimension region
        if (!DimensionRegions) {
            RIFF::List* _3prg = rgnList->GetSubList(LIST_TYPE_3PRG);
            if (!_3prg) _3prg = rgnList->AddSubList(LIST_TYPE_3PRG);
            RIFF::List* _3ewl = _3prg->AddSubList(LIST_TYPE_3EWL);
            pDimensionRegions[0] = new DimensionRegion(this, _3ewl);
            DimensionRegions = 1;
        }
    }

    /**
     * Apply Region settings and all its DimensionRegions to the respective
     * RIFF chunks. You have to call File::Save() to make changes persistent.
     *
     * Usually there is absolutely no need to call this method explicitly.
     * It will be called automatically when File::Save() was called.
     *
     * @param pProgress - callback function for progress notification
     * @throws gig::Exception if samples cannot be dereferenced
     */
    void Region::UpdateChunks(progress_t* pProgress) {
        // in the gig format we don't care about the Region's sample reference
        // but we still have to provide some existing one to not corrupt the
        // file, so to avoid the latter we simply always assign the sample of
        // the first dimension region of this region
        pSample = pDimensionRegions[0]->pSample;

        // first update base class's chunks
        DLS::Region::UpdateChunks(pProgress);

        // update dimension region's chunks
        for (int i = 0; i < DimensionRegions; i++) {
            pDimensionRegions[i]->UpdateChunks(pProgress);
        }

        File* pFile = (File*) GetParent()->GetParent();
        const bool versiongt2 = pFile->pVersion && pFile->pVersion->major > 2;
        const int iMaxDimensions =  versiongt2 ? 8 : 5;
        const int iMaxDimensionRegions = versiongt2 ? 256 : 32;

        // make sure '3lnk' chunk exists
        RIFF::Chunk* _3lnk = pCkRegion->GetSubChunk(CHUNK_ID_3LNK);
        if (!_3lnk) {
            const int _3lnkChunkSize = versiongt2 ? 1092 : 172;
            _3lnk = pCkRegion->AddSubChunk(CHUNK_ID_3LNK, _3lnkChunkSize);
            memset(_3lnk->LoadChunkData(), 0, _3lnkChunkSize);

            // move 3prg to last position
            pCkRegion->MoveSubChunk(pCkRegion->GetSubList(LIST_TYPE_3PRG), (RIFF::Chunk*)NULL);
        }

        // update dimension definitions in '3lnk' chunk
        uint8_t* pData = (uint8_t*) _3lnk->LoadChunkData();
        store32(&pData[0], DimensionRegions);
        int shift = 0;
        for (int i = 0; i < iMaxDimensions; i++) {
            pData[4 + i * 8] = (uint8_t) pDimensionDefinitions[i].dimension;
            pData[5 + i * 8] = pDimensionDefinitions[i].bits;
            pData[6 + i * 8] = pDimensionDefinitions[i].dimension == dimension_none ? 0 : shift;
            pData[7 + i * 8] = (1 << (shift + pDimensionDefinitions[i].bits)) - (1 << shift);
            pData[8 + i * 8] = pDimensionDefinitions[i].zones;
            // next 3 bytes unknown, always zero?

            shift += pDimensionDefinitions[i].bits;
        }

        // update wave pool table in '3lnk' chunk
        const int iWavePoolOffset = versiongt2 ? 68 : 44;
        for (uint i = 0; i < iMaxDimensionRegions; i++) {
            int iWaveIndex = -1;
            if (i < DimensionRegions) {
                if (!pFile->pSamples || !pFile->pSamples->size()) throw gig::Exception("Could not update gig::Region, there are no samples");
                File::SampleList::iterator iter = pFile->pSamples->begin();
                File::SampleList::iterator end  = pFile->pSamples->end();
                for (int index = 0; iter != end; ++iter, ++index) {
                    if (*iter == pDimensionRegions[i]->pSample) {
                        iWaveIndex = index;
                        break;
                    }
                }
            }
            store32(&pData[iWavePoolOffset + i * 4], iWaveIndex);
        }

        // The following chunks are just added for compatibility with the
        // GigaStudio software, which would show a warning if these were
        // missing. However currently these chunks don't cover any useful
        // data. So if this gig file uses any of our own gig format
        // extensions which would cause this gig file to be unloadable
        // with GSt software anyway, then just skip these GSt compatibility
        // chunks here as well.
        if (versiongt2 && !UsesAnyGigFormatExtension()) {
            // add 3dnm list which always seems to be empty
            RIFF::List* _3dnm = pCkRegion->GetSubList(LIST_TYPE_3DNM);
            if (!_3dnm) _3dnm = pCkRegion->AddSubList(LIST_TYPE_3DNM);

            // add 3ddp chunk which always seems to have 16 bytes of 0xFF
            RIFF::Chunk* _3ddp = pCkRegion->GetSubChunk(CHUNK_ID_3DDP);
            if (!_3ddp) _3ddp =  pCkRegion->AddSubChunk(CHUNK_ID_3DDP, 16);
            uint8_t* pData = (uint8_t*) _3ddp->LoadChunkData();
            for (int i = 0; i < 16; i += 4) {
                store32(&pData[i], 0xFFFFFFFF);
            }

            // move 3dnm and 3ddp to the end of the region list
            pCkRegion->MoveSubChunk(pCkRegion->GetSubList(LIST_TYPE_3DNM), (RIFF::Chunk*)NULL);
            pCkRegion->MoveSubChunk(pCkRegion->GetSubChunk(CHUNK_ID_3DDP), (RIFF::Chunk*)NULL);
        } else {
            // this is intended for the user switching from GSt >= 3 version
            // back to an older format version, delete GSt3 chunks ...
            RIFF::List* _3dnm = pCkRegion->GetSubList(LIST_TYPE_3DNM);
            if (_3dnm) pCkRegion->DeleteSubChunk(_3dnm);

            RIFF::Chunk* _3ddp = pCkRegion->GetSubChunk(CHUNK_ID_3DDP);
            if (_3ddp) pCkRegion->DeleteSubChunk(_3ddp);
        }
    }

    void Region::LoadDimensionRegions(RIFF::List* rgn) {
        RIFF::List* _3prg = rgn->GetSubList(LIST_TYPE_3PRG);
        if (_3prg) {
            int dimensionRegionNr = 0;
            RIFF::List* _3ewl = _3prg->GetFirstSubList();
            while (_3ewl) {
                if (_3ewl->GetListType() == LIST_TYPE_3EWL) {
                    pDimensionRegions[dimensionRegionNr] = new DimensionRegion(this, _3ewl);
                    dimensionRegionNr++;
                }
                _3ewl = _3prg->GetNextSubList();
            }
            if (dimensionRegionNr == 0) throw gig::Exception("No dimension region found.");
        }
    }

    void Region::SetKeyRange(uint16_t Low, uint16_t High) {
        // update KeyRange struct and make sure regions are in correct order
        DLS::Region::SetKeyRange(Low, High);
        // update Region key table for fast lookup
        ((gig::Instrument*)GetParent())->UpdateRegionKeyTable();
    }

    void Region::UpdateVelocityTable() {
        // get velocity dimension's index
        int veldim = -1;
        for (int i = 0 ; i < Dimensions ; i++) {
            if (pDimensionDefinitions[i].dimension == gig::dimension_velocity) {
                veldim = i;
                break;
            }
        }
        if (veldim == -1) return;

        int step = 1;
        for (int i = 0 ; i < veldim ; i++) step <<= pDimensionDefinitions[i].bits;
        int skipveldim = (step << pDimensionDefinitions[veldim].bits) - step;

        // loop through all dimension regions for all dimensions except the velocity dimension
        int dim[8] = { 0 };
        for (int i = 0 ; i < DimensionRegions ; i++) {
            const int end = i + step * pDimensionDefinitions[veldim].zones;

            // create a velocity table for all cases where the velocity zone is zero
            if (pDimensionRegions[i]->DimensionUpperLimits[veldim] ||
                pDimensionRegions[i]->VelocityUpperLimit) {
                // create the velocity table
                uint8_t* table = pDimensionRegions[i]->VelocityTable;
                if (!table) {
                    table = new uint8_t[128];
                    pDimensionRegions[i]->VelocityTable = table;
                }
                int tableidx = 0;
                int velocityZone = 0;
                if (pDimensionRegions[i]->DimensionUpperLimits[veldim]) { // gig3
                    for (int k = i ; k < end ; k += step) {
                        DimensionRegion *d = pDimensionRegions[k];
                        for (; tableidx <= d->DimensionUpperLimits[veldim] ; tableidx++) table[tableidx] = velocityZone;
                        velocityZone++;
                    }
                } else { // gig2
                    for (int k = i ; k < end ; k += step) {
                        DimensionRegion *d = pDimensionRegions[k];
                        for (; tableidx <= d->VelocityUpperLimit ; tableidx++) table[tableidx] = velocityZone;
                        velocityZone++;
                    }
                }
            } else {
                if (pDimensionRegions[i]->VelocityTable) {
                    delete[] pDimensionRegions[i]->VelocityTable;
                    pDimensionRegions[i]->VelocityTable = 0;
                }
            }

            // jump to the next case where the velocity zone is zero
            int j;
            int shift = 0;
            for (j = 0 ; j < Dimensions ; j++) {
                if (j == veldim) i += skipveldim; // skip velocity dimension
                else {
                    dim[j]++;
                    if (dim[j] < pDimensionDefinitions[j].zones) break;
                    else {
                        // skip unused dimension regions
                        dim[j] = 0;
                        i += ((1 << pDimensionDefinitions[j].bits) -
                              pDimensionDefinitions[j].zones) << shift;
                    }
                }
                shift += pDimensionDefinitions[j].bits;
            }
            if (j == Dimensions) break;
        }
    }

    /** @brief Einstein would have dreamed of it - create a new dimension.
     *
     * Creates a new dimension with the dimension definition given by
     * \a pDimDef. The appropriate amount of DimensionRegions will be created.
     * There is a hard limit of dimensions and total amount of "bits" all
     * dimensions can have. This limit is dependant to what gig file format
     * version this file refers to. The gig v2 (and lower) format has a
     * dimension limit and total amount of bits limit of 5, whereas the gig v3
     * format has a limit of 8.
     *
     * @param pDimDef - defintion of the new dimension
     * @throws gig::Exception if dimension of the same type exists already
     * @throws gig::Exception if amount of dimensions or total amount of
     *                        dimension bits limit is violated
     */
    void Region::AddDimension(dimension_def_t* pDimDef) {
        // some initial sanity checks of the given dimension definition
        if (pDimDef->zones < 2)
            throw gig::Exception("Could not add new dimension, amount of requested zones must always be at least two");
        if (pDimDef->bits < 1)
            throw gig::Exception("Could not add new dimension, amount of requested requested zone bits must always be at least one");
        if (pDimDef->dimension == dimension_samplechannel) {
            if (pDimDef->zones != 2)
                throw gig::Exception("Could not add new 'sample channel' dimensions, the requested amount of zones must always be 2 for this dimension type");
            if (pDimDef->bits != 1)
                throw gig::Exception("Could not add new 'sample channel' dimensions, the requested amount of zone bits must always be 1 for this dimension type");
        }

        // check if max. amount of dimensions reached
        File* file = (File*) GetParent()->GetParent();
        const int iMaxDimensions = (file->pVersion && file->pVersion->major > 2) ? 8 : 5;
        if (Dimensions >= iMaxDimensions)
            throw gig::Exception("Could not add new dimension, max. amount of " + ToString(iMaxDimensions) + " dimensions already reached");
        // check if max. amount of dimension bits reached
        int iCurrentBits = 0;
        for (int i = 0; i < Dimensions; i++)
            iCurrentBits += pDimensionDefinitions[i].bits;
        if (iCurrentBits >= iMaxDimensions)
            throw gig::Exception("Could not add new dimension, max. amount of " + ToString(iMaxDimensions) + " dimension bits already reached");
        const int iNewBits = iCurrentBits + pDimDef->bits;
        if (iNewBits > iMaxDimensions)
            throw gig::Exception("Could not add new dimension, new dimension would exceed max. amount of " + ToString(iMaxDimensions) + " dimension bits");
        // check if there's already a dimensions of the same type
        for (int i = 0; i < Dimensions; i++)
            if (pDimensionDefinitions[i].dimension == pDimDef->dimension)
                throw gig::Exception("Could not add new dimension, there is already a dimension of the same type");

        // pos is where the new dimension should be placed, normally
        // last in list, except for the samplechannel dimension which
        // has to be first in list
        int pos = pDimDef->dimension == dimension_samplechannel ? 0 : Dimensions;
        int bitpos = 0;
        for (int i = 0 ; i < pos ; i++)
            bitpos += pDimensionDefinitions[i].bits;

        // make room for the new dimension
        for (int i = Dimensions ; i > pos ; i--) pDimensionDefinitions[i] = pDimensionDefinitions[i - 1];
        for (int i = 0 ; i < (1 << iCurrentBits) ; i++) {
            for (int j = Dimensions ; j > pos ; j--) {
                pDimensionRegions[i]->DimensionUpperLimits[j] =
                    pDimensionRegions[i]->DimensionUpperLimits[j - 1];
            }
        }

        // assign definition of new dimension
        pDimensionDefinitions[pos] = *pDimDef;

        // auto correct certain dimension definition fields (where possible)
        pDimensionDefinitions[pos].split_type  =
            __resolveSplitType(pDimensionDefinitions[pos].dimension);
        pDimensionDefinitions[pos].zone_size =
            __resolveZoneSize(pDimensionDefinitions[pos]);

        // create new dimension region(s) for this new dimension, and make
        // sure that the dimension regions are placed correctly in both the
        // RIFF list and the pDimensionRegions array
        RIFF::Chunk* moveTo = NULL;
        RIFF::List* _3prg = pCkRegion->GetSubList(LIST_TYPE_3PRG);
        for (int i = (1 << iCurrentBits) - (1 << bitpos) ; i >= 0 ; i -= (1 << bitpos)) {
            for (int k = 0 ; k < (1 << bitpos) ; k++) {
                pDimensionRegions[(i << pDimDef->bits) + k] = pDimensionRegions[i + k];
            }
            for (int j = 1 ; j < (1 << pDimDef->bits) ; j++) {
                for (int k = 0 ; k < (1 << bitpos) ; k++) {
                    RIFF::List* pNewDimRgnListChunk = _3prg->AddSubList(LIST_TYPE_3EWL);
                    if (moveTo) _3prg->MoveSubChunk(pNewDimRgnListChunk, moveTo);
                    // create a new dimension region and copy all parameter values from
                    // an existing dimension region
                    pDimensionRegions[(i << pDimDef->bits) + (j << bitpos) + k] =
                        new DimensionRegion(pNewDimRgnListChunk, *pDimensionRegions[i + k]);

                    DimensionRegions++;
                }
            }
            moveTo = pDimensionRegions[i]->pParentList;
        }

        // initialize the upper limits for this dimension
        int mask = (1 << bitpos) - 1;
        for (int z = 0 ; z < pDimDef->zones ; z++) {
            uint8_t upperLimit = uint8_t((z + 1) * 128.0 / pDimDef->zones - 1);
            for (int i = 0 ; i < 1 << iCurrentBits ; i++) {
                pDimensionRegions[((i & ~mask) << pDimDef->bits) |
                                  (z << bitpos) |
                                  (i & mask)]->DimensionUpperLimits[pos] = upperLimit;
            }
        }

        Dimensions++;

        // if this is a layer dimension, update 'Layers' attribute
        if (pDimDef->dimension == dimension_layer) Layers = pDimDef->zones;

        UpdateVelocityTable();
    }

    /** @brief Delete an existing dimension.
     *
     * Deletes the dimension given by \a pDimDef and deletes all respective
     * dimension regions, that is all dimension regions where the dimension's
     * bit(s) part is greater than 0. In case of a 'sustain pedal' dimension
     * for example this would delete all dimension regions for the case(s)
     * where the sustain pedal is pressed down.
     *
     * @param pDimDef - dimension to delete
     * @throws gig::Exception if given dimension cannot be found
     */
    void Region::DeleteDimension(dimension_def_t* pDimDef) {
        // get dimension's index
        int iDimensionNr = -1;
        for (int i = 0; i < Dimensions; i++) {
            if (&pDimensionDefinitions[i] == pDimDef) {
                iDimensionNr = i;
                break;
            }
        }
        if (iDimensionNr < 0) throw gig::Exception("Invalid dimension_def_t pointer");

        // get amount of bits below the dimension to delete
        int iLowerBits = 0;
        for (int i = 0; i < iDimensionNr; i++)
            iLowerBits += pDimensionDefinitions[i].bits;

        // get amount ot bits above the dimension to delete
        int iUpperBits = 0;
        for (int i = iDimensionNr + 1; i < Dimensions; i++)
            iUpperBits += pDimensionDefinitions[i].bits;

        RIFF::List* _3prg = pCkRegion->GetSubList(LIST_TYPE_3PRG);

        // delete dimension regions which belong to the given dimension
        // (that is where the dimension's bit > 0)
        for (int iUpperBit = 0; iUpperBit < 1 << iUpperBits; iUpperBit++) {
            for (int iObsoleteBit = 1; iObsoleteBit < 1 << pDimensionDefinitions[iDimensionNr].bits; iObsoleteBit++) {
                for (int iLowerBit = 0; iLowerBit < 1 << iLowerBits; iLowerBit++) {
                    int iToDelete = iUpperBit    << (pDimensionDefinitions[iDimensionNr].bits + iLowerBits) |
                                    iObsoleteBit << iLowerBits |
                                    iLowerBit;

                    _3prg->DeleteSubChunk(pDimensionRegions[iToDelete]->pParentList);
                    delete pDimensionRegions[iToDelete];
                    pDimensionRegions[iToDelete] = NULL;
                    DimensionRegions--;
                }
            }
        }

        // defrag pDimensionRegions array
        // (that is remove the NULL spaces within the pDimensionRegions array)
        for (int iFrom = 2, iTo = 1; iFrom < 256 && iTo < 256 - 1; iTo++) {
            if (!pDimensionRegions[iTo]) {
                if (iFrom <= iTo) iFrom = iTo + 1;
                while (!pDimensionRegions[iFrom] && iFrom < 256) iFrom++;
                if (iFrom < 256 && pDimensionRegions[iFrom]) {
                    pDimensionRegions[iTo]   = pDimensionRegions[iFrom];
                    pDimensionRegions[iFrom] = NULL;
                }
            }
        }

        // remove the this dimension from the upper limits arrays
        for (int j = 0 ; j < 256 && pDimensionRegions[j] ; j++) {
            DimensionRegion* d = pDimensionRegions[j];
            for (int i = iDimensionNr + 1; i < Dimensions; i++) {
                d->DimensionUpperLimits[i - 1] = d->DimensionUpperLimits[i];
            }
            d->DimensionUpperLimits[Dimensions - 1] = 127;
        }

        // 'remove' dimension definition
        for (int i = iDimensionNr + 1; i < Dimensions; i++) {
            pDimensionDefinitions[i - 1] = pDimensionDefinitions[i];
        }
        pDimensionDefinitions[Dimensions - 1].dimension = dimension_none;
        pDimensionDefinitions[Dimensions - 1].bits      = 0;
        pDimensionDefinitions[Dimensions - 1].zones     = 0;

        Dimensions--;

        // if this was a layer dimension, update 'Layers' attribute
        if (pDimDef->dimension == dimension_layer) Layers = 1;
    }

    /** @brief Delete one split zone of a dimension (decrement zone amount).
     *
     * Instead of deleting an entire dimensions, this method will only delete
     * one particular split zone given by @a zone of the Region's dimension
     * given by @a type. So this method will simply decrement the amount of
     * zones by one of the dimension in question. To be able to do that, the
     * respective dimension must exist on this Region and it must have at least
     * 3 zones. All DimensionRegion objects associated with the zone will be
     * deleted.
     *
     * @param type - identifies the dimension where a zone shall be deleted
     * @param zone - index of the dimension split zone that shall be deleted
     * @throws gig::Exception if requested zone could not be deleted
     */
    void Region::DeleteDimensionZone(dimension_t type, int zone) {
        dimension_def_t* oldDef = GetDimensionDefinition(type);
        if (!oldDef)
            throw gig::Exception("Could not delete dimension zone, no such dimension of given type");
        if (oldDef->zones <= 2)
            throw gig::Exception("Could not delete dimension zone, because it would end up with only one zone.");
        if (zone < 0 || zone >= oldDef->zones)
            throw gig::Exception("Could not delete dimension zone, requested zone index out of bounds.");

        const int newZoneSize = oldDef->zones - 1;

        // create a temporary Region which just acts as a temporary copy
        // container and will be deleted at the end of this function and will
        // also not be visible through the API during this process
        gig::Region* tempRgn = NULL;
        {
            // adding these temporary chunks is probably not even necessary
            Instrument* instr = static_cast<Instrument*>(GetParent());
            RIFF::List* pCkInstrument = instr->pCkInstrument;
            RIFF::List* lrgn = pCkInstrument->GetSubList(LIST_TYPE_LRGN);
            if (!lrgn)  lrgn = pCkInstrument->AddSubList(LIST_TYPE_LRGN);
            RIFF::List* rgn = lrgn->AddSubList(LIST_TYPE_RGN);
            tempRgn = new Region(instr, rgn);
        }

        // copy this region's dimensions (with already the dimension split size
        // requested by the arguments of this method call) to the temporary
        // region, and don't use Region::CopyAssign() here for this task, since
        // it would also alter fast lookup helper variables here and there
        dimension_def_t newDef;
        for (int i = 0; i < Dimensions; ++i) {
            dimension_def_t def = pDimensionDefinitions[i]; // copy, don't reference
            // is this the dimension requested by the method arguments? ...
            if (def.dimension == type) { // ... if yes, decrement zone amount by one
                def.zones = newZoneSize;
                if ((1 << (def.bits - 1)) == def.zones) def.bits--;
                newDef = def;
            }
            tempRgn->AddDimension(&def);
        }

        // find the dimension index in the tempRegion which is the dimension
        // type passed to this method (paranoidly expecting different order)
        int tempReducedDimensionIndex = -1;
        for (int d = 0; d < tempRgn->Dimensions; ++d) {
            if (tempRgn->pDimensionDefinitions[d].dimension == type) {
                tempReducedDimensionIndex = d;
                break;
            }
        }

        // copy dimension regions from this region to the temporary region
        for (int iDst = 0; iDst < 256; ++iDst) {
            DimensionRegion* dstDimRgn = tempRgn->pDimensionRegions[iDst];
            if (!dstDimRgn) continue;
            std::map<dimension_t,int> dimCase;
            bool isValidZone = true;
            for (int d = 0, baseBits = 0; d < tempRgn->Dimensions; ++d) {
                const int dstBits = tempRgn->pDimensionDefinitions[d].bits;
                dimCase[tempRgn->pDimensionDefinitions[d].dimension] =
                    (iDst >> baseBits) & ((1 << dstBits) - 1);
                baseBits += dstBits;
                // there are also DimensionRegion objects of unused zones, skip them
                if (dimCase[tempRgn->pDimensionDefinitions[d].dimension] >= tempRgn->pDimensionDefinitions[d].zones) {
                    isValidZone = false;
                    break;
                }
            }
            if (!isValidZone) continue;
            // a bit paranoid: cope with the chance that the dimensions would
            // have different order in source and destination regions
            const bool isLastZone = (dimCase[type] == newZoneSize - 1);
            if (dimCase[type] >= zone) dimCase[type]++;
            DimensionRegion* srcDimRgn = GetDimensionRegionByBit(dimCase);
            dstDimRgn->CopyAssign(srcDimRgn);
            // if this is the upper most zone of the dimension passed to this
            // method, then correct (raise) its upper limit to 127
            if (newDef.split_type == split_type_normal && isLastZone)
                dstDimRgn->DimensionUpperLimits[tempReducedDimensionIndex] = 127;
        }

        // now tempRegion's dimensions and DimensionRegions basically reflect
        // what we wanted to get for this actual Region here, so we now just
        // delete and recreate the dimension in question with the new amount
        // zones and then copy back from tempRegion. we're actually deleting and
        // recreating all dimensions here, to avoid altering the precise order
        // of the dimensions (which would not be an error per se, but it would
        // cause usability issues with instrument editors)
        {
            std::vector<dimension_def_t> oldDefs;
            for (int i = 0; i < Dimensions; ++i)
                oldDefs.push_back(pDimensionDefinitions[i]); // copy, don't reference
            for (int i = Dimensions - 1; i >= 0; --i)
                DeleteDimension(&pDimensionDefinitions[i]);
            for (int i = 0; i < oldDefs.size(); ++i) {
                dimension_def_t& def = oldDefs[i];
                AddDimension(
                    (def.dimension == newDef.dimension) ? &newDef : &def
                );
            }
        }
        for (int iSrc = 0; iSrc < 256; ++iSrc) {
            DimensionRegion* srcDimRgn = tempRgn->pDimensionRegions[iSrc];
            if (!srcDimRgn) continue;
            std::map<dimension_t,int> dimCase;
            for (int d = 0, baseBits = 0; d < tempRgn->Dimensions; ++d) {
                const int srcBits = tempRgn->pDimensionDefinitions[d].bits;
                dimCase[tempRgn->pDimensionDefinitions[d].dimension] =
                    (iSrc >> baseBits) & ((1 << srcBits) - 1);
                baseBits += srcBits;
            }
            // a bit paranoid: cope with the chance that the dimensions would
            // have different order in source and destination regions
            DimensionRegion* dstDimRgn = GetDimensionRegionByBit(dimCase);
            if (!dstDimRgn) continue;
            dstDimRgn->CopyAssign(srcDimRgn);
        }

        // delete temporary region
        tempRgn->DeleteChunks();
        delete tempRgn;

        UpdateVelocityTable();
    }

    /** @brief Divide split zone of a dimension in two (increment zone amount).
     *
     * This will increment the amount of zones for the dimension (given by
     * @a type) by one. It will do so by dividing the zone (given by @a zone)
     * in the middle of its zone range in two. So the two zones resulting from
     * the zone being splitted, will be an equivalent copy regarding all their
     * articulation informations and sample reference. The two zones will only
     * differ in their zone's upper limit
     * (DimensionRegion::DimensionUpperLimits).
     *
     * @param type - identifies the dimension where a zone shall be splitted
     * @param zone - index of the dimension split zone that shall be splitted
     * @throws gig::Exception if requested zone could not be splitted
     */
    void Region::SplitDimensionZone(dimension_t type, int zone) {
        dimension_def_t* oldDef = GetDimensionDefinition(type);
        if (!oldDef)
            throw gig::Exception("Could not split dimension zone, no such dimension of given type");
        if (zone < 0 || zone >= oldDef->zones)
            throw gig::Exception("Could not split dimension zone, requested zone index out of bounds.");

        const int newZoneSize = oldDef->zones + 1;

        // create a temporary Region which just acts as a temporary copy
        // container and will be deleted at the end of this function and will
        // also not be visible through the API during this process
        gig::Region* tempRgn = NULL;
        {
            // adding these temporary chunks is probably not even necessary
            Instrument* instr = static_cast<Instrument*>(GetParent());
            RIFF::List* pCkInstrument = instr->pCkInstrument;
            RIFF::List* lrgn = pCkInstrument->GetSubList(LIST_TYPE_LRGN);
            if (!lrgn)  lrgn = pCkInstrument->AddSubList(LIST_TYPE_LRGN);
            RIFF::List* rgn = lrgn->AddSubList(LIST_TYPE_RGN);
            tempRgn = new Region(instr, rgn);
        }

        // copy this region's dimensions (with already the dimension split size
        // requested by the arguments of this method call) to the temporary
        // region, and don't use Region::CopyAssign() here for this task, since
        // it would also alter fast lookup helper variables here and there
        dimension_def_t newDef;
        for (int i = 0; i < Dimensions; ++i) {
            dimension_def_t def = pDimensionDefinitions[i]; // copy, don't reference
            // is this the dimension requested by the method arguments? ...
            if (def.dimension == type) { // ... if yes, increment zone amount by one
                def.zones = newZoneSize;
                if ((1 << oldDef->bits) < newZoneSize) def.bits++;
                newDef = def;
            }
            tempRgn->AddDimension(&def);
        }

        // find the dimension index in the tempRegion which is the dimension
        // type passed to this method (paranoidly expecting different order)
        int tempIncreasedDimensionIndex = -1;
        for (int d = 0; d < tempRgn->Dimensions; ++d) {
            if (tempRgn->pDimensionDefinitions[d].dimension == type) {
                tempIncreasedDimensionIndex = d;
                break;
            }
        }

        // copy dimension regions from this region to the temporary region
        for (int iSrc = 0; iSrc < 256; ++iSrc) {
            DimensionRegion* srcDimRgn = pDimensionRegions[iSrc];
            if (!srcDimRgn) continue;
            std::map<dimension_t,int> dimCase;
            bool isValidZone = true;
            for (int d = 0, baseBits = 0; d < Dimensions; ++d) {
                const int srcBits = pDimensionDefinitions[d].bits;
                dimCase[pDimensionDefinitions[d].dimension] =
                    (iSrc >> baseBits) & ((1 << srcBits) - 1);
                // there are also DimensionRegion objects for unused zones, skip them
                if (dimCase[pDimensionDefinitions[d].dimension] >= pDimensionDefinitions[d].zones) {
                    isValidZone = false;
                    break;
                }
                baseBits += srcBits;
            }
            if (!isValidZone) continue;
            // a bit paranoid: cope with the chance that the dimensions would
            // have different order in source and destination regions            
            if (dimCase[type] > zone) dimCase[type]++;
            DimensionRegion* dstDimRgn = tempRgn->GetDimensionRegionByBit(dimCase);
            dstDimRgn->CopyAssign(srcDimRgn);
            // if this is the requested zone to be splitted, then also copy
            // the source DimensionRegion to the newly created target zone
            // and set the old zones upper limit lower
            if (dimCase[type] == zone) {
                // lower old zones upper limit
                if (newDef.split_type == split_type_normal) {
                    const int high =
                        dstDimRgn->DimensionUpperLimits[tempIncreasedDimensionIndex];
                    int low = 0;
                    if (zone > 0) {
                        std::map<dimension_t,int> lowerCase = dimCase;
                        lowerCase[type]--;
                        DimensionRegion* dstDimRgnLow = tempRgn->GetDimensionRegionByBit(lowerCase);
                        low = dstDimRgnLow->DimensionUpperLimits[tempIncreasedDimensionIndex];
                    }
                    dstDimRgn->DimensionUpperLimits[tempIncreasedDimensionIndex] = low + (high - low) / 2;
                }
                // fill the newly created zone of the divided zone as well
                dimCase[type]++;
                dstDimRgn = tempRgn->GetDimensionRegionByBit(dimCase);
                dstDimRgn->CopyAssign(srcDimRgn);
            }
        }

        // now tempRegion's dimensions and DimensionRegions basically reflect
        // what we wanted to get for this actual Region here, so we now just
        // delete and recreate the dimension in question with the new amount
        // zones and then copy back from tempRegion. we're actually deleting and
        // recreating all dimensions here, to avoid altering the precise order
        // of the dimensions (which would not be an error per se, but it would
        // cause usability issues with instrument editors)
        {
            std::vector<dimension_def_t> oldDefs;
            for (int i = 0; i < Dimensions; ++i)
                oldDefs.push_back(pDimensionDefinitions[i]); // copy, don't reference
            for (int i = Dimensions - 1; i >= 0; --i)
                DeleteDimension(&pDimensionDefinitions[i]);
            for (int i = 0; i < oldDefs.size(); ++i) {
                dimension_def_t& def = oldDefs[i];
                AddDimension(
                    (def.dimension == newDef.dimension) ? &newDef : &def
                );
            }
        }
        for (int iSrc = 0; iSrc < 256; ++iSrc) {
            DimensionRegion* srcDimRgn = tempRgn->pDimensionRegions[iSrc];
            if (!srcDimRgn) continue;
            std::map<dimension_t,int> dimCase;
            for (int d = 0, baseBits = 0; d < tempRgn->Dimensions; ++d) {
                const int srcBits = tempRgn->pDimensionDefinitions[d].bits;
                dimCase[tempRgn->pDimensionDefinitions[d].dimension] =
                    (iSrc >> baseBits) & ((1 << srcBits) - 1);
                baseBits += srcBits;
            }
            // a bit paranoid: cope with the chance that the dimensions would
            // have different order in source and destination regions
            DimensionRegion* dstDimRgn = GetDimensionRegionByBit(dimCase);
            if (!dstDimRgn) continue;
            dstDimRgn->CopyAssign(srcDimRgn);
        }

        // delete temporary region
        tempRgn->DeleteChunks();
        delete tempRgn;

        UpdateVelocityTable();
    }

    /** @brief Change type of an existing dimension.
     *
     * Alters the dimension type of a dimension already existing on this
     * region. If there is currently no dimension on this Region with type
     * @a oldType, then this call with throw an Exception. Likewise there are
     * cases where the requested dimension type cannot be performed. For example
     * if the new dimension type shall be gig::dimension_samplechannel, and the
     * current dimension has more than 2 zones. In such cases an Exception is
     * thrown as well.
     *
     * @param oldType - identifies the existing dimension to be changed
     * @param newType - to which dimension type it should be changed to
     * @throws gig::Exception if requested change cannot be performed
     */
    void Region::SetDimensionType(dimension_t oldType, dimension_t newType) {
        if (oldType == newType) return;
        dimension_def_t* def = GetDimensionDefinition(oldType);
        if (!def)
            throw gig::Exception("No dimension with provided old dimension type exists on this region");
        if (newType == dimension_samplechannel && def->zones != 2)
            throw gig::Exception("Cannot change to dimension type 'sample channel', because existing dimension does not have 2 zones");
        if (GetDimensionDefinition(newType))
            throw gig::Exception("There is already a dimension with requested new dimension type on this region");
        def->dimension  = newType;
        def->split_type = __resolveSplitType(newType);
    }

    DimensionRegion* Region::GetDimensionRegionByBit(const std::map<dimension_t,int>& DimCase) {
        uint8_t bits[8] = {};
        for (std::map<dimension_t,int>::const_iterator it = DimCase.begin();
             it != DimCase.end(); ++it)
        {
            for (int d = 0; d < Dimensions; ++d) {
                if (pDimensionDefinitions[d].dimension == it->first) {
                    bits[d] = it->second;
                    goto nextDimCaseSlice;
                }
            }
            assert(false); // do crash ... too harsh maybe ? ignore it instead ?
            nextDimCaseSlice:
            ; // noop
        }
        return GetDimensionRegionByBit(bits);
    }

    /**
     * Searches in the current Region for a dimension of the given dimension
     * type and returns the precise configuration of that dimension in this
     * Region.
     *
     * @param type - dimension type of the sought dimension
     * @returns dimension definition or NULL if there is no dimension with
     *          sought type in this Region.
     */
    dimension_def_t* Region::GetDimensionDefinition(dimension_t type) {
        for (int i = 0; i < Dimensions; ++i)
            if (pDimensionDefinitions[i].dimension == type)
                return &pDimensionDefinitions[i];
        return NULL;
    }

    Region::~Region() {
        for (int i = 0; i < 256; i++) {
            if (pDimensionRegions[i]) delete pDimensionRegions[i];
        }
    }

    /**
     * Use this method in your audio engine to get the appropriate dimension
     * region with it's articulation data for the current situation. Just
     * call the method with the current MIDI controller values and you'll get
     * the DimensionRegion with the appropriate articulation data for the
     * current situation (for this Region of course only). To do that you'll
     * first have to look which dimensions with which controllers and in
     * which order are defined for this Region when you load the .gig file.
     * Special cases are e.g. layer or channel dimensions where you just put
     * in the index numbers instead of a MIDI controller value (means 0 for
     * left channel, 1 for right channel or 0 for layer 0, 1 for layer 1,
     * etc.).
     *
     * @param  DimValues  MIDI controller values (0-127) for dimension 0 to 7
     * @returns         adress to the DimensionRegion for the given situation
     * @see             pDimensionDefinitions
     * @see             Dimensions
     */
    DimensionRegion* Region::GetDimensionRegionByValue(const uint DimValues[8]) {
        uint8_t bits;
        int veldim = -1;
        int velbitpos = 0;
        int bitpos = 0;
        int dimregidx = 0;
        for (uint i = 0; i < Dimensions; i++) {
            if (pDimensionDefinitions[i].dimension == dimension_velocity) {
                // the velocity dimension must be handled after the other dimensions
                veldim = i;
                velbitpos = bitpos;
            } else {
                switch (pDimensionDefinitions[i].split_type) {
                    case split_type_normal:
                        if (pDimensionRegions[0]->DimensionUpperLimits[i]) {
                            // gig3: all normal dimensions (not just the velocity dimension) have custom zone ranges
                            for (bits = 0 ; bits < pDimensionDefinitions[i].zones ; bits++) {
                                if (DimValues[i] <= pDimensionRegions[bits << bitpos]->DimensionUpperLimits[i]) break;
                            }
                        } else {
                            // gig2: evenly sized zones
                            bits = uint8_t(DimValues[i] / pDimensionDefinitions[i].zone_size);
                        }
                        break;
                    case split_type_bit: // the value is already the sought dimension bit number
                        const uint8_t limiter_mask = (0xff << pDimensionDefinitions[i].bits) ^ 0xff;
                        bits = DimValues[i] & limiter_mask; // just make sure the value doesn't use more bits than allowed
                        break;
                }
                dimregidx |= bits << bitpos;
            }
            bitpos += pDimensionDefinitions[i].bits;
        }
        DimensionRegion* dimreg = pDimensionRegions[dimregidx & 255];
        if (!dimreg) return NULL;
        if (veldim != -1) {
            // (dimreg is now the dimension region for the lowest velocity)
            if (dimreg->VelocityTable) // custom defined zone ranges
                bits = dimreg->VelocityTable[DimValues[veldim] & 127];
            else // normal split type
                bits = uint8_t((DimValues[veldim] & 127) / pDimensionDefinitions[veldim].zone_size);

            const uint8_t limiter_mask = (1 << pDimensionDefinitions[veldim].bits) - 1;
            dimregidx |= (bits & limiter_mask) << velbitpos;
            dimreg = pDimensionRegions[dimregidx & 255];
        }
        return dimreg;
    }

    int Region::GetDimensionRegionIndexByValue(const uint DimValues[8]) {
        uint8_t bits;
        int veldim = -1;
        int velbitpos = 0;
        int bitpos = 0;
        int dimregidx = 0;
        for (uint i = 0; i < Dimensions; i++) {
            if (pDimensionDefinitions[i].dimension == dimension_velocity) {
                // the velocity dimension must be handled after the other dimensions
                veldim = i;
                velbitpos = bitpos;
            } else {
                switch (pDimensionDefinitions[i].split_type) {
                    case split_type_normal:
                        if (pDimensionRegions[0]->DimensionUpperLimits[i]) {
                            // gig3: all normal dimensions (not just the velocity dimension) have custom zone ranges
                            for (bits = 0 ; bits < pDimensionDefinitions[i].zones ; bits++) {
                                if (DimValues[i] <= pDimensionRegions[bits << bitpos]->DimensionUpperLimits[i]) break;
                            }
                        } else {
                            // gig2: evenly sized zones
                            bits = uint8_t(DimValues[i] / pDimensionDefinitions[i].zone_size);
                        }
                        break;
                    case split_type_bit: // the value is already the sought dimension bit number
                        const uint8_t limiter_mask = (0xff << pDimensionDefinitions[i].bits) ^ 0xff;
                        bits = DimValues[i] & limiter_mask; // just make sure the value doesn't use more bits than allowed
                        break;
                }
                dimregidx |= bits << bitpos;
            }
            bitpos += pDimensionDefinitions[i].bits;
        }
        dimregidx &= 255;
        DimensionRegion* dimreg = pDimensionRegions[dimregidx];
        if (!dimreg) return -1;
        if (veldim != -1) {
            // (dimreg is now the dimension region for the lowest velocity)
            if (dimreg->VelocityTable) // custom defined zone ranges
                bits = dimreg->VelocityTable[DimValues[veldim] & 127];
            else // normal split type
                bits = uint8_t((DimValues[veldim] & 127) / pDimensionDefinitions[veldim].zone_size);

            const uint8_t limiter_mask = (1 << pDimensionDefinitions[veldim].bits) - 1;
            dimregidx |= (bits & limiter_mask) << velbitpos;
            dimregidx &= 255;
        }
        return dimregidx;
    }

    /**
     * Returns the appropriate DimensionRegion for the given dimension bit
     * numbers (zone index). You usually use <i>GetDimensionRegionByValue</i>
     * instead of calling this method directly!
     *
     * @param DimBits  Bit numbers for dimension 0 to 7
     * @returns        adress to the DimensionRegion for the given dimension
     *                 bit numbers
     * @see            GetDimensionRegionByValue()
     */
    DimensionRegion* Region::GetDimensionRegionByBit(const uint8_t DimBits[8]) {
        return pDimensionRegions[((((((DimBits[7] << pDimensionDefinitions[6].bits | DimBits[6])
                                                  << pDimensionDefinitions[5].bits | DimBits[5])
                                                  << pDimensionDefinitions[4].bits | DimBits[4])
                                                  << pDimensionDefinitions[3].bits | DimBits[3])
                                                  << pDimensionDefinitions[2].bits | DimBits[2])
                                                  << pDimensionDefinitions[1].bits | DimBits[1])
                                                  << pDimensionDefinitions[0].bits | DimBits[0]];
    }

    /**
     * Returns pointer address to the Sample referenced with this region.
     * This is the global Sample for the entire Region (not sure if this is
     * actually used by the Gigasampler engine - I would only use the Sample
     * referenced by the appropriate DimensionRegion instead of this sample).
     *
     * @returns  address to Sample or NULL if there is no reference to a
     *           sample saved in the .gig file
     */
    Sample* Region::GetSample() {
        if (pSample) return static_cast<gig::Sample*>(pSample);
        else         return static_cast<gig::Sample*>(pSample = GetSampleFromWavePool(WavePoolTableIndex));
    }

    Sample* Region::GetSampleFromWavePool(unsigned int WavePoolTableIndex, progress_t* pProgress) {
        if ((int32_t)WavePoolTableIndex == -1) return NULL;
        File* file = (File*) GetParent()->GetParent();
        if (!file->pWavePoolTable) return NULL;
        if (WavePoolTableIndex + 1 > file->WavePoolCount) return NULL;
        // for new files or files >= 2 GB use 64 bit wave pool offsets
        if (file->pRIFF->IsNew() || (file->pRIFF->GetCurrentFileSize() >> 31)) {
            // use 64 bit wave pool offsets (treating this as large file)
            uint64_t soughtoffset =
                uint64_t(file->pWavePoolTable[WavePoolTableIndex]) |
                uint64_t(file->pWavePoolTableHi[WavePoolTableIndex]) << 32;
            Sample* sample = file->GetFirstSample(pProgress);
            while (sample) {
                if (sample->ullWavePoolOffset == soughtoffset)
                    return static_cast<gig::Sample*>(sample);
                sample = file->GetNextSample();
            }
        } else {
            // use extension files and 32 bit wave pool offsets
            file_offset_t soughtoffset = file->pWavePoolTable[WavePoolTableIndex];
            file_offset_t soughtfileno = file->pWavePoolTableHi[WavePoolTableIndex];
            Sample* sample = file->GetFirstSample(pProgress);
            while (sample) {
                if (sample->ullWavePoolOffset == soughtoffset &&
                    sample->FileNo == soughtfileno) return static_cast<gig::Sample*>(sample);
                sample = file->GetNextSample();
            }
        }
        return NULL;
    }
    
    /**
     * Make a (semi) deep copy of the Region object given by @a orig
     * and assign it to this object.
     *
     * Note that all sample pointers referenced by @a orig are simply copied as
     * memory address. Thus the respective samples are shared, not duplicated!
     *
     * @param orig - original Region object to be copied from
     */
    void Region::CopyAssign(const Region* orig) {
        CopyAssign(orig, NULL);
    }
    
    /**
     * Make a (semi) deep copy of the Region object given by @a orig and
     * assign it to this object
     *
     * @param mSamples - crosslink map between the foreign file's samples and
     *                   this file's samples
     */
    void Region::CopyAssign(const Region* orig, const std::map<Sample*,Sample*>* mSamples) {
        // handle base classes
        DLS::Region::CopyAssign(orig);
        
        if (mSamples && mSamples->count((gig::Sample*)orig->pSample)) {
            pSample = mSamples->find((gig::Sample*)orig->pSample)->second;
        }
        
        // handle own member variables
        for (int i = Dimensions - 1; i >= 0; --i) {
            DeleteDimension(&pDimensionDefinitions[i]);
        }
        Layers = 0; // just to be sure
        for (int i = 0; i < orig->Dimensions; i++) {
            // we need to copy the dim definition here, to avoid the compiler
            // complaining about const-ness issue
            dimension_def_t def = orig->pDimensionDefinitions[i];
            AddDimension(&def);
        }
        for (int i = 0; i < 256; i++) {
            if (pDimensionRegions[i] && orig->pDimensionRegions[i]) {
                pDimensionRegions[i]->CopyAssign(
                    orig->pDimensionRegions[i],
                    mSamples
                );
            }
        }
        Layers = orig->Layers;
    }

    /**
     * Returns @c true in case this Region object uses any gig format
     * extension, that is e.g. whether any DimensionRegion object currently
     * has any setting effective that would require our "LSDE" RIFF chunk to
     * be stored to the gig file.
     *
     * Right now this is a private method. It is considerable though this method
     * to become (in slightly modified form) a public API method in future, i.e.
     * to allow instrument editors to visualize and/or warn the user of any gig
     * format extension being used. See also comments on
     * DimensionRegion::UsesAnyGigFormatExtension() for details about such a
     * potential public API change in future.
     */
    bool Region::UsesAnyGigFormatExtension() const {
        for (int i = 0; i < 256; i++) {
            if (pDimensionRegions[i]) {
                if (pDimensionRegions[i]->UsesAnyGigFormatExtension())
                    return true;
            }
        }
        return false;
    }


// *************** MidiRule ***************
// *

    MidiRuleCtrlTrigger::MidiRuleCtrlTrigger(RIFF::Chunk* _3ewg) {
        _3ewg->SetPos(36);
        Triggers = _3ewg->ReadUint8();
        _3ewg->SetPos(40);
        ControllerNumber = _3ewg->ReadUint8();
        _3ewg->SetPos(46);
        for (int i = 0 ; i < Triggers ; i++) {
            pTriggers[i].TriggerPoint = _3ewg->ReadUint8();
            pTriggers[i].Descending = _3ewg->ReadUint8();
            pTriggers[i].VelSensitivity = _3ewg->ReadUint8();
            pTriggers[i].Key = _3ewg->ReadUint8();
            pTriggers[i].NoteOff = _3ewg->ReadUint8();
            pTriggers[i].Velocity = _3ewg->ReadUint8();
            pTriggers[i].OverridePedal = _3ewg->ReadUint8();
            _3ewg->ReadUint8();
        }
    }

    MidiRuleCtrlTrigger::MidiRuleCtrlTrigger() :
        ControllerNumber(0),
        Triggers(0) {
    }

    void MidiRuleCtrlTrigger::UpdateChunks(uint8_t* pData) const {
        pData[32] = 4;
        pData[33] = 16;
        pData[36] = Triggers;
        pData[40] = ControllerNumber;
        for (int i = 0 ; i < Triggers ; i++) {
            pData[46 + i * 8] = pTriggers[i].TriggerPoint;
            pData[47 + i * 8] = pTriggers[i].Descending;
            pData[48 + i * 8] = pTriggers[i].VelSensitivity;
            pData[49 + i * 8] = pTriggers[i].Key;
            pData[50 + i * 8] = pTriggers[i].NoteOff;
            pData[51 + i * 8] = pTriggers[i].Velocity;
            pData[52 + i * 8] = pTriggers[i].OverridePedal;
        }
    }

    MidiRuleLegato::MidiRuleLegato(RIFF::Chunk* _3ewg) {
        _3ewg->SetPos(36);
        LegatoSamples = _3ewg->ReadUint8(); // always 12
        _3ewg->SetPos(40);
        BypassUseController = _3ewg->ReadUint8();
        BypassKey = _3ewg->ReadUint8();
        BypassController = _3ewg->ReadUint8();
        ThresholdTime = _3ewg->ReadUint16();
        _3ewg->ReadInt16();
        ReleaseTime = _3ewg->ReadUint16();
        _3ewg->ReadInt16();
        KeyRange.low = _3ewg->ReadUint8();
        KeyRange.high = _3ewg->ReadUint8();
        _3ewg->SetPos(64);
        ReleaseTriggerKey = _3ewg->ReadUint8();
        AltSustain1Key = _3ewg->ReadUint8();
        AltSustain2Key = _3ewg->ReadUint8();
    }

    MidiRuleLegato::MidiRuleLegato() :
        LegatoSamples(12),
        BypassUseController(false),
        BypassKey(0),
        BypassController(1),
        ThresholdTime(20),
        ReleaseTime(20),
        ReleaseTriggerKey(0),
        AltSustain1Key(0),
        AltSustain2Key(0)
    {
        KeyRange.low = KeyRange.high = 0;
    }

    void MidiRuleLegato::UpdateChunks(uint8_t* pData) const {
        pData[32] = 0;
        pData[33] = 16;
        pData[36] = LegatoSamples;
        pData[40] = BypassUseController;
        pData[41] = BypassKey;
        pData[42] = BypassController;
        store16(&pData[43], ThresholdTime);
        store16(&pData[47], ReleaseTime);
        pData[51] = KeyRange.low;
        pData[52] = KeyRange.high;
        pData[64] = ReleaseTriggerKey;
        pData[65] = AltSustain1Key;
        pData[66] = AltSustain2Key;
    }

    MidiRuleAlternator::MidiRuleAlternator(RIFF::Chunk* _3ewg) {
        _3ewg->SetPos(36);
        Articulations = _3ewg->ReadUint8();
        int flags = _3ewg->ReadUint8();
        Polyphonic = flags & 8;
        Chained = flags & 4;
        Selector = (flags & 2) ? selector_controller :
            (flags & 1) ? selector_key_switch : selector_none;
        Patterns = _3ewg->ReadUint8();
        _3ewg->ReadUint8(); // chosen row
        _3ewg->ReadUint8(); // unknown
        _3ewg->ReadUint8(); // unknown
        _3ewg->ReadUint8(); // unknown
        KeySwitchRange.low = _3ewg->ReadUint8();
        KeySwitchRange.high = _3ewg->ReadUint8();
        Controller = _3ewg->ReadUint8();
        PlayRange.low = _3ewg->ReadUint8();
        PlayRange.high = _3ewg->ReadUint8();

        int n = std::min(int(Articulations), 32);
        for (int i = 0 ; i < n ; i++) {
            _3ewg->ReadString(pArticulations[i], 32);
        }
        _3ewg->SetPos(1072);
        n = std::min(int(Patterns), 32);
        for (int i = 0 ; i < n ; i++) {
            _3ewg->ReadString(pPatterns[i].Name, 16);
            pPatterns[i].Size = _3ewg->ReadUint8();
            _3ewg->Read(&pPatterns[i][0], 1, 32);
        }
    }

    MidiRuleAlternator::MidiRuleAlternator() :
        Articulations(0),
        Patterns(0),
        Selector(selector_none),
        Controller(0),
        Polyphonic(false),
        Chained(false)
    {
        PlayRange.low = PlayRange.high = 0;
        KeySwitchRange.low = KeySwitchRange.high = 0;
    }

    void MidiRuleAlternator::UpdateChunks(uint8_t* pData) const {
        pData[32] = 3;
        pData[33] = 16;
        pData[36] = Articulations;
        pData[37] = (Polyphonic ? 8 : 0) | (Chained ? 4 : 0) |
            (Selector == selector_controller ? 2 :
             (Selector == selector_key_switch ? 1 : 0));
        pData[38] = Patterns;

        pData[43] = KeySwitchRange.low;
        pData[44] = KeySwitchRange.high;
        pData[45] = Controller;
        pData[46] = PlayRange.low;
        pData[47] = PlayRange.high;

        char* str = reinterpret_cast<char*>(pData);
        int pos = 48;
        int n = std::min(int(Articulations), 32);
        for (int i = 0 ; i < n ; i++, pos += 32) {
            strncpy(&str[pos], pArticulations[i].c_str(), 32);
        }

        pos = 1072;
        n = std::min(int(Patterns), 32);
        for (int i = 0 ; i < n ; i++, pos += 49) {
            strncpy(&str[pos], pPatterns[i].Name.c_str(), 16);
            pData[pos + 16] = pPatterns[i].Size;
            memcpy(&pData[pos + 16], &(pPatterns[i][0]), 32);
        }
    }

// *************** Script ***************
// *

    Script::Script(ScriptGroup* group, RIFF::Chunk* ckScri) {
        pGroup = group;
        pChunk = ckScri;
        if (ckScri) { // object is loaded from file ...
            ckScri->SetPos(0);

            // read header
            uint32_t headerSize = ckScri->ReadUint32();
            Compression = (Compression_t) ckScri->ReadUint32();
            Encoding    = (Encoding_t) ckScri->ReadUint32();
            Language    = (Language_t) ckScri->ReadUint32();
            Bypass      = ckScri->ReadUint32() & 1;
            crc         = ckScri->ReadUint32();
            uint32_t nameSize = ckScri->ReadUint32();
            Name.resize(nameSize, ' ');
            for (int i = 0; i < nameSize; ++i)
                Name[i] = ckScri->ReadUint8();
            // check if an uuid was already stored along with this script
            if (headerSize >= 6*sizeof(int32_t) + nameSize + 16) { // yes ...
                for (uint i = 0; i < 16; ++i) {
                    Uuid[i] = ckScri->ReadUint8();
                }
            } else { // no uuid yet, generate one now ...
                GenerateUuid();
            }
            // to handle potential future extensions of the header
            ckScri->SetPos(sizeof(int32_t) + headerSize);
            // read actual script data
            uint32_t scriptSize = uint32_t(ckScri->GetSize() - ckScri->GetPos());
            data.resize(scriptSize);
            for (int i = 0; i < scriptSize; ++i)
                data[i] = ckScri->ReadUint8();
        } else { // this is a new script object, so just initialize it as such ...
            Compression = COMPRESSION_NONE;
            Encoding = ENCODING_ASCII;
            Language = LANGUAGE_NKSP;
            Bypass   = false;
            crc      = 0;
            Name     = "Unnamed Script";
            GenerateUuid();
        }
    }

    Script::~Script() {
    }

    /**
     * Returns the current script (i.e. as source code) in text format.
     */
    String Script::GetScriptAsText() {
        String s;
        s.resize(data.size(), ' ');
        memcpy(&s[0], &data[0], data.size());
        return s;
    }

    /**
     * Replaces the current script with the new script source code text given
     * by @a text.
     *
     * @param text - new script source code
     */
    void Script::SetScriptAsText(const String& text) {
        data.resize(text.size());
        memcpy(&data[0], &text[0], text.size());
    }

    /** @brief Remove all RIFF chunks associated with this Script object.
     *
     * At the moment Script::DeleteChunks() does nothing. It is
     * recommended to call this method explicitly though from deriving classes's
     * own overridden implementation of this method to avoid potential future
     * compatiblity issues.
     *
     * See DLS::Storage::DeleteChunks() for details.
     */
    void Script::DeleteChunks() {
    }

    /**
     * Apply this script to the respective RIFF chunks. You have to call
     * File::Save() to make changes persistent.
     *
     * Usually there is absolutely no need to call this method explicitly.
     * It will be called automatically when File::Save() was called.
     *
     * @param pProgress - callback function for progress notification
     */
    void Script::UpdateChunks(progress_t* pProgress) {
        // recalculate CRC32 check sum
        __resetCRC(crc);
        __calculateCRC(&data[0], data.size(), crc);
        __finalizeCRC(crc);
        // make sure chunk exists and has the required size
        const file_offset_t chunkSize =
            (file_offset_t) 7*sizeof(int32_t) + Name.size() + 16 + data.size();
        if (!pChunk) pChunk = pGroup->pList->AddSubChunk(CHUNK_ID_SCRI, chunkSize);
        else pChunk->Resize(chunkSize);
        // fill the chunk data to be written to disk
        uint8_t* pData = (uint8_t*) pChunk->LoadChunkData();
        int pos = 0;
        store32(&pData[pos], uint32_t(6*sizeof(int32_t) + Name.size() + 16)); // total header size
        pos += sizeof(int32_t);
        store32(&pData[pos], Compression);
        pos += sizeof(int32_t);
        store32(&pData[pos], Encoding);
        pos += sizeof(int32_t);
        store32(&pData[pos], Language);
        pos += sizeof(int32_t);
        store32(&pData[pos], Bypass ? 1 : 0);
        pos += sizeof(int32_t);
        store32(&pData[pos], crc);
        pos += sizeof(int32_t);
        store32(&pData[pos], (uint32_t) Name.size());
        pos += sizeof(int32_t);
        for (int i = 0; i < Name.size(); ++i, ++pos)
            pData[pos] = Name[i];
        for (int i = 0; i < 16; ++i, ++pos)
            pData[pos] = Uuid[i];
        for (int i = 0; i < data.size(); ++i, ++pos)
            pData[pos] = data[i];
    }

    /**
     * Generate a new Universally Unique Identifier (UUID) for this script.
     */
    void Script::GenerateUuid() {
        DLS::dlsid_t dlsid;
        DLS::Resource::GenerateDLSID(&dlsid);
        Uuid[0]  = dlsid.ulData1       & 0xff;
        Uuid[1]  = dlsid.ulData1 >>  8 & 0xff;
        Uuid[2]  = dlsid.ulData1 >> 16 & 0xff;
        Uuid[3]  = dlsid.ulData1 >> 24 & 0xff;
        Uuid[4]  = dlsid.usData2       & 0xff;
        Uuid[5]  = dlsid.usData2 >>  8 & 0xff;
        Uuid[6]  = dlsid.usData3       & 0xff;
        Uuid[7]  = dlsid.usData3 >>  8 & 0xff;
        Uuid[8]  = dlsid.abData[0];
        Uuid[9]  = dlsid.abData[1];
        Uuid[10] = dlsid.abData[2];
        Uuid[11] = dlsid.abData[3];
        Uuid[12] = dlsid.abData[4];
        Uuid[13] = dlsid.abData[5];
        Uuid[14] = dlsid.abData[6];
        Uuid[15] = dlsid.abData[7];
    }

    /**
     * Move this script from its current ScriptGroup to another ScriptGroup
     * given by @a pGroup.
     *
     * @param pGroup - script's new group
     */
    void Script::SetGroup(ScriptGroup* pGroup) {
        if (this->pGroup == pGroup) return;
        if (pChunk)
            pChunk->GetParent()->MoveSubChunk(pChunk, pGroup->pList);
        this->pGroup = pGroup;
    }

    /**
     * Returns the script group this script currently belongs to. Each script
     * is a member of exactly one ScriptGroup.
     *
     * @returns current script group
     */
    ScriptGroup* Script::GetGroup() const {
        return pGroup;
    }

    /**
     * Make a (semi) deep copy of the Script object given by @a orig
     * and assign it to this object. Note: the ScriptGroup this Script
     * object belongs to remains untouched by this call.
     *
     * @param orig - original Script object to be copied from
     */
    void Script::CopyAssign(const Script* orig) {
        Name        = orig->Name;
        Compression = orig->Compression;
        Encoding    = orig->Encoding;
        Language    = orig->Language;
        Bypass      = orig->Bypass;
        data        = orig->data;
    }

    void Script::RemoveAllScriptReferences() {
        File* pFile = pGroup->pFile;
        for (int i = 0; pFile->GetInstrument(i); ++i) {
            Instrument* instr = pFile->GetInstrument(i);
            instr->RemoveScript(this);
        }
    }

// *************** ScriptGroup ***************
// *

    ScriptGroup::ScriptGroup(File* file, RIFF::List* lstRTIS) {
        pFile = file;
        pList = lstRTIS;
        pScripts = NULL;
        if (lstRTIS) {
            RIFF::Chunk* ckName = lstRTIS->GetSubChunk(CHUNK_ID_LSNM);
            ::LoadString(ckName, Name);
        } else {
            Name = "Default Group";
        }
    }

    ScriptGroup::~ScriptGroup() {
        if (pScripts) {
            std::list<Script*>::iterator iter = pScripts->begin();
            std::list<Script*>::iterator end  = pScripts->end();
            while (iter != end) {
                delete *iter;
                ++iter;
            }
            delete pScripts;
        }
    }

    /** @brief Remove all RIFF chunks associated with this ScriptGroup object.
     *
     * At the moment ScriptGroup::DeleteChunks() does nothing. It is
     * recommended to call this method explicitly though from deriving classes's
     * own overridden implementation of this method to avoid potential future
     * compatiblity issues.
     *
     * See DLS::Storage::DeleteChunks() for details.
     */
    void ScriptGroup::DeleteChunks() {
    }

    /**
     * Apply this script group to the respective RIFF chunks. You have to call
     * File::Save() to make changes persistent.
     *
     * Usually there is absolutely no need to call this method explicitly.
     * It will be called automatically when File::Save() was called.
     *
     * @param pProgress - callback function for progress notification
     */
    void ScriptGroup::UpdateChunks(progress_t* pProgress) {
        if (pScripts) {
            if (!pList)
                pList = pFile->pRIFF->GetSubList(LIST_TYPE_3LS)->AddSubList(LIST_TYPE_RTIS);

            // now store the name of this group as <LSNM> chunk as subchunk of the <RTIS> list chunk
            ::SaveString(CHUNK_ID_LSNM, NULL, pList, Name, String("Unnamed Group"), true, 64);

            for (std::list<Script*>::iterator it = pScripts->begin();
                 it != pScripts->end(); ++it)
            {
                (*it)->UpdateChunks(pProgress);
            }
        }
    }

    /** @brief Get instrument script.
     *
     * Returns the real-time instrument script with the given index.
     *
     * @param index - number of the sought script (0..n)
     * @returns sought script or NULL if there's no such script
     */
    Script* ScriptGroup::GetScript(uint index) {
        if (!pScripts) LoadScripts();
        std::list<Script*>::iterator it = pScripts->begin();
        for (uint i = 0; it != pScripts->end(); ++i, ++it)
            if (i == index) return *it;
        return NULL;
    }

    /** @brief Add new instrument script.
     *
     * Adds a new real-time instrument script to the file. The script is not
     * actually used / executed unless it is referenced by an instrument to be
     * used. This is similar to samples, which you can add to a file, without
     * an instrument necessarily actually using it.
     *
     * You have to call Save() to make this persistent to the file.
     *
     * @return new empty script object
     */
    Script* ScriptGroup::AddScript() {
        if (!pScripts) LoadScripts();
        Script* pScript = new Script(this, NULL);
        pScripts->push_back(pScript);
        return pScript;
    }

    /** @brief Delete an instrument script.
     *
     * This will delete the given real-time instrument script. References of
     * instruments that are using that script will be removed accordingly.
     *
     * You have to call Save() to make this persistent to the file.
     *
     * @param pScript - script to delete
     * @throws gig::Exception if given script could not be found
     */
    void ScriptGroup::DeleteScript(Script* pScript) {
        if (!pScripts) LoadScripts();
        std::list<Script*>::iterator iter =
            find(pScripts->begin(), pScripts->end(), pScript);
        if (iter == pScripts->end())
            throw gig::Exception("Could not delete script, could not find given script");
        pScripts->erase(iter);
        pScript->RemoveAllScriptReferences();
        if (pScript->pChunk)
            pScript->pChunk->GetParent()->DeleteSubChunk(pScript->pChunk);
        delete pScript;
    }

    void ScriptGroup::LoadScripts() {
        if (pScripts) return;
        pScripts = new std::list<Script*>;
        if (!pList) return;

        for (RIFF::Chunk* ck = pList->GetFirstSubChunk(); ck;
             ck = pList->GetNextSubChunk())
        {
            if (ck->GetChunkID() == CHUNK_ID_SCRI) {
                pScripts->push_back(new Script(this, ck));
            }
        }
    }

// *************** Instrument ***************
// *

    Instrument::Instrument(File* pFile, RIFF::List* insList, progress_t* pProgress) : DLS::Instrument((DLS::File*)pFile, insList) {
        static const DLS::Info::string_length_t fixedStringLengths[] = {
            { CHUNK_ID_INAM, 64 },
            { CHUNK_ID_ISFT, 12 },
            { 0, 0 }
        };
        pInfo->SetFixedStringLengths(fixedStringLengths);

        // Initialization
        for (int i = 0; i < 128; i++) RegionKeyTable[i] = NULL;
        EffectSend = 0;
        Attenuation = 0;
        FineTune = 0;
        PitchbendRange = 2;
        PianoReleaseMode = false;
        DimensionKeyRange.low = 0;
        DimensionKeyRange.high = 0;
        pMidiRules = new MidiRule*[3];
        pMidiRules[0] = NULL;
        pScriptRefs = NULL;

        // Loading
        RIFF::List* lart = insList->GetSubList(LIST_TYPE_LART);
        if (lart) {
            RIFF::Chunk* _3ewg = lart->GetSubChunk(CHUNK_ID_3EWG);
            if (_3ewg) {
                _3ewg->SetPos(0);

                EffectSend             = _3ewg->ReadUint16();
                Attenuation            = _3ewg->ReadInt32();
                FineTune               = _3ewg->ReadInt16();
                PitchbendRange         = _3ewg->ReadInt16();
                uint8_t dimkeystart    = _3ewg->ReadUint8();
                PianoReleaseMode       = dimkeystart & 0x01;
                DimensionKeyRange.low  = dimkeystart >> 1;
                DimensionKeyRange.high = _3ewg->ReadUint8();

                if (_3ewg->GetSize() > 32) {
                    // read MIDI rules
                    int i = 0;
                    _3ewg->SetPos(32);
                    uint8_t id1 = _3ewg->ReadUint8();
                    uint8_t id2 = _3ewg->ReadUint8();

                    if (id2 == 16) {
                        if (id1 == 4) {
                            pMidiRules[i++] = new MidiRuleCtrlTrigger(_3ewg);
                        } else if (id1 == 0) {
                            pMidiRules[i++] = new MidiRuleLegato(_3ewg);
                        } else if (id1 == 3) {
                            pMidiRules[i++] = new MidiRuleAlternator(_3ewg);
                        } else {
                            pMidiRules[i++] = new MidiRuleUnknown;
                        }
                    }
                    else if (id1 != 0 || id2 != 0) {
                        pMidiRules[i++] = new MidiRuleUnknown;
                    }
                    //TODO: all the other types of rules

                    pMidiRules[i] = NULL;
                }
            }
        }

        if (pFile->GetAutoLoad()) {
            if (!pRegions) pRegions = new RegionList;
            RIFF::List* lrgn = insList->GetSubList(LIST_TYPE_LRGN);
            if (lrgn) {
                RIFF::List* rgn = lrgn->GetFirstSubList();
                while (rgn) {
                    if (rgn->GetListType() == LIST_TYPE_RGN) {
                        if (pProgress)
                            __notify_progress(pProgress, (float) pRegions->size() / (float) Regions);
                        pRegions->push_back(new Region(this, rgn));
                    }
                    rgn = lrgn->GetNextSubList();
                }
                // Creating Region Key Table for fast lookup
                UpdateRegionKeyTable();
            }
        }

        // own gig format extensions
        RIFF::List* lst3LS = insList->GetSubList(LIST_TYPE_3LS);
        if (lst3LS) {
            // script slots (that is references to instrument scripts)
            RIFF::Chunk* ckSCSL = lst3LS->GetSubChunk(CHUNK_ID_SCSL);
            if (ckSCSL) {
                ckSCSL->SetPos(0);

                int headerSize = ckSCSL->ReadUint32();
                int slotCount  = ckSCSL->ReadUint32();
                if (slotCount) {
                    int slotSize  = ckSCSL->ReadUint32();
                    ckSCSL->SetPos(headerSize); // in case of future header extensions
                    int unknownSpace = slotSize - 2*sizeof(uint32_t); // in case of future slot extensions
                    for (int i = 0; i < slotCount; ++i) {
                        _ScriptPooolEntry e;
                        e.fileOffset = ckSCSL->ReadUint32();
                        e.bypass     = ckSCSL->ReadUint32() & 1;
                        if (unknownSpace) ckSCSL->SetPos(unknownSpace, RIFF::stream_curpos); // in case of future extensions
                        scriptPoolFileOffsets.push_back(e);
                    }
                }
            }

            // overridden script 'patch' variables
            RIFF::Chunk* ckSCPV = lst3LS->GetSubChunk(CHUNK_ID_SCPV);
            if (ckSCPV) {
                ckSCPV->SetPos(0);

                int nScripts = ckSCPV->ReadUint32();
                for (int iScript = 0; iScript < nScripts; ++iScript) {
                    _UUID uuid;
                    for (int i = 0; i < 16; ++i)
                        uuid[i] = ckSCPV->ReadUint8();
                    uint slot = ckSCPV->ReadUint32();
                    ckSCPV->ReadUint32(); // unused, reserved 32 bit
                    int nVars = ckSCPV->ReadUint32();
                    for (int iVar = 0; iVar < nVars; ++iVar) {
                        uint8_t type = ckSCPV->ReadUint8();
                        ckSCPV->ReadUint8();  // unused, reserved byte
                        int blobSize = ckSCPV->ReadUint16();
                        RIFF::file_offset_t pos = ckSCPV->GetPos();
                        // assuming 1st bit is set in 'type', otherwise blob not
                        // supported for decoding
                        if (type & 1) {
                            String name, value;
                            int len = ckSCPV->ReadUint16();
                            for (int i = 0; i < len; ++i)
                                name += (char) ckSCPV->ReadUint8();
                            len = ckSCPV->ReadUint16();
                            for (int i = 0; i < len; ++i)
                                value += (char) ckSCPV->ReadUint8();
                            if (!name.empty()) // 'name' should never be empty, but just to be sure
                                scriptVars[uuid][slot][name] = value;
                        }
                        // also for potential future extensions: seek forward
                        // according to blob size
                        ckSCPV->SetPos(pos + blobSize);
                    }
                }
            }
        }

        if (pProgress)
            __notify_progress(pProgress, 1.0f); // notify done
    }

    void Instrument::UpdateRegionKeyTable() {
        for (int i = 0; i < 128; i++) RegionKeyTable[i] = NULL;
        RegionList::iterator iter = pRegions->begin();
        RegionList::iterator end  = pRegions->end();
        for (; iter != end; ++iter) {
            gig::Region* pRegion = static_cast<gig::Region*>(*iter);
            const int low  = std::max(int(pRegion->KeyRange.low), 0);
            const int high = std::min(int(pRegion->KeyRange.high), 127);
            for (int iKey = low; iKey <= high; iKey++) {
                RegionKeyTable[iKey] = pRegion;
            }
        }
    }

    Instrument::~Instrument() {
        for (int i = 0 ; pMidiRules[i] ; i++) {
            delete pMidiRules[i];
        }
        delete[] pMidiRules;
        if (pScriptRefs) delete pScriptRefs;
    }

    /**
     * Apply Instrument with all its Regions to the respective RIFF chunks.
     * You have to call File::Save() to make changes persistent.
     *
     * Usually there is absolutely no need to call this method explicitly.
     * It will be called automatically when File::Save() was called.
     *
     * @param pProgress - callback function for progress notification
     * @throws gig::Exception if samples cannot be dereferenced
     */
    void Instrument::UpdateChunks(progress_t* pProgress) {
        // first update base classes' chunks
        DLS::Instrument::UpdateChunks(pProgress);

        // update Regions' chunks
        {
            RegionList::iterator iter = pRegions->begin();
            RegionList::iterator end  = pRegions->end();
            for (; iter != end; ++iter)
                (*iter)->UpdateChunks(pProgress);
        }

        // make sure 'lart' RIFF list chunk exists
        RIFF::List* lart = pCkInstrument->GetSubList(LIST_TYPE_LART);
        if (!lart)  lart = pCkInstrument->AddSubList(LIST_TYPE_LART);
        // make sure '3ewg' RIFF chunk exists
        RIFF::Chunk* _3ewg = lart->GetSubChunk(CHUNK_ID_3EWG);
        if (!_3ewg)  {
            File* pFile = (File*) GetParent();

            // 3ewg is bigger in gig3, as it includes the iMIDI rules
            int size = (pFile->pVersion && pFile->pVersion->major > 2) ? 16416 : 12;
            _3ewg = lart->AddSubChunk(CHUNK_ID_3EWG, size);
            memset(_3ewg->LoadChunkData(), 0, size);
        }
        // update '3ewg' RIFF chunk
        uint8_t* pData = (uint8_t*) _3ewg->LoadChunkData();
        store16(&pData[0], EffectSend);
        store32(&pData[2], Attenuation);
        store16(&pData[6], FineTune);
        store16(&pData[8], PitchbendRange);
        const uint8_t dimkeystart = (PianoReleaseMode ? 0x01 : 0x00) |
                                    DimensionKeyRange.low << 1;
        pData[10] = dimkeystart;
        pData[11] = DimensionKeyRange.high;

        if (pMidiRules[0] == 0 && _3ewg->GetSize() >= 34) {
            pData[32] = 0;
            pData[33] = 0;
        } else {
            for (int i = 0 ; pMidiRules[i] ; i++) {
                pMidiRules[i]->UpdateChunks(pData);
            }
        }

        // own gig format extensions
       if (ScriptSlotCount()) {
           // make sure we have converted the original loaded script file
           // offsets into valid Script object pointers
           LoadScripts();

           RIFF::List* lst3LS = pCkInstrument->GetSubList(LIST_TYPE_3LS);
           if (!lst3LS) lst3LS = pCkInstrument->AddSubList(LIST_TYPE_3LS);

           // save script slots (that is references to instrument scripts)
           const int slotCount = (int) pScriptRefs->size();
           const int headerSize = 3 * sizeof(uint32_t);
           const int slotSize  = 2 * sizeof(uint32_t);
           const int totalChunkSize = headerSize + slotCount * slotSize;
           RIFF::Chunk* ckSCSL = lst3LS->GetSubChunk(CHUNK_ID_SCSL);
           if (!ckSCSL) ckSCSL = lst3LS->AddSubChunk(CHUNK_ID_SCSL, totalChunkSize);
           else ckSCSL->Resize(totalChunkSize);
           uint8_t* pData = (uint8_t*) ckSCSL->LoadChunkData();
           int pos = 0;
           store32(&pData[pos], headerSize);
           pos += sizeof(uint32_t);
           store32(&pData[pos], slotCount);
           pos += sizeof(uint32_t);
           store32(&pData[pos], slotSize);
           pos += sizeof(uint32_t);
           for (int i = 0; i < slotCount; ++i) {
               // arbitrary value, the actual file offset will be updated in
               // UpdateScriptFileOffsets() after the file has been resized
               int bogusFileOffset = 0;
               store32(&pData[pos], bogusFileOffset);
               pos += sizeof(uint32_t);
               store32(&pData[pos], (*pScriptRefs)[i].bypass ? 1 : 0);
               pos += sizeof(uint32_t);
           }

           // save overridden script 'patch' variables ...

           // the actual 'scriptVars' member variable might contain variables of
           // scripts which are currently no longer assigned to any script slot
           // of this instrument, we need to get rid of these variables here to
           // prevent saving those persistently, however instead of touching the
           // member variable 'scriptVars' directly, rather strip a separate
           // copy such that the overridden values are not lost during an
           // instrument editor session (i.e. if script might be re-assigned)
           _VarsByScript vars = stripScriptVars();
           if (!vars.empty()) {
               // determine total size required for 'SCPV' RIFF chunk, and the
               // total amount of scripts being overridden (the latter is
               // required because a script might be used on several script
               // slots, hence vars.size() could then not be used here instead)
               size_t totalChunkSize = 4;
               size_t totalScriptsOverridden = 0;
               for (const auto& script : vars) {
                   for (const auto& slot : script.second) {
                       totalScriptsOverridden++;
                       totalChunkSize += 16 + 4 + 4 + 4;
                       for (const auto& var : slot.second) {
                           totalChunkSize += 4 + 2 + var.first.length() +
                                                 2 + var.second.length();
                       }
                   }
               }

               // ensure 'SCPV' RIFF chunk exists (with required size)
               RIFF::Chunk* ckSCPV = lst3LS->GetSubChunk(CHUNK_ID_SCPV);
               if (!ckSCPV) ckSCPV = lst3LS->AddSubChunk(CHUNK_ID_SCPV, totalChunkSize);
               else ckSCPV->Resize(totalChunkSize);

               // store the actual data to 'SCPV' RIFF chunk
               uint8_t* pData = (uint8_t*) ckSCPV->LoadChunkData();
               int pos = 0;
               store32(&pData[pos], (uint32_t) totalScriptsOverridden); // scripts count
               pos += 4;
               for (const auto& script : vars) {
                   for (const auto& slot : script.second) {
                       for (int i = 0; i < 16; ++i)
                           pData[pos+i] = script.first[i]; // uuid
                       pos += 16;
                       store32(&pData[pos], (uint32_t) slot.first); // slot index
                       pos += 4;
                       store32(&pData[pos], (uint32_t) 0); // unused, reserved 32 bit
                       pos += 4;
                       store32(&pData[pos], (uint32_t) slot.second.size()); // variables count
                       pos += 4;
                       for (const auto& var : slot.second) {
                           pData[pos++] = 1; // type
                           pData[pos++] = 0; // reserved byte
                           store16(&pData[pos], 2 + var.first.size() + 2 + var.second.size()); // blob size
                           pos += 2;
                           store16(&pData[pos], var.first.size()); // variable name length
                           pos += 2;
                           for (int i = 0; i < var.first.size(); ++i)
                               pData[pos++] = var.first[i];
                           store16(&pData[pos], var.second.size()); // variable value length
                           pos += 2;
                           for (int i = 0; i < var.second.size(); ++i)
                               pData[pos++] = var.second[i];
                       }
                   }
               }
           } else {
               // no script variable overridden by this instrument, so get rid
               // of 'SCPV' RIFF chunk (if any)
               RIFF::Chunk* ckSCPV = lst3LS->GetSubChunk(CHUNK_ID_SCPV);
               if (ckSCPV) lst3LS->DeleteSubChunk(ckSCPV);
           }
       } else {
           // no script slots, so get rid of any LS custom RIFF chunks (if any)
           RIFF::List* lst3LS = pCkInstrument->GetSubList(LIST_TYPE_3LS);
           if (lst3LS) pCkInstrument->DeleteSubChunk(lst3LS);
       }
    }

    void Instrument::UpdateScriptFileOffsets() { 
       // own gig format extensions
       if (pScriptRefs && pScriptRefs->size() > 0) {
           RIFF::List* lst3LS = pCkInstrument->GetSubList(LIST_TYPE_3LS);
           RIFF::Chunk* ckSCSL = lst3LS->GetSubChunk(CHUNK_ID_SCSL);
           const int slotCount = (int) pScriptRefs->size();
           const int headerSize = 3 * sizeof(uint32_t);
           ckSCSL->SetPos(headerSize);
           for (int i = 0; i < slotCount; ++i) {
               uint32_t fileOffset = uint32_t(
                    (*pScriptRefs)[i].script->pChunk->GetFilePos() -
                    (*pScriptRefs)[i].script->pChunk->GetPos() -
                    CHUNK_HEADER_SIZE(ckSCSL->GetFile()->GetFileOffsetSize())
               );
               ckSCSL->WriteUint32(&fileOffset);
               // jump over flags entry (containing the bypass flag)
               ckSCSL->SetPos(sizeof(uint32_t), RIFF::stream_curpos);
           }
       }        
    }

    /**
     * Returns the appropriate Region for a triggered note.
     *
     * @param Key  MIDI Key number of triggered note / key (0 - 127)
     * @returns    pointer adress to the appropriate Region or NULL if there
     *             there is no Region defined for the given \a Key
     */
    Region* Instrument::GetRegion(unsigned int Key) {
        if (!pRegions || pRegions->empty() || Key > 127) return NULL;
        return RegionKeyTable[Key];

        /*for (int i = 0; i < Regions; i++) {
            if (Key <= pRegions[i]->KeyRange.high &&
                Key >= pRegions[i]->KeyRange.low) return pRegions[i];
        }
        return NULL;*/
    }

    /**
     * Returns the first Region of the instrument. You have to call this
     * method once before you use GetNextRegion().
     *
     * @returns  pointer address to first region or NULL if there is none
     * @see      GetNextRegion()
     */
    Region* Instrument::GetFirstRegion() {
        if (!pRegions) return NULL;
        RegionsIterator = pRegions->begin();
        return static_cast<gig::Region*>( (RegionsIterator != pRegions->end()) ? *RegionsIterator : NULL );
    }

    /**
     * Returns the next Region of the instrument. You have to call
     * GetFirstRegion() once before you can use this method. By calling this
     * method multiple times it iterates through the available Regions.
     *
     * @returns  pointer address to the next region or NULL if end reached
     * @see      GetFirstRegion()
     */
    Region* Instrument::GetNextRegion() {
        if (!pRegions) return NULL;
        RegionsIterator++;
        return static_cast<gig::Region*>( (RegionsIterator != pRegions->end()) ? *RegionsIterator : NULL );
    }

    Region* Instrument::AddRegion() {
        // create new Region object (and its RIFF chunks)
        RIFF::List* lrgn = pCkInstrument->GetSubList(LIST_TYPE_LRGN);
        if (!lrgn)  lrgn = pCkInstrument->AddSubList(LIST_TYPE_LRGN);
        RIFF::List* rgn = lrgn->AddSubList(LIST_TYPE_RGN);
        Region* pNewRegion = new Region(this, rgn);
        pRegions->push_back(pNewRegion);
        Regions = (uint32_t) pRegions->size();
        // update Region key table for fast lookup
        UpdateRegionKeyTable();
        // done
        return pNewRegion;
    }

    void Instrument::DeleteRegion(Region* pRegion) {
        if (!pRegions) return;
        DLS::Instrument::DeleteRegion((DLS::Region*) pRegion);
        // update Region key table for fast lookup
        UpdateRegionKeyTable();
    }

    /**
     * Move this instrument at the position before @arg dst.
     *
     * This method can be used to reorder the sequence of instruments in a
     * .gig file. This might be helpful especially on large .gig files which
     * contain a large number of instruments within the same .gig file. So
     * grouping such instruments to similar ones, can help to keep track of them
     * when working with such complex .gig files.
     *
     * When calling this method, this instrument will be removed from in its
     * current position in the instruments list and moved to the requested
     * target position provided by @param dst. You may also pass NULL as
     * argument to this method, in that case this intrument will be moved to the
     * very end of the .gig file's instrument list.
     *
     * You have to call Save() to make the order change persistent to the .gig
     * file.
     *
     * Currently this method is limited to moving the instrument within the same
     * .gig file. Trying to move it to another .gig file by calling this method
     * will throw an exception.
     *
     * @param dst - destination instrument at which this instrument will be
     *              moved to, or pass NULL for moving to end of list
     * @throw gig::Exception if this instrument and target instrument are not
     *                       part of the same file
     */
    void Instrument::MoveTo(Instrument* dst) {
        if (dst && GetParent() != dst->GetParent())
            throw Exception(
                "gig::Instrument::MoveTo() can only be used for moving within "
                "the same gig file."
            );

        File* pFile = (File*) GetParent();

        // move this instrument within the instrument list
        {
            File::InstrumentList& list = *pFile->pInstruments;

            File::InstrumentList::iterator itFrom =
                std::find(list.begin(), list.end(), static_cast<DLS::Instrument*>(this));

            File::InstrumentList::iterator itTo =
                std::find(list.begin(), list.end(), static_cast<DLS::Instrument*>(dst));

            list.splice(itTo, list, itFrom);
        }

        // move the instrument's actual list RIFF chunk appropriately
        RIFF::List* lstCkInstruments = pFile->pRIFF->GetSubList(LIST_TYPE_LINS);
        lstCkInstruments->MoveSubChunk(
            this->pCkInstrument,
            (RIFF::Chunk*) ((dst) ? dst->pCkInstrument : NULL)
        );
    }

    /**
     * Returns a MIDI rule of the instrument.
     *
     * The list of MIDI rules, at least in gig v3, always contains at
     * most two rules. The second rule can only be the DEF filter
     * (which currently isn't supported by libgig).
     *
     * @param i - MIDI rule number
     * @returns   pointer address to MIDI rule number i or NULL if there is none
     */
    MidiRule* Instrument::GetMidiRule(int i) {
        return pMidiRules[i];
    }

    /**
     * Adds the "controller trigger" MIDI rule to the instrument.
     *
     * @returns the new MIDI rule
     */
    MidiRuleCtrlTrigger* Instrument::AddMidiRuleCtrlTrigger() {
        delete pMidiRules[0];
        MidiRuleCtrlTrigger* r = new MidiRuleCtrlTrigger;
        pMidiRules[0] = r;
        pMidiRules[1] = 0;
        return r;
    }

    /**
     * Adds the legato MIDI rule to the instrument.
     *
     * @returns the new MIDI rule
     */
    MidiRuleLegato* Instrument::AddMidiRuleLegato() {
        delete pMidiRules[0];
        MidiRuleLegato* r = new MidiRuleLegato;
        pMidiRules[0] = r;
        pMidiRules[1] = 0;
        return r;
    }

    /**
     * Adds the alternator MIDI rule to the instrument.
     *
     * @returns the new MIDI rule
     */
    MidiRuleAlternator* Instrument::AddMidiRuleAlternator() {
        delete pMidiRules[0];
        MidiRuleAlternator* r = new MidiRuleAlternator;
        pMidiRules[0] = r;
        pMidiRules[1] = 0;
        return r;
    }

    /**
     * Deletes a MIDI rule from the instrument.
     *
     * @param i - MIDI rule number
     */
    void Instrument::DeleteMidiRule(int i) {
        delete pMidiRules[i];
        pMidiRules[i] = 0;
    }

    void Instrument::LoadScripts() {
        if (pScriptRefs) return;
        pScriptRefs = new std::vector<_ScriptPooolRef>;
        if (scriptPoolFileOffsets.empty()) return;
        File* pFile = (File*) GetParent();
        for (uint k = 0; k < scriptPoolFileOffsets.size(); ++k) {
            uint32_t soughtOffset = scriptPoolFileOffsets[k].fileOffset;
            for (uint i = 0; pFile->GetScriptGroup(i); ++i) {
                ScriptGroup* group = pFile->GetScriptGroup(i);
                for (uint s = 0; group->GetScript(s); ++s) {
                    Script* script = group->GetScript(s);
                    if (script->pChunk) {
                        uint32_t offset = uint32_t(
                            script->pChunk->GetFilePos() -
                            script->pChunk->GetPos() -
                            CHUNK_HEADER_SIZE(script->pChunk->GetFile()->GetFileOffsetSize())
                        );
                        if (offset == soughtOffset)
                        {
                            _ScriptPooolRef ref;
                            ref.script = script;
                            ref.bypass = scriptPoolFileOffsets[k].bypass;
                            pScriptRefs->push_back(ref);
                            break;
                        }
                    }
                }
            }
        }
        // we don't need that anymore
        scriptPoolFileOffsets.clear();
    }

    /** @brief Get instrument script (gig format extension).
     *
     * Returns the real-time instrument script of instrument script slot
     * @a index.
     *
     * @note This is an own format extension which did not exist i.e. in the
     * GigaStudio 4 software. It will currently only work with LinuxSampler and
     * gigedit.
     *
     * @param index - instrument script slot index
     * @returns script or NULL if index is out of bounds
     */
    Script* Instrument::GetScriptOfSlot(uint index) {
        LoadScripts();
        if (index >= pScriptRefs->size()) return NULL;
        return pScriptRefs->at(index).script;
    }

    /** @brief Add new instrument script slot (gig format extension).
     *
     * Add the given real-time instrument script reference to this instrument,
     * which shall be executed by the sampler for for this instrument. The
     * script will be added to the end of the script list of this instrument.
     * The positions of the scripts in the Instrument's Script list are
     * relevant, because they define in which order they shall be executed by
     * the sampler. For this reason it is also legal to add the same script
     * twice to an instrument, for example you might have a script called
     * "MyFilter" which performs an event filter task, and you might have
     * another script called "MyNoteTrigger" which triggers new notes, then you
     * might for example have the following list of scripts on the instrument:
     *
     * 1. Script "MyFilter"
     * 2. Script "MyNoteTrigger"
     * 3. Script "MyFilter"
     *
     * Which would make sense, because the 2nd script launched new events, which
     * you might need to filter as well.
     *
     * There are two ways to disable / "bypass" scripts. You can either disable
     * a script locally for the respective script slot on an instrument (i.e. by
     * passing @c false to the 2nd argument of this method, or by calling
     * SetScriptBypassed()). Or you can disable a script globally for all slots
     * and all instruments by setting Script::Bypass.
     *
     * @note This is an own format extension which did not exist i.e. in the
     * GigaStudio 4 software. It will currently only work with LinuxSampler and
     * gigedit.
     *
     * @param pScript - script that shall be executed for this instrument
     * @param bypass  - if enabled, the sampler shall skip executing this
     *                  script (in the respective list position)
     * @see SetScriptBypassed()
     */
    void Instrument::AddScriptSlot(Script* pScript, bool bypass) {
        LoadScripts();
        _ScriptPooolRef ref = { pScript, bypass };
        pScriptRefs->push_back(ref);
    }

    /** @brief Flip two script slots with each other (gig format extension).
     *
     * Swaps the position of the two given scripts in the Instrument's Script
     * list. The positions of the scripts in the Instrument's Script list are
     * relevant, because they define in which order they shall be executed by
     * the sampler.
     *
     * @note This is an own format extension which did not exist i.e. in the
     * GigaStudio 4 software. It will currently only work with LinuxSampler and
     * gigedit.
     *
     * @param index1 - index of the first script slot to swap
     * @param index2 - index of the second script slot to swap
     */
    void Instrument::SwapScriptSlots(uint index1, uint index2) {
        LoadScripts();
        if (index1 >= pScriptRefs->size() || index2 >= pScriptRefs->size())
            return;
        _ScriptPooolRef tmp = (*pScriptRefs)[index1];
        (*pScriptRefs)[index1] = (*pScriptRefs)[index2];
        (*pScriptRefs)[index2] = tmp;
    }

    /** @brief Remove script slot.
     *
     * Removes the script slot with the given slot index.
     *
     * @param index - index of script slot to remove
     */
    void Instrument::RemoveScriptSlot(uint index) {
        LoadScripts();
        if (index >= pScriptRefs->size()) return;
        pScriptRefs->erase( pScriptRefs->begin() + index );
    }

    /** @brief Remove reference to given Script (gig format extension).
     *
     * This will remove all script slots on the instrument which are referencing
     * the given script.
     *
     * @note This is an own format extension which did not exist i.e. in the
     * GigaStudio 4 software. It will currently only work with LinuxSampler and
     * gigedit.
     *
     * @param pScript - script reference to remove from this instrument
     * @see RemoveScriptSlot()
     */
    void Instrument::RemoveScript(Script* pScript) {
        LoadScripts();
        for (ssize_t i = pScriptRefs->size() - 1; i >= 0; --i) {
            if ((*pScriptRefs)[i].script == pScript) {
                pScriptRefs->erase( pScriptRefs->begin() + i );
            }
        }
    }

    /** @brief Instrument's amount of script slots.
     *
     * This method returns the amount of script slots this instrument currently
     * uses.
     *
     * A script slot is a reference of a real-time instrument script to be
     * executed by the sampler. The scripts will be executed by the sampler in
     * sequence of the slots. One (same) script may be referenced multiple
     * times in different slots.
     *
     * @note This is an own format extension which did not exist i.e. in the
     * GigaStudio 4 software. It will currently only work with LinuxSampler and
     * gigedit.
     */
    uint Instrument::ScriptSlotCount() const {
        return uint(pScriptRefs ? pScriptRefs->size() : scriptPoolFileOffsets.size());
    }

    /** @brief Whether script execution shall be skipped.
     *
     * Defines locally for the Script reference slot in the Instrument's Script
     * list, whether the script shall be skipped by the sampler regarding
     * execution.
     *
     * It is also possible to ignore exeuction of the script globally, for all
     * slots and for all instruments by setting Script::Bypass.
     *
     * @note This is an own format extension which did not exist i.e. in the
     * GigaStudio 4 software. It will currently only work with LinuxSampler and
     * gigedit.
     *
     * @param index - index of the script slot on this instrument
     * @see Script::Bypass
     */
    bool Instrument::IsScriptSlotBypassed(uint index) {
        if (index >= ScriptSlotCount()) return false;
        return pScriptRefs ? pScriptRefs->at(index).bypass
                           : scriptPoolFileOffsets.at(index).bypass;
        
    }

    /** @brief Defines whether execution shall be skipped.
     *
     * You can call this method to define locally whether or whether not the
     * given script slot shall be executed by the sampler.
     *
     * @note This is an own format extension which did not exist i.e. in the
     * GigaStudio 4 software. It will currently only work with LinuxSampler and
     * gigedit.
     *
     * @param index - script slot index on this instrument
     * @param bBypass - if true, the script slot will be skipped by the sampler
     * @see Script::Bypass
     */
    void Instrument::SetScriptSlotBypassed(uint index, bool bBypass) {
        if (index >= ScriptSlotCount()) return;
        if (pScriptRefs)
            pScriptRefs->at(index).bypass = bBypass;
        else
            scriptPoolFileOffsets.at(index).bypass = bBypass;
    }

    /// type cast (by copy) uint8_t[16] -> std::array<uint8_t,16>
    inline std::array<uint8_t,16> _UUIDFromCArray(const uint8_t* pData) {
        std::array<uint8_t,16> uuid;
        memcpy(&uuid[0], pData, 16);
        return uuid;
    }

    /**
     * Returns true if this @c Instrument has any script slot which references
     * the @c Script identified by passed @p uuid.
     */
    bool Instrument::ReferencesScriptWithUuid(const _UUID& uuid) {
        const uint nSlots = ScriptSlotCount();
        for (uint iSlot = 0; iSlot < nSlots; ++iSlot)
            if (_UUIDFromCArray(&GetScriptOfSlot(iSlot)->Uuid[0]) == uuid)
                return true;
        return false;
    }

    /** @brief Checks whether a certain script 'patch' variable value is set.
     *
     * Returns @c true if the initial value for the requested script variable is
     * currently overridden by this instrument.
     *
     * @remarks Real-time instrument scripts allow to declare special 'patch'
     * variables, which essentially behave like regular variables of their data
     * type, however their initial value may optionally be overridden on a per
     * instrument basis. That allows to share scripts between instruments while
     * still being able to fine tune certain aspects of the script for each
     * instrument individually.
     *
     * @note This is an own format extension which did not exist i.e. in the
     * GigaStudio 4 software. It will currently only work with LinuxSampler and
     * Gigedit.
     *
     * @param slot - script slot index of the variable to be retrieved
     * @param variable - name of the 'patch' variable in that script
     */
    bool Instrument::IsScriptPatchVariableSet(int slot, String variable) {
        if (variable.empty()) return false;
        Script* script = GetScriptOfSlot(slot);
        if (!script) return false;
        const _UUID uuid = _UUIDFromCArray(&script->Uuid[0]);
        if (!scriptVars.count(uuid)) return false;
        const _VarsBySlot& slots = scriptVars.find(uuid)->second;
        if (slots.empty()) return false;
        if (slots.count(slot))
            return slots.find(slot)->second.count(variable);
        else
            return slots.begin()->second.count(variable);
    }

    /** @brief Get all overridden script 'patch' variables.
     *
     * Returns map of key-value pairs reflecting all patch variables currently
     * being overridden by this instrument for the given script @p slot, where
     * key is the variable name and value is the hereby currently overridden
     * value for that variable.
     *
     * @remarks Real-time instrument scripts allow to declare special 'patch'
     * variables, which essentially behave like regular variables of their data
     * type, however their initial value may optionally be overridden on a per
     * instrument basis. That allows to share scripts between instruments while
     * still being able to fine tune certain aspects of the script for each
     * instrument individually.
     *
     * @note This is an own format extension which did not exist i.e. in the
     * GigaStudio 4 software. It will currently only work with LinuxSampler and
     * Gigedit.
     *
     * @param slot - script slot index of the variable to be retrieved
     */
    std::map<String,String> Instrument::GetScriptPatchVariables(int slot) {
        Script* script = GetScriptOfSlot(slot);
        if (!script) return std::map<String,String>();
        const _UUID uuid = _UUIDFromCArray(&script->Uuid[0]);
        if (!scriptVars.count(uuid)) return std::map<String,String>();
        const _VarsBySlot& slots = scriptVars.find(uuid)->second;
        if (slots.empty()) return std::map<String,String>();
        const _PatchVars& vars =
            (slots.count(slot)) ?
                slots.find(slot)->second : slots.begin()->second;
        return vars;
    }

    /** @brief Get overridden initial value for 'patch' variable.
     *
     * Returns current initial value for the requested script variable being
     * overridden by this instrument.
     *
     * @remarks Real-time instrument scripts allow to declare special 'patch'
     * variables, which essentially behave like regular variables of their data
     * type, however their initial value may optionally be overridden on a per
     * instrument basis. That allows to share scripts between instruments while
     * still being able to fine tune certain aspects of the script for each
     * instrument individually.
     *
     * @note This is an own format extension which did not exist i.e. in the
     * GigaStudio 4 software. It will currently only work with LinuxSampler and
     * Gigedit.
     *
     * @param slot - script slot index of the variable to be retrieved
     * @param variable - name of the 'patch' variable in that script
     */
    String Instrument::GetScriptPatchVariable(int slot, String variable) {
        std::map<String,String> vars = GetScriptPatchVariables(slot);
        return (vars.count(variable)) ? vars.find(variable)->second : "";
    }

    /** @brief Override initial value for 'patch' variable.
     *
     * Overrides initial value for the requested script variable for this
     * instrument with the passed value.
     *
     * @remarks Real-time instrument scripts allow to declare special 'patch'
     * variables, which essentially behave like regular variables of their data
     * type, however their initial value may optionally be overridden on a per
     * instrument basis. That allows to share scripts between instruments while
     * still being able to fine tune certain aspects of the script for each
     * instrument individually.
     *
     * @note This is an own format extension which did not exist i.e. in the
     * GigaStudio 4 software. It will currently only work with LinuxSampler and
     * Gigedit.
     *
     * @param slot - script slot index of the variable to be set
     * @param variable - name of the 'patch' variable in that script
     * @param value - overridden initial value for that script variable
     * @throws gig::Exception if given script @p slot index is invalid or given
     *         @p variable name is empty
     */
    void Instrument::SetScriptPatchVariable(int slot, String variable, String value) {
        if (variable.empty())
            throw Exception("Variable name must not be empty");
        Script* script = GetScriptOfSlot(slot);
        if (!script)
            throw Exception("No script slot with index " + ToString(slot));
        const _UUID uuid = _UUIDFromCArray(&script->Uuid[0]);
        scriptVars[uuid][slot][variable] = value;
    }

    /** @brief Drop overridden initial value(s) for 'patch' variable(s).
     *
     * Reverts initial value(s) for requested script variable(s) back to their
     * default initial value(s) defined in the script itself.
     *
     * Both arguments of this method are optional. The most obvious use case of
     * this method would be passing a valid script @p slot index and a
     * (non-emtpy string as) @p variable name to this method, which would cause
     * that single variable to be unset for that specific script slot (on this
     * @c Instrument level).
     *
     * Not passing a value (or @c -1 for @p slot and/or empty string for
     * @p variable) means 'wildcard'. So accordingly absence of argument(s) will
     * cause all variables and/or for all script slots being unset. Hence this
     * method serves 2^2 = 4 possible use cases in total and accordingly covers
     * 4 different behaviours in one method.
     *
     * @remarks Real-time instrument scripts allow to declare special 'patch'
     * variables, which essentially behave like regular variables of their data
     * type, however their initial value may optionally be overridden on a per
     * instrument basis. That allows to share scripts between instruments while
     * still being able to fine tune certain aspects of the script for each
     * instrument individually.
     *
     * @note This is an own format extension which did not exist i.e. in the
     * GigaStudio 4 software. It will currently only work with LinuxSampler and
     * Gigedit.
     *
     * @param slot - script slot index of the variable to be unset
     * @param variable - name of the 'patch' variable in that script
     */
    void Instrument::UnsetScriptPatchVariable(int slot, String variable) {
        Script* script = GetScriptOfSlot(slot);

        // option 1: unset a particular variable of one particular script slot
        if (slot != -1 && !variable.empty()) {
            if (!script) return;
            const _UUID uuid = _UUIDFromCArray(&script->Uuid[0]);
            if (!scriptVars.count(uuid)) return;
            if (!scriptVars[uuid].count(slot)) return;
            if (scriptVars[uuid][slot].count(variable))
                scriptVars[uuid][slot].erase(
                    scriptVars[uuid][slot].find(variable)
                );
            if (scriptVars[uuid][slot].empty())
                scriptVars[uuid].erase( scriptVars[uuid].find(slot) );
            if (scriptVars[uuid].empty())
                scriptVars.erase( scriptVars.find(uuid) );
            return;
        }

        // option 2: unset all variables of all script slots
        if (slot == -1 && variable.empty()) {
            scriptVars.clear();
            return;
        }

        // option 3: unset all variables of one particular script slot only
        if (slot != -1) {
            if (!script) return;
            const _UUID uuid = _UUIDFromCArray(&script->Uuid[0]);
            if (scriptVars.count(uuid))
                scriptVars.erase( scriptVars.find(uuid) );
            return;
        }

        // option 4: unset a particular variable of all script slots
        _VarsByScript::iterator itScript = scriptVars.begin();
        _VarsByScript::iterator endScript = scriptVars.end();
        while (itScript != endScript) {
            _VarsBySlot& slots = itScript->second;
            _VarsBySlot::iterator itSlot = slots.begin();
            _VarsBySlot::iterator endSlot = slots.end();
            while (itSlot != endSlot) {
                _PatchVars& vars = itSlot->second;
                if (vars.count(variable))
                    vars.erase( vars.find(variable) );
                if (vars.empty())
                    slots.erase(itSlot++); // postfix increment to avoid iterator invalidation
                else
                    ++itSlot;
            }
            if (slots.empty())
                scriptVars.erase(itScript++); // postfix increment to avoid iterator invalidation
            else
                ++itScript;
        }
    }

    /**
     * Returns stripped version of member variable @c scriptVars, where scripts
     * no longer referenced by this @c Instrument are filtered out, and so are
     * variables of meanwhile obsolete slots (i.e. a script still being
     * referenced, but previously overridden on a script slot which either no
     * longer exists or is hosting another script now).
     */
    Instrument::_VarsByScript Instrument::stripScriptVars() {
        _VarsByScript vars;
        _VarsByScript::const_iterator itScript = scriptVars.begin();
        _VarsByScript::const_iterator endScript = scriptVars.end();
        for (; itScript != endScript; ++itScript) {
            const _UUID& uuid = itScript->first;
            if (!ReferencesScriptWithUuid(uuid))
                continue;
            const _VarsBySlot& slots = itScript->second;
            _VarsBySlot::const_iterator itSlot = slots.begin();
            _VarsBySlot::const_iterator endSlot = slots.end();
            for (; itSlot != endSlot; ++itSlot) {
                Script* script = GetScriptOfSlot(itSlot->first);
                if (!script) continue;
                if (_UUIDFromCArray(&script->Uuid[0]) != uuid) continue;
                if (itSlot->second.empty()) continue;
                vars[uuid][itSlot->first] = itSlot->second;
            }
        }
        return vars;
    }

    /**
     * Make a (semi) deep copy of the Instrument object given by @a orig
     * and assign it to this object.
     *
     * Note that all sample pointers referenced by @a orig are simply copied as
     * memory address. Thus the respective samples are shared, not duplicated!
     *
     * @param orig - original Instrument object to be copied from
     */
    void Instrument::CopyAssign(const Instrument* orig) {
        CopyAssign(orig, NULL);
    }
        
    /**
     * Make a (semi) deep copy of the Instrument object given by @a orig
     * and assign it to this object.
     *
     * @param orig - original Instrument object to be copied from
     * @param mSamples - crosslink map between the foreign file's samples and
     *                   this file's samples
     */
    void Instrument::CopyAssign(const Instrument* orig, const std::map<Sample*,Sample*>* mSamples) {
        // handle base class
        // (without copying DLS region stuff)
        DLS::Instrument::CopyAssignCore(orig);
        
        // handle own member variables
        Attenuation = orig->Attenuation;
        EffectSend = orig->EffectSend;
        FineTune = orig->FineTune;
        PitchbendRange = orig->PitchbendRange;
        PianoReleaseMode = orig->PianoReleaseMode;
        DimensionKeyRange = orig->DimensionKeyRange;
        scriptPoolFileOffsets = orig->scriptPoolFileOffsets;
        // deep copy of pScriptRefs required (to avoid undefined behaviour)
        if (pScriptRefs) delete pScriptRefs;
        pScriptRefs = new std::vector<_ScriptPooolRef>;
        if (orig->pScriptRefs)
            *pScriptRefs = *orig->pScriptRefs;
        scriptVars = orig->scriptVars;
        
        // free old midi rules
        for (int i = 0 ; pMidiRules[i] ; i++) {
            delete pMidiRules[i];
        }
        //TODO: MIDI rule copying
        pMidiRules[0] = NULL;
        
        // delete all old regions
        while (Regions) DeleteRegion(GetFirstRegion());
        // create new regions and copy them from original
        {
            RegionList::const_iterator it = orig->pRegions->begin();
            for (int i = 0; i < orig->Regions; ++i, ++it) {
                Region* dstRgn = AddRegion();
                //NOTE: Region does semi-deep copy !
                dstRgn->CopyAssign(
                    static_cast<gig::Region*>(*it),
                    mSamples
                );
            }
        }

        UpdateRegionKeyTable();
    }

    /**
     * Returns @c true in case this Instrument object uses any gig format
     * extension, that is e.g. whether any DimensionRegion object currently
     * has any setting effective that would require our "LSDE" RIFF chunk to
     * be stored to the gig file.
     *
     * Right now this is a private method. It is considerable though this method
     * to become (in slightly modified form) a public API method in future, i.e.
     * to allow instrument editors to visualize and/or warn the user of any gig
     * format extension being used. See also comments on
     * DimensionRegion::UsesAnyGigFormatExtension() for details about such a
     * potential public API change in future.
     */
    bool Instrument::UsesAnyGigFormatExtension() const {
        if (!pRegions) return false;
        if (!scriptVars.empty()) return true;
        RegionList::const_iterator iter = pRegions->begin();
        RegionList::const_iterator end  = pRegions->end();
        for (; iter != end; ++iter) {
            gig::Region* rgn = static_cast<gig::Region*>(*iter);
            if (rgn->UsesAnyGigFormatExtension())
                return true;
        }
        return false;
    }


// *************** Group ***************
// *

    /** @brief Constructor.
     *
     * @param file   - pointer to the gig::File object
     * @param ck3gnm - pointer to 3gnm chunk associated with this group or
     *                 NULL if this is a new Group
     */
    Group::Group(File* file, RIFF::Chunk* ck3gnm) {
        pFile      = file;
        pNameChunk = ck3gnm;
        ::LoadString(pNameChunk, Name);
    }

    /** @brief Destructor.
     *
     * Currently this destructor implementation does nothing.
     */
    Group::~Group() {
    }

    /** @brief Remove all RIFF chunks associated with this Group object.
     *
     * See DLS::Storage::DeleteChunks() for details.
     */
    void Group::DeleteChunks() {
        // handle own RIFF chunks
        if (pNameChunk) {
            pNameChunk->GetParent()->DeleteSubChunk(pNameChunk);
            pNameChunk = NULL;
        }
    }

    /** @brief Update chunks with current group settings.
     *
     * Apply current Group field values to the respective chunks. You have
     * to call File::Save() to make changes persistent.
     *
     * Usually there is absolutely no need to call this method explicitly.
     * It will be called automatically when File::Save() was called.
     *
     * @param pProgress - callback function for progress notification
     */
    void Group::UpdateChunks(progress_t* pProgress) {
        // make sure <3gri> and <3gnl> list chunks exist
        RIFF::List* _3gri = pFile->pRIFF->GetSubList(LIST_TYPE_3GRI);
        if (!_3gri) {
            _3gri = pFile->pRIFF->AddSubList(LIST_TYPE_3GRI);
            pFile->pRIFF->MoveSubChunk(_3gri, pFile->pRIFF->GetSubChunk(CHUNK_ID_PTBL));
        }
        RIFF::List* _3gnl = _3gri->GetSubList(LIST_TYPE_3GNL);
        if (!_3gnl) _3gnl = _3gri->AddSubList(LIST_TYPE_3GNL);

        if (!pNameChunk && pFile->pVersion && pFile->pVersion->major > 2) {
            // v3 has a fixed list of 128 strings, find a free one
            for (RIFF::Chunk* ck = _3gnl->GetFirstSubChunk() ; ck ; ck = _3gnl->GetNextSubChunk()) {
                if (strcmp(static_cast<char*>(ck->LoadChunkData()), "") == 0) {
                    pNameChunk = ck;
                    break;
                }
            }
        }

        // now store the name of this group as <3gnm> chunk as subchunk of the <3gnl> list chunk
        ::SaveString(CHUNK_ID_3GNM, pNameChunk, _3gnl, Name, String("Unnamed Group"), true, 64);
    }

    /**
     * Returns the first Sample of this Group. You have to call this method
     * once before you use GetNextSample().
     *
     * <b>Notice:</b> this method might block for a long time, in case the
     * samples of this .gig file were not scanned yet
     *
     * @returns  pointer address to first Sample or NULL if there is none
     *           applied to this Group
     * @see      GetNextSample()
     */
    Sample* Group::GetFirstSample() {
        // FIXME: lazy und unsafe implementation, should be an autonomous iterator
        for (Sample* pSample = pFile->GetFirstSample(); pSample; pSample = pFile->GetNextSample()) {
            if (pSample->GetGroup() == this) return pSample;
        }
        return NULL;
    }

    /**
     * Returns the next Sample of the Group. You have to call
     * GetFirstSample() once before you can use this method. By calling this
     * method multiple times it iterates through the Samples assigned to
     * this Group.
     *
     * @returns  pointer address to the next Sample of this Group or NULL if
     *           end reached
     * @see      GetFirstSample()
     */
    Sample* Group::GetNextSample() {
        // FIXME: lazy und unsafe implementation, should be an autonomous iterator
        for (Sample* pSample = pFile->GetNextSample(); pSample; pSample = pFile->GetNextSample()) {
            if (pSample->GetGroup() == this) return pSample;
        }
        return NULL;
    }

    /**
     * Move Sample given by \a pSample from another Group to this Group.
     */
    void Group::AddSample(Sample* pSample) {
        pSample->pGroup = this;
    }

    /**
     * Move all members of this group to another group (preferably the 1st
     * one except this). This method is called explicitly by
     * File::DeleteGroup() thus when a Group was deleted. This code was
     * intentionally not placed in the destructor!
     */
    void Group::MoveAll() {
        // get "that" other group first
        Group* pOtherGroup = NULL;
        for (pOtherGroup = pFile->GetFirstGroup(); pOtherGroup; pOtherGroup = pFile->GetNextGroup()) {
            if (pOtherGroup != this) break;
        }
        if (!pOtherGroup) throw Exception(
            "Could not move samples to another group, since there is no "
            "other Group. This is a bug, report it!"
        );
        // now move all samples of this group to the other group
        for (Sample* pSample = GetFirstSample(); pSample; pSample = GetNextSample()) {
            pOtherGroup->AddSample(pSample);
        }
    }



// *************** File ***************
// *

    /// Reflects Gigasampler file format version 2.0 (1998-06-28).
    const DLS::version_t File::VERSION_2 = {
        0, 2, 19980628 & 0xffff, 19980628 >> 16
    };

    /// Reflects Gigasampler file format version 3.0 (2003-03-31).
    const DLS::version_t File::VERSION_3 = {
        0, 3, 20030331 & 0xffff, 20030331 >> 16
    };

    /// Reflects Gigasampler file format version 4.0 (2007-10-12).
    const DLS::version_t File::VERSION_4 = {
        0, 4, 20071012 & 0xffff, 20071012 >> 16
    };

    static const DLS::Info::string_length_t _FileFixedStringLengths[] = {
        { CHUNK_ID_IARL, 256 },
        { CHUNK_ID_IART, 128 },
        { CHUNK_ID_ICMS, 128 },
        { CHUNK_ID_ICMT, 1024 },
        { CHUNK_ID_ICOP, 128 },
        { CHUNK_ID_ICRD, 128 },
        { CHUNK_ID_IENG, 128 },
        { CHUNK_ID_IGNR, 128 },
        { CHUNK_ID_IKEY, 128 },
        { CHUNK_ID_IMED, 128 },
        { CHUNK_ID_INAM, 128 },
        { CHUNK_ID_IPRD, 128 },
        { CHUNK_ID_ISBJ, 128 },
        { CHUNK_ID_ISFT, 128 },
        { CHUNK_ID_ISRC, 128 },
        { CHUNK_ID_ISRF, 128 },
        { CHUNK_ID_ITCH, 128 },
        { 0, 0 }
    };

    File::File() : DLS::File() {
        bAutoLoad = true;
        *pVersion = VERSION_3;
        pGroups = NULL;
        pScriptGroups = NULL;
        pInfo->SetFixedStringLengths(_FileFixedStringLengths);
        pInfo->ArchivalLocation = String(256, ' ');

        // add some mandatory chunks to get the file chunks in right
        // order (INFO chunk will be moved to first position later)
        pRIFF->AddSubChunk(CHUNK_ID_VERS, 8);
        pRIFF->AddSubChunk(CHUNK_ID_COLH, 4);
        pRIFF->AddSubChunk(CHUNK_ID_DLID, 16);

        GenerateDLSID();
    }

    File::File(RIFF::File* pRIFF) : DLS::File(pRIFF) {
        bAutoLoad = true;
        pGroups = NULL;
        pScriptGroups = NULL;
        pInfo->SetFixedStringLengths(_FileFixedStringLengths);
    }

    File::~File() {
        if (pGroups) {
            std::list<Group*>::iterator iter = pGroups->begin();
            std::list<Group*>::iterator end  = pGroups->end();
            while (iter != end) {
                delete *iter;
                ++iter;
            }
            delete pGroups;
        }
        if (pScriptGroups) {
            std::list<ScriptGroup*>::iterator iter = pScriptGroups->begin();
            std::list<ScriptGroup*>::iterator end  = pScriptGroups->end();
            while (iter != end) {
                delete *iter;
                ++iter;
            }
            delete pScriptGroups;
        }
    }

    Sample* File::GetFirstSample(progress_t* pProgress) {
        if (!pSamples) LoadSamples(pProgress);
        if (!pSamples) return NULL;
        SamplesIterator = pSamples->begin();
        return static_cast<gig::Sample*>( (SamplesIterator != pSamples->end()) ? *SamplesIterator : NULL );
    }

    Sample* File::GetNextSample() {
        if (!pSamples) return NULL;
        SamplesIterator++;
        return static_cast<gig::Sample*>( (SamplesIterator != pSamples->end()) ? *SamplesIterator : NULL );
    }
    
    /**
     * Returns Sample object of @a index.
     *
     * @returns sample object or NULL if index is out of bounds
     */
    Sample* File::GetSample(uint index) {
        if (!pSamples) LoadSamples();
        if (!pSamples) return NULL;
        DLS::File::SampleList::iterator it = pSamples->begin();
        for (int i = 0; i < index; ++i) {
            ++it;
            if (it == pSamples->end()) return NULL;
        }
        if (it == pSamples->end()) return NULL;
        return static_cast<gig::Sample*>( *it );
    }

    /**
     * Returns the total amount of samples of this gig file.
     *
     * Note that this method might block for a long time in case it is required
     * to load the sample info for the first time.
     *
     * @returns total amount of samples
     */
    size_t File::CountSamples() {
        if (!pSamples) LoadSamples();
        if (!pSamples) return 0;
        return pSamples->size();
    }

    /** @brief Add a new sample.
     *
     * This will create a new Sample object for the gig file. You have to
     * call Save() to make this persistent to the file.
     *
     * @returns pointer to new Sample object
     */
    Sample* File::AddSample() {
       if (!pSamples) LoadSamples();
       __ensureMandatoryChunksExist();
       RIFF::List* wvpl = pRIFF->GetSubList(LIST_TYPE_WVPL);
       // create new Sample object and its respective 'wave' list chunk
       RIFF::List* wave = wvpl->AddSubList(LIST_TYPE_WAVE);
       Sample* pSample = new Sample(this, wave, 0 /*arbitrary value, we update offsets when we save*/);

       // add mandatory chunks to get the chunks in right order
       wave->AddSubChunk(CHUNK_ID_FMT, 16);
       wave->AddSubList(LIST_TYPE_INFO);

       pSamples->push_back(pSample);
       return pSample;
    }

    /** @brief Delete a sample.
     *
     * This will delete the given Sample object from the gig file. Any
     * references to this sample from Regions and DimensionRegions will be
     * removed. You have to call Save() to make this persistent to the file.
     *
     * @param pSample - sample to delete
     * @throws gig::Exception if given sample could not be found
     */
    void File::DeleteSample(Sample* pSample) {
        if (!pSamples || !pSamples->size()) throw gig::Exception("Could not delete sample as there are no samples");
        SampleList::iterator iter = find(pSamples->begin(), pSamples->end(), (DLS::Sample*) pSample);
        if (iter == pSamples->end()) throw gig::Exception("Could not delete sample, could not find given sample");
        if (SamplesIterator != pSamples->end() && *SamplesIterator == pSample) ++SamplesIterator; // avoid iterator invalidation
        pSamples->erase(iter);
        pSample->DeleteChunks();
        delete pSample;

        SampleList::iterator tmp = SamplesIterator;
        // remove all references to the sample
        for (Instrument* instrument = GetFirstInstrument() ; instrument ;
             instrument = GetNextInstrument()) {
            for (Region* region = instrument->GetFirstRegion() ; region ;
                 region = instrument->GetNextRegion()) {

                if (region->GetSample() == pSample) region->SetSample(NULL);

                for (int i = 0 ; i < region->DimensionRegions ; i++) {
                    gig::DimensionRegion *d = region->pDimensionRegions[i];
                    if (d->pSample == pSample) d->pSample = NULL;
                }
            }
        }
        SamplesIterator = tmp; // restore iterator
    }

    void File::LoadSamples() {
        LoadSamples(NULL);
    }

    void File::LoadSamples(progress_t* pProgress) {
        // Groups must be loaded before samples, because samples will try
        // to resolve the group they belong to
        if (!pGroups) LoadGroups();

        if (!pSamples) pSamples = new SampleList;

        RIFF::File* file = pRIFF;

        // just for progress calculation
        int iSampleIndex  = 0;
        int iTotalSamples = WavePoolCount;

        // just for assembling path of optional extension files to be read
        const std::string folder = parentPath(pRIFF->GetFileName());
        const std::string baseName = pathWithoutExtension(pRIFF->GetFileName());

        // the main gig file and the extension files (.gx01, ... , .gx98) may
        // contain wave data (wave pool)
        std::vector<RIFF::File*> poolFiles;
        poolFiles.push_back(pRIFF);

        // get info about all extension files
        RIFF::Chunk* ckXfil = pRIFF->GetSubChunk(CHUNK_ID_XFIL);
        if (ckXfil) { // there are extension files (.gx01, ... , .gx98) ...
            const uint32_t n = ckXfil->ReadInt32();
            for (int i = 0; i < n; i++) {
                // read the filename and load the extension file
                std::string name;
                ckXfil->ReadString(name, 128);
                std::string path = concatPath(folder, name);
                RIFF::File* pExtFile = new RIFF::File(path);
                // check that the dlsids match
                RIFF::Chunk* ckDLSID = pExtFile->GetSubChunk(CHUNK_ID_DLID);
                if (ckDLSID) {
                    ::DLS::dlsid_t idExpected;
                    idExpected.ulData1 = ckXfil->ReadInt32();
                    idExpected.usData2 = ckXfil->ReadInt16();
                    idExpected.usData3 = ckXfil->ReadInt16();
                    ckXfil->Read(idExpected.abData, 8, 1);
                    ::DLS::dlsid_t idFound;
                    ckDLSID->Read(&idFound.ulData1, 1, 4);
                    ckDLSID->Read(&idFound.usData2, 1, 2);
                    ckDLSID->Read(&idFound.usData3, 1, 2);
                    ckDLSID->Read(idFound.abData, 8, 1);
                    if (memcmp(&idExpected, &idFound, 16) != 0)
                        throw gig::Exception("dlsid mismatch for extension file: %s", path.c_str());
                }
                poolFiles.push_back(pExtFile);
                ExtensionFiles.push_back(pExtFile);
            }
        }

        // check if a .gx99 (GigaPulse) file exists
        RIFF::Chunk* ckDoxf = pRIFF->GetSubChunk(CHUNK_ID_DOXF);
        if (ckDoxf) { // there is a .gx99 (GigaPulse) file ...
            std::string path = baseName + ".gx99";
            RIFF::File* pExtFile = new RIFF::File(path);

            // skip unused int and filename
            ckDoxf->SetPos(132, RIFF::stream_curpos);

            // check that the dlsids match
            RIFF::Chunk* ckDLSID = pExtFile->GetSubChunk(CHUNK_ID_DLID);
            if (ckDLSID) {
                ::DLS::dlsid_t idExpected;
                idExpected.ulData1 = ckDoxf->ReadInt32();
                idExpected.usData2 = ckDoxf->ReadInt16();
                idExpected.usData3 = ckDoxf->ReadInt16();
                ckDoxf->Read(idExpected.abData, 8, 1);
                ::DLS::dlsid_t idFound;
                ckDLSID->Read(&idFound.ulData1, 1, 4);
                ckDLSID->Read(&idFound.usData2, 1, 2);
                ckDLSID->Read(&idFound.usData3, 1, 2);
                ckDLSID->Read(idFound.abData, 8, 1);
                if (memcmp(&idExpected, &idFound, 16) != 0)
                    throw gig::Exception("dlsid mismatch for GigaPulse file: %s", path.c_str());
            }
            poolFiles.push_back(pExtFile);
            ExtensionFiles.push_back(pExtFile);
        }

        // load samples from extension files (if required)
        for (int i = 0; i < poolFiles.size(); i++) {
            RIFF::File* file = poolFiles[i];
            RIFF::List* wvpl = file->GetSubList(LIST_TYPE_WVPL);
            if (wvpl) {
                file_offset_t wvplFileOffset = wvpl->GetFilePos() -
                                               wvpl->GetPos(); // should be zero, but just to be sure
                RIFF::List* wave = wvpl->GetFirstSubList();
                while (wave) {
                    if (wave->GetListType() == LIST_TYPE_WAVE) {
                        // notify current progress
                        if (pProgress) {
                            const float subprogress = (float) iSampleIndex / (float) iTotalSamples;
                            __notify_progress(pProgress, subprogress);
                        }

                        file_offset_t waveFileOffset = wave->GetFilePos();
                        pSamples->push_back(new Sample(this, wave, waveFileOffset - wvplFileOffset, i, iSampleIndex));

                        iSampleIndex++;
                    }
                    wave = wvpl->GetNextSubList();
                }
            }
        }

        if (pProgress)
            __notify_progress(pProgress, 1.0); // notify done
    }

    Instrument* File::GetFirstInstrument() {
        if (!pInstruments) LoadInstruments();
        if (!pInstruments) return NULL;
        InstrumentsIterator = pInstruments->begin();
        return static_cast<gig::Instrument*>( (InstrumentsIterator != pInstruments->end()) ? *InstrumentsIterator : NULL );
    }

    Instrument* File::GetNextInstrument() {
        if (!pInstruments) return NULL;
        InstrumentsIterator++;
        return static_cast<gig::Instrument*>( (InstrumentsIterator != pInstruments->end()) ? *InstrumentsIterator : NULL );
    }

    /**
     * Returns the total amount of instruments of this gig file.
     *
     * Note that this method might block for a long time in case it is required
     * to load the instruments info for the first time.
     *
     * @returns total amount of instruments
     */
    size_t File::CountInstruments() {
        if (!pInstruments) LoadInstruments();
        if (!pInstruments) return 0;
        return pInstruments->size();
    }

    /**
     * Returns the instrument with the given index.
     *
     * @param index     - number of the sought instrument (0..n)
     * @param pProgress - optional: callback function for progress notification
     * @returns  sought instrument or NULL if there's no such instrument
     */
    Instrument* File::GetInstrument(uint index, progress_t* pProgress) {
        if (!pInstruments) {
            // TODO: hack - we simply load ALL samples here, it would have been done in the Region constructor anyway (ATM)

            if (pProgress) {
                // sample loading subtask
                progress_t subprogress;
                __divide_progress(pProgress, &subprogress, 3.0f, 0.0f); // randomly schedule 33% for this subtask
                __notify_progress(&subprogress, 0.0f);
                if (GetAutoLoad())
                    GetFirstSample(&subprogress); // now force all samples to be loaded
                __notify_progress(&subprogress, 1.0f);

                // instrument loading subtask
                if (pProgress->callback) {
                    subprogress.__range_min = subprogress.__range_max;
                    subprogress.__range_max = pProgress->__range_max; // schedule remaining percentage for this subtask
                }
                __notify_progress(&subprogress, 0.0f);
                LoadInstruments(&subprogress);
                __notify_progress(&subprogress, 1.0f);
            } else {
                // sample loading subtask
                if (GetAutoLoad())
                    GetFirstSample(); // now force all samples to be loaded

                // instrument loading subtask
                LoadInstruments();
            }
        }
        if (!pInstruments) return NULL;
        InstrumentsIterator = pInstruments->begin();
        for (uint i = 0; InstrumentsIterator != pInstruments->end(); i++) {
            if (i == index) return static_cast<gig::Instrument*>( *InstrumentsIterator );
            InstrumentsIterator++;
        }
        return NULL;
    }

    /** @brief Add a new instrument definition.
     *
     * This will create a new Instrument object for the gig file. You have
     * to call Save() to make this persistent to the file.
     *
     * @returns pointer to new Instrument object
     */
    Instrument* File::AddInstrument() {
       if (!pInstruments) LoadInstruments();
       __ensureMandatoryChunksExist();
       RIFF::List* lstInstruments = pRIFF->GetSubList(LIST_TYPE_LINS);
       RIFF::List* lstInstr = lstInstruments->AddSubList(LIST_TYPE_INS);

       // add mandatory chunks to get the chunks in right order
       lstInstr->AddSubList(LIST_TYPE_INFO);
       lstInstr->AddSubChunk(CHUNK_ID_DLID, 16);

       Instrument* pInstrument = new Instrument(this, lstInstr);
       pInstrument->GenerateDLSID();

       lstInstr->AddSubChunk(CHUNK_ID_INSH, 12);

       // this string is needed for the gig to be loadable in GSt:
       pInstrument->pInfo->Software = "Endless Wave";

       pInstruments->push_back(pInstrument);
       return pInstrument;
    }
    
    /** @brief Add a duplicate of an existing instrument.
     *
     * Duplicates the instrument definition given by @a orig and adds it
     * to this file. This allows in an instrument editor application to
     * easily create variations of an instrument, which will be stored in
     * the same .gig file, sharing i.e. the same samples.
     *
     * Note that all sample pointers referenced by @a orig are simply copied as
     * memory address. Thus the respective samples are shared, not duplicated!
     *
     * You have to call Save() to make this persistent to the file.
     *
     * @param orig - original instrument to be copied
     * @returns duplicated copy of the given instrument
     */
    Instrument* File::AddDuplicateInstrument(const Instrument* orig) {
        Instrument* instr = AddInstrument();
        instr->CopyAssign(orig);
        return instr;
    }
    
    /** @brief Add content of another existing file.
     *
     * Duplicates the samples, groups and instruments of the original file
     * given by @a pFile and adds them to @c this File. In case @c this File is
     * a new one that you haven't saved before, then you have to call
     * SetFileName() before calling AddContentOf(), because this method will
     * automatically save this file during operation, which is required for
     * writing the sample waveform data by disk streaming.
     *
     * @param pFile - original file whose's content shall be copied from
     */
    void File::AddContentOf(File* pFile) {
        static int iCallCount = -1;
        iCallCount++;
        std::map<Group*,Group*> mGroups;
        std::map<Sample*,Sample*> mSamples;
        
        // clone sample groups
        for (int i = 0; pFile->GetGroup(i); ++i) {
            Group* g = AddGroup();
            g->Name =
                "COPY" + ToString(iCallCount) + "_" + pFile->GetGroup(i)->Name;
            mGroups[pFile->GetGroup(i)] = g;
        }
        
        // clone samples (not waveform data here yet)
        for (int i = 0; pFile->GetSample(i); ++i) {
            Sample* s = AddSample();
            s->CopyAssignMeta(pFile->GetSample(i));
            mGroups[pFile->GetSample(i)->GetGroup()]->AddSample(s);
            mSamples[pFile->GetSample(i)] = s;
        }

        // clone script groups and their scripts
        for (int iGroup = 0; pFile->GetScriptGroup(iGroup); ++iGroup) {
            ScriptGroup* sg = pFile->GetScriptGroup(iGroup);
            ScriptGroup* dg = AddScriptGroup();
            dg->Name = "COPY" + ToString(iCallCount) + "_" + sg->Name;
            for (int iScript = 0; sg->GetScript(iScript); ++iScript) {
                Script* ss = sg->GetScript(iScript);
                Script* ds = dg->AddScript();
                ds->CopyAssign(ss);
            }
        }

        //BUG: For some reason this method only works with this additional
        //     Save() call in between here.
        //
        // Important: The correct one of the 2 Save() methods has to be called
        // here, depending on whether the file is completely new or has been
        // saved to disk already, otherwise it will result in data corruption.
        if (pRIFF->IsNew())
            Save(GetFileName());
        else
            Save();
        
        // clone instruments
        // (passing the crosslink table here for the cloned samples)
        for (int i = 0; pFile->GetInstrument(i); ++i) {
            Instrument* instr = AddInstrument();
            instr->CopyAssign(pFile->GetInstrument(i), &mSamples);
        }
        
        // Mandatory: file needs to be saved to disk at this point, so this
        // file has the correct size and data layout for writing the samples'
        // waveform data to disk.
        Save();
        
        // clone samples' waveform data
        // (using direct read & write disk streaming)
        for (int i = 0; pFile->GetSample(i); ++i) {
            mSamples[pFile->GetSample(i)]->CopyAssignWave(pFile->GetSample(i));
        }
    }

    /** @brief Delete an instrument.
     *
     * This will delete the given Instrument object from the gig file. You
     * have to call Save() to make this persistent to the file.
     *
     * @param pInstrument - instrument to delete
     * @throws gig::Exception if given instrument could not be found
     */
    void File::DeleteInstrument(Instrument* pInstrument) {
        if (!pInstruments) throw gig::Exception("Could not delete instrument as there are no instruments");
        InstrumentList::iterator iter = find(pInstruments->begin(), pInstruments->end(), (DLS::Instrument*) pInstrument);
        if (iter == pInstruments->end()) throw gig::Exception("Could not delete instrument, could not find given instrument");
        pInstruments->erase(iter);
        pInstrument->DeleteChunks();
        delete pInstrument;
    }

    void File::LoadInstruments() {
        LoadInstruments(NULL);
    }

    void File::LoadInstruments(progress_t* pProgress) {
        if (!pInstruments) pInstruments = new InstrumentList;
        RIFF::List* lstInstruments = pRIFF->GetSubList(LIST_TYPE_LINS);
        if (lstInstruments) {
            int iInstrumentIndex = 0;
            RIFF::List* lstInstr = lstInstruments->GetFirstSubList();
            while (lstInstr) {
                if (lstInstr->GetListType() == LIST_TYPE_INS) {
                    if (pProgress) {
                        // notify current progress
                        const float localProgress = (float) iInstrumentIndex / (float) Instruments;
                        __notify_progress(pProgress, localProgress);

                        // divide local progress into subprogress for loading current Instrument
                        progress_t subprogress;
                        __divide_progress(pProgress, &subprogress, Instruments, iInstrumentIndex);

                        pInstruments->push_back(new Instrument(this, lstInstr, &subprogress));
                    } else {
                        pInstruments->push_back(new Instrument(this, lstInstr));
                    }

                    iInstrumentIndex++;
                }
                lstInstr = lstInstruments->GetNextSubList();
            }
            if (pProgress)
                __notify_progress(pProgress, 1.0); // notify done
        }
    }

    /// Updates the 3crc chunk with the checksum of a sample. The
    /// update is done directly to disk, as this method is called
    /// after File::Save()
    void File::SetSampleChecksum(Sample* pSample, uint32_t crc) {
        RIFF::Chunk* _3crc = pRIFF->GetSubChunk(CHUNK_ID_3CRC);
        if (!_3crc) return;

        // get the index of the sample
        int iWaveIndex = GetWaveTableIndexOf(pSample);
        if (iWaveIndex < 0) throw gig::Exception("Could not update crc, could not find sample");

        // write the CRC-32 checksum to disk
        _3crc->SetPos(iWaveIndex * 8);
        uint32_t one = 1;
        _3crc->WriteUint32(&one); // always 1
        _3crc->WriteUint32(&crc);
    }

    uint32_t File::GetSampleChecksum(Sample* pSample) {
        // get the index of the sample
        int iWaveIndex = GetWaveTableIndexOf(pSample);
        if (iWaveIndex < 0) throw gig::Exception("Could not retrieve reference crc of sample, could not resolve sample's wave table index");

        return GetSampleChecksumByIndex(iWaveIndex);
    }

    uint32_t File::GetSampleChecksumByIndex(int index) {
        if (index < 0) throw gig::Exception("Could not retrieve reference crc of sample, invalid wave pool index of sample");

        RIFF::Chunk* _3crc = pRIFF->GetSubChunk(CHUNK_ID_3CRC);
        if (!_3crc) throw gig::Exception("Could not retrieve reference crc of sample, no checksums stored for this file yet");
        uint8_t* pData = (uint8_t*) _3crc->LoadChunkData();
        if (!pData) throw gig::Exception("Could not retrieve reference crc of sample, no checksums stored for this file yet");

        // read the CRC-32 checksum directly from disk
        size_t pos = index * 8;
        if (pos + 8 > _3crc->GetNewSize())
            throw gig::Exception("Could not retrieve reference crc of sample, could not seek to required position in crc chunk");

        uint32_t one = load32(&pData[pos]); // always 1
        if (one != 1)
            throw gig::Exception("Could not retrieve reference crc of sample, because reference checksum table is damaged");

        return load32(&pData[pos+4]);
    }

    int File::GetWaveTableIndexOf(gig::Sample* pSample) {
        if (!pSamples) GetFirstSample(); // make sure sample chunks were scanned
        File::SampleList::iterator iter = pSamples->begin();
        File::SampleList::iterator end  = pSamples->end();
        for (int index = 0; iter != end; ++iter, ++index)
            if (*iter == pSample)
                return index;
        return -1;
    }

    /**
     * Checks whether the file's "3CRC" chunk was damaged. This chunk contains
     * the CRC32 check sums of all samples' raw wave data.
     *
     * @return true if 3CRC chunk is OK, or false if 3CRC chunk is damaged
     */
    bool File::VerifySampleChecksumTable() {
        RIFF::Chunk* _3crc = pRIFF->GetSubChunk(CHUNK_ID_3CRC);
        if (!_3crc) return false;
        if (_3crc->GetNewSize() <= 0) return false;
        if (_3crc->GetNewSize() % 8) return false;
        if (!pSamples) GetFirstSample(); // make sure sample chunks were scanned
        if (_3crc->GetNewSize() != pSamples->size() * 8) return false;

        const file_offset_t n = _3crc->GetNewSize() / 8;

        uint32_t* pData = (uint32_t*) _3crc->LoadChunkData();
        if (!pData) return false;

        for (file_offset_t i = 0; i < n; ++i) {
            uint32_t one = pData[i*2];
            if (one != 1) return false;
        }

        return true;
    }

    /**
     * Recalculates CRC32 checksums for all samples and rebuilds this gig
     * file's checksum table with those new checksums. This might usually
     * just be necessary if the checksum table was damaged.
     *
     * @e IMPORTANT: The current implementation of this method only works
     * with files that have not been modified since it was loaded, because
     * it expects that no externally caused file structure changes are
     * required!
     *
     * Due to the expectation above, this method is currently protected
     * and actually only used by the command line tool "gigdump" yet.
     *
     * @returns true if Save() is required to be called after this call,
     *          false if no further action is required
     */
    bool File::RebuildSampleChecksumTable() {
        // make sure sample chunks were scanned
        if (!pSamples) GetFirstSample();

        bool bRequiresSave = false;

        // make sure "3CRC" chunk exists with required size
        RIFF::Chunk* _3crc = pRIFF->GetSubChunk(CHUNK_ID_3CRC);
        if (!_3crc) {
            _3crc = pRIFF->AddSubChunk(CHUNK_ID_3CRC, pSamples->size() * 8);
            // the order of einf and 3crc is not the same in v2 and v3
            RIFF::Chunk* einf = pRIFF->GetSubChunk(CHUNK_ID_EINF);
            if (einf && pVersion && pVersion->major > 2) pRIFF->MoveSubChunk(_3crc, einf);
            bRequiresSave = true;
        } else if (_3crc->GetNewSize() != pSamples->size() * 8) {
            _3crc->Resize(pSamples->size() * 8);
            bRequiresSave = true;
        }

        if (bRequiresSave) { // refill CRC table for all samples in RAM ...
            uint32_t* pData = (uint32_t*) _3crc->LoadChunkData();
            {
                File::SampleList::iterator iter = pSamples->begin();
                File::SampleList::iterator end  = pSamples->end();
                for (; iter != end; ++iter) {
                    gig::Sample* pSample = (gig::Sample*) *iter;
                    int index = GetWaveTableIndexOf(pSample);
                    if (index < 0) throw gig::Exception("Could not rebuild crc table for samples, wave table index of a sample could not be resolved");
                    pData[index*2]   = 1; // always 1
                    pData[index*2+1] = pSample->CalculateWaveDataChecksum();
                }
            }
        } else { // no file structure changes necessary, so directly write to disk and we are done ...
            // make sure file is in write mode
            pRIFF->SetMode(RIFF::stream_mode_read_write);
            {
                File::SampleList::iterator iter = pSamples->begin();
                File::SampleList::iterator end  = pSamples->end();
                for (; iter != end; ++iter) {
                    gig::Sample* pSample = (gig::Sample*) *iter;
                    int index = GetWaveTableIndexOf(pSample);
                    if (index < 0) throw gig::Exception("Could not rebuild crc table for samples, wave table index of a sample could not be resolved");
                    pSample->crc  = pSample->CalculateWaveDataChecksum();
                    SetSampleChecksum(pSample, pSample->crc);
                }
            }
        }

        return bRequiresSave;
    }

    Group* File::GetFirstGroup() {
        if (!pGroups) LoadGroups();
        // there must always be at least one group
        GroupsIterator = pGroups->begin();
        return *GroupsIterator;
    }

    Group* File::GetNextGroup() {
        if (!pGroups) return NULL;
        ++GroupsIterator;
        return (GroupsIterator == pGroups->end()) ? NULL : *GroupsIterator;
    }

    /**
     * Returns the group with the given index.
     *
     * @param index - number of the sought group (0..n)
     * @returns sought group or NULL if there's no such group
     */
    Group* File::GetGroup(uint index) {
        if (!pGroups) LoadGroups();
        GroupsIterator = pGroups->begin();
        for (uint i = 0; GroupsIterator != pGroups->end(); i++) {
            if (i == index) return *GroupsIterator;
            ++GroupsIterator;
        }
        return NULL;
    }

    /**
     * Returns the group with the given group name.
     *
     * Note: group names don't have to be unique in the gig format! So there
     * can be multiple groups with the same name. This method will simply
     * return the first group found with the given name.
     *
     * @param name - name of the sought group
     * @returns sought group or NULL if there's no group with that name
     */
    Group* File::GetGroup(String name) {
        if (!pGroups) LoadGroups();
        GroupsIterator = pGroups->begin();
        for (uint i = 0; GroupsIterator != pGroups->end(); ++GroupsIterator, ++i)
            if ((*GroupsIterator)->Name == name) return *GroupsIterator;
        return NULL;
    }

    Group* File::AddGroup() {
        if (!pGroups) LoadGroups();
        // there must always be at least one group
        __ensureMandatoryChunksExist();
        Group* pGroup = new Group(this, NULL);
        pGroups->push_back(pGroup);
        return pGroup;
    }

    /** @brief Delete a group and its samples.
     *
     * This will delete the given Group object and all the samples that
     * belong to this group from the gig file. You have to call Save() to
     * make this persistent to the file.
     *
     * @param pGroup - group to delete
     * @throws gig::Exception if given group could not be found
     */
    void File::DeleteGroup(Group* pGroup) {
        if (!pGroups) LoadGroups();
        std::list<Group*>::iterator iter = find(pGroups->begin(), pGroups->end(), pGroup);
        if (iter == pGroups->end()) throw gig::Exception("Could not delete group, could not find given group");
        if (pGroups->size() == 1) throw gig::Exception("Cannot delete group, there must be at least one default group!");
        // delete all members of this group
        for (Sample* pSample = pGroup->GetFirstSample(); pSample; pSample = pGroup->GetNextSample()) {
            DeleteSample(pSample);
        }
        // now delete this group object
        pGroups->erase(iter);
        pGroup->DeleteChunks();
        delete pGroup;
    }

    /** @brief Delete a group.
     *
     * This will delete the given Group object from the gig file. All the
     * samples that belong to this group will not be deleted, but instead
     * be moved to another group. You have to call Save() to make this
     * persistent to the file.
     *
     * @param pGroup - group to delete
     * @throws gig::Exception if given group could not be found
     */
    void File::DeleteGroupOnly(Group* pGroup) {
        if (!pGroups) LoadGroups();
        std::list<Group*>::iterator iter = find(pGroups->begin(), pGroups->end(), pGroup);
        if (iter == pGroups->end()) throw gig::Exception("Could not delete group, could not find given group");
        if (pGroups->size() == 1) throw gig::Exception("Cannot delete group, there must be at least one default group!");
        // move all members of this group to another group
        pGroup->MoveAll();
        pGroups->erase(iter);
        pGroup->DeleteChunks();
        delete pGroup;
    }

    void File::LoadGroups() {
        if (!pGroups) pGroups = new std::list<Group*>;
        // try to read defined groups from file
        RIFF::List* lst3gri = pRIFF->GetSubList(LIST_TYPE_3GRI);
        if (lst3gri) {
            RIFF::List* lst3gnl = lst3gri->GetSubList(LIST_TYPE_3GNL);
            if (lst3gnl) {
                RIFF::Chunk* ck = lst3gnl->GetFirstSubChunk();
                while (ck) {
                    if (ck->GetChunkID() == CHUNK_ID_3GNM) {
                        if (pVersion && pVersion->major > 2 &&
                            strcmp(static_cast<char*>(ck->LoadChunkData()), "") == 0) break;

                        pGroups->push_back(new Group(this, ck));
                    }
                    ck = lst3gnl->GetNextSubChunk();
                }
            }
        }
        // if there were no group(s), create at least the mandatory default group
        if (!pGroups->size()) {
            Group* pGroup = new Group(this, NULL);
            pGroup->Name = "Default Group";
            pGroups->push_back(pGroup);
        }
    }

    /** @brief Get instrument script group (by index).
     *
     * Returns the real-time instrument script group with the given index.
     *
     * @param index - number of the sought group (0..n)
     * @returns sought script group or NULL if there's no such group
     */
    ScriptGroup* File::GetScriptGroup(uint index) {
        if (!pScriptGroups) LoadScriptGroups();
        std::list<ScriptGroup*>::iterator it = pScriptGroups->begin();
        for (uint i = 0; it != pScriptGroups->end(); ++i, ++it)
            if (i == index) return *it;
        return NULL;
    }

    /** @brief Get instrument script group (by name).
     *
     * Returns the first real-time instrument script group found with the given
     * group name. Note that group names may not necessarily be unique.
     *
     * @param name - name of the sought script group
     * @returns sought script group or NULL if there's no such group
     */
    ScriptGroup* File::GetScriptGroup(const String& name) {
        if (!pScriptGroups) LoadScriptGroups();
        std::list<ScriptGroup*>::iterator it = pScriptGroups->begin();
        for (uint i = 0; it != pScriptGroups->end(); ++i, ++it)
            if ((*it)->Name == name) return *it;
        return NULL;
    }

    /** @brief Add new instrument script group.
     *
     * Adds a new, empty real-time instrument script group to the file.
     *
     * You have to call Save() to make this persistent to the file.
     *
     * @return new empty script group
     */
    ScriptGroup* File::AddScriptGroup() {
        if (!pScriptGroups) LoadScriptGroups();
        ScriptGroup* pScriptGroup = new ScriptGroup(this, NULL);
        pScriptGroups->push_back(pScriptGroup);
        return pScriptGroup;
    }

    /** @brief Delete an instrument script group.
     *
     * This will delete the given real-time instrument script group and all its
     * instrument scripts it contains. References inside instruments that are
     * using the deleted scripts will be removed from the respective instruments
     * accordingly.
     *
     * You have to call Save() to make this persistent to the file.
     *
     * @param pScriptGroup - script group to delete
     * @throws gig::Exception if given script group could not be found
     */
    void File::DeleteScriptGroup(ScriptGroup* pScriptGroup) {
        if (!pScriptGroups) LoadScriptGroups();
        std::list<ScriptGroup*>::iterator iter =
            find(pScriptGroups->begin(), pScriptGroups->end(), pScriptGroup);
        if (iter == pScriptGroups->end())
            throw gig::Exception("Could not delete script group, could not find given script group");
        pScriptGroups->erase(iter);
        for (int i = 0; pScriptGroup->GetScript(i); ++i)
            pScriptGroup->DeleteScript(pScriptGroup->GetScript(i));
        if (pScriptGroup->pList)
            pScriptGroup->pList->GetParent()->DeleteSubChunk(pScriptGroup->pList);
        pScriptGroup->DeleteChunks();
        delete pScriptGroup;
    }

    void File::LoadScriptGroups() {
        if (pScriptGroups) return;
        pScriptGroups = new std::list<ScriptGroup*>;
        RIFF::List* lstLS = pRIFF->GetSubList(LIST_TYPE_3LS);
        if (lstLS) {
            for (RIFF::List* lst = lstLS->GetFirstSubList(); lst;
                 lst = lstLS->GetNextSubList())
            {
                if (lst->GetListType() == LIST_TYPE_RTIS) {
                    pScriptGroups->push_back(new ScriptGroup(this, lst));
                }
            }
        }
    }

    /**
     * Apply all the gig file's current instruments, samples, groups and settings
     * to the respective RIFF chunks. You have to call Save() to make changes
     * persistent.
     *
     * Usually there is absolutely no need to call this method explicitly.
     * It will be called automatically when File::Save() was called.
     *
     * @param pProgress - callback function for progress notification
     * @throws Exception - on errors
     */
    void File::UpdateChunks(progress_t* pProgress) {
        bool newFile = pRIFF->GetSubList(LIST_TYPE_INFO) == NULL;

        // update own gig format extension chunks
        // (not part of the GigaStudio 4 format)
        RIFF::List* lst3LS = pRIFF->GetSubList(LIST_TYPE_3LS);
        if (!lst3LS) {
            lst3LS = pRIFF->AddSubList(LIST_TYPE_3LS);
        }
        // Make sure <3LS > chunk is placed before <ptbl> chunk. The precise
        // location of <3LS > is irrelevant, however it should be located
        // before  the actual wave data
        RIFF::Chunk* ckPTBL = pRIFF->GetSubChunk(CHUNK_ID_PTBL);
        pRIFF->MoveSubChunk(lst3LS, ckPTBL);

        // This must be performed before writing the chunks for instruments,
        // because the instruments' script slots will write the file offsets
        // of the respective instrument script chunk as reference.
        if (pScriptGroups) {
            // Update instrument script (group) chunks.
            for (std::list<ScriptGroup*>::iterator it = pScriptGroups->begin();
                 it != pScriptGroups->end(); ++it)
            {
                (*it)->UpdateChunks(pProgress);
            }
        }

        // in case no libgig custom format data was added, then remove the
        // custom "3LS " chunk again
        if (!lst3LS->CountSubChunks()) {
            pRIFF->DeleteSubChunk(lst3LS);
            lst3LS = NULL;
        }

        // first update base class's chunks
        DLS::File::UpdateChunks(pProgress);

        if (newFile) {
            // INFO was added by Resource::UpdateChunks - make sure it
            // is placed first in file
            RIFF::Chunk* info = pRIFF->GetSubList(LIST_TYPE_INFO);
            RIFF::Chunk* first = pRIFF->GetFirstSubChunk();
            if (first != info) {
                pRIFF->MoveSubChunk(info, first);
            }
        }

        // update group's chunks
        if (pGroups) {
            // make sure '3gri' and '3gnl' list chunks exist
            // (before updating the Group chunks)
            RIFF::List* _3gri = pRIFF->GetSubList(LIST_TYPE_3GRI);
            if (!_3gri) {
                _3gri = pRIFF->AddSubList(LIST_TYPE_3GRI);
                pRIFF->MoveSubChunk(_3gri, pRIFF->GetSubChunk(CHUNK_ID_PTBL));
            }
            RIFF::List* _3gnl = _3gri->GetSubList(LIST_TYPE_3GNL);
            if (!_3gnl) _3gnl = _3gri->AddSubList(LIST_TYPE_3GNL);

            // v3: make sure the file has 128 3gnm chunks
            // (before updating the Group chunks)
            if (pVersion && pVersion->major > 2) {
                RIFF::Chunk* _3gnm = _3gnl->GetFirstSubChunk();
                for (int i = 0 ; i < 128 ; i++) {
                    // create 128 empty placeholder strings which will either
                    // be filled by Group::UpdateChunks below or left empty.
                    ::SaveString(CHUNK_ID_3GNM, _3gnm, _3gnl, "", "", true, 64);
                    if (_3gnm) _3gnm = _3gnl->GetNextSubChunk();
                }
            }

            std::list<Group*>::iterator iter = pGroups->begin();
            std::list<Group*>::iterator end  = pGroups->end();
            for (; iter != end; ++iter) {
                (*iter)->UpdateChunks(pProgress);
            }
        }

        // update einf chunk

        // The einf chunk contains statistics about the gig file, such
        // as the number of regions and samples used by each
        // instrument. It is divided in equally sized parts, where the
        // first part contains information about the whole gig file,
        // and the rest of the parts map to each instrument in the
        // file.
        //
        // At the end of each part there is a bit map of each sample
        // in the file, where a set bit means that the sample is used
        // by the file/instrument.
        //
        // Note that there are several fields with unknown use. These
        // are set to zero.

        int sublen = int(pSamples->size() / 8 + 49);
        int einfSize = (Instruments + 1) * sublen;

        RIFF::Chunk* einf = pRIFF->GetSubChunk(CHUNK_ID_EINF);
        if (einf) {
            if (einf->GetSize() != einfSize) {
                einf->Resize(einfSize);
                memset(einf->LoadChunkData(), 0, einfSize);
            }
        } else if (newFile) {
            einf = pRIFF->AddSubChunk(CHUNK_ID_EINF, einfSize);
        }
        if (einf) {
            uint8_t* pData = (uint8_t*) einf->LoadChunkData();

            std::map<gig::Sample*,int> sampleMap;
            int sampleIdx = 0;
            for (Sample* pSample = GetFirstSample(); pSample; pSample = GetNextSample()) {
                sampleMap[pSample] = sampleIdx++;
            }

            int totnbusedsamples = 0;
            int totnbusedchannels = 0;
            int totnbregions = 0;
            int totnbdimregions = 0;
            int totnbloops = 0;
            int instrumentIdx = 0;

            memset(&pData[48], 0, sublen - 48);

            for (Instrument* instrument = GetFirstInstrument() ; instrument ;
                 instrument = GetNextInstrument()) {
                int nbusedsamples = 0;
                int nbusedchannels = 0;
                int nbdimregions = 0;
                int nbloops = 0;

                memset(&pData[(instrumentIdx + 1) * sublen + 48], 0, sublen - 48);

                for (Region* region = instrument->GetFirstRegion() ; region ;
                     region = instrument->GetNextRegion()) {
                    for (int i = 0 ; i < region->DimensionRegions ; i++) {
                        gig::DimensionRegion *d = region->pDimensionRegions[i];
                        if (d->pSample) {
                            int sampleIdx = sampleMap[d->pSample];
                            int byte = 48 + sampleIdx / 8;
                            int bit = 1 << (sampleIdx & 7);
                            if ((pData[(instrumentIdx + 1) * sublen + byte] & bit) == 0) {
                                pData[(instrumentIdx + 1) * sublen + byte] |= bit;
                                nbusedsamples++;
                                nbusedchannels += d->pSample->Channels;

                                if ((pData[byte] & bit) == 0) {
                                    pData[byte] |= bit;
                                    totnbusedsamples++;
                                    totnbusedchannels += d->pSample->Channels;
                                }
                            }
                        }
                        if (d->SampleLoops) nbloops++;
                    }
                    nbdimregions += region->DimensionRegions;
                }
                // first 4 bytes unknown - sometimes 0, sometimes length of einf part
                // store32(&pData[(instrumentIdx + 1) * sublen], sublen);
                store32(&pData[(instrumentIdx + 1) * sublen + 4], nbusedchannels);
                store32(&pData[(instrumentIdx + 1) * sublen + 8], nbusedsamples);
                store32(&pData[(instrumentIdx + 1) * sublen + 12], 1);
                store32(&pData[(instrumentIdx + 1) * sublen + 16], instrument->Regions);
                store32(&pData[(instrumentIdx + 1) * sublen + 20], nbdimregions);
                store32(&pData[(instrumentIdx + 1) * sublen + 24], nbloops);
                // next 8 bytes unknown
                store32(&pData[(instrumentIdx + 1) * sublen + 36], instrumentIdx);
                store32(&pData[(instrumentIdx + 1) * sublen + 40], (uint32_t) pSamples->size());
                // next 4 bytes unknown

                totnbregions += instrument->Regions;
                totnbdimregions += nbdimregions;
                totnbloops += nbloops;
                instrumentIdx++;
            }
            // first 4 bytes unknown - sometimes 0, sometimes length of einf part
            // store32(&pData[0], sublen);
            store32(&pData[4], totnbusedchannels);
            store32(&pData[8], totnbusedsamples);
            store32(&pData[12], Instruments);
            store32(&pData[16], totnbregions);
            store32(&pData[20], totnbdimregions);
            store32(&pData[24], totnbloops);
            // next 8 bytes unknown
            // next 4 bytes unknown, not always 0
            store32(&pData[40], (uint32_t) pSamples->size());
            // next 4 bytes unknown
        }

        // update 3crc chunk

        // The 3crc chunk contains CRC-32 checksums for the
        // samples. When saving a gig file to disk, we first update the 3CRC
        // chunk here (in RAM) with the old crc values which we read from the
        // 3CRC chunk when we opened the file (available with gig::Sample::crc
        // member variable). This step is required, because samples might have
        // been deleted by the user since the file was opened, which in turn
        // changes the order of the (i.e. old) checksums within the 3crc chunk.
        // If a sample was conciously modified by the user (that is if
        // Sample::Write() was called later on) then Sample::Write() will just
        // update the respective individual checksum(s) directly on disk and
        // leaves all other sample checksums untouched.

        RIFF::Chunk* _3crc = pRIFF->GetSubChunk(CHUNK_ID_3CRC);
        if (_3crc) {
            _3crc->Resize(pSamples->size() * 8);
        } else /*if (newFile)*/ {
            _3crc = pRIFF->AddSubChunk(CHUNK_ID_3CRC, pSamples->size() * 8);
            // the order of einf and 3crc is not the same in v2 and v3
            if (einf && pVersion && pVersion->major > 2) pRIFF->MoveSubChunk(_3crc, einf);
        }
        { // must be performed in RAM here ...
            uint32_t* pData = (uint32_t*) _3crc->LoadChunkData();
            if (pData) {
                File::SampleList::iterator iter = pSamples->begin();
                File::SampleList::iterator end  = pSamples->end();
                for (int index = 0; iter != end; ++iter, ++index) {
                    gig::Sample* pSample = (gig::Sample*) *iter;
                    pData[index*2]   = 1; // always 1
                    pData[index*2+1] = pSample->crc;
                }
            }
        }
    }
    
    void File::UpdateFileOffsets() {
        DLS::File::UpdateFileOffsets();

        for (Instrument* instrument = GetFirstInstrument(); instrument;
             instrument = GetNextInstrument())
        {
            instrument->UpdateScriptFileOffsets();
        }
    }

    /**
     * Enable / disable automatic loading. By default this property is
     * enabled and every information is loaded automatically. However
     * loading all Regions, DimensionRegions and especially samples might
     * take a long time for large .gig files, and sometimes one might only
     * be interested in retrieving very superficial informations like the
     * amount of instruments and their names. In this case one might disable
     * automatic loading to avoid very slow response times.
     *
     * @e CAUTION: by disabling this property many pointers (i.e. sample
     * references) and attributes will have invalid or even undefined
     * data! This feature is currently only intended for retrieving very
     * superficial information in a very fast way. Don't use it to retrieve
     * details like synthesis information or even to modify .gig files!
     */
    void File::SetAutoLoad(bool b) {
        bAutoLoad = b;
    }

    /**
     * Returns whether automatic loading is enabled.
     * @see SetAutoLoad()
     */
    bool File::GetAutoLoad() {
        return bAutoLoad;
    }

    /**
     * Returns @c true in case this gig File object uses any gig format
     * extension, that is e.g. whether any DimensionRegion object currently
     * has any setting effective that would require our "LSDE" RIFF chunk to
     * be stored to the gig file.
     *
     * Right now this is a private method. It is considerable though this method
     * to become (in slightly modified form) a public API method in future, i.e.
     * to allow instrument editors to visualize and/or warn the user of any gig
     * format extension being used. See also comments on
     * DimensionRegion::UsesAnyGigFormatExtension() for details about such a
     * potential public API change in future.
     */
    bool File::UsesAnyGigFormatExtension() const {
        if (!pInstruments) return false;
        InstrumentList::iterator iter = pInstruments->begin();
        InstrumentList::iterator end  = pInstruments->end();
        for (; iter != end; ++iter) {
            Instrument* pInstrument = static_cast<gig::Instrument*>(*iter);
            if (pInstrument->UsesAnyGigFormatExtension())
                return true;
        }
        return false;
    }


// *************** Exception ***************
// *

    Exception::Exception() : DLS::Exception() {
    }

    Exception::Exception(String format, ...) : DLS::Exception() {
        va_list arg;
        va_start(arg, format);
        Message = assemble(format, arg);
        va_end(arg);
    }

    Exception::Exception(String format, va_list arg) : DLS::Exception() {
        Message = assemble(format, arg);
    }

    void Exception::PrintMessage() {
        std::cout << "gig::Exception: " << Message << std::endl;
    }


// *************** functions ***************
// *

    /**
     * Returns the name of this C++ library. This is usually "libgig" of
     * course. This call is equivalent to RIFF::libraryName() and
     * DLS::libraryName().
     */
    String libraryName() {
        return PACKAGE;
    }

    /**
     * Returns version of this C++ library. This call is equivalent to
     * RIFF::libraryVersion() and DLS::libraryVersion().
     */
    String libraryVersion() {
        return VERSION;
    }

} // namespace gig
