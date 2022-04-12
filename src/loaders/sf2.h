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

#if !WINDOWS
#include "windows_compat.h"
#endif

typedef enum
{
    monoSample = 1,
    rightSample = 2,
    leftSample = 4,
    linkedSample = 8,
    RomMonoSample = 0x8001,
    RomRightSample = 0x8002,
    RomLeftSample = 0x8004,
    RomLinkedSample = 0x8008
} SFSampleLink;

struct sf2_Sample
{
    CHAR achSampleName[20];
    DWORD dwStart;
    DWORD dwEnd;
    DWORD dwStartloop;
    DWORD dwEndloop;
    DWORD dwSampleRate;
    BYTE byOriginalKey;
    CHAR chCorrection;
    WORD wSampleLink;
    WORD sfSampleType;
};

struct sf2_PresetHeader
{
    CHAR achPresetName[20];
    WORD wPreset;
    WORD wBank;
    WORD wPresetBagNdx;
    DWORD dwLibrary;
    DWORD dwGenre;
    DWORD dwMorphology;
};

struct sf2_PresetBag
{
    WORD wGenNdx;
    WORD wModNdx;
};

typedef struct
{
    BYTE byLo;
    BYTE byHi;
} rangesType;

typedef union
{
    rangesType ranges;
    SHORT shAmount;
    WORD wAmount;
} genAmountType;

struct sf2_PresetGenList
{
    WORD sfGenOper;
    genAmountType genAmount;
};

struct sf2_InstHeader
{
    CHAR achInstName[20];
    WORD wInstBagNdx;
};

struct sf2_InstBag
{
    WORD wInstGenNdx;
    WORD wInstModNdx;
};

struct sf2_InstGenList
{
    WORD sfGenOper;
    genAmountType genAmount;
};

enum sf2_generators
{
    startAddrsOffset = 0,
    endAddrsOffset,
    startloopAddrsOffset,
    endloopAddrsOffset,
    startAddrsCoarseOffset,
    modLfoToPitch,
    vibLfoToPitch,
    modEnvToPitch,
    initialFilterFc,
    initialFilterQ,
    modLfoToFilterFc,
    modEnvToFilterFc,
    endAddrsCoarseOffset,
    modLfoToVolume,
    chorusEffectsSend = 15,
    reverbEffectsSend,
    pan,
    delayModLFO = 21,
    freqModLFO,
    delayVibLFO,
    freqVibLFO,
    delayModEnv,
    attackModEnv,
    holdModEnv,
    decayModEnv,
    sustainModEnv,
    releaseModEnv,
    keynumToModEnvHold,
    keynumToModEnvDecay,
    delayVolEnv,
    attackVolEnv,
    holdVolEnv,
    decayVolEnv,
    sustainVolEnv,
    releaseVolEnv,
    keynumToVolEnvHold,
    keynumToVolEnvDecay,
    instrument,
    keyRange = 43,
    velRange,
    startloopAddrsCoarseOffset,
    keynum,
    velocity,
    initialAttenuation,
    endloopAddrsCoarseOffset = 50,
    coarseTune,
    fineTune,
    sampleID,
    sampleModes,
    scaleTuning = 56,
    exclusiveClass,
    overridingRootKey,
    endOper = 60,
    sf2_numgenerators,
};

#pragma pack(pop)
