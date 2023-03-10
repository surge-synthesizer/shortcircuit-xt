/***************************************************************************
 *                                                                         *
 *   libgig - C++ cross-platform Gigasampler format file access library    *
 *                                                                         *
 *   Copyright (C) 2003-2019 by Christian Schoenebeck                      *
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

#ifndef __RIFF_H__
#define __RIFF_H__

#ifdef WIN32
# define POSIX 0
#endif

#ifndef POSIX
# define POSIX 1
#endif

#ifndef DEBUG
# define DEBUG 0
#endif

#ifndef OVERRIDE
# if defined(__cplusplus) && __cplusplus >= 201103L
#  define OVERRIDE override
# else
#  define OVERRIDE
# endif
#endif

#include <string>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <stdarg.h>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if POSIX
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <unistd.h>
#endif // POSIX

#if defined _MSC_VER && _MSC_VER < 1600
// Visual C++ 2008 doesn't have stdint.h
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#ifdef WIN32
# if (_WIN32 && !_WIN64) || (__GNUC__ && !(__x86_64__ || __ppc64__)) /* if 32 bit windows compilation */
#  if _WIN32_WINNT < 0x0501
#   undef _WIN32_WINNT
#   define _WIN32_WINNT 0x0501 /* Win XP (no service pack): required for 32 bit compilation for GetFileSizeEx() to be declared by windows.h */
#  endif
# endif
# include <windows.h>
  typedef unsigned int   uint;
#endif // WIN32

#include <stdio.h>

#if WORDS_BIGENDIAN
# define CHUNK_ID_RIFF	0x52494646
# define CHUNK_ID_RIFX	0x52494658
# define CHUNK_ID_LIST	0x4C495354

# define LIST_TYPE_INFO	0x494E464F
# define CHUNK_ID_ICMT	0x49434D54
# define CHUNK_ID_ICOP	0x49434F50
# define CHUNK_ID_ICRD	0x49435244
# define CHUNK_ID_IENG	0x49454E47
# define CHUNK_ID_INAM	0x494E414D
# define CHUNK_ID_IPRD	0x49505244
# define CHUNK_ID_ISFT	0x49534654

# define CHUNK_ID_SMPL	0x736D706C

#else  // little endian
# define CHUNK_ID_RIFF	0x46464952
# define CHUNK_ID_RIFX	0x58464952
# define CHUNK_ID_LIST	0x5453494C

# define LIST_TYPE_INFO	0x4F464E49
# define CHUNK_ID_ICMT	0x544D4349
# define CHUNK_ID_ICOP	0x504F4349
# define CHUNK_ID_ICRD	0x44524349
# define CHUNK_ID_IENG	0x474E4549
# define CHUNK_ID_INAM	0x4D414E49
# define CHUNK_ID_IPRD	0x44525049
# define CHUNK_ID_ISFT	0x54465349

# define CHUNK_ID_SMPL	0x6C706D73

#endif // WORDS_BIGENDIAN

#define CHUNK_HEADER_SIZE(fileOffsetSize)   (4 + fileOffsetSize)
#define LIST_HEADER_SIZE(fileOffsetSize)    (8 + fileOffsetSize)
#define RIFF_HEADER_SIZE(fileOffsetSize)    (8 + fileOffsetSize)

/**
 * @brief RIFF specific classes and definitions
 *
 * The Resource Interchange File Format (RIFF) is a generic tree-structured
 * meta-format which stores data in so called "chunks". It can be compared
 * to XML, but in contrast to XML, RIFF is entirely binary encoded, that is
 * not ASCII based. RIFF is used as basis for many file formats like AVI,
 * WAV, DLS and of course the Gigasampler file format. ;-)
 *
 * RIFF chunks can be seen as containers for data. There are two distinct
 * types of chunks:
 *
 * - @e ordinary @e chunks are the leafs of the data tree which encapsulate
 *   the actual data of the file (i.e. the sample data of a .wav file)
 *
 * - @e list @e chunks are the nodes of the data tree which hold an
 *   arbitrary amount of subchunks (can be both, list chunks and/or ordinary
 *   chunks)
 */
namespace RIFF {

    /* just symbol prototyping */
    class Chunk;
    class List;
    class File;

    typedef std::string String;

    /** Type used by libgig for handling file positioning during file I/O tasks. */
    typedef uint64_t file_offset_t;

    /** Whether file stream is open in read or in read/write mode. */
    enum stream_mode_t {
        stream_mode_read       = 0,
        stream_mode_read_write = 1,
        stream_mode_closed     = 2
    };

    /** Current state of the file stream. */
    enum stream_state_t {
        stream_ready       = 0,
        stream_end_reached = 1,
        stream_closed      = 2
    };

    /** File stream position dependent to these relations. */
    enum stream_whence_t {
        stream_start    = 0,
        stream_curpos   = 1,
        stream_backward = 2,
        stream_end      = 3
    };

    /** Alignment of data bytes in memory (system dependant). */
    enum endian_t {
        endian_little = 0,
        endian_big    = 1,
        endian_native = 2
    };

    /** General RIFF chunk structure of a RIFF file. */
    enum layout_t {
        layout_standard = 0, ///< Standard RIFF file layout: First chunk in file is a List chunk which contains all other chunks and there are no chunks outside the scope of that very first (List) chunk.
        layout_flat     = 1  ///< Not a "real" RIFF file: First chunk in file is an ordinary data chunk, not a List chunk, and there might be other chunks after that first chunk.
    };

    /** Size of RIFF file offsets used in all RIFF chunks' headers. @see File::GetFileOffsetSize() */
    enum offset_size_t {
        offset_size_auto  = 0, ///< Use 32 bit offsets for files smaller than 4 GB, use 64 bit offsets for files equal or larger than 4 GB.
        offset_size_32bit = 4, ///< Always use 32 bit offsets (even for files larger than 4 GB).
        offset_size_64bit = 8  ///< Always use 64 bit offsets (even for files smaller than 4 GB).
    };

    /**
     * @brief Used for indicating the progress of a certain task.
     *
     * The function pointer argument has to be supplied with a valid
     * function of the given signature which will then be called on
     * progress changes. An equivalent progress_t structure will be passed
     * back as argument to the callback function on each progress change.
     * The factor field of the supplied progress_t structure will then
     * reflect the current progress as value between 0.0 and 1.0. You might
     * want to use the custom field for data needed in your callback
     * function.
     */
    struct progress_t {
        void (*callback)(progress_t*); ///< Callback function pointer which has to be assigned to a function for progress notification.
        float factor;                  ///< Reflects current progress as value between 0.0 and 1.0.
        void* custom;                  ///< This pointer can be used for arbitrary data.
        float __range_min;             ///< Only for internal usage, do not modify!
        float __range_max;             ///< Only for internal usage, do not modify!
        progress_t();
        std::vector<progress_t> subdivide(int iSubtasks);
        std::vector<progress_t> subdivide(std::vector<float> vSubTaskPortions);
    };

    /** @brief Ordinary RIFF Chunk
     *
     * Provides convenient methods to access data of ordinary RIFF chunks
     * in general.
     */
    class Chunk {
        public:
            Chunk(File* pFile, file_offset_t StartPos, List* Parent);
            String         GetChunkIDString() const;
            uint32_t       GetChunkID() const { return ChunkID; }       ///< Chunk ID in unsigned integer representation.
            File*          GetFile() const    { return pFile;   }       ///< Returns pointer to the chunk's File object.
            List*          GetParent() const  { return pParent; }       ///< Returns pointer to the chunk's parent list chunk.
            file_offset_t  GetSize() const { return ullCurrentChunkSize; } ///< Chunk size in bytes (without header, thus the chunk data body)
            file_offset_t  GetNewSize() const { return ullNewChunkSize; } ///< New chunk size if it was modified with Resize(), otherwise value returned will be equal to GetSize().
            file_offset_t  GetPos() const { return ullPos; }            ///< Position within the chunk data body (starting with 0).
            file_offset_t  GetFilePos() const { return ullStartPos + ullPos; } ///< Current, actual offset in file of current chunk data body read/write position.
            file_offset_t  SetPos(file_offset_t Where, stream_whence_t Whence = stream_start);
            file_offset_t  RemainingBytes() const;
            stream_state_t GetState() const;
            file_offset_t  Read(void* pData, file_offset_t WordCount, file_offset_t WordSize);
            file_offset_t  ReadInt8(int8_t* pData,     file_offset_t WordCount = 1);
            file_offset_t  ReadUint8(uint8_t* pData,   file_offset_t WordCount = 1);
            file_offset_t  ReadInt16(int16_t* pData,   file_offset_t WordCount = 1);
            file_offset_t  ReadUint16(uint16_t* pData, file_offset_t WordCount = 1);
            file_offset_t  ReadInt32(int32_t* pData,   file_offset_t WordCount = 1);
            file_offset_t  ReadUint32(uint32_t* pData, file_offset_t WordCount = 1);
            int8_t         ReadInt8();
            uint8_t        ReadUint8();
            int16_t        ReadInt16();
            uint16_t       ReadUint16();
            int32_t        ReadInt32();
            uint32_t       ReadUint32();
            void           ReadString(String& s, int size);
            file_offset_t  Write(void* pData, file_offset_t WordCount, file_offset_t WordSize);
            file_offset_t  WriteInt8(int8_t* pData,     file_offset_t WordCount = 1);
            file_offset_t  WriteUint8(uint8_t* pData,   file_offset_t WordCount = 1);
            file_offset_t  WriteInt16(int16_t* pData,   file_offset_t WordCount = 1);
            file_offset_t  WriteUint16(uint16_t* pData, file_offset_t WordCount = 1);
            file_offset_t  WriteInt32(int32_t* pData,   file_offset_t WordCount = 1);
            file_offset_t  WriteUint32(uint32_t* pData, file_offset_t WordCount = 1);
            void*          LoadChunkData();
            void           ReleaseChunkData();
            void           Resize(file_offset_t NewSize);
            virtual ~Chunk();
        protected:
            uint32_t      ChunkID;
            file_offset_t ullCurrentChunkSize;		/* in bytes */
            file_offset_t ullNewChunkSize;			/* in bytes (if chunk was scheduled to be resized) */
            List*         pParent;
            File*         pFile;
            file_offset_t ullStartPos;  /* actual position in file where chunk data (without header) starts */
            file_offset_t ullPos;       /* # of bytes from ulStartPos */
            uint8_t*      pChunkData;
            file_offset_t ullChunkDataSize;

            Chunk(File* pFile);
            Chunk(File* pFile, List* pParent, uint32_t uiChunkID, file_offset_t ullBodySize);
            void ReadHeader(file_offset_t filePos);
            void WriteHeader(file_offset_t filePos);
            file_offset_t ReadSceptical(void* pData, file_offset_t WordCount, file_offset_t WordSize);
            inline static String convertToString(uint32_t word) {
                String result;
                for (int i = 0; i < 4; i++) {
                    uint8_t byte = *((uint8_t*)(&word) + i);
                    char c = byte;
                    result += c;
                }
                return result;
            }
            virtual file_offset_t RequiredPhysicalSize(int fileOffsetSize);
            virtual file_offset_t WriteChunk(file_offset_t ullWritePos, file_offset_t ullCurrentDataOffset, progress_t* pProgress = NULL);
            virtual void __resetPos(); ///< Sets Chunk's read/write position to zero.

            friend class List;
    };

    /** @brief RIFF List Chunk
     *
     * Provides convenient methods to access data of RIFF list chunks and
     * their subchunks.
     */
    class List : public Chunk {
        public:
            List(File* pFile, file_offset_t StartPos, List* Parent);
            String       GetListTypeString() const;
            uint32_t     GetListType() const { return ListType; } ///< Returns unsigned integer representation of the list's ID
            Chunk*       GetSubChunk(uint32_t ChunkID);
            List*        GetSubList(uint32_t ListType);
            Chunk*       GetFirstSubChunk();
            Chunk*       GetNextSubChunk();
            List*        GetFirstSubList();
            List*        GetNextSubList();
            size_t       CountSubChunks();
            size_t       CountSubChunks(uint32_t ChunkID);
            size_t       CountSubLists();
            size_t       CountSubLists(uint32_t ListType);
            Chunk*       AddSubChunk(uint32_t uiChunkID, file_offset_t ullBodySize);
            List*        AddSubList(uint32_t uiListType);
            void         DeleteSubChunk(Chunk* pSubChunk);
            void         MoveSubChunk(Chunk* pSrc, Chunk* pDst); // read API doc comments !!!
            void         MoveSubChunk(Chunk* pSrc, List* pNewParent);
            virtual ~List();
        protected:
            typedef std::map<uint32_t, RIFF::Chunk*>  ChunkMap;
            typedef std::list<Chunk*>                 ChunkList;
            typedef std::set<Chunk*>                  ChunkSet;

            uint32_t   ListType;
            ChunkList* pSubChunks;
            ChunkMap*  pSubChunksMap;
            ChunkList::iterator ChunksIterator;
            ChunkList::iterator ListIterator;

            List(File* pFile);
            List(File* pFile, List* pParent, uint32_t uiListID);
            void ReadHeader(file_offset_t filePos);
            void WriteHeader(file_offset_t filePos);
            void LoadSubChunks(progress_t* pProgress = NULL);
            void LoadSubChunksRecursively(progress_t* pProgress = NULL);
            virtual file_offset_t RequiredPhysicalSize(int fileOffsetSize);
            virtual file_offset_t WriteChunk(file_offset_t ullWritePos, file_offset_t ullCurrentDataOffset, progress_t* pProgress = NULL);
            virtual void __resetPos(); ///< Sets List Chunk's read/write position to zero and causes all sub chunks to do the same.
            void DeleteChunkList();
    };

    /** @brief RIFF File
     *
     * Handles arbitrary RIFF files and provides together with its base
     * classes convenient methods to walk through, read and modify the
     * file's RIFF tree.
     */
    class File : public List {
        public:
            File(uint32_t FileType);
            File(const String& path);
            File(const String& path, uint32_t FileType, endian_t Endian, layout_t layout, offset_size_t fileOffsetSize = offset_size_auto);
            stream_mode_t GetMode() const;
            bool          SetMode(stream_mode_t NewMode);
            void SetByteOrder(endian_t Endian);
            String GetFileName() const;
            void SetFileName(const String& path);
            bool IsNew() const;
            layout_t GetLayout() const;
            file_offset_t GetCurrentFileSize() const;
            file_offset_t GetRequiredFileSize();
            file_offset_t GetRequiredFileSize(offset_size_t fileOffsetSize);
            int GetFileOffsetSize() const;
            int GetRequiredFileOffsetSize();

            virtual void Save(progress_t* pProgress = NULL);
            virtual void Save(const String& path, progress_t* pProgress = NULL);
            virtual ~File();
        protected:
            #if POSIX
            int    hFileRead;  ///< handle / descriptor for reading from file
            int    hFileWrite; ///< handle / descriptor for writing to (some) file
            #elif defined(WIN32)
            HANDLE hFileRead;  ///< handle / descriptor for reading from file
            HANDLE hFileWrite; ///< handle / descriptor for writing to (some) file
            #else
            FILE*  hFileRead;  ///< handle / descriptor for reading from file
            FILE*  hFileWrite; ///< handle / descriptor for writing to (some) file
            #endif // POSIX
            String Filename;
            bool   bEndianNative;
            bool   bIsNewFile;
            layout_t Layout; ///< An ordinary RIFF file is always set to layout_standard.
            offset_size_t FileOffsetPreference;
            int FileOffsetSize; ///< Size of file offsets (in bytes) when this file was opened (or saved the last time).

            friend class Chunk;
            friend class List;
        private:
            stream_mode_t  Mode;

            void __openExistingFile(const String& path, uint32_t* FileType = NULL);
            void ResizeFile(file_offset_t ullNewSize);
            #if POSIX
            file_offset_t __GetFileSize(int hFile) const;
            #elif defined(WIN32)
            file_offset_t __GetFileSize(HANDLE hFile) const;
            #else
            file_offset_t __GetFileSize(FILE* hFile) const;
            #endif
            int FileOffsetSizeFor(file_offset_t fileSize) const;
            void Cleanup();
    };

    /**
     * Will be thrown whenever an error occurs while handling a RIFF file.
     */
    class Exception {
        public:
            String Message;

            Exception(String format, ...);
            Exception(String format, va_list arg);
            void PrintMessage();
            virtual ~Exception() {}

        protected:
            Exception();
            static String assemble(String format, va_list arg);
    };

    String libraryName();
    String libraryVersion();

} // namespace RIFF
#endif // __RIFF_H__
