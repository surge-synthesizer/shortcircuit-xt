/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2014 Christian Schoenebeck                              *
 *                      <cuse@users.sourceforge.net>                       *
 *                                                                         *
 *   This library is part of libgig.                                       *
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

#include "Korg.h"

#include <string.h> // for memset()

#if WORDS_BIGENDIAN
# define CHUNK_ID_MSP1  0x4d535031
# define CHUNK_ID_RLP1  0x524c5031
# define CHUNK_ID_SMP1  0x534d5031
# define CHUNK_ID_SMD1  0x534d4431
# define CHUNK_ID_NAME  0x4e414d45
#else  // little endian
# define CHUNK_ID_MSP1  0x3150534d
# define CHUNK_ID_RLP1  0x31504c52
# define CHUNK_ID_SMP1  0x31504d53
# define CHUNK_ID_SMD1  0x31444d53
# define CHUNK_ID_NAME  0x454d414e
#endif // WORDS_BIGENDIAN

#define SMD1_CHUNK_HEADER_SZ    12

namespace Korg {

    #if defined(WIN32)
    static const String PATH_SEP = "\\";
    #else
    static const String PATH_SEP = "/";
    #endif

// *************** Internal functions **************
// *

    template<unsigned int SZ>
    inline String readText(RIFF::Chunk* ck) {
        char buf[SZ+1] = {};
        int n = (int) ck->Read(buf, SZ, 1);
        if (n != SZ)
            throw Exception("Premature end while reading text field");
        String s = buf;
        return s;
    }

    /// Read 24 bytes of ASCII text from given chunk and return it as String.
    inline String readText24(RIFF::Chunk* ck) {
        return readText<24>(ck);
    }

    /// Read 16 bytes of ASCII text from given chunk and return it as String.
    inline String readText16(RIFF::Chunk* ck) {
        return readText<16>(ck);
    }

    /// Read 12 bytes of ASCII text from given chunk and return it as String.
    inline String readText12(RIFF::Chunk* ck) {
        return readText<12>(ck);
    }

    /// For example passing "FOO.KMP" will return "FOO".
    inline String removeFileTypeExtension(const String& filename) {
        size_t pos = filename.find_last_of('.');
        if (pos == String::npos) return filename;
        return filename.substr(0, pos);
    }

// *************** KSFSample ***************
// *

    KSFSample::KSFSample(const String& filename) {
        RAMCache.Size              = 0;
        RAMCache.pStart            = NULL;
        RAMCache.NullExtensionSize = 0;

        riff = new RIFF::File(
            filename, CHUNK_ID_SMP1, RIFF::endian_big, RIFF::layout_flat
        );
        
        // read 'SMP1' chunk
        RIFF::Chunk* smp1 = riff->GetSubChunk(CHUNK_ID_SMP1);
        if (!smp1)
            throw Exception("Not a Korg sample file ('SMP1' chunk not found)");
        if (smp1->GetSize() < 32)
            throw Exception("Not a Korg sample file ('SMP1' chunk size too small)");
        Name = readText16(smp1);
        DefaultBank = smp1->ReadUint8();
        Start = smp1->ReadUint8() << 16 | smp1->ReadUint8() << 8 | smp1->ReadUint8();
        Start2    = smp1->ReadUint32();
        LoopStart = smp1->ReadUint32();
        LoopEnd   = smp1->ReadUint32();

        // read 'SMD1' chunk
        RIFF::Chunk* smd1 = riff->GetSubChunk(CHUNK_ID_SMD1);
        if (!smd1)
            throw Exception("Not a Korg sample file ('SMD1' chunk not found)");
        if (smd1->GetSize() < 12)
            throw Exception("Not a Korg sample file ('SMD1' chunk size too small)");
        SampleRate   = smd1->ReadUint32();
        Attributes   = smd1->ReadUint8();
        LoopTune     = smd1->ReadInt8();
        Channels     = smd1->ReadUint8();
        BitDepth     = smd1->ReadUint8();
        SamplePoints = smd1->ReadUint32();
    }

    KSFSample::~KSFSample() {
        if (RAMCache.pStart) delete[] (int8_t*) RAMCache.pStart;
        if (riff) delete riff;
    }

    /**
     * Loads the whole sample wave into RAM. Use ReleaseSampleData() to free
     * the memory if you don't need the cached sample data anymore.
     *
     * @returns  buffer_t structure with start address and size of the buffer
     *           in bytes
     * @see      ReleaseSampleData(), Read(), SetPos()
     */
    buffer_t KSFSample::LoadSampleData() {
        return LoadSampleDataWithNullSamplesExtension(this->SamplePoints, 0); // 0 amount of NullSamples
    }

    /**
     * Reads and caches the first \a SampleCount numbers of SamplePoints in RAM.
     * Use ReleaseSampleData() to free the memory space if you don't need the
     * cached samples anymore.
     *
     * @param SampleCount - number of sample points to load into RAM
     * @returns             buffer_t structure with start address and size of
     *                      the cached sample data in bytes
     * @see                 ReleaseSampleData(), Read(), SetPos()
     */
    buffer_t KSFSample::LoadSampleData(unsigned long SampleCount) {
        return LoadSampleDataWithNullSamplesExtension(SampleCount, 0); // 0 amount of NullSamples
    }

    /**
     * Loads the whole sample wave into RAM. Use ReleaseSampleData() to free the
     * memory if you don't need the cached sample data anymore. The method will
     * add \a NullSamplesCount silence samples past the official buffer end
     * (this won't affect the 'Size' member of the buffer_t structure, that
     * means 'Size' always reflects the size of the actual sample data, the
     * buffer might be bigger though). Silence samples past the official buffer
     * are needed for differential algorithms that always have to take
     * subsequent samples into account (resampling/interpolation would be an
     * important example) and avoids memory access faults in such cases.
     *
     * @param NullSamplesCount - number of silence samples the buffer should
     *                           be extended past it's data end
     * @returns                  buffer_t structure with start address and
     *                           size of the buffer in bytes
     * @see                      ReleaseSampleData(), Read(), SetPos()
     */
    buffer_t KSFSample::LoadSampleDataWithNullSamplesExtension(uint NullSamplesCount) {
        return LoadSampleDataWithNullSamplesExtension(this->SamplePoints, NullSamplesCount);
    }

    /**
     * Reads and caches the first \a SampleCount numbers of SamplePoints in RAM.
     * Use ReleaseSampleData() to free the memory space if you don't need the
     * cached samples anymore. The method will add \a NullSamplesCount silence
     * samples past the official buffer end (this won't affect the 'Size' member
     * of the buffer_t structure, that means 'Size' always reflects the size of
     * the actual sample data, the buffer might be bigger though). Silence
     * samples past the official buffer are needed for differential algorithms
     * that always have to take subsequent samples into account
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
    buffer_t KSFSample::LoadSampleDataWithNullSamplesExtension(unsigned long SampleCount, uint NullSamplesCount) {
        if (SampleCount > this->SamplePoints) SampleCount = this->SamplePoints;
        if (RAMCache.pStart) delete[] (int8_t*) RAMCache.pStart;
        unsigned long allocationsize = (SampleCount + NullSamplesCount) * FrameSize();
        SetPos(0); // reset read position to beginning of sample
        RAMCache.pStart            = new int8_t[allocationsize];
        RAMCache.Size              = Read(RAMCache.pStart, SampleCount) * FrameSize();
        RAMCache.NullExtensionSize = allocationsize - RAMCache.Size;
        // fill the remaining buffer space with silence samples
        memset((int8_t*)RAMCache.pStart + RAMCache.Size, 0, RAMCache.NullExtensionSize);
        return GetCache();
    }

    /**
     * Returns current cached sample points. A buffer_t structure will be
     * returned which contains address pointer to the begin of the cache and
     * the size of the cached sample data in bytes. Use LoadSampleData() to
     * cache a specific amount of sample points in RAM.
     *
     * @returns  buffer_t structure with current cached sample points
     * @see      LoadSampleData();
     */
    buffer_t KSFSample::GetCache() const {
        // return a copy of the buffer_t structure
        buffer_t result;
        result.Size              = this->RAMCache.Size;
        result.pStart            = this->RAMCache.pStart;
        result.NullExtensionSize = this->RAMCache.NullExtensionSize;
        return result;
    }

    /**
     * Frees the cached sample from RAM if loaded with LoadSampleData()
     * previously.
     *
     * @see  LoadSampleData();
     */
    void KSFSample::ReleaseSampleData() {
        if (RAMCache.pStart) delete[] (int8_t*) RAMCache.pStart;
        RAMCache.pStart = NULL;
        RAMCache.Size   = 0;
        RAMCache.NullExtensionSize = 0;
    }

    /**
     * Sets the position within the sample (in sample points, not in
     * bytes). Use this method and <i>Read()</i> if you don't want to load
     * the sample into RAM, thus for disk streaming.
     *
     * @param SampleCount  number of sample points to jump
     * @param Whence       optional: to which relation \a SampleCount refers
     *                     to, if omited <i>RIFF::stream_start</i> is assumed
     * @returns            the new sample position
     * @see                Read()
     */
    unsigned long KSFSample::SetPos(unsigned long SampleCount, RIFF::stream_whence_t Whence) {
        unsigned long samplePos = GetPos();
        switch (Whence) {
            case RIFF::stream_curpos:
                samplePos += SampleCount;
                break;
            case RIFF::stream_end:
                samplePos = this->SamplePoints - 1 - SampleCount;
                break;
            case RIFF::stream_backward:
                samplePos -= SampleCount;
                break;
            case RIFF::stream_start:
            default:
                samplePos = SampleCount;
                break;
        }
        if (samplePos > this->SamplePoints) samplePos = this->SamplePoints;
        unsigned long bytes = samplePos * FrameSize();
        RIFF::Chunk* smd1 = riff->GetSubChunk(CHUNK_ID_SMD1);
        unsigned long result = smd1->SetPos(SMD1_CHUNK_HEADER_SZ + bytes);
        return (result - SMD1_CHUNK_HEADER_SZ) / FrameSize();
    }

    /**
     * Returns the current position in the sample (in sample points).
     */
    unsigned long KSFSample::GetPos() const {
        RIFF::Chunk* smd1 = riff->GetSubChunk(CHUNK_ID_SMD1);
        return (smd1->GetPos() - SMD1_CHUNK_HEADER_SZ) / FrameSize();
    }

    /**
     * Reads \a SampleCount number of sample points from the current
     * position into the buffer pointed by \a pBuffer and increments the
     * position within the sample. Use this method and SetPos() if you don't
     * want to load the sample into RAM, thus for disk streaming.
     *
     * <b>Caution:</b> If you are using more than one streaming thread, you
     * have to use an external decompression buffer for <b>EACH</b>
     * streaming thread to avoid race conditions and crashes (which is not
     * implemented for this class yet)!
     *
     * @param pBuffer      destination buffer
     * @param SampleCount  number of sample points to read
     * @returns            number of successfully read sample points
     * @see                SetPos()
     */
    unsigned long KSFSample::Read(void* pBuffer, unsigned long SampleCount) {
        RIFF::Chunk* smd1 = riff->GetSubChunk(CHUNK_ID_SMD1);

        unsigned long samplestoread = SampleCount, totalreadsamples = 0, readsamples;

        if (samplestoread) do {
            readsamples       = smd1->Read(pBuffer, SampleCount, FrameSize()); // FIXME: channel inversion due to endian correction?
            samplestoread    -= readsamples;
            totalreadsamples += readsamples;
        } while (readsamples && samplestoread);

        return totalreadsamples;
    }

    /**
     * Returns the size of one sample point of this sample in bytes.
     */
    int KSFSample::FrameSize() const {
        return BitDepth / 8 * Channels;
    }

    uint8_t KSFSample::CompressionID() const {
        return Attributes & 0x04;
    }

    bool KSFSample::IsCompressed() const {
        return Attributes & 0x10;
    }

    bool KSFSample::Use2ndStart() const {
        return !(Attributes & 0x20);
    }

    String KSFSample::FileName() const {
        return riff->GetFileName();
    }

// *************** KMPRegion ***************
// *

    KMPRegion::KMPRegion(KMPInstrument* parent, RIFF::Chunk* rlp1)
        : parent(parent), rlp1(rlp1)
    {
        OriginalKey = rlp1->ReadUint8();
        Transpose   = (OriginalKey >> 7) & 1;
        OriginalKey &= 0x7F;
        TopKey = rlp1->ReadUint8() & 0x7F;
        Tune   = rlp1->ReadInt8();
        Level  = rlp1->ReadInt8();
        Pan    = rlp1->ReadUint8() & 0x7F;
        FilterCutoff = rlp1->ReadInt8();
        SampleFileName = readText12(rlp1);
    }

    KMPRegion::~KMPRegion() {
    }

    String KMPRegion::FullSampleFileName() const {
        return removeFileTypeExtension(rlp1->GetFile()->GetFileName()) +
               PATH_SEP + SampleFileName;
    }

    KMPInstrument* KMPRegion::GetInstrument() const {
        return parent;
    }

// *************** KMPInstrument ***************
// *

    KMPInstrument::KMPInstrument(const String& filename) {
        riff = new RIFF::File(
            filename, CHUNK_ID_MSP1, RIFF::endian_big, RIFF::layout_flat
        );

        // read 'MSP1' chunk
        RIFF::Chunk* msp1 = riff->GetSubChunk(CHUNK_ID_MSP1);
        if (!msp1)
            throw Exception("Not a Korg instrument file ('MSP1' chunk not found)");
        if (msp1->GetSize() < 18)
            throw Exception("Not a Korg instrument file ('MSP1' chunk size too small)");
        Name16 = readText16(msp1);
        int nSamples = msp1->ReadUint8();
        Attributes   = msp1->ReadUint8();
        
        // read optional 'NAME' chunk
        RIFF::Chunk* name = riff->GetSubChunk(CHUNK_ID_NAME);
        if (name) {
            Name24 = readText24(name);
        }

        // read 'RLP1' chunk ...
        RIFF::Chunk* rlp1 = riff->GetSubChunk(CHUNK_ID_RLP1);
        if (!rlp1)
            throw Exception("Not a Korg instrument file ('RLP1' chunk not found)");
        if (rlp1->GetSize() < 18 * nSamples)
            throw Exception("Not a Korg instrument file ('RLP1' chunk size too small)");
        for (int i = 0; i < nSamples; ++i) {
            KMPRegion* region = new KMPRegion(this, rlp1);
            regions.push_back(region);
        }
    }

    KMPInstrument::~KMPInstrument() {
        if (riff) delete riff;
        for (int i = 0; i < regions.size(); ++i)
            delete regions[i];
    }

    KMPRegion* KMPInstrument::GetRegion(int index) {
        if (index < 0 || index >= regions.size())
            return NULL;
        return regions[index];
    }

    int KMPInstrument::GetRegionCount() const {
        return (int) regions.size();
    }

    bool KMPInstrument::Use2ndStart() const {
        return !(Attributes & 1);
    }

    /**
     * Returns the long name (Name24) if it was stored in the file, otherwise
     * returns the short name (Name16).
     */
    String KMPInstrument::Name() const {
        return (!Name24.empty()) ? Name24 : Name16;
    }

    String KMPInstrument::FileName() const {
        return riff->GetFileName();
    }

// *************** Exception ***************
// *

    Exception::Exception(String Message) : RIFF::Exception(Message) {
    }

    void Exception::PrintMessage() {
        std::cout << "Korg::Exception: " << Message << std::endl;
    }
    
// *************** functions ***************
// *

    /**
     * Returns the name of this C++ library. This is usually "libgig" of
     * course. This call is equivalent to RIFF::libraryName() and
     * gig::libraryName().
     */
    String libraryName() {
        return PACKAGE;
    }

    /**
     * Returns version of this C++ library. This call is equivalent to
     * RIFF::libraryVersion() and gig::libraryVersion().
     */
    String libraryVersion() {
        return VERSION;
    }

} // namespace Korg
