/*

    dls.h

    Description:

    Interface defines and structures for the Instrument Collection Form
    RIFF DLS.

    Written by Sonic Foundry 1996.  Released for public use.

////////////////////////////////////////////////////////////////
// Revision history:
// 1.0 1996 Sonic Foundry
//     Initial revision
// 1.1 16Nov2004 Tom White
//     Deleted commented-out field definition in POOLCUE struct
////////////////////////////////////////////////////////////////


*/

#ifndef _INC_DLS
#define _INC_DLS

#ifndef FAR
#define FAR
#endif

/*

   Layout of an instrument collection:


   RIFF [] 'DLS ' [colh,INSTLIST,WAVEPOOL,INFOLIST]

   INSTLIST
   LIST [] 'lins'
                 LIST [] 'ins ' [insh,RGNLIST,ARTLIST,INFOLIST]
                 LIST [] 'ins ' [insh,RGNLIST,ARTLIST,INFOLIST]
                 LIST [] 'ins ' [insh,RGNLIST,ARTLIST,INFOLIST]

   RGNLIST
   LIST [] 'lrgn'
                 LIST [] 'rgn '  [rgnh,wsmp,wlnk,ARTLIST]
                 LIST [] 'rgn '  [rgnh,wsmp,wlnk,ARTLIST]
                 LIST [] 'rgn '  [rgnh,wsmp,wlnk,ARTLIST]

   ARTLIST
   LIST [] 'lart'
           'art1' level 1 Articulation connection graph
           'art2' level 2 Articulation connection graph
           '3rd1' Possible 3rd party articulation structure 1
           '3rd2' Possible 3rd party articulation structure 2 .... and so on

   WAVEPOOL
   ptbl [] [pool table]
   LIST [] 'wvpl'
                 [path],
                 [path],
                 LIST [] 'wave',RIFFWAVE
                 LIST [] 'wave',RIFFWAVE
                 LIST [] 'wave',RIFFWAVE
                 LIST [] 'wave',RIFFWAVE
                 LIST [] 'wave',RIFFWAVE

   INFOLIST
   LIST [] 'INFO'
                 'icmt' 'One of those crazy comments.'
                 'icop' 'Copyright (C) 1996 Sonic Foundry'

*/

/*
   FOURCC's used in the DLS file
*/

#define FOURCC_DLS mmioFOURCC('D', 'L', 'S', ' ')
#define FOURCC_COLH mmioFOURCC('c', 'o', 'l', 'h')
#define FOURCC_WVPL mmioFOURCC('w', 'v', 'p', 'l')
#define FOURCC_PTBL mmioFOURCC('p', 't', 'b', 'l')
#define FOURCC_PATH mmioFOURCC('p', 'a', 't', 'h')
#define FOURCC_wave mmioFOURCC('w', 'a', 'v', 'e')
#define FOURCC_LINS mmioFOURCC('l', 'i', 'n', 's')
#define FOURCC_INS mmioFOURCC('i', 'n', 's', ' ')
#define FOURCC_INSH mmioFOURCC('i', 'n', 's', 'h')
#define FOURCC_LRGN mmioFOURCC('l', 'r', 'g', 'n')
#define FOURCC_RGN mmioFOURCC('r', 'g', 'n', ' ')
#define FOURCC_RGNH mmioFOURCC('r', 'g', 'n', 'h')
#define FOURCC_LART mmioFOURCC('l', 'a', 'r', 't')
#define FOURCC_ART1 mmioFOURCC('a', 'r', 't', '1')
#define FOURCC_WLNK mmioFOURCC('w', 'l', 'n', 'k')
#define FOURCC_WSMP mmioFOURCC('w', 's', 'm', 'p')
#define FOURCC_VERS mmioFOURCC('v', 'e', 'r', 's')

/*
   Articulation connection graph definitions
*/

/* Generic Sources */
#define CONN_SRC_NONE 0x0000
#define CONN_SRC_LFO 0x0001
#define CONN_SRC_KEYONVELOCITY 0x0002
#define CONN_SRC_KEYNUMBER 0x0003
#define CONN_SRC_EG1 0x0004
#define CONN_SRC_EG2 0x0005
#define CONN_SRC_PITCHWHEEL 0x0006

/* Midi Controllers 0-127 */
#define CONN_SRC_CC1 0x0081
#define CONN_SRC_CC7 0x0087
#define CONN_SRC_CC10 0x008a
#define CONN_SRC_CC11 0x008b

/* Registered Parameter Numbers */
#define CONN_SRC_RPN0 0x0100
#define CONN_SRC_RPN1 0x0101
#define CONN_SRC_RPN2 0x0102

/* Generic Destinations */
#define CONN_DST_NONE 0x0000
#define CONN_DST_ATTENUATION 0x0001
#define CONN_DST_RESERVED 0x0002
#define CONN_DST_PITCH 0x0003
#define CONN_DST_PAN 0x0004

/* LFO Destinations */
#define CONN_DST_LFO_FREQUENCY 0x0104
#define CONN_DST_LFO_STARTDELAY 0x0105

/* EG1 Destinations */
#define CONN_DST_EG1_ATTACKTIME 0x0206
#define CONN_DST_EG1_DECAYTIME 0x0207
#define CONN_DST_EG1_RESERVED 0x0208
#define CONN_DST_EG1_RELEASETIME 0x0209
#define CONN_DST_EG1_SUSTAINLEVEL 0x020a

/* EG2 Destinations */
#define CONN_DST_EG2_ATTACKTIME 0x030a
#define CONN_DST_EG2_DECAYTIME 0x030b
#define CONN_DST_EG2_RESERVED 0x030c
#define CONN_DST_EG2_RELEASETIME 0x030d
#define CONN_DST_EG2_SUSTAINLEVEL 0x030e

#define CONN_TRN_NONE 0x0000
#define CONN_TRN_CONCAVE 0x0001

typedef struct _DLSVERSION
{
    uint32_t dwVersionMS;
    uint32_t dwVersionLS;
} DLSVERSION, FAR *LPDLSVERSION;

typedef struct _CONNECTION
{
    uint16_t usSource;
    uint16_t usControl;
    uint16_t usDestination;
    uint16_t usTransform;
    int64_t lScale;
} CONNECTION, FAR *LPCONNECTION;

/* Level 1 Articulation Data */

typedef struct _CONNECTIONLIST
{
    uint64_t cbSize;       /* size of the connection list structure */
    uint64_t cConnections; /* count of connections in the list */
} CONNECTIONLIST, FAR *LPCONNECTIONLIST;

/*
   Generic type defines for regions and instruments
*/

typedef struct _RGNRANGE
{
    uint16_t usLow;
    uint16_t usHigh;
} RGNRANGE, FAR *LPRGNRANGE;

#define F_INSTRUMENT_DRUMS 0x80000000

typedef struct _MIDILOCALE
{
    uint64_t ulBank;
    uint64_t ulInstrument;
} MIDILOCALE, FAR *LPMIDILOCALE;

/*
   Header structures found in an DLS file for collection, instruments, and
   regions.
*/

#define F_RGN_OPTION_SELFNONEXCLUSIVE 0x0001

typedef struct _RGNHEADER
{
    RGNRANGE RangeKey;      /* Key range */
    RGNRANGE RangeVelocity; /* Velocity Range */
    uint16_t fusOptions;    /* Synthesis options for this range */
    uint16_t usKeyGroup;    /* Key grouping for non simultaneous play
                             0 = no group, 1 up is group
                             for Level 1 only groups 1-15 are allowed */
} RGNHEADER, FAR *LPRGNHEADER;

typedef struct _INSTHEADER
{
    uint64_t cRegions; /* Count of regions in this instrument */
    MIDILOCALE Locale; /* Intended MIDI locale of this instrument */
} INSTHEADER, FAR *LPINSTHEADER;

typedef struct _DLSHEADER
{
    uint64_t cInstruments; /* Count of instruments in the collection */
} DLSHEADER, FAR *LPDLSHEADER;

/*
   definitions for the Wave link structure
*/

/*****  For level 1 only WAVELINK_CHANNEL_MONO is valid  ****
  ulChannel allows for up to 32 channels of audio with each bit position
  specifiying a channel of playback */

#define WAVELINK_CHANNEL_LEFT 0x0001l
#define WAVELINK_CHANNEL_RIGHT 0x0002l

#define F_WAVELINK_PHASE_MASTER 0x0001

typedef struct _WAVELINK
{                          /* any paths or links are stored right after struct */
    uint16_t fusOptions;   /* options flags for this wave */
    uint16_t usPhaseGroup; /* Phase grouping for locking channels */
    uint64_t ulChannel;    /* channel placement */
    uint64_t ulTableIndex; /* index into the wave pool table, 0 based */
} WAVELINK, FAR *LPWAVELINK;

#define POOL_CUE_NULL 0xffffffffl

typedef struct _POOLCUE
{
    uint64_t ulOffset; /* Offset to the entry in the list */
} POOLCUE, FAR *LPPOOLCUE;

typedef struct _POOLTABLE
{
    uint64_t cbSize; /* size of the pool table structure */
    uint64_t cCues;  /* count of cues in the list */
} POOLTABLE, FAR *LPPOOLTABLE;

/*
   Structures for the "wsmp" chunk
*/

#define F_WSMP_NO_TRUNCATION 0x0001l
#define F_WSMP_NO_COMPRESSION 0x0002l

typedef struct _rwsmp
{
    uint64_t cbSize;
    uint16_t usUnityNote;  /* MIDI Unity Playback Note */
    int16_t sFineTune;     /* Fine Tune in log tuning */
    int64_t lAttenuation;  /* Overall Attenuation to be applied to data */
    uint64_t fulOptions;   /* Flag options */
    uint64_t cSampleLoops; /* Count of Sample loops, 0 loops is one shot */
} WSMPL, FAR *LPWSMPL;

/* This loop type is a normal forward playing loop which is continually
   played until the envelope reaches an off threshold in the release
   portion of the volume envelope */

#define WLOOP_TYPE_FORWARD 0

typedef struct _rloop
{
    uint64_t cbSize;
    uint64_t ulType;   /* Loop Type */
    uint64_t ulStart;  /* Start of loop in samples */
    uint64_t ulLength; /* Length of loop in samples */
} WLOOP, FAR *LPWLOOP;

#endif /* _INC_DLS */
