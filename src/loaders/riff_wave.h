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

#pragma once

#pragma pack(push, 1)
#include <cstdint>

struct wavheader
{
    int16_t wFormatTag;      /* format type */
    int16_t nChannels;       /* number of channels (i.e. mono, stereo...) */
    int32_t nSamplesPerSec;  /* sample rate */
    int32_t nAvgBytesPerSec; /* for buffer estimation */
    int16_t nBlockAlign;     /* block size of data */
    int16_t wBitsPerSample;  /* Number of bits per sample of mono data */

    /*int16_t    cbSize;*/ /* The count in bytes of the size of
                                                                           extra information (after
                              cbSize) */
};

struct CuePoint
{
    int32_t dwIdentifier;
    int32_t dwPosition;
    int32_t fccChunk;
    int32_t dwChunkStart;
    int32_t dwBlockStart;
    int32_t dwSampleOffset;
};

struct wave_strc_header
{
    int32_t unknown;
    int32_t subchunks;
    int32_t unknown2[5];
};

struct wave_acid_chunk
{
    int32_t type;
    /*
    0x01 On: One Shot         Off: Loop
    0x02 On: Root note is Set Off: No root
    0x04 On: Stretch is On,   Off: Strech is OFF
    0x08 On: Disk Based       Off: Ram based
    0x10 On: ??????????       Off: ????????? (Acidizer puts that ON)
    */
    int16_t rootnote, unknown;
    float unknown2;
    int32_t n_beats;
    int16_t met_denom, met_num;
    float tempo;
};

struct wave_strc_entry
{
    int32_t unknown[2];
    int32_t spos1;
    int32_t unknown2;
    int32_t spos2;
    int32_t unknown3[2];
    float unknown4;
};

struct SampleLoop
{
    int32_t dwIdentifier;
    int32_t dwType;
    int32_t dwStart;
    int32_t dwEnd;
    int32_t dwFraction;
    int32_t dwPlayCount;
};

struct SamplerChunk
{
    int32_t dwManufacturer;
    int32_t dwProduct;
    int32_t dwSamplePeriod;
    int32_t dwMIDIUnityNote;
    int32_t dwMIDIPitchFraction;
    int32_t dwSMPTEFormat;
    int32_t dwSMPTEOffset;
    int32_t cSampleLoops;
    int32_t cbSamplerData;
    // SampleLoop Loops[];
};

struct wave_inst_chunk
{
    char key_root, detune_cents, gain;
    char key_low, key_high;
    char vel_low, vel_high;
};

#pragma pack(pop)

// from MMsystem.h

/* flags for wFormatTag field of WAVEFORMAT */
#define WAVE_FORMAT_PCM 1

// from mmreg.h

/* WAVE form wFormatTag IDs */
#define WAVE_FORMAT_UNKNOWN 0x0000                    /* Microsoft Corporation */
#define WAVE_FORMAT_ADPCM 0x0002                      /* Microsoft Corporation */
#define WAVE_FORMAT_IEEE_FLOAT 0x0003                 /* Microsoft Corporation */
#define WAVE_FORMAT_VSELP 0x0004                      /* Compaq Computer Corp. */
#define WAVE_FORMAT_IBM_CVSD 0x0005                   /* IBM Corporation */
#define WAVE_FORMAT_ALAW 0x0006                       /* Microsoft Corporation */
#define WAVE_FORMAT_MULAW 0x0007                      /* Microsoft Corporation */
#define WAVE_FORMAT_DTS 0x0008                        /* Microsoft Corporation */
#define WAVE_FORMAT_DRM 0x0009                        /* Microsoft Corporation */
#define WAVE_FORMAT_OKI_ADPCM 0x0010                  /* OKI */
#define WAVE_FORMAT_DVI_ADPCM 0x0011                  /* Intel Corporation */
#define WAVE_FORMAT_IMA_ADPCM (WAVE_FORMAT_DVI_ADPCM) /*  Intel Corporation */
#define WAVE_FORMAT_MEDIASPACE_ADPCM 0x0012           /* Videologic */
#define WAVE_FORMAT_SIERRA_ADPCM 0x0013               /* Sierra Semiconductor Corp */
#define WAVE_FORMAT_G723_ADPCM 0x0014                 /* Antex Electronics Corporation */
#define WAVE_FORMAT_DIGISTD 0x0015                    /* DSP Solutions, Inc. */
#define WAVE_FORMAT_DIGIFIX 0x0016                    /* DSP Solutions, Inc. */
#define WAVE_FORMAT_DIALOGIC_OKI_ADPCM 0x0017         /* Dialogic Corporation */
#define WAVE_FORMAT_MEDIAVISION_ADPCM 0x0018          /* Media Vision, Inc. */
#define WAVE_FORMAT_CU_CODEC 0x0019                   /* Hewlett-Packard Company */
#define WAVE_FORMAT_YAMAHA_ADPCM 0x0020               /* Yamaha Corporation of America */
#define WAVE_FORMAT_SONARC 0x0021                     /* Speech Compression */
#define WAVE_FORMAT_DSPGROUP_TRUESPEECH 0x0022        /* DSP Group, Inc */
#define WAVE_FORMAT_ECHOSC1 0x0023                    /* Echo Speech Corporation */
#define WAVE_FORMAT_AUDIOFILE_AF36 0x0024             /* Virtual Music, Inc. */
#define WAVE_FORMAT_APTX 0x0025                       /* Audio Processing Technology */
#define WAVE_FORMAT_AUDIOFILE_AF10 0x0026             /* Virtual Music, Inc. */
#define WAVE_FORMAT_PROSODY_1612 0x0027               /* Aculab plc */
#define WAVE_FORMAT_LRC 0x0028                        /* Merging Technologies S.A. */
#define WAVE_FORMAT_DOLBY_AC2 0x0030                  /* Dolby Laboratories */
#define WAVE_FORMAT_GSM610 0x0031                     /* Microsoft Corporation */
#define WAVE_FORMAT_MSNAUDIO 0x0032                   /* Microsoft Corporation */
#define WAVE_FORMAT_ANTEX_ADPCME 0x0033               /* Antex Electronics Corporation */
#define WAVE_FORMAT_CONTROL_RES_VQLPC 0x0034          /* Control Resources Limited */
#define WAVE_FORMAT_DIGIREAL 0x0035                   /* DSP Solutions, Inc. */
#define WAVE_FORMAT_DIGIADPCM 0x0036                  /* DSP Solutions, Inc. */
#define WAVE_FORMAT_CONTROL_RES_CR10 0x0037           /* Control Resources Limited */
#define WAVE_FORMAT_NMS_VBXADPCM 0x0038               /* Natural MicroSystems */
#define WAVE_FORMAT_CS_IMAADPCM 0x0039                /* Crystal Semiconductor IMA ADPCM */
#define WAVE_FORMAT_ECHOSC3 0x003A                    /* Echo Speech Corporation */
#define WAVE_FORMAT_ROCKWELL_ADPCM 0x003B             /* Rockwell International */
#define WAVE_FORMAT_ROCKWELL_DIGITALK 0x003C          /* Rockwell International */
#define WAVE_FORMAT_XEBEC 0x003D                      /* Xebec Multimedia Solutions Limited */
#define WAVE_FORMAT_G721_ADPCM 0x0040                 /* Antex Electronics Corporation */
#define WAVE_FORMAT_G728_CELP 0x0041                  /* Antex Electronics Corporation */
#define WAVE_FORMAT_MSG723 0x0042                     /* Microsoft Corporation */
#define WAVE_FORMAT_MPEG 0x0050                       /* Microsoft Corporation */
#define WAVE_FORMAT_RT24 0x0052                       /* InSoft, Inc. */
#define WAVE_FORMAT_PAC 0x0053                        /* InSoft, Inc. */
#define WAVE_FORMAT_MPEGLAYER3 0x0055                 /* ISO/MPEG Layer3 Format Tag */
#define WAVE_FORMAT_LUCENT_G723 0x0059                /* Lucent Technologies */
#define WAVE_FORMAT_CIRRUS 0x0060                     /* Cirrus Logic */
#define WAVE_FORMAT_ESPCM 0x0061                      /* ESS Technology */
#define WAVE_FORMAT_VOXWARE 0x0062                    /* Voxware Inc */
#define WAVE_FORMAT_CANOPUS_ATRAC 0x0063              /* Canopus, co., Ltd. */
#define WAVE_FORMAT_G726_ADPCM 0x0064                 /* APICOM */
#define WAVE_FORMAT_G722_ADPCM 0x0065                 /* APICOM */
#define WAVE_FORMAT_DSAT_DISPLAY 0x0067               /* Microsoft Corporation */
#define WAVE_FORMAT_VOXWARE_BYTE_ALIGNED 0x0069       /* Voxware Inc */
#define WAVE_FORMAT_VOXWARE_AC8 0x0070                /* Voxware Inc */
#define WAVE_FORMAT_VOXWARE_AC10 0x0071               /* Voxware Inc */
#define WAVE_FORMAT_VOXWARE_AC16 0x0072               /* Voxware Inc */
#define WAVE_FORMAT_VOXWARE_AC20 0x0073               /* Voxware Inc */
#define WAVE_FORMAT_VOXWARE_RT24 0x0074               /* Voxware Inc */
#define WAVE_FORMAT_VOXWARE_RT29 0x0075               /* Voxware Inc */
#define WAVE_FORMAT_VOXWARE_RT29HW 0x0076             /* Voxware Inc */
#define WAVE_FORMAT_VOXWARE_VR12 0x0077               /* Voxware Inc */
#define WAVE_FORMAT_VOXWARE_VR18 0x0078               /* Voxware Inc */
#define WAVE_FORMAT_VOXWARE_TQ40 0x0079               /* Voxware Inc */
#define WAVE_FORMAT_SOFTSOUND 0x0080                  /* Softsound, Ltd. */
#define WAVE_FORMAT_VOXWARE_TQ60 0x0081               /* Voxware Inc */
#define WAVE_FORMAT_MSRT24 0x0082                     /* Microsoft Corporation */
#define WAVE_FORMAT_G729A 0x0083                      /* AT&T Labs, Inc. */
#define WAVE_FORMAT_MVI_MVI2 0x0084                   /* Motion Pixels */
#define WAVE_FORMAT_DF_G726 0x0085                    /* DataFusion Systems (Pty) (Ltd) */
#define WAVE_FORMAT_DF_GSM610 0x0086                  /* DataFusion Systems (Pty) (Ltd) */
#define WAVE_FORMAT_ISIAUDIO 0x0088                   /* Iterated Systems, Inc. */
#define WAVE_FORMAT_ONLIVE 0x0089                     /* OnLive! Technologies, Inc. */
#define WAVE_FORMAT_SBC24 0x0091                      /* Siemens Business Communications Sys */
#define WAVE_FORMAT_DOLBY_AC3_SPDIF 0x0092            /* Sonic Foundry */
#define WAVE_FORMAT_MEDIASONIC_G723 0x0093            /* MediaSonic */
#define WAVE_FORMAT_PROSODY_8KBPS 0x0094              /* Aculab plc */
#define WAVE_FORMAT_ZYXEL_ADPCM 0x0097                /* ZyXEL Communications, Inc. */
#define WAVE_FORMAT_PHILIPS_LPCBB 0x0098              /* Philips Speech Processing */
#define WAVE_FORMAT_PACKED 0x0099                     /* Studer Professional Audio AG */
#define WAVE_FORMAT_MALDEN_PHONYTALK 0x00A0           /* Malden Electronics Ltd. */
#define WAVE_FORMAT_RHETOREX_ADPCM 0x0100             /* Rhetorex Inc. */
#define WAVE_FORMAT_IRAT 0x0101                       /* BeCubed Software Inc. */
#define WAVE_FORMAT_VIVO_G723 0x0111                  /* Vivo Software */
#define WAVE_FORMAT_VIVO_SIREN 0x0112                 /* Vivo Software */
#define WAVE_FORMAT_DIGITAL_G723 0x0123               /* Digital Equipment Corporation */
#define WAVE_FORMAT_SANYO_LD_ADPCM 0x0125             /* Sanyo Electric Co., Ltd. */
#define WAVE_FORMAT_SIPROLAB_ACEPLNET 0x0130          /* Sipro Lab Telecom Inc. */
#define WAVE_FORMAT_SIPROLAB_ACELP4800 0x0131         /* Sipro Lab Telecom Inc. */
#define WAVE_FORMAT_SIPROLAB_ACELP8V3 0x0132          /* Sipro Lab Telecom Inc. */
#define WAVE_FORMAT_SIPROLAB_G729 0x0133              /* Sipro Lab Telecom Inc. */
#define WAVE_FORMAT_SIPROLAB_G729A 0x0134             /* Sipro Lab Telecom Inc. */
#define WAVE_FORMAT_SIPROLAB_KELVIN 0x0135            /* Sipro Lab Telecom Inc. */
#define WAVE_FORMAT_G726ADPCM 0x0140                  /* Dictaphone Corporation */
#define WAVE_FORMAT_QUALCOMM_PUREVOICE 0x0150         /* Qualcomm, Inc. */
#define WAVE_FORMAT_QUALCOMM_HALFRATE 0x0151          /* Qualcomm, Inc. */
#define WAVE_FORMAT_TUBGSM 0x0155                     /* Ring Zero Systems, Inc. */
#define WAVE_FORMAT_MSAUDIO1 0x0160                   /* Microsoft Corporation */
#define WAVE_FORMAT_UNISYS_NAP_ADPCM 0x0170           /* Unisys Corp. */
#define WAVE_FORMAT_UNISYS_NAP_ULAW 0x0171            /* Unisys Corp. */
#define WAVE_FORMAT_UNISYS_NAP_ALAW 0x0172            /* Unisys Corp. */
#define WAVE_FORMAT_UNISYS_NAP_16K 0x0173             /* Unisys Corp. */
#define WAVE_FORMAT_CREATIVE_ADPCM 0x0200             /* Creative Labs, Inc */
#define WAVE_FORMAT_CREATIVE_FASTSPEECH8 0x0202       /* Creative Labs, Inc */
#define WAVE_FORMAT_CREATIVE_FASTSPEECH10 0x0203      /* Creative Labs, Inc */
#define WAVE_FORMAT_UHER_ADPCM 0x0210                 /* UHER informatic GmbH */
#define WAVE_FORMAT_QUARTERDECK 0x0220                /* Quarterdeck Corporation */
#define WAVE_FORMAT_ILINK_VC 0x0230                   /* I-link Worldwide */
#define WAVE_FORMAT_RAW_SPORT 0x0240                  /* Aureal Semiconductor */
#define WAVE_FORMAT_ESST_AC3 0x0241                   /* ESS Technology, Inc. */
#define WAVE_FORMAT_IPI_HSX 0x0250                    /* Interactive Products, Inc. */
#define WAVE_FORMAT_IPI_RPELP 0x0251                  /* Interactive Products, Inc. */
#define WAVE_FORMAT_CS2 0x0260                        /* Consistent Software */
#define WAVE_FORMAT_SONY_SCX 0x0270                   /* Sony Corp. */
#define WAVE_FORMAT_FM_TOWNS_SND 0x0300               /* Fujitsu Corp. */
#define WAVE_FORMAT_BTV_DIGITAL 0x0400                /* Brooktree Corporation */
#define WAVE_FORMAT_QDESIGN_MUSIC 0x0450              /* QDesign Corporation */
#define WAVE_FORMAT_VME_VMPCM 0x0680                  /* AT&T Labs, Inc. */
#define WAVE_FORMAT_TPC 0x0681                        /* AT&T Labs, Inc. */
#define WAVE_FORMAT_OLIGSM 0x1000                     /* Ing C. Olivetti & C., S.p.A. */
#define WAVE_FORMAT_OLIADPCM 0x1001                   /* Ing C. Olivetti & C., S.p.A. */
#define WAVE_FORMAT_OLICELP 0x1002                    /* Ing C. Olivetti & C., S.p.A. */
#define WAVE_FORMAT_OLISBC 0x1003                     /* Ing C. Olivetti & C., S.p.A. */
#define WAVE_FORMAT_OLIOPR 0x1004                     /* Ing C. Olivetti & C., S.p.A. */
#define WAVE_FORMAT_LH_CODEC 0x1100                   /* Lernout & Hauspie */
#define WAVE_FORMAT_NORRIS 0x1400                     /* Norris Communications, Inc. */
#define WAVE_FORMAT_SOUNDSPACE_MUSICOMPRESS 0x1500    /* AT&T Labs, Inc. */
#define WAVE_FORMAT_DVM 0x2000                        /* FAST Multimedia AG */
