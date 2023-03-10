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

#include "DLS.h"

#include <algorithm>
#include <vector>
#include <time.h>

#ifdef __APPLE__
#include <CoreFoundation/CFUUID.h>
#elif defined(HAVE_UUID_UUID_H)
#include <uuid/uuid.h>
#endif

#include "helper.h"

// macros to decode connection transforms
#define CONN_TRANSFORM_SRC(x)			((x >> 10) & 0x000F)
#define CONN_TRANSFORM_CTL(x)			((x >> 4) & 0x000F)
#define CONN_TRANSFORM_DST(x)			(x & 0x000F)
#define CONN_TRANSFORM_BIPOLAR_SRC(x)	(x & 0x4000)
#define CONN_TRANSFORM_BIPOLAR_CTL(x)	(x & 0x0100)
#define CONN_TRANSFORM_INVERT_SRC(x)	(x & 0x8000)
#define CONN_TRANSFORM_INVERT_CTL(x)	(x & 0x0200)

// macros to encode connection transforms
#define CONN_TRANSFORM_SRC_ENCODE(x)			((x & 0x000F) << 10)
#define CONN_TRANSFORM_CTL_ENCODE(x)			((x & 0x000F) << 4)
#define CONN_TRANSFORM_DST_ENCODE(x)			(x & 0x000F)
#define CONN_TRANSFORM_BIPOLAR_SRC_ENCODE(x)	((x) ? 0x4000 : 0)
#define CONN_TRANSFORM_BIPOLAR_CTL_ENCODE(x)	((x) ? 0x0100 : 0)
#define CONN_TRANSFORM_INVERT_SRC_ENCODE(x)		((x) ? 0x8000 : 0)
#define CONN_TRANSFORM_INVERT_CTL_ENCODE(x)		((x) ? 0x0200 : 0)

#define DRUM_TYPE_MASK			0x80000000

#define F_RGN_OPTION_SELFNONEXCLUSIVE	0x0001

#define F_WAVELINK_PHASE_MASTER		0x0001
#define F_WAVELINK_MULTICHANNEL		0x0002

#define F_WSMP_NO_TRUNCATION		0x0001
#define F_WSMP_NO_COMPRESSION		0x0002

#define MIDI_BANK_COARSE(x)		((x & 0x00007F00) >> 8)			// CC0
#define MIDI_BANK_FINE(x)		(x & 0x0000007F)			// CC32
#define MIDI_BANK_MERGE(coarse, fine)	((((uint16_t) coarse) << 7) | fine)	// CC0 + CC32
#define MIDI_BANK_ENCODE(coarse, fine)	(((coarse & 0x0000007F) << 8) | (fine & 0x0000007F))

namespace DLS {

// *************** Connection  ***************
// *

    void Connection::Init(conn_block_t* Header) {
        Source               = (conn_src_t) Header->source;
        Control              = (conn_src_t) Header->control;
        Destination          = (conn_dst_t) Header->destination;
        Scale                = Header->scale;
        SourceTransform      = (conn_trn_t) CONN_TRANSFORM_SRC(Header->transform);
        ControlTransform     = (conn_trn_t) CONN_TRANSFORM_CTL(Header->transform);
        DestinationTransform = (conn_trn_t) CONN_TRANSFORM_DST(Header->transform);
        SourceInvert         = CONN_TRANSFORM_INVERT_SRC(Header->transform);
        SourceBipolar        = CONN_TRANSFORM_BIPOLAR_SRC(Header->transform);
        ControlInvert        = CONN_TRANSFORM_INVERT_CTL(Header->transform);
        ControlBipolar       = CONN_TRANSFORM_BIPOLAR_CTL(Header->transform);
    }

    Connection::conn_block_t Connection::ToConnBlock() {
        conn_block_t c;
        c.source = Source;
        c.control = Control;
        c.destination = Destination;
        c.scale = Scale;
        c.transform = CONN_TRANSFORM_SRC_ENCODE(SourceTransform) |
                      CONN_TRANSFORM_CTL_ENCODE(ControlTransform) |
                      CONN_TRANSFORM_DST_ENCODE(DestinationTransform) |
                      CONN_TRANSFORM_INVERT_SRC_ENCODE(SourceInvert) |
                      CONN_TRANSFORM_BIPOLAR_SRC_ENCODE(SourceBipolar) |
                      CONN_TRANSFORM_INVERT_CTL_ENCODE(ControlInvert) |
                      CONN_TRANSFORM_BIPOLAR_CTL_ENCODE(ControlBipolar);
        return c;
    }



// *************** Articulation  ***************
// *

    /** @brief Constructor.
     *
     * Expects an 'artl' or 'art2' chunk to be given where the articulation
     * connections will be read from.
     *
     * @param artl - pointer to an 'artl' or 'art2' chunk
     * @throws Exception if no 'artl' or 'art2' chunk was given
     */
    Articulation::Articulation(RIFF::Chunk* artl) {
        pArticulationCk = artl;
        if (artl->GetChunkID() != CHUNK_ID_ART2 &&
            artl->GetChunkID() != CHUNK_ID_ARTL) {
              throw DLS::Exception("<artl-ck> or <art2-ck> chunk expected");
        }

        artl->SetPos(0);

        HeaderSize  = artl->ReadUint32();
        Connections = artl->ReadUint32();
        artl->SetPos(HeaderSize);

        pConnections = new Connection[Connections];
        Connection::conn_block_t connblock;
        for (uint32_t i = 0; i < Connections; i++) {
            artl->Read(&connblock.source, 1, 2);
            artl->Read(&connblock.control, 1, 2);
            artl->Read(&connblock.destination, 1, 2);
            artl->Read(&connblock.transform, 1, 2);
            artl->Read(&connblock.scale, 1, 4);
            pConnections[i].Init(&connblock);
        }
    }

    Articulation::~Articulation() {
       if (pConnections) delete[] pConnections;
    }

    /**
     * Apply articulation connections to the respective RIFF chunks. You
     * have to call File::Save() to make changes persistent.
     *
     * @param pProgress - callback function for progress notification
     */
    void Articulation::UpdateChunks(progress_t* pProgress) {
        const int iEntrySize = 12; // 12 bytes per connection block
        pArticulationCk->Resize(HeaderSize + Connections * iEntrySize);
        uint8_t* pData = (uint8_t*) pArticulationCk->LoadChunkData();
        store16(&pData[0], HeaderSize);
        store16(&pData[2], Connections);
        for (uint32_t i = 0; i < Connections; i++) {
            Connection::conn_block_t c = pConnections[i].ToConnBlock();
            store16(&pData[HeaderSize + i * iEntrySize],     c.source);
            store16(&pData[HeaderSize + i * iEntrySize + 2], c.control);
            store16(&pData[HeaderSize + i * iEntrySize + 4], c.destination);
            store16(&pData[HeaderSize + i * iEntrySize + 6], c.transform);
            store32(&pData[HeaderSize + i * iEntrySize + 8], c.scale);
        }
    }

    /** @brief Remove all RIFF chunks associated with this Articulation object.
     *
     * At the moment Articulation::DeleteChunks() does nothing. It is
     * recommended to call this method explicitly though from deriving classes's
     * own overridden implementation of this method to avoid potential future
     * compatiblity issues.
     *
     * See Storage::DeleteChunks() for details.
     */
    void Articulation::DeleteChunks() {
    }



// *************** Articulator  ***************
// *

    Articulator::Articulator(RIFF::List* ParentList) {
        pParentList    = ParentList;
        pArticulations = NULL;
    }

    Articulation* Articulator::GetFirstArticulation() {
        if (!pArticulations) LoadArticulations();
        if (!pArticulations) return NULL;
        ArticulationsIterator = pArticulations->begin();
        return (ArticulationsIterator != pArticulations->end()) ? *ArticulationsIterator : NULL;
    }

    Articulation* Articulator::GetNextArticulation() {
        if (!pArticulations) return NULL;
        ArticulationsIterator++;
        return (ArticulationsIterator != pArticulations->end()) ? *ArticulationsIterator : NULL;
    }

    void Articulator::LoadArticulations() {
        // prefer articulation level 2
        RIFF::List* lart = pParentList->GetSubList(LIST_TYPE_LAR2);
        if (!lart)  lart = pParentList->GetSubList(LIST_TYPE_LART);
        if (lart) {
            uint32_t artCkType = (lart->GetListType() == LIST_TYPE_LAR2) ? CHUNK_ID_ART2
                                                                         : CHUNK_ID_ARTL;
            RIFF::Chunk* art = lart->GetFirstSubChunk();
            while (art) {
                if (art->GetChunkID() == artCkType) {
                    if (!pArticulations) pArticulations = new ArticulationList;
                    pArticulations->push_back(new Articulation(art));
                }
                art = lart->GetNextSubChunk();
            }
        }
    }

    Articulator::~Articulator() {
        if (pArticulations) {
            ArticulationList::iterator iter = pArticulations->begin();
            ArticulationList::iterator end  = pArticulations->end();
            while (iter != end) {
                delete *iter;
                iter++;
            }
            delete pArticulations;
        }
    }

    /**
     * Apply all articulations to the respective RIFF chunks. You have to
     * call File::Save() to make changes persistent.
     *
     * @param pProgress - callback function for progress notification
     */
    void Articulator::UpdateChunks(progress_t* pProgress) {
        if (pArticulations) {
            ArticulationList::iterator iter = pArticulations->begin();
            ArticulationList::iterator end  = pArticulations->end();
            for (; iter != end; ++iter) {
                (*iter)->UpdateChunks(pProgress);
            }
        }
    }

    /** @brief Remove all RIFF chunks associated with this Articulator object.
     *
     * See Storage::DeleteChunks() for details.
     */
    void Articulator::DeleteChunks() {
        if (pArticulations) {
            ArticulationList::iterator iter = pArticulations->begin();
            ArticulationList::iterator end  = pArticulations->end();
            for (; iter != end; ++iter) {
                (*iter)->DeleteChunks();
            }
        }
    }

    /**
     * Not yet implemented in this version, since the .gig format does
     * not need to copy DLS articulators and so far nobody used pure
     * DLS instrument AFAIK.
     */
    void Articulator::CopyAssign(const Articulator* orig) {
        //TODO: implement deep copy assignment for this class
    }



// *************** Info  ***************
// *

    /** @brief Constructor.
     *
     * Initializes the info strings with values provided by an INFO list chunk.
     *
     * @param list - pointer to a list chunk which contains an INFO list chunk
     */
    Info::Info(RIFF::List* list) {
        pFixedStringLengths = NULL;
        pResourceListChunk = list;
        if (list) {
            RIFF::List* lstINFO = list->GetSubList(LIST_TYPE_INFO);
            if (lstINFO) {
                LoadString(CHUNK_ID_INAM, lstINFO, Name);
                LoadString(CHUNK_ID_IARL, lstINFO, ArchivalLocation);
                LoadString(CHUNK_ID_ICRD, lstINFO, CreationDate);
                LoadString(CHUNK_ID_ICMT, lstINFO, Comments);
                LoadString(CHUNK_ID_IPRD, lstINFO, Product);
                LoadString(CHUNK_ID_ICOP, lstINFO, Copyright);
                LoadString(CHUNK_ID_IART, lstINFO, Artists);
                LoadString(CHUNK_ID_IGNR, lstINFO, Genre);
                LoadString(CHUNK_ID_IKEY, lstINFO, Keywords);
                LoadString(CHUNK_ID_IENG, lstINFO, Engineer);
                LoadString(CHUNK_ID_ITCH, lstINFO, Technician);
                LoadString(CHUNK_ID_ISFT, lstINFO, Software);
                LoadString(CHUNK_ID_IMED, lstINFO, Medium);
                LoadString(CHUNK_ID_ISRC, lstINFO, Source);
                LoadString(CHUNK_ID_ISRF, lstINFO, SourceForm);
                LoadString(CHUNK_ID_ICMS, lstINFO, Commissioned);
                LoadString(CHUNK_ID_ISBJ, lstINFO, Subject);
            }
        }
    }

    Info::~Info() {
    }

    /**
     * Forces specific Info fields to be of a fixed length when being saved
     * to a file. By default the respective RIFF chunk of an Info field
     * will have a size analogue to its actual string length. With this
     * method however this behavior can be overridden, allowing to force an
     * arbitrary fixed size individually for each Info field.
     *
     * This method is used as a workaround for the gig format, not for DLS.
     *
     * @param lengths - NULL terminated array of string_length_t elements
     */
    void Info::SetFixedStringLengths(const string_length_t* lengths) {
        pFixedStringLengths = lengths;
    }

    /** @brief Load given INFO field.
     *
     * Load INFO field from INFO chunk with chunk ID \a ChunkID from INFO
     * list chunk \a lstINFO and save value to \a s.
     */
    void Info::LoadString(uint32_t ChunkID, RIFF::List* lstINFO, String& s) {
        RIFF::Chunk* ck = lstINFO->GetSubChunk(ChunkID);
        ::LoadString(ck, s); // function from helper.h
    }

    /** @brief Apply given INFO field to the respective chunk.
     *
     * Apply given info value to info chunk with ID \a ChunkID, which is a
     * subchunk of INFO list chunk \a lstINFO. If the given chunk already
     * exists, value \a s will be applied. Otherwise if it doesn't exist yet
     * and either \a s or \a sDefault is not an empty string, such a chunk
     * will be created and either \a s or \a sDefault will be applied
     * (depending on which one is not an empty string, if both are not an
     * empty string \a s will be preferred).
     *
     * @param ChunkID  - 32 bit RIFF chunk ID of INFO subchunk
     * @param lstINFO  - parent (INFO) RIFF list chunk
     * @param s        - current value of info field
     * @param sDefault - default value
     */
    void Info::SaveString(uint32_t ChunkID, RIFF::List* lstINFO, const String& s, const String& sDefault) {
        int size = 0;
        if (pFixedStringLengths) {
            for (int i = 0 ; pFixedStringLengths[i].length ; i++) {
                if (pFixedStringLengths[i].chunkId == ChunkID) {
                    size = pFixedStringLengths[i].length;
                    break;
                }
            }
        }
        RIFF::Chunk* ck = lstINFO->GetSubChunk(ChunkID);
        ::SaveString(ChunkID, ck, lstINFO, s, sDefault, size != 0, size); // function from helper.h
    }

    /** @brief Update chunks with current info values.
     *
     * Apply current INFO field values to the respective INFO chunks. You
     * have to call File::Save() to make changes persistent.
     *
     * @param pProgress - callback function for progress notification
     */
    void Info::UpdateChunks(progress_t* pProgress) {
        if (!pResourceListChunk) return;

        // make sure INFO list chunk exists
        RIFF::List* lstINFO   = pResourceListChunk->GetSubList(LIST_TYPE_INFO);

        String defaultName = "";
        String defaultCreationDate = "";
        String defaultSoftware = "";
        String defaultComments = "";

        uint32_t resourceType = pResourceListChunk->GetListType();

        if (!lstINFO) {
            lstINFO = pResourceListChunk->AddSubList(LIST_TYPE_INFO);

            // assemble default values
            defaultName = "NONAME";

            if (resourceType == RIFF_TYPE_DLS) {
                // get current date
                time_t now = time(NULL);
                tm* pNowBroken = localtime(&now);
                char buf[11];
                strftime(buf, 11, "%F", pNowBroken);
                defaultCreationDate = buf;

                defaultComments = "Created with " + libraryName() + " " + libraryVersion();
            }
            if (resourceType == RIFF_TYPE_DLS || resourceType == LIST_TYPE_INS)
            {
                defaultSoftware = libraryName() + " " + libraryVersion();
            }
        }

        // save values

        SaveString(CHUNK_ID_IARL, lstINFO, ArchivalLocation, String(""));
        SaveString(CHUNK_ID_IART, lstINFO, Artists, String(""));
        SaveString(CHUNK_ID_ICMS, lstINFO, Commissioned, String(""));
        SaveString(CHUNK_ID_ICMT, lstINFO, Comments, defaultComments);
        SaveString(CHUNK_ID_ICOP, lstINFO, Copyright, String(""));
        SaveString(CHUNK_ID_ICRD, lstINFO, CreationDate, defaultCreationDate);
        SaveString(CHUNK_ID_IENG, lstINFO, Engineer, String(""));
        SaveString(CHUNK_ID_IGNR, lstINFO, Genre, String(""));
        SaveString(CHUNK_ID_IKEY, lstINFO, Keywords, String(""));
        SaveString(CHUNK_ID_IMED, lstINFO, Medium, String(""));
        SaveString(CHUNK_ID_INAM, lstINFO, Name, defaultName);
        SaveString(CHUNK_ID_IPRD, lstINFO, Product, String(""));
        SaveString(CHUNK_ID_ISBJ, lstINFO, Subject, String(""));
        SaveString(CHUNK_ID_ISFT, lstINFO, Software, defaultSoftware);
        SaveString(CHUNK_ID_ISRC, lstINFO, Source, String(""));
        SaveString(CHUNK_ID_ISRF, lstINFO, SourceForm, String(""));
        SaveString(CHUNK_ID_ITCH, lstINFO, Technician, String(""));
    }

    /** @brief Remove all RIFF chunks associated with this Info object.
     *
     * At the moment Info::DeleteChunks() does nothing. It is
     * recommended to call this method explicitly though from deriving classes's
     * own overridden implementation of this method to avoid potential future
     * compatiblity issues.
     *
     * See Storage::DeleteChunks() for details.
     */
    void Info::DeleteChunks() {
    }

    /**
     * Make a deep copy of the Info object given by @a orig and assign it to
     * this object.
     *
     * @param orig - original Info object to be copied from
     */
    void Info::CopyAssign(const Info* orig) {
        Name = orig->Name;
        ArchivalLocation = orig->ArchivalLocation;
        CreationDate = orig->CreationDate;
        Comments = orig->Comments;
        Product = orig->Product;
        Copyright = orig->Copyright;
        Artists = orig->Artists;
        Genre = orig->Genre;
        Keywords = orig->Keywords;
        Engineer = orig->Engineer;
        Technician = orig->Technician;
        Software = orig->Software;
        Medium = orig->Medium;
        Source = orig->Source;
        SourceForm = orig->SourceForm;
        Commissioned = orig->Commissioned;
        Subject = orig->Subject;
        //FIXME: hmm, is copying this pointer a good idea?
        pFixedStringLengths = orig->pFixedStringLengths;
    }



// *************** Resource ***************
// *

    /** @brief Constructor.
     *
     * Initializes the 'Resource' object with values provided by a given
     * INFO list chunk and a DLID chunk (the latter optional).
     *
     * @param Parent      - pointer to parent 'Resource', NULL if this is
     *                      the toplevel 'Resource' object
     * @param lstResource - pointer to an INFO list chunk
     */
    Resource::Resource(Resource* Parent, RIFF::List* lstResource) {
        pParent = Parent;
        pResourceList = lstResource;

        pInfo = new Info(lstResource);

        RIFF::Chunk* ckDLSID = lstResource->GetSubChunk(CHUNK_ID_DLID);
        if (ckDLSID) {
            ckDLSID->SetPos(0);

            pDLSID = new dlsid_t;
            ckDLSID->Read(&pDLSID->ulData1, 1, 4);
            ckDLSID->Read(&pDLSID->usData2, 1, 2);
            ckDLSID->Read(&pDLSID->usData3, 1, 2);
            ckDLSID->Read(pDLSID->abData, 8, 1);
        }
        else pDLSID = NULL;
    }

    Resource::~Resource() {
        if (pDLSID) delete pDLSID;
        if (pInfo)  delete pInfo;
    }

    /** @brief Remove all RIFF chunks associated with this Resource object.
     *
     * At the moment Resource::DeleteChunks() does nothing. It is recommended
     * to call this method explicitly though from deriving classes's own
     * overridden implementation of this method to avoid potential future
     * compatiblity issues.
     *
     * See Storage::DeleteChunks() for details.
     */
    void Resource::DeleteChunks() {
    }

    /** @brief Update chunks with current Resource data.
     *
     * Apply Resource data persistently below the previously given resource
     * list chunk. This will currently only include the INFO data. The DLSID
     * will not be applied at the moment (yet).
     *
     * You have to call File::Save() to make changes persistent.
     *
     * @param pProgress - callback function for progress notification
     */
    void Resource::UpdateChunks(progress_t* pProgress) {
        pInfo->UpdateChunks(pProgress);

        if (pDLSID) {
            // make sure 'dlid' chunk exists
            RIFF::Chunk* ckDLSID = pResourceList->GetSubChunk(CHUNK_ID_DLID);
            if (!ckDLSID) ckDLSID = pResourceList->AddSubChunk(CHUNK_ID_DLID, 16);
            uint8_t* pData = (uint8_t*)ckDLSID->LoadChunkData();
            // update 'dlid' chunk
            store32(&pData[0], pDLSID->ulData1);
            store16(&pData[4], pDLSID->usData2);
            store16(&pData[6], pDLSID->usData3);
            memcpy(&pData[8], pDLSID->abData, 8);
        }
    }

    /**
     * Generates a new DLSID for the resource.
     */
    void Resource::GenerateDLSID() {
        #if defined(WIN32) || defined(__APPLE__) || defined(HAVE_UUID_GENERATE)
        if (!pDLSID) pDLSID = new dlsid_t;
        GenerateDLSID(pDLSID);
        #endif
    }

    void Resource::GenerateDLSID(dlsid_t* pDLSID) {
#ifdef WIN32
        UUID uuid;
        UuidCreate(&uuid);
        pDLSID->ulData1 = uuid.Data1;
        pDLSID->usData2 = uuid.Data2;
        pDLSID->usData3 = uuid.Data3;
        memcpy(pDLSID->abData, uuid.Data4, 8);

#elif defined(__APPLE__)

        CFUUIDRef uuidRef = CFUUIDCreate(NULL);
        CFUUIDBytes uuid = CFUUIDGetUUIDBytes(uuidRef);
        CFRelease(uuidRef);
        pDLSID->ulData1 = uuid.byte0 | uuid.byte1 << 8 | uuid.byte2 << 16 | uuid.byte3 << 24;
        pDLSID->usData2 = uuid.byte4 | uuid.byte5 << 8;
        pDLSID->usData3 = uuid.byte6 | uuid.byte7 << 8;
        pDLSID->abData[0] = uuid.byte8;
        pDLSID->abData[1] = uuid.byte9;
        pDLSID->abData[2] = uuid.byte10;
        pDLSID->abData[3] = uuid.byte11;
        pDLSID->abData[4] = uuid.byte12;
        pDLSID->abData[5] = uuid.byte13;
        pDLSID->abData[6] = uuid.byte14;
        pDLSID->abData[7] = uuid.byte15;
#elif defined(HAVE_UUID_GENERATE)
        uuid_t uuid;
        uuid_generate(uuid);
        pDLSID->ulData1 = uuid[0] | uuid[1] << 8 | uuid[2] << 16 | uuid[3] << 24;
        pDLSID->usData2 = uuid[4] | uuid[5] << 8;
        pDLSID->usData3 = uuid[6] | uuid[7] << 8;
        memcpy(pDLSID->abData, &uuid[8], 8);
#else
# error "Missing support for uuid generation"
#endif
    }
    
    /**
     * Make a deep copy of the Resource object given by @a orig and assign it
     * to this object.
     *
     * @param orig - original Resource object to be copied from
     */
    void Resource::CopyAssign(const Resource* orig) {
        pInfo->CopyAssign(orig->pInfo);
    }


// *************** Sampler ***************
// *

    Sampler::Sampler(RIFF::List* ParentList) {
        pParentList       = ParentList;
        RIFF::Chunk* wsmp = ParentList->GetSubChunk(CHUNK_ID_WSMP);
        if (wsmp) {
            wsmp->SetPos(0);

            uiHeaderSize   = wsmp->ReadUint32();
            UnityNote      = wsmp->ReadUint16();
            FineTune       = wsmp->ReadInt16();
            Gain           = wsmp->ReadInt32();
            SamplerOptions = wsmp->ReadUint32();
            SampleLoops    = wsmp->ReadUint32();
        } else { // 'wsmp' chunk missing
            uiHeaderSize   = 20;
            UnityNote      = 60;
            FineTune       = 0; // +- 0 cents
            Gain           = 0; // 0 dB
            SamplerOptions = F_WSMP_NO_COMPRESSION;
            SampleLoops    = 0;
        }
        NoSampleDepthTruncation = SamplerOptions & F_WSMP_NO_TRUNCATION;
        NoSampleCompression     = SamplerOptions & F_WSMP_NO_COMPRESSION;
        pSampleLoops            = (SampleLoops) ? new sample_loop_t[SampleLoops] : NULL;
        if (SampleLoops) {
            wsmp->SetPos(uiHeaderSize);
            for (uint32_t i = 0; i < SampleLoops; i++) {
                wsmp->Read(pSampleLoops + i, 4, 4);
                if (pSampleLoops[i].Size > sizeof(sample_loop_t)) { // if loop struct was extended
                    wsmp->SetPos(pSampleLoops[i].Size - sizeof(sample_loop_t), RIFF::stream_curpos);
                }
            }
        }
    }

    Sampler::~Sampler() {
        if (pSampleLoops) delete[] pSampleLoops;
    }

    void Sampler::SetGain(int32_t gain) {
        Gain = gain;
    }

    /**
     * Apply all sample player options to the respective RIFF chunk. You
     * have to call File::Save() to make changes persistent.
     *
     * @param pProgress - callback function for progress notification
     */
    void Sampler::UpdateChunks(progress_t* pProgress) {
        // make sure 'wsmp' chunk exists
        RIFF::Chunk* wsmp = pParentList->GetSubChunk(CHUNK_ID_WSMP);
        int wsmpSize = uiHeaderSize + SampleLoops * 16;
        if (!wsmp) {
            wsmp = pParentList->AddSubChunk(CHUNK_ID_WSMP, wsmpSize);
        } else if (wsmp->GetSize() != wsmpSize) {
            wsmp->Resize(wsmpSize);
        }
        uint8_t* pData = (uint8_t*) wsmp->LoadChunkData();
        // update headers size
        store32(&pData[0], uiHeaderSize);
        // update respective sampler options bits
        SamplerOptions = (NoSampleDepthTruncation) ? SamplerOptions | F_WSMP_NO_TRUNCATION
                                                   : SamplerOptions & (~F_WSMP_NO_TRUNCATION);
        SamplerOptions = (NoSampleCompression) ? SamplerOptions | F_WSMP_NO_COMPRESSION
                                               : SamplerOptions & (~F_WSMP_NO_COMPRESSION);
        store16(&pData[4], UnityNote);
        store16(&pData[6], FineTune);
        store32(&pData[8], Gain);
        store32(&pData[12], SamplerOptions);
        store32(&pData[16], SampleLoops);
        // update loop definitions
        for (uint32_t i = 0; i < SampleLoops; i++) {
            //FIXME: this does not handle extended loop structs correctly
            store32(&pData[uiHeaderSize + i * 16], pSampleLoops[i].Size);
            store32(&pData[uiHeaderSize + i * 16 + 4], pSampleLoops[i].LoopType);
            store32(&pData[uiHeaderSize + i * 16 + 8], pSampleLoops[i].LoopStart);
            store32(&pData[uiHeaderSize + i * 16 + 12], pSampleLoops[i].LoopLength);
        }
    }

    /** @brief Remove all RIFF chunks associated with this Sampler object.
     *
     * At the moment Sampler::DeleteChunks() does nothing. It is
     * recommended to call this method explicitly though from deriving classes's
     * own overridden implementation of this method to avoid potential future
     * compatiblity issues.
     *
     * See Storage::DeleteChunks() for details.
     */
    void Sampler::DeleteChunks() {
    }

    /**
     * Adds a new sample loop with the provided loop definition.
     *
     * @param pLoopDef - points to a loop definition that is to be copied
     */
    void Sampler::AddSampleLoop(sample_loop_t* pLoopDef) {
        sample_loop_t* pNewLoops = new sample_loop_t[SampleLoops + 1];
        // copy old loops array
        for (int i = 0; i < SampleLoops; i++) {
            pNewLoops[i] = pSampleLoops[i];
        }
        // add the new loop
        pNewLoops[SampleLoops] = *pLoopDef;
        // auto correct size field
        pNewLoops[SampleLoops].Size = sizeof(DLS::sample_loop_t);
        // free the old array and update the member variables
        if (SampleLoops) delete[] pSampleLoops;
        pSampleLoops = pNewLoops;
        SampleLoops++;
    }

    /**
     * Deletes an existing sample loop.
     *
     * @param pLoopDef - pointer to existing loop definition
     * @throws Exception - if given loop definition does not exist
     */
    void Sampler::DeleteSampleLoop(sample_loop_t* pLoopDef) {
        sample_loop_t* pNewLoops = new sample_loop_t[SampleLoops - 1];
        // copy old loops array (skipping given loop)
        for (int i = 0, o = 0; i < SampleLoops; i++) {
            if (&pSampleLoops[i] == pLoopDef) continue;
            if (o == SampleLoops - 1) {
                delete[] pNewLoops;
                throw Exception("Could not delete Sample Loop, because it does not exist");
            }
            pNewLoops[o] = pSampleLoops[i];
            o++;
        }
        // free the old array and update the member variables
        if (SampleLoops) delete[] pSampleLoops;
        pSampleLoops = pNewLoops;
        SampleLoops--;
    }
    
    /**
     * Make a deep copy of the Sampler object given by @a orig and assign it
     * to this object.
     *
     * @param orig - original Sampler object to be copied from
     */
    void Sampler::CopyAssign(const Sampler* orig) {
        // copy trivial scalars
        UnityNote = orig->UnityNote;
        FineTune = orig->FineTune;
        Gain = orig->Gain;
        NoSampleDepthTruncation = orig->NoSampleDepthTruncation;
        NoSampleCompression = orig->NoSampleCompression;
        SamplerOptions = orig->SamplerOptions;
        
        // copy sample loops
        if (SampleLoops) delete[] pSampleLoops;
        pSampleLoops = new sample_loop_t[orig->SampleLoops];
        memcpy(pSampleLoops, orig->pSampleLoops, orig->SampleLoops * sizeof(sample_loop_t));
        SampleLoops = orig->SampleLoops;
    }


// *************** Sample ***************
// *

    /** @brief Constructor.
     *
     * Load an existing sample or create a new one. A 'wave' list chunk must
     * be given to this constructor. In case the given 'wave' list chunk
     * contains a 'fmt' and 'data' chunk, the format and sample data will be
     * loaded from there, otherwise default values will be used and those
     * chunks will be created when File::Save() will be called later on.
     *
     * @param pFile          - pointer to DLS::File where this sample is
     *                         located (or will be located)
     * @param waveList       - pointer to 'wave' list chunk which is (or
     *                         will be) associated with this sample
     * @param WavePoolOffset - offset of this sample data from wave pool
     *                         ('wvpl') list chunk
     */
    Sample::Sample(File* pFile, RIFF::List* waveList, file_offset_t WavePoolOffset) : Resource(pFile, waveList) {
        pWaveList = waveList;
        ullWavePoolOffset = WavePoolOffset - LIST_HEADER_SIZE(waveList->GetFile()->GetFileOffsetSize());
        pCkFormat = waveList->GetSubChunk(CHUNK_ID_FMT);
        pCkData   = waveList->GetSubChunk(CHUNK_ID_DATA);
        if (pCkFormat) {
            pCkFormat->SetPos(0);

            // common fields
            FormatTag              = pCkFormat->ReadUint16();
            Channels               = pCkFormat->ReadUint16();
            SamplesPerSecond       = pCkFormat->ReadUint32();
            AverageBytesPerSecond  = pCkFormat->ReadUint32();
            BlockAlign             = pCkFormat->ReadUint16();
            // PCM format specific
            if (FormatTag == DLS_WAVE_FORMAT_PCM) {
                BitDepth     = pCkFormat->ReadUint16();
                FrameSize    = (BitDepth / 8) * Channels;
            } else { // unsupported sample data format
                BitDepth     = 0;
                FrameSize    = 0;
            }
        } else { // 'fmt' chunk missing
            FormatTag              = DLS_WAVE_FORMAT_PCM;
            BitDepth               = 16;
            Channels               = 1;
            SamplesPerSecond       = 44100;
            AverageBytesPerSecond  = (BitDepth / 8) * SamplesPerSecond * Channels;
            FrameSize              = (BitDepth / 8) * Channels;
            BlockAlign             = FrameSize;
        }
        SamplesTotal = (pCkData) ? (FormatTag == DLS_WAVE_FORMAT_PCM) ? pCkData->GetSize() / FrameSize
                                                                      : 0
                                 : 0;
    }

    /** @brief Destructor.
     *
     * Frees all memory occupied by this sample.
     */
    Sample::~Sample() {
        if (pCkData)
            pCkData->ReleaseChunkData();
        if (pCkFormat)
            pCkFormat->ReleaseChunkData();
    }

    /** @brief Remove all RIFF chunks associated with this Sample object.
     *
     * See Storage::DeleteChunks() for details.
     */
    void Sample::DeleteChunks() {
        // handle base class
        Resource::DeleteChunks();

        // handle own RIFF chunks
        if (pWaveList) {
            RIFF::List* pParent = pWaveList->GetParent();
            pParent->DeleteSubChunk(pWaveList);
            pWaveList = NULL;
        }
    }

    /**
     * Make a deep copy of the Sample object given by @a orig (without the
     * actual sample waveform data however) and assign it to this object.
     *
     * This is a special internal variant of CopyAssign() which only copies the
     * most mandatory member variables. It will be called by gig::Sample
     * descendent instead of CopyAssign() since gig::Sample has its own
     * implementation to access and copy the actual sample waveform data.
     *
     * @param orig - original Sample object to be copied from
     */
    void Sample::CopyAssignCore(const Sample* orig) {
        // handle base classes
        Resource::CopyAssign(orig);
        // handle actual own attributes of this class
        FormatTag = orig->FormatTag;
        Channels = orig->Channels;
        SamplesPerSecond = orig->SamplesPerSecond;
        AverageBytesPerSecond = orig->AverageBytesPerSecond;
        BlockAlign = orig->BlockAlign;
        BitDepth = orig->BitDepth;
        SamplesTotal = orig->SamplesTotal;
        FrameSize = orig->FrameSize;
    }
    
    /**
     * Make a deep copy of the Sample object given by @a orig and assign it to
     * this object.
     *
     * @param orig - original Sample object to be copied from
     */
    void Sample::CopyAssign(const Sample* orig) {
        CopyAssignCore(orig);
        
        // copy sample waveform data (reading directly from disc)
        Resize(orig->GetSize());
        char* buf = (char*) LoadSampleData();
        Sample* pOrig = (Sample*) orig; //HACK: circumventing the constness here for now
        const file_offset_t restorePos = pOrig->pCkData->GetPos();
        pOrig->SetPos(0);
        for (file_offset_t todo = pOrig->GetSize(), i = 0; todo; ) {
            const int iReadAtOnce = 64*1024;
            file_offset_t n = (iReadAtOnce < todo) ? iReadAtOnce : todo;
            n = pOrig->Read(&buf[i], n);
            if (!n) break;
            todo -= n;
            i += (n * pOrig->FrameSize);
        }
        pOrig->pCkData->SetPos(restorePos);
    }

    /** @brief Load sample data into RAM.
     *
     * In case the respective 'data' chunk exists, the sample data will be
     * loaded into RAM (if not done already) and a pointer to the data in
     * RAM will be returned. If this is a new sample, you have to call
     * Resize() with the desired sample size to create the mandatory RIFF
     * chunk for the sample wave data.
     *
     * You can call LoadChunkData() again if you previously scheduled to
     * enlarge the sample data RIFF chunk with a Resize() call. In that case
     * the buffer will be enlarged to the new, scheduled size and you can
     * already place the sample wave data to the buffer and finally call
     * File::Save() to enlarge the sample data's chunk physically and write
     * the new sample wave data in one rush. This approach is definitely
     * recommended if you have to enlarge and write new sample data to a lot
     * of samples.
     *
     * <b>Caution:</b> the buffer pointer will be invalidated once
     * File::Save() was called. You have to call LoadChunkData() again to
     * get a new, valid pointer whenever File::Save() was called.
     *
     * @returns pointer to sample data in RAM, NULL in case respective
     *          'data' chunk does not exist (yet)
     * @throws Exception if data buffer could not be enlarged
     * @see Resize(), File::Save()
     */
    void* Sample::LoadSampleData() {
        return (pCkData) ? pCkData->LoadChunkData() : NULL;
    }

    /** @brief Free sample data from RAM.
     *
     * In case sample data was previously successfully loaded into RAM with
     * LoadSampleData(), this method will free the sample data from RAM.
     */
    void Sample::ReleaseSampleData() {
        if (pCkData) pCkData->ReleaseChunkData();
    }

    /** @brief Returns sample size.
     *
     * Returns the sample wave form's data size (in sample points). This is
     * actually the current, physical size (converted to sample points) of
     * the RIFF chunk which encapsulates the sample's wave data. The
     * returned value is dependant to the current FrameSize value.
     *
     * @returns number of sample points or 0 if FormatTag != DLS_WAVE_FORMAT_PCM
     * @see FrameSize, FormatTag
     */
    file_offset_t Sample::GetSize() const {
        if (FormatTag != DLS_WAVE_FORMAT_PCM) return 0;
        return (pCkData) ? pCkData->GetSize() / FrameSize : 0;
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
     * <b>Caution:</b> You cannot directly write to enlarged samples before
     * calling File::Save() as this might exceed the current sample's
     * boundary!
     *
     * Also note: only DLS_WAVE_FORMAT_PCM is currently supported, that is
     * FormatTag must be DLS_WAVE_FORMAT_PCM. Trying to resize samples with
     * other formats will fail!
     *
     * @param NewSize - new sample wave data size in sample points (must be
     *                  greater than zero)
     * @throws Exception if FormatTag != DLS_WAVE_FORMAT_PCM
     * @throws Exception if \a NewSize is less than 1 or unrealistic large
     * @see File::Save(), FrameSize, FormatTag
     */
    void Sample::Resize(file_offset_t NewSize) {
        if (FormatTag != DLS_WAVE_FORMAT_PCM) throw Exception("Sample's format is not DLS_WAVE_FORMAT_PCM");
        if (NewSize < 1) throw Exception("Sample size must be at least one sample point");
        if ((NewSize >> 48) != 0)
            throw Exception("Unrealistic high DLS sample size detected");
        const file_offset_t sizeInBytes = NewSize * FrameSize;
        pCkData = pWaveList->GetSubChunk(CHUNK_ID_DATA);
        if (pCkData) pCkData->Resize(sizeInBytes);
        else pCkData = pWaveList->AddSubChunk(CHUNK_ID_DATA, sizeInBytes);
    }

    /**
     * Sets the position within the sample (in sample points, not in
     * bytes). Use this method and <i>Read()</i> if you don't want to load
     * the sample into RAM, thus for disk streaming.
     *
     * Also note: only DLS_WAVE_FORMAT_PCM is currently supported, that is
     * FormatTag must be DLS_WAVE_FORMAT_PCM. Trying to reposition the sample
     * with other formats will fail!
     *
     * @param SampleCount  number of sample points
     * @param Whence       to which relation \a SampleCount refers to
     * @returns new position within the sample, 0 if
     *          FormatTag != DLS_WAVE_FORMAT_PCM
     * @throws Exception if no data RIFF chunk was created for the sample yet
     * @see FrameSize, FormatTag
     */
    file_offset_t Sample::SetPos(file_offset_t SampleCount, RIFF::stream_whence_t Whence) {
        if (FormatTag != DLS_WAVE_FORMAT_PCM) return 0; // failed: wave data not PCM format
        if (!pCkData) throw Exception("No data chunk created for sample yet, call Sample::Resize() to create one");
        file_offset_t orderedBytes = SampleCount * FrameSize;
        file_offset_t result = pCkData->SetPos(orderedBytes, Whence);
        return (result == orderedBytes) ? SampleCount
                                        : result / FrameSize;
    }

    /**
     * Reads \a SampleCount number of sample points from the current
     * position into the buffer pointed by \a pBuffer and increments the
     * position within the sample. Use this method and <i>SetPos()</i> if you
     * don't want to load the sample into RAM, thus for disk streaming.
     *
     * @param pBuffer      destination buffer
     * @param SampleCount  number of sample points to read
     */
    file_offset_t Sample::Read(void* pBuffer, file_offset_t SampleCount) {
        if (FormatTag != DLS_WAVE_FORMAT_PCM) return 0; // failed: wave data not PCM format
        return pCkData->Read(pBuffer, SampleCount, FrameSize); // FIXME: channel inversion due to endian correction?
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
     * @param pBuffer     - source buffer
     * @param SampleCount - number of sample points to write
     * @throws Exception if current sample size is too small
     * @see LoadSampleData()
     */
    file_offset_t Sample::Write(void* pBuffer, file_offset_t SampleCount) {
        if (FormatTag != DLS_WAVE_FORMAT_PCM) return 0; // failed: wave data not PCM format
        if (GetSize() < SampleCount) throw Exception("Could not write sample data, current sample size to small");
        return pCkData->Write(pBuffer, SampleCount, FrameSize); // FIXME: channel inversion due to endian correction?
    }

    /**
     * Apply sample and its settings to the respective RIFF chunks. You have
     * to call File::Save() to make changes persistent.
     *
     * @param pProgress - callback function for progress notification
     * @throws Exception if FormatTag != DLS_WAVE_FORMAT_PCM or no sample data
     *                   was provided yet
     */
    void Sample::UpdateChunks(progress_t* pProgress) {
        if (FormatTag != DLS_WAVE_FORMAT_PCM)
            throw Exception("Could not save sample, only PCM format is supported");
        // we refuse to do anything if not sample wave form was provided yet
        if (!pCkData)
            throw Exception("Could not save sample, there is no sample data to save");
        // update chunks of base class as well
        Resource::UpdateChunks(pProgress);
        // make sure 'fmt' chunk exists
        RIFF::Chunk* pCkFormat = pWaveList->GetSubChunk(CHUNK_ID_FMT);
        if (!pCkFormat) pCkFormat = pWaveList->AddSubChunk(CHUNK_ID_FMT, 16); // assumes PCM format
        uint8_t* pData = (uint8_t*) pCkFormat->LoadChunkData();
        // update 'fmt' chunk
        store16(&pData[0], FormatTag);
        store16(&pData[2], Channels);
        store32(&pData[4], SamplesPerSecond);
        store32(&pData[8], AverageBytesPerSecond);
        store16(&pData[12], BlockAlign);
        store16(&pData[14], BitDepth); // assuming PCM format
    }



// *************** Region ***************
// *

    Region::Region(Instrument* pInstrument, RIFF::List* rgnList) : Resource(pInstrument, rgnList), Articulator(rgnList), Sampler(rgnList) {
        pCkRegion = rgnList;

        // articulation information
        RIFF::Chunk* rgnh = rgnList->GetSubChunk(CHUNK_ID_RGNH);
        if (rgnh) {
            rgnh->SetPos(0);

            rgnh->Read(&KeyRange, 2, 2);
            rgnh->Read(&VelocityRange, 2, 2);
            FormatOptionFlags = rgnh->ReadUint16();
            KeyGroup = rgnh->ReadUint16();
            // Layer is optional
            if (rgnh->RemainingBytes() >= sizeof(uint16_t)) {
                rgnh->Read(&Layer, 1, sizeof(uint16_t));
            } else Layer = 0;
        } else { // 'rgnh' chunk is missing
            KeyRange.low  = 0;
            KeyRange.high = 127;
            VelocityRange.low  = 0;
            VelocityRange.high = 127;
            FormatOptionFlags = F_RGN_OPTION_SELFNONEXCLUSIVE;
            KeyGroup = 0;
            Layer = 0;
        }
        SelfNonExclusive = FormatOptionFlags & F_RGN_OPTION_SELFNONEXCLUSIVE;

        // sample information
        RIFF::Chunk* wlnk = rgnList->GetSubChunk(CHUNK_ID_WLNK);
        if (wlnk) {
            wlnk->SetPos(0);

            WaveLinkOptionFlags = wlnk->ReadUint16();
            PhaseGroup          = wlnk->ReadUint16();
            Channel             = wlnk->ReadUint32();
            WavePoolTableIndex  = wlnk->ReadUint32();
        } else { // 'wlnk' chunk is missing
            WaveLinkOptionFlags = 0;
            PhaseGroup          = 0;
            Channel             = 0; // mono
            WavePoolTableIndex  = 0; // first entry in wave pool table
        }
        PhaseMaster  = WaveLinkOptionFlags & F_WAVELINK_PHASE_MASTER;
        MultiChannel = WaveLinkOptionFlags & F_WAVELINK_MULTICHANNEL;

        pSample = NULL;
    }

    /** @brief Destructor.
     *
     * Intended to free up all memory occupied by this Region object. ATM this
     * destructor implementation does nothing though.
     */
    Region::~Region() {
    }

    /** @brief Remove all RIFF chunks associated with this Region object.
     *
     * See Storage::DeleteChunks() for details.
     */
    void Region::DeleteChunks() {
        // handle base classes
        Resource::DeleteChunks();
        Articulator::DeleteChunks();
        Sampler::DeleteChunks();

        // handle own RIFF chunks
        if (pCkRegion) {
            RIFF::List* pParent = pCkRegion->GetParent();
            pParent->DeleteSubChunk(pCkRegion);
            pCkRegion = NULL;
        }
    }

    Sample* Region::GetSample() {
        if (pSample) return pSample;
        File* file = (File*) GetParent()->GetParent();
        uint64_t soughtoffset = file->pWavePoolTable[WavePoolTableIndex];
        Sample* sample = file->GetFirstSample();
        while (sample) {
            if (sample->ullWavePoolOffset == soughtoffset) return (pSample = sample);
            sample = file->GetNextSample();
        }
        return NULL;
    }

    /**
     * Assign another sample to this Region.
     *
     * @param pSample - sample to be assigned
     */
    void Region::SetSample(Sample* pSample) {
        this->pSample = pSample;
        WavePoolTableIndex = 0; // we update this offset when we Save()
    }

    /**
     * Modifies the key range of this Region and makes sure the respective
     * chunks are in correct order.
     *
     * @param Low  - lower end of key range
     * @param High - upper end of key range
     */
    void Region::SetKeyRange(uint16_t Low, uint16_t High) {
        KeyRange.low  = Low;
        KeyRange.high = High;

        // make sure regions are already loaded
        Instrument* pInstrument = (Instrument*) GetParent();
        if (!pInstrument->pRegions) pInstrument->LoadRegions();
        if (!pInstrument->pRegions) return;

        // find the r which is the first one to the right of this region
        // at its new position
        Region* r = NULL;
        Region* prev_region = NULL;
        for (
            Instrument::RegionList::iterator iter = pInstrument->pRegions->begin();
            iter != pInstrument->pRegions->end(); iter++
        ) {
            if ((*iter)->KeyRange.low > this->KeyRange.low) {
                r = *iter;
                break;
            }
            prev_region = *iter;
        }

        // place this region before r if it's not already there
        if (prev_region != this) pInstrument->MoveRegion(this, r);
    }

    /**
     * Apply Region settings to the respective RIFF chunks. You have to
     * call File::Save() to make changes persistent.
     *
     * @param pProgress - callback function for progress notification
     * @throws Exception - if the Region's sample could not be found
     */
    void Region::UpdateChunks(progress_t* pProgress) {
        // make sure 'rgnh' chunk exists
        RIFF::Chunk* rgnh = pCkRegion->GetSubChunk(CHUNK_ID_RGNH);
        if (!rgnh) rgnh = pCkRegion->AddSubChunk(CHUNK_ID_RGNH, Layer ? 14 : 12);
        uint8_t* pData = (uint8_t*) rgnh->LoadChunkData();
        FormatOptionFlags = (SelfNonExclusive)
                                ? FormatOptionFlags | F_RGN_OPTION_SELFNONEXCLUSIVE
                                : FormatOptionFlags & (~F_RGN_OPTION_SELFNONEXCLUSIVE);
        // update 'rgnh' chunk
        store16(&pData[0], KeyRange.low);
        store16(&pData[2], KeyRange.high);
        store16(&pData[4], VelocityRange.low);
        store16(&pData[6], VelocityRange.high);
        store16(&pData[8], FormatOptionFlags);
        store16(&pData[10], KeyGroup);
        if (rgnh->GetSize() >= 14) store16(&pData[12], Layer);

        // update chunks of base classes as well (but skip Resource,
        // as a rgn doesn't seem to have dlid and INFO chunks)
        Articulator::UpdateChunks(pProgress);
        Sampler::UpdateChunks(pProgress);

        // make sure 'wlnk' chunk exists
        RIFF::Chunk* wlnk = pCkRegion->GetSubChunk(CHUNK_ID_WLNK);
        if (!wlnk) wlnk = pCkRegion->AddSubChunk(CHUNK_ID_WLNK, 12);
        pData = (uint8_t*) wlnk->LoadChunkData();
        WaveLinkOptionFlags = (PhaseMaster)
                                  ? WaveLinkOptionFlags | F_WAVELINK_PHASE_MASTER
                                  : WaveLinkOptionFlags & (~F_WAVELINK_PHASE_MASTER);
        WaveLinkOptionFlags = (MultiChannel)
                                  ? WaveLinkOptionFlags | F_WAVELINK_MULTICHANNEL
                                  : WaveLinkOptionFlags & (~F_WAVELINK_MULTICHANNEL);
        // get sample's wave pool table index
        int index = -1;
        File* pFile = (File*) GetParent()->GetParent();
        if (pFile->pSamples) {
            File::SampleList::iterator iter = pFile->pSamples->begin();
            File::SampleList::iterator end  = pFile->pSamples->end();
            for (int i = 0; iter != end; ++iter, i++) {
                if (*iter == pSample) {
                    index = i;
                    break;
                }
            }
        }
        WavePoolTableIndex = index;
        // update 'wlnk' chunk
        store16(&pData[0], WaveLinkOptionFlags);
        store16(&pData[2], PhaseGroup);
        store32(&pData[4], Channel);
        store32(&pData[8], WavePoolTableIndex);
    }
    
    /**
     * Make a (semi) deep copy of the Region object given by @a orig and assign
     * it to this object.
     *
     * Note that the sample pointer referenced by @a orig is simply copied as
     * memory address. Thus the respective sample is shared, not duplicated!
     *
     * @param orig - original Region object to be copied from
     */
    void Region::CopyAssign(const Region* orig) {
        // handle base classes
        Resource::CopyAssign(orig);
        Articulator::CopyAssign(orig);
        Sampler::CopyAssign(orig);
        // handle actual own attributes of this class
        // (the trivial ones)
        VelocityRange = orig->VelocityRange;
        KeyGroup = orig->KeyGroup;
        Layer = orig->Layer;
        SelfNonExclusive = orig->SelfNonExclusive;
        PhaseMaster = orig->PhaseMaster;
        PhaseGroup = orig->PhaseGroup;
        MultiChannel = orig->MultiChannel;
        Channel = orig->Channel;
        // only take the raw sample reference if the two Region objects are
        // part of the same file
        if (GetParent()->GetParent() == orig->GetParent()->GetParent()) {
            WavePoolTableIndex = orig->WavePoolTableIndex;
            pSample = orig->pSample;
        } else {
            WavePoolTableIndex = -1;
            pSample = NULL;
        }
        FormatOptionFlags = orig->FormatOptionFlags;
        WaveLinkOptionFlags = orig->WaveLinkOptionFlags;
        // handle the last, a bit sensible attribute
        SetKeyRange(orig->KeyRange.low, orig->KeyRange.high);
    }


// *************** Instrument ***************
// *

    /** @brief Constructor.
     *
     * Load an existing instrument definition or create a new one. An 'ins'
     * list chunk must be given to this constructor. In case this 'ins' list
     * chunk contains a 'insh' chunk, the instrument data fields will be
     * loaded from there, otherwise default values will be used and the
     * 'insh' chunk will be created once File::Save() was called.
     *
     * @param pFile   - pointer to DLS::File where this instrument is
     *                  located (or will be located)
     * @param insList - pointer to 'ins' list chunk which is (or will be)
     *                  associated with this instrument
     */
    Instrument::Instrument(File* pFile, RIFF::List* insList) : Resource(pFile, insList), Articulator(insList) {
        pCkInstrument = insList;

        midi_locale_t locale;
        RIFF::Chunk* insh = pCkInstrument->GetSubChunk(CHUNK_ID_INSH);
        if (insh) {
            insh->SetPos(0);

            Regions = insh->ReadUint32();
            insh->Read(&locale, 2, 4);
        } else { // 'insh' chunk missing
            Regions = 0;
            locale.bank       = 0;
            locale.instrument = 0;
        }

        MIDIProgram    = locale.instrument;
        IsDrum         = locale.bank & DRUM_TYPE_MASK;
        MIDIBankCoarse = (uint8_t) MIDI_BANK_COARSE(locale.bank);
        MIDIBankFine   = (uint8_t) MIDI_BANK_FINE(locale.bank);
        MIDIBank       = MIDI_BANK_MERGE(MIDIBankCoarse, MIDIBankFine);

        pRegions = NULL;
    }

    Region* Instrument::GetFirstRegion() {
        if (!pRegions) LoadRegions();
        if (!pRegions) return NULL;
        RegionsIterator = pRegions->begin();
        return (RegionsIterator != pRegions->end()) ? *RegionsIterator : NULL;
    }

    Region* Instrument::GetNextRegion() {
        if (!pRegions) return NULL;
        RegionsIterator++;
        return (RegionsIterator != pRegions->end()) ? *RegionsIterator : NULL;
    }

    void Instrument::LoadRegions() {
        if (!pRegions) pRegions = new RegionList;
        RIFF::List* lrgn = pCkInstrument->GetSubList(LIST_TYPE_LRGN);
        if (lrgn) {
            uint32_t regionCkType = (lrgn->GetSubList(LIST_TYPE_RGN2)) ? LIST_TYPE_RGN2 : LIST_TYPE_RGN; // prefer regions level 2
            RIFF::List* rgn = lrgn->GetFirstSubList();
            while (rgn) {
                if (rgn->GetListType() == regionCkType) {
                    pRegions->push_back(new Region(this, rgn));
                }
                rgn = lrgn->GetNextSubList();
            }
        }
    }

    Region* Instrument::AddRegion() {
        if (!pRegions) LoadRegions();
        RIFF::List* lrgn = pCkInstrument->GetSubList(LIST_TYPE_LRGN);
        if (!lrgn)  lrgn = pCkInstrument->AddSubList(LIST_TYPE_LRGN);
        RIFF::List* rgn = lrgn->AddSubList(LIST_TYPE_RGN);
        Region* pNewRegion = new Region(this, rgn);
        pRegions->push_back(pNewRegion);
        Regions = (uint32_t) pRegions->size();
        return pNewRegion;
    }

    void Instrument::MoveRegion(Region* pSrc, Region* pDst) {
        RIFF::List* lrgn = pCkInstrument->GetSubList(LIST_TYPE_LRGN);
        lrgn->MoveSubChunk(pSrc->pCkRegion, (RIFF::Chunk*) (pDst ? pDst->pCkRegion : 0));

        pRegions->remove(pSrc);
        RegionList::iterator iter = find(pRegions->begin(), pRegions->end(), pDst);
        pRegions->insert(iter, pSrc);
    }

    void Instrument::DeleteRegion(Region* pRegion) {
        if (!pRegions) return;
        RegionList::iterator iter = find(pRegions->begin(), pRegions->end(), pRegion);
        if (iter == pRegions->end()) return;
        pRegions->erase(iter);
        Regions = (uint32_t) pRegions->size();
        pRegion->DeleteChunks();
        delete pRegion;
    }

    /**
     * Apply Instrument with all its Regions to the respective RIFF chunks.
     * You have to call File::Save() to make changes persistent.
     *
     * @param pProgress - callback function for progress notification
     * @throws Exception - on errors
     */
    void Instrument::UpdateChunks(progress_t* pProgress) {
        // first update base classes' chunks
        Resource::UpdateChunks(pProgress);
        Articulator::UpdateChunks(pProgress);
        // make sure 'insh' chunk exists
        RIFF::Chunk* insh = pCkInstrument->GetSubChunk(CHUNK_ID_INSH);
        if (!insh) insh = pCkInstrument->AddSubChunk(CHUNK_ID_INSH, 12);
        uint8_t* pData = (uint8_t*) insh->LoadChunkData();
        // update 'insh' chunk
        Regions = (pRegions) ? uint32_t(pRegions->size()) : 0;
        midi_locale_t locale;
        locale.instrument = MIDIProgram;
        locale.bank       = MIDI_BANK_ENCODE(MIDIBankCoarse, MIDIBankFine);
        locale.bank       = (IsDrum) ? locale.bank | DRUM_TYPE_MASK : locale.bank & (~DRUM_TYPE_MASK);
        MIDIBank          = MIDI_BANK_MERGE(MIDIBankCoarse, MIDIBankFine); // just a sync, when we're at it
        store32(&pData[0], Regions);
        store32(&pData[4], locale.bank);
        store32(&pData[8], locale.instrument);
        // update Region's chunks
        if (!pRegions) return;
        RegionList::iterator iter = pRegions->begin();
        RegionList::iterator end  = pRegions->end();
        for (int i = 0; iter != end; ++iter, ++i) {
            if (pProgress) {
                // divide local progress into subprogress
                progress_t subprogress;
                __divide_progress(pProgress, &subprogress, pRegions->size(), i);
                // do the actual work
                (*iter)->UpdateChunks(&subprogress);
            } else
                (*iter)->UpdateChunks(NULL);
        }
        if (pProgress)
            __notify_progress(pProgress, 1.0); // notify done
    }

    /** @brief Destructor.
     *
     * Frees all memory occupied by this instrument.
     */
    Instrument::~Instrument() {
        if (pRegions) {
            RegionList::iterator iter = pRegions->begin();
            RegionList::iterator end  = pRegions->end();
            while (iter != end) {
                delete *iter;
                iter++;
            }
            delete pRegions;
        }
    }

    /** @brief Remove all RIFF chunks associated with this Instrument object.
     *
     * See Storage::DeleteChunks() for details.
     */
    void Instrument::DeleteChunks() {
        // handle base classes
        Resource::DeleteChunks();
        Articulator::DeleteChunks();

        // handle RIFF chunks of members
        if (pRegions) {
            RegionList::iterator it  = pRegions->begin();
            RegionList::iterator end = pRegions->end();
            for (; it != end; ++it)
                (*it)->DeleteChunks();
        }

        // handle own RIFF chunks
        if (pCkInstrument) {
            RIFF::List* pParent = pCkInstrument->GetParent();
            pParent->DeleteSubChunk(pCkInstrument);
            pCkInstrument = NULL;
        }
    }

    void Instrument::CopyAssignCore(const Instrument* orig) {
        // handle base classes
        Resource::CopyAssign(orig);
        Articulator::CopyAssign(orig);
        // handle actual own attributes of this class
        // (the trivial ones)
        IsDrum = orig->IsDrum;
        MIDIBank = orig->MIDIBank;
        MIDIBankCoarse = orig->MIDIBankCoarse;
        MIDIBankFine = orig->MIDIBankFine;
        MIDIProgram = orig->MIDIProgram;
    }
    
    /**
     * Make a (semi) deep copy of the Instrument object given by @a orig and assign
     * it to this object.
     *
     * Note that all sample pointers referenced by @a orig are simply copied as
     * memory address. Thus the respective samples are shared, not duplicated!
     *
     * @param orig - original Instrument object to be copied from
     */
    void Instrument::CopyAssign(const Instrument* orig) {
        CopyAssignCore(orig);
        // delete all regions first
        while (Regions) DeleteRegion(GetFirstRegion());
        // now recreate and copy regions
        {
            RegionList::const_iterator it = orig->pRegions->begin();
            for (int i = 0; i < orig->Regions; ++i, ++it) {
                Region* dstRgn = AddRegion();
                //NOTE: Region does semi-deep copy !
                dstRgn->CopyAssign(*it);
            }
        }
    }


// *************** File ***************
// *

    /** @brief Constructor.
     *
     * Default constructor, use this to create an empty DLS file. You have
     * to add samples, instruments and finally call Save() to actually write
     * a DLS file.
     */
    File::File() : Resource(NULL, pRIFF = new RIFF::File(RIFF_TYPE_DLS)) {
        pRIFF->SetByteOrder(RIFF::endian_little);
        bOwningRiff = true;
        pVersion = new version_t;
        pVersion->major   = 0;
        pVersion->minor   = 0;
        pVersion->release = 0;
        pVersion->build   = 0;

        Instruments      = 0;
        WavePoolCount    = 0;
        pWavePoolTable   = NULL;
        pWavePoolTableHi = NULL;
        WavePoolHeaderSize = 8;

        pSamples     = NULL;
        pInstruments = NULL;

        b64BitWavePoolOffsets = false;
    }

    /** @brief Constructor.
     *
     * Load an existing DLS file.
     *
     * @param pRIFF - pointer to a RIFF file which is actually the DLS file
     *                to load
     * @throws Exception if given file is not a DLS file, expected chunks
     *                   are missing
     */
    File::File(RIFF::File* pRIFF) : Resource(NULL, pRIFF) {
        if (!pRIFF) throw DLS::Exception("NULL pointer reference to RIFF::File object.");
        this->pRIFF = pRIFF;
        bOwningRiff = false;
        RIFF::Chunk* ckVersion = pRIFF->GetSubChunk(CHUNK_ID_VERS);
        if (ckVersion) {
            ckVersion->SetPos(0);

            pVersion = new version_t;
            ckVersion->Read(pVersion, 4, 2);
        }
        else pVersion = NULL;

        RIFF::Chunk* colh = pRIFF->GetSubChunk(CHUNK_ID_COLH);
        if (!colh) throw DLS::Exception("Mandatory chunks in RIFF list chunk not found.");
        colh->SetPos(0);
        Instruments = colh->ReadUint32();

        RIFF::Chunk* ptbl = pRIFF->GetSubChunk(CHUNK_ID_PTBL);
        if (!ptbl) { // pool table is missing - this is probably an ".art" file
            WavePoolCount    = 0;
            pWavePoolTable   = NULL;
            pWavePoolTableHi = NULL;
            WavePoolHeaderSize = 8;
            b64BitWavePoolOffsets = false;
        } else {
            ptbl->SetPos(0);

            WavePoolHeaderSize = ptbl->ReadUint32();
            WavePoolCount  = ptbl->ReadUint32();
            pWavePoolTable = new uint32_t[WavePoolCount];
            pWavePoolTableHi = new uint32_t[WavePoolCount];
            ptbl->SetPos(WavePoolHeaderSize);

            // Check for 64 bit offsets (used in gig v3 files)
            b64BitWavePoolOffsets = (ptbl->GetSize() - WavePoolHeaderSize == WavePoolCount * 8);
            if (b64BitWavePoolOffsets) {
                for (int i = 0 ; i < WavePoolCount ; i++) {
                    pWavePoolTableHi[i] = ptbl->ReadUint32();
                    pWavePoolTable[i] = ptbl->ReadUint32();
                    //NOTE: disabled this 2GB check, not sure why this check was still left here (Christian, 2016-05-12)
                    //if (pWavePoolTable[i] & 0x80000000)
                    //    throw DLS::Exception("Files larger than 2 GB not yet supported");
                }
            } else { // conventional 32 bit offsets
                ptbl->Read(pWavePoolTable, WavePoolCount, sizeof(uint32_t));
                for (int i = 0 ; i < WavePoolCount ; i++) pWavePoolTableHi[i] = 0;
            }
        }

        pSamples     = NULL;
        pInstruments = NULL;
    }

    File::~File() {
        if (pInstruments) {
            InstrumentList::iterator iter = pInstruments->begin();
            InstrumentList::iterator end  = pInstruments->end();
            while (iter != end) {
                delete *iter;
                iter++;
            }
            delete pInstruments;
        }

        if (pSamples) {
            SampleList::iterator iter = pSamples->begin();
            SampleList::iterator end  = pSamples->end();
            while (iter != end) {
                delete *iter;
                iter++;
            }
            delete pSamples;
        }

        if (pWavePoolTable) delete[] pWavePoolTable;
        if (pWavePoolTableHi) delete[] pWavePoolTableHi;
        if (pVersion) delete pVersion;
        for (std::list<RIFF::File*>::iterator i = ExtensionFiles.begin() ; i != ExtensionFiles.end() ; i++)
            delete *i;
        if (bOwningRiff)
            delete pRIFF;
    }

    Sample* File::GetFirstSample() {
        if (!pSamples) LoadSamples();
        if (!pSamples) return NULL;
        SamplesIterator = pSamples->begin();
        return (SamplesIterator != pSamples->end()) ? *SamplesIterator : NULL;
    }

    Sample* File::GetNextSample() {
        if (!pSamples) return NULL;
        SamplesIterator++;
        return (SamplesIterator != pSamples->end()) ? *SamplesIterator : NULL;
    }

    void File::LoadSamples() {
        if (!pSamples) pSamples = new SampleList;
        RIFF::List* wvpl = pRIFF->GetSubList(LIST_TYPE_WVPL);
        if (wvpl) {
            file_offset_t wvplFileOffset = wvpl->GetFilePos() -
                                           wvpl->GetPos(); // should be zero, but just to be sure
            RIFF::List* wave = wvpl->GetFirstSubList();
            while (wave) {
                if (wave->GetListType() == LIST_TYPE_WAVE) {
                    file_offset_t waveFileOffset = wave->GetFilePos() -
                                                   wave->GetPos(); // should be zero, but just to be sure
                    pSamples->push_back(new Sample(this, wave, waveFileOffset - wvplFileOffset));
                }
                wave = wvpl->GetNextSubList();
            }
        }
        else { // Seen a dwpl list chunk instead of a wvpl list chunk in some file (officially not DLS compliant)
            RIFF::List* dwpl = pRIFF->GetSubList(LIST_TYPE_DWPL);
            if (dwpl) {
                file_offset_t dwplFileOffset = dwpl->GetFilePos() -
                                               dwpl->GetPos(); // should be zero, but just to be sure
                RIFF::List* wave = dwpl->GetFirstSubList();
                while (wave) {
                    if (wave->GetListType() == LIST_TYPE_WAVE) {
                        file_offset_t waveFileOffset = wave->GetFilePos() -
                                                       wave->GetPos(); // should be zero, but just to be sure
                        pSamples->push_back(new Sample(this, wave, waveFileOffset - dwplFileOffset));
                    }
                    wave = dwpl->GetNextSubList();
                }
            }
        }
    }

    /** @brief Add a new sample.
     *
     * This will create a new Sample object for the DLS file. You have to
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
       pSamples->push_back(pSample);
       return pSample;
    }

    /** @brief Delete a sample.
     *
     * This will delete the given Sample object from the DLS file. You have
     * to call Save() to make this persistent to the file.
     *
     * @param pSample - sample to delete
     */
    void File::DeleteSample(Sample* pSample) {
        if (!pSamples) return;
        SampleList::iterator iter = find(pSamples->begin(), pSamples->end(), pSample);
        if (iter == pSamples->end()) return;
        pSamples->erase(iter);
        pSample->DeleteChunks();
        delete pSample;
    }

    Instrument* File::GetFirstInstrument() {
        if (!pInstruments) LoadInstruments();
        if (!pInstruments) return NULL;
        InstrumentsIterator = pInstruments->begin();
        return (InstrumentsIterator != pInstruments->end()) ? *InstrumentsIterator : NULL;
    }

    Instrument* File::GetNextInstrument() {
        if (!pInstruments) return NULL;
        InstrumentsIterator++;
        return (InstrumentsIterator != pInstruments->end()) ? *InstrumentsIterator : NULL;
    }

    void File::LoadInstruments() {
        if (!pInstruments) pInstruments = new InstrumentList;
        RIFF::List* lstInstruments = pRIFF->GetSubList(LIST_TYPE_LINS);
        if (lstInstruments) {
            RIFF::List* lstInstr = lstInstruments->GetFirstSubList();
            while (lstInstr) {
                if (lstInstr->GetListType() == LIST_TYPE_INS) {
                    pInstruments->push_back(new Instrument(this, lstInstr));
                }
                lstInstr = lstInstruments->GetNextSubList();
            }
        }
    }

    /** @brief Add a new instrument definition.
     *
     * This will create a new Instrument object for the DLS file. You have
     * to call Save() to make this persistent to the file.
     *
     * @returns pointer to new Instrument object
     */
    Instrument* File::AddInstrument() {
       if (!pInstruments) LoadInstruments();
       __ensureMandatoryChunksExist();
       RIFF::List* lstInstruments = pRIFF->GetSubList(LIST_TYPE_LINS);
       RIFF::List* lstInstr = lstInstruments->AddSubList(LIST_TYPE_INS);
       Instrument* pInstrument = new Instrument(this, lstInstr);
       pInstruments->push_back(pInstrument);
       return pInstrument;
    }

    /** @brief Delete an instrument.
     *
     * This will delete the given Instrument object from the DLS file. You
     * have to call Save() to make this persistent to the file.
     *
     * @param pInstrument - instrument to delete
     */
    void File::DeleteInstrument(Instrument* pInstrument) {
        if (!pInstruments) return;
        InstrumentList::iterator iter = find(pInstruments->begin(), pInstruments->end(), pInstrument);
        if (iter == pInstruments->end()) return;
        pInstruments->erase(iter);
        pInstrument->DeleteChunks();
        delete pInstrument;
    }

    /**
     * Returns the underlying RIFF::File used for persistency of this DLS::File
     * object.
     */
    RIFF::File* File::GetRiffFile() {
        return pRIFF;
    }

    /**
     * Returns extension file of given index. Extension files are used
     * sometimes to circumvent the 2 GB file size limit of the RIFF format and
     * of certain operating systems in general. In this case, instead of just
     * using one file, the content is spread among several files with similar
     * file name scheme. This is especially used by some GigaStudio sound
     * libraries.
     *
     * @param index - index of extension file
     * @returns sought extension file, NULL if index out of bounds
     * @see GetFileName()
     */
    RIFF::File* File::GetExtensionFile(int index) {
        if (index < 0 || index >= ExtensionFiles.size()) return NULL;
        std::list<RIFF::File*>::iterator iter = ExtensionFiles.begin();
        for (int i = 0; iter != ExtensionFiles.end(); ++iter, ++i)
            if (i == index) return *iter;
        return NULL;
    }

    /** @brief File name of this DLS file.
     *
     * This method returns the file name as it was provided when loading
     * the respective DLS file. However in case the File object associates
     * an empty, that is new DLS file, which was not yet saved to disk,
     * this method will return an empty string.
     *
     * @see GetExtensionFile()
     */
    String File::GetFileName() {
        return pRIFF->GetFileName();
    }
    
    /**
     * You may call this method store a future file name, so you don't have to
     * to pass it to the Save() call later on.
     */
    void File::SetFileName(const String& name) {
        pRIFF->SetFileName(name);
    }

    /**
     * Apply all the DLS file's current instruments, samples and settings to
     * the respective RIFF chunks. You have to call Save() to make changes
     * persistent.
     *
     * @param pProgress - callback function for progress notification
     * @throws Exception - on errors
     */
    void File::UpdateChunks(progress_t* pProgress) {
        // first update base class's chunks
        Resource::UpdateChunks(pProgress);

        // if version struct exists, update 'vers' chunk
        if (pVersion) {
            RIFF::Chunk* ckVersion    = pRIFF->GetSubChunk(CHUNK_ID_VERS);
            if (!ckVersion) ckVersion = pRIFF->AddSubChunk(CHUNK_ID_VERS, 8);
            uint8_t* pData = (uint8_t*) ckVersion->LoadChunkData();
            store16(&pData[0], pVersion->minor);
            store16(&pData[2], pVersion->major);
            store16(&pData[4], pVersion->build);
            store16(&pData[6], pVersion->release);
        }

        // update 'colh' chunk
        Instruments = (pInstruments) ? uint32_t(pInstruments->size()) : 0;
        RIFF::Chunk* colh = pRIFF->GetSubChunk(CHUNK_ID_COLH);
        if (!colh)   colh = pRIFF->AddSubChunk(CHUNK_ID_COLH, 4);
        uint8_t* pData = (uint8_t*) colh->LoadChunkData();
        store32(pData, Instruments);

        // update instrument's chunks
        if (pInstruments) {
            if (pProgress) {
                // divide local progress into subprogress
                progress_t subprogress;
                __divide_progress(pProgress, &subprogress, 20.f, 0.f); // arbitrarily subdivided into 5% of total progress

                // do the actual work
                InstrumentList::iterator iter = pInstruments->begin();
                InstrumentList::iterator end  = pInstruments->end();
                for (int i = 0; iter != end; ++iter, ++i) {
                    // divide subprogress into sub-subprogress
                    progress_t subsubprogress;
                    __divide_progress(&subprogress, &subsubprogress, pInstruments->size(), i);
                    // do the actual work
                    (*iter)->UpdateChunks(&subsubprogress);
                }

                __notify_progress(&subprogress, 1.0); // notify subprogress done
            } else {
                InstrumentList::iterator iter = pInstruments->begin();
                InstrumentList::iterator end  = pInstruments->end();
                for (int i = 0; iter != end; ++iter, ++i) {
                    (*iter)->UpdateChunks(NULL);
                }
            }
        }

        // update 'ptbl' chunk
        const int iSamples = (pSamples) ? int(pSamples->size()) : 0;
        int iPtblOffsetSize = (b64BitWavePoolOffsets) ? 8 : 4;
        RIFF::Chunk* ptbl = pRIFF->GetSubChunk(CHUNK_ID_PTBL);
        if (!ptbl)   ptbl = pRIFF->AddSubChunk(CHUNK_ID_PTBL, 1 /*anything, we'll resize*/);
        int iPtblSize = WavePoolHeaderSize + iPtblOffsetSize * iSamples;
        ptbl->Resize(iPtblSize);
        pData = (uint8_t*) ptbl->LoadChunkData();
        WavePoolCount = iSamples;
        store32(&pData[4], WavePoolCount);
        // we actually update the sample offsets in the pool table when we Save()
        memset(&pData[WavePoolHeaderSize], 0, iPtblSize - WavePoolHeaderSize);

        // update sample's chunks
        if (pSamples) {
            if (pProgress) {
                // divide local progress into subprogress
                progress_t subprogress;
                __divide_progress(pProgress, &subprogress, 20.f, 1.f); // arbitrarily subdivided into 95% of total progress

                // do the actual work
                SampleList::iterator iter = pSamples->begin();
                SampleList::iterator end  = pSamples->end();
                for (int i = 0; iter != end; ++iter, ++i) {
                    // divide subprogress into sub-subprogress
                    progress_t subsubprogress;
                    __divide_progress(&subprogress, &subsubprogress, pSamples->size(), i);
                    // do the actual work
                    (*iter)->UpdateChunks(&subsubprogress);
                }

                __notify_progress(&subprogress, 1.0); // notify subprogress done
            } else {
                SampleList::iterator iter = pSamples->begin();
                SampleList::iterator end  = pSamples->end();
                for (int i = 0; iter != end; ++iter, ++i) {
                    (*iter)->UpdateChunks(NULL);
                }
            }
        }

        // if there are any extension files, gather which ones are regular
        // extension files used as wave pool files (.gx00, .gx01, ... , .gx98)
        // and which one is probably a convolution (GigaPulse) file (always to
        // be saved as .gx99)
        std::list<RIFF::File*> poolFiles;  // < for (.gx00, .gx01, ... , .gx98) files
        RIFF::File* pGigaPulseFile = NULL; // < for .gx99 file
        if (!ExtensionFiles.empty()) {
            std::list<RIFF::File*>::iterator it = ExtensionFiles.begin();
            for (; it != ExtensionFiles.end(); ++it) {
                //FIXME: the .gx99 file is always used by GSt for convolution
                // data (GigaPulse); so we should better detect by subchunk
                // whether the extension file is intended for convolution
                // instead of checkking for a file name, because the latter does
                // not work for saving new gigs created from scratch
                const std::string oldName = (*it)->GetFileName();
                const bool isGigaPulseFile = (extensionOfPath(oldName) == "gx99");
                if (isGigaPulseFile)
                    pGigaPulseFile = *it;
                else
                    poolFiles.push_back(*it);
            }
        }

        // update the 'xfil' chunk which describes all extension files (wave
        // pool files) except the .gx99 file
        if (!poolFiles.empty()) {
            const int n = poolFiles.size();
            const int iHeaderSize = 4;
            const int iEntrySize = 144;

            // make sure chunk exists, and with correct size
            RIFF::Chunk* ckXfil = pRIFF->GetSubChunk(CHUNK_ID_XFIL);
            if (ckXfil)
                ckXfil->Resize(iHeaderSize + n * iEntrySize);
            else
                ckXfil = pRIFF->AddSubChunk(CHUNK_ID_XFIL, iHeaderSize + n * iEntrySize);

            uint8_t* pData = (uint8_t*) ckXfil->LoadChunkData();

            // re-assemble the chunk's content
            store32(pData, n);
            std::list<RIFF::File*>::iterator itExtFile = poolFiles.begin();
            for (int i = 0, iOffset = 4; i < n;
                 ++itExtFile, ++i, iOffset += iEntrySize)
            {
                // update the filename string and 5 byte extension of each extension file
                std::string file = lastPathComponent(
                    (*itExtFile)->GetFileName()
                );
                if (file.length() + 6 > 128)
                    throw Exception("Fatal error, extension filename length exceeds 122 byte maximum");
                uint8_t* pStrings = &pData[iOffset];
                memset(pStrings, 0, 128);
                memcpy(pStrings, file.c_str(), file.length());
                pStrings += file.length() + 1;
                std::string ext = file.substr(file.length()-5);
                memcpy(pStrings, ext.c_str(), 5);
                // update the dlsid of the extension file
                uint8_t* pId = &pData[iOffset + 128];
                dlsid_t id;
                RIFF::Chunk* ckDLSID = (*itExtFile)->GetSubChunk(CHUNK_ID_DLID);
                if (ckDLSID) {
                    ckDLSID->Read(&id.ulData1, 1, 4);
                    ckDLSID->Read(&id.usData2, 1, 2);
                    ckDLSID->Read(&id.usData3, 1, 2);
                    ckDLSID->Read(id.abData, 8, 1);
                } else {
                    ckDLSID = (*itExtFile)->AddSubChunk(CHUNK_ID_DLID, 16);
                    Resource::GenerateDLSID(&id);
                    uint8_t* pData = (uint8_t*)ckDLSID->LoadChunkData();
                    store32(&pData[0], id.ulData1);
                    store16(&pData[4], id.usData2);
                    store16(&pData[6], id.usData3);
                    memcpy(&pData[8], id.abData, 8);
                }
                store32(&pId[0], id.ulData1);
                store16(&pId[4], id.usData2);
                store16(&pId[6], id.usData3);
                memcpy(&pId[8], id.abData, 8);
            }
        } else {
            // in case there was a 'xfil' chunk, remove it
            RIFF::Chunk* ckXfil = pRIFF->GetSubChunk(CHUNK_ID_XFIL);
            if (ckXfil) pRIFF->DeleteSubChunk(ckXfil);
        }

        // update the 'doxf' chunk which describes a .gx99 extension file
        // which contains convolution data (GigaPulse)
        if (pGigaPulseFile) {
            RIFF::Chunk* ckDoxf = pRIFF->GetSubChunk(CHUNK_ID_DOXF);
            if (!ckDoxf) ckDoxf = pRIFF->AddSubChunk(CHUNK_ID_DOXF, 148);

            uint8_t* pData = (uint8_t*) ckDoxf->LoadChunkData();

            // update the dlsid from the extension file
            uint8_t* pId = &pData[132];
            RIFF::Chunk* ckDLSID = pGigaPulseFile->GetSubChunk(CHUNK_ID_DLID);
            if (!ckDLSID) { //TODO: auto generate DLS ID if missing
                throw Exception("Fatal error, GigaPulse file does not contain a DLS ID chunk");
            } else {
                dlsid_t id;
                // read DLS ID from extension files's DLS ID chunk
                uint8_t* pData = (uint8_t*) ckDLSID->LoadChunkData();
                id.ulData1 = load32(&pData[0]);
                id.usData2 = load16(&pData[4]);
                id.usData3 = load16(&pData[6]);
                memcpy(id.abData, &pData[8], 8);
                // store DLS ID to 'doxf' chunk
                store32(&pId[0], id.ulData1);
                store16(&pId[4], id.usData2);
                store16(&pId[6], id.usData3);
                memcpy(&pId[8], id.abData, 8);
            }
        } else {
            // in case there was a 'doxf' chunk, remove it
            RIFF::Chunk* ckDoxf = pRIFF->GetSubChunk(CHUNK_ID_DOXF);
            if (ckDoxf) pRIFF->DeleteSubChunk(ckDoxf);
        }

        // the RIFF file to be written might now been grown >= 4GB or might
        // been shrunk < 4GB, so we might need to update the wave pool offset
        // size and thus accordingly we would need to resize the wave pool
        // chunk
        const file_offset_t finalFileSize = pRIFF->GetRequiredFileSize();
        const bool bRequires64Bit = (finalFileSize >> 32) != 0 || // < native 64 bit gig file
                                     poolFiles.size() > 0;        // < 32 bit gig file where the hi 32 bits are used as extension file nr
        if (b64BitWavePoolOffsets != bRequires64Bit) {
            b64BitWavePoolOffsets = bRequires64Bit;
            iPtblOffsetSize = (b64BitWavePoolOffsets) ? 8 : 4;
            iPtblSize = WavePoolHeaderSize + iPtblOffsetSize * iSamples;
            ptbl->Resize(iPtblSize);
        }

        if (pProgress)
            __notify_progress(pProgress, 1.0); // notify done
    }

    /** @brief Save changes to another file.
     *
     * Make all changes persistent by writing them to another file.
     * <b>Caution:</b> this method is optimized for writing to
     * <b>another</b> file, do not use it to save the changes to the same
     * file! Use Save() (without path argument) in that case instead!
     * Ignoring this might result in a corrupted file!
     *
     * After calling this method, this File object will be associated with
     * the new file (given by \a Path) afterwards.
     *
     * @param Path - path and file name where everything should be written to
     * @param pProgress - optional: callback function for progress notification
     */
    void File::Save(const String& Path, progress_t* pProgress) {
        // calculate number of tasks to notify progress appropriately
        const size_t nExtFiles = ExtensionFiles.size();
        const float tasks = 2.f + nExtFiles;

        // save extension files (if required)
        if (!ExtensionFiles.empty()) {
            // for assembling path of extension files to be saved to
            const std::string baseName = pathWithoutExtension(Path);
            // save the individual extension files
            std::list<RIFF::File*>::iterator it = ExtensionFiles.begin();
            for (int i = 0; it != ExtensionFiles.end(); ++i, ++it) {
                //FIXME: the .gx99 file is always used by GSt for convolution
                // data (GigaPulse); so we should better detect by subchunk
                // whether the extension file is intended for convolution
                // instead of checkking for a file name, because the latter does
                // not work for saving new gigs created from scratch
                const std::string oldName = (*it)->GetFileName();
                const bool isGigaPulseFile = (extensionOfPath(oldName) == "gx99");
                std::string ext = (isGigaPulseFile) ? ".gx99" : strPrint(".gx%02d", i+1);
                std::string newPath = baseName + ext;
                // save extension file to its new location
                if (pProgress) {
                     // divide local progress into subprogress
                    progress_t subprogress;
                    __divide_progress(pProgress, &subprogress, tasks, 0.f + i); // subdivided into amount of extension files
                    // do the actual work
                    (*it)->Save(newPath, &subprogress);
                } else
                    (*it)->Save(newPath);
            }
        }

        if (pProgress) {
            // divide local progress into subprogress
            progress_t subprogress;
            __divide_progress(pProgress, &subprogress, tasks, 1.f + nExtFiles); // arbitrarily subdivided into 50% (minus extension files progress)
            // do the actual work
            UpdateChunks(&subprogress);
        } else
            UpdateChunks(NULL);

        if (pProgress) {
            // divide local progress into subprogress
            progress_t subprogress;
            __divide_progress(pProgress, &subprogress, tasks, 2.f + nExtFiles); // arbitrarily subdivided into 50% (minus extension files progress)
            // do the actual work
            pRIFF->Save(Path, &subprogress);
        } else
            pRIFF->Save(Path);

        UpdateFileOffsets();

        if (pProgress)
            __notify_progress(pProgress, 1.0); // notify done
    }

    /** @brief Save changes to same file.
     *
     * Make all changes persistent by writing them to the actual (same)
     * file. The file might temporarily grow to a higher size than it will
     * have at the end of the saving process.
     *
     * @param pProgress - optional: callback function for progress notification
     * @throws RIFF::Exception if any kind of IO error occurred
     * @throws DLS::Exception  if any kind of DLS specific error occurred
     */
    void File::Save(progress_t* pProgress) {
        // calculate number of tasks to notify progress appropriately
        const size_t nExtFiles = ExtensionFiles.size();
        const float tasks = 2.f + nExtFiles;

        // save extension files (if required)
        if (!ExtensionFiles.empty()) {
            std::list<RIFF::File*>::iterator it = ExtensionFiles.begin();
            for (int i = 0; it != ExtensionFiles.end(); ++i, ++it) {
                // save extension file
                if (pProgress) {
                    // divide local progress into subprogress
                    progress_t subprogress;
                    __divide_progress(pProgress, &subprogress, tasks, 0.f + i); // subdivided into amount of extension files
                    // do the actual work
                    (*it)->Save(&subprogress);
                } else
                    (*it)->Save();
            }
        }

        if (pProgress) {
            // divide local progress into subprogress
            progress_t subprogress;
            __divide_progress(pProgress, &subprogress, tasks, 1.f + nExtFiles); // arbitrarily subdivided into 50% (minus extension files progress)
            // do the actual work
            UpdateChunks(&subprogress);
        } else
            UpdateChunks(NULL);

        if (pProgress) {
            // divide local progress into subprogress
            progress_t subprogress;
            __divide_progress(pProgress, &subprogress, tasks, 2.f + nExtFiles); // arbitrarily subdivided into 50% (minus extension files progress)
            // do the actual work
            pRIFF->Save(&subprogress);
        } else
            pRIFF->Save();

        UpdateFileOffsets();

        if (pProgress)
            __notify_progress(pProgress, 1.0); // notify done
    }

    /** @brief Updates all file offsets stored all over the file.
     *
     * This virtual method is called whenever the overall file layout has been
     * changed (i.e. file or individual RIFF chunks have been resized). It is
     * then the responsibility of this method to update all file offsets stored
     * in the file format. For example samples are referenced by instruments by
     * file offsets. The gig format also stores references to instrument
     * scripts as file offsets, and thus it overrides this method to update
     * those file offsets as well.
     */
    void File::UpdateFileOffsets() {
        __UpdateWavePoolTableChunk();
    }

    /**
     * Checks if all (for DLS) mandatory chunks exist, if not they will be
     * created. Note that those chunks will not be made persistent until
     * Save() was called.
     */
    void File::__ensureMandatoryChunksExist() {
       // enusre 'lins' list chunk exists (mandatory for instrument definitions)
       RIFF::List* lstInstruments = pRIFF->GetSubList(LIST_TYPE_LINS);
       if (!lstInstruments) pRIFF->AddSubList(LIST_TYPE_LINS);
       // ensure 'ptbl' chunk exists (mandatory for samples)
       RIFF::Chunk* ptbl = pRIFF->GetSubChunk(CHUNK_ID_PTBL);
       if (!ptbl) {
           const int iOffsetSize = (b64BitWavePoolOffsets) ? 8 : 4;
           ptbl = pRIFF->AddSubChunk(CHUNK_ID_PTBL, WavePoolHeaderSize + iOffsetSize);
       }
       // enusre 'wvpl' list chunk exists (mandatory for samples)
       RIFF::List* wvpl = pRIFF->GetSubList(LIST_TYPE_WVPL);
       if (!wvpl) pRIFF->AddSubList(LIST_TYPE_WVPL);
    }

    /**
     * Updates (persistently) the wave pool table with offsets to all
     * currently available samples. <b>Caution:</b> this method assumes the
     * 'ptbl' chunk to be already of the correct size and the file to be
     * writable, so usually this method is only called after a Save() call.
     *
     * @throws Exception - if 'ptbl' chunk is too small (should only occur
     *                     if there's a bug)
     */
    void File::__UpdateWavePoolTableChunk() {
        __UpdateWavePoolTable();
        RIFF::Chunk* ptbl = pRIFF->GetSubChunk(CHUNK_ID_PTBL);
        const int iOffsetSize = (b64BitWavePoolOffsets) ? 8 : 4;
        // check if 'ptbl' chunk is large enough
        WavePoolCount = (pSamples) ? uint32_t(pSamples->size()) : 0;
        const file_offset_t ulRequiredSize = WavePoolHeaderSize + iOffsetSize * WavePoolCount;
        if (ptbl->GetSize() < ulRequiredSize) throw Exception("Fatal error, 'ptbl' chunk too small");
        // save the 'ptbl' chunk's current read/write position
        file_offset_t ullOriginalPos = ptbl->GetPos();
        // update headers
        ptbl->SetPos(0);
        uint32_t tmp = WavePoolHeaderSize;
        ptbl->WriteUint32(&tmp);
        tmp = WavePoolCount;
        ptbl->WriteUint32(&tmp);
        // update offsets
        ptbl->SetPos(WavePoolHeaderSize);
        if (b64BitWavePoolOffsets) {
            for (int i = 0 ; i < WavePoolCount ; i++) {
                tmp = pWavePoolTableHi[i];
                ptbl->WriteUint32(&tmp);
                tmp = pWavePoolTable[i];
                ptbl->WriteUint32(&tmp);
            }
        } else { // conventional 32 bit offsets
            for (int i = 0 ; i < WavePoolCount ; i++) {
                tmp = pWavePoolTable[i];
                ptbl->WriteUint32(&tmp);
            }
        }
        // restore 'ptbl' chunk's original read/write position
        ptbl->SetPos(ullOriginalPos);
    }

    /**
     * Updates the wave pool table with offsets to all currently available
     * samples. <b>Caution:</b> this method assumes the 'wvpl' list chunk
     * exists already.
     */
    void File::__UpdateWavePoolTable() {
        WavePoolCount = (pSamples) ? uint32_t(pSamples->size()) : 0;
        // resize wave pool table arrays
        if (pWavePoolTable)   delete[] pWavePoolTable;
        if (pWavePoolTableHi) delete[] pWavePoolTableHi;
        pWavePoolTable   = new uint32_t[WavePoolCount];
        pWavePoolTableHi = new uint32_t[WavePoolCount];
        if (!pSamples) return;
        // update offsets in wave pool table
        RIFF::List* wvpl = pRIFF->GetSubList(LIST_TYPE_WVPL);
        uint64_t wvplFileOffset = wvpl->GetFilePos() -
                                  wvpl->GetPos(); // mandatory, since position might have changed
        if (!b64BitWavePoolOffsets) { // conventional 32 bit offsets (and no extension files) ...
            SampleList::iterator iter = pSamples->begin();
            SampleList::iterator end  = pSamples->end();
            for (int i = 0 ; iter != end ; ++iter, i++) {
                uint64_t _64BitOffset =
                    (*iter)->pWaveList->GetFilePos() -
                    (*iter)->pWaveList->GetPos() - // should be zero, but just to be sure
                    wvplFileOffset -
                    LIST_HEADER_SIZE(pRIFF->GetFileOffsetSize());
                (*iter)->ullWavePoolOffset = _64BitOffset;
                pWavePoolTable[i] = (uint32_t) _64BitOffset;
            }
        } else { // a) native 64 bit offsets without extension files or b) 32 bit offsets with extension files ...
            if (ExtensionFiles.empty()) { // native 64 bit offsets (and no extension files) [not compatible with GigaStudio] ...
                SampleList::iterator iter = pSamples->begin();
                SampleList::iterator end  = pSamples->end();
                for (int i = 0 ; iter != end ; ++iter, i++) {
                    uint64_t _64BitOffset =
                        (*iter)->pWaveList->GetFilePos() -
                        (*iter)->pWaveList->GetPos() - // should be zero, but just to be sure
                        wvplFileOffset -
                        LIST_HEADER_SIZE(pRIFF->GetFileOffsetSize());
                    (*iter)->ullWavePoolOffset = _64BitOffset;
                    pWavePoolTableHi[i] = (uint32_t) (_64BitOffset >> 32);
                    pWavePoolTable[i]   = (uint32_t) _64BitOffset;
                }
            } else { // 32 bit offsets with extension files (GigaStudio legacy support) ...
                // the main gig and the extension files may contain wave data
                std::vector<RIFF::File*> poolFiles;
                poolFiles.push_back(pRIFF);
                poolFiles.insert(poolFiles.end(), ExtensionFiles.begin(), ExtensionFiles.end());

                RIFF::File* pCurPoolFile = NULL;
                int fileNo = 0;
                int waveOffset = 0;
                SampleList::iterator iter = pSamples->begin();
                SampleList::iterator end  = pSamples->end();
                for (int i = 0 ; iter != end ; ++iter, i++) {
                    RIFF::File* pPoolFile = (*iter)->pWaveList->GetFile();
                    // if this sample is located in the same pool file as the
                    // last we reuse the previously computed fileNo and waveOffset
                    if (pPoolFile != pCurPoolFile) { // it is a different pool file than the last sample ...
                        pCurPoolFile = pPoolFile;

                        std::vector<RIFF::File*>::iterator sIter;
                        sIter = std::find(poolFiles.begin(), poolFiles.end(), pPoolFile);
                        if (sIter != poolFiles.end())
                            fileNo = std::distance(poolFiles.begin(), sIter);
                        else
                            throw DLS::Exception("Fatal error, unknown pool file");

                        RIFF::List* extWvpl = pCurPoolFile->GetSubList(LIST_TYPE_WVPL);
                        if (!extWvpl)
                            throw DLS::Exception("Fatal error, pool file has no 'wvpl' list chunk");
                        waveOffset =
                            extWvpl->GetFilePos() -
                            extWvpl->GetPos() + // mandatory, since position might have changed
                            LIST_HEADER_SIZE(pCurPoolFile->GetFileOffsetSize());
                    }
                    uint64_t _64BitOffset =
                        (*iter)->pWaveList->GetFilePos() -
                        (*iter)->pWaveList->GetPos() - // should be zero, but just to be sure
                        waveOffset;
                    // pWavePoolTableHi stores file number when extension files are in use
                    pWavePoolTableHi[i] = (uint32_t) fileNo;
                    pWavePoolTable[i]   = (uint32_t) _64BitOffset;
                    (*iter)->ullWavePoolOffset = _64BitOffset;
                }
            }
        }
    }


// *************** Exception ***************
// *

    Exception::Exception() : RIFF::Exception() {
    }

    Exception::Exception(String format, ...) : RIFF::Exception() {
        va_list arg;
        va_start(arg, format);
        Message = assemble(format, arg);
        va_end(arg);
    }

    Exception::Exception(String format, va_list arg) : RIFF::Exception() {
        Message = assemble(format, arg);
    }

    void Exception::PrintMessage() {
        std::cout << "DLS::Exception: " << Message << std::endl;
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

} // namespace DLS
