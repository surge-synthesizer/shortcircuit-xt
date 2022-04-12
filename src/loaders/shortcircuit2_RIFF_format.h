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
#pragma pack(push, 1) // force byte packing for the structs

/*
File structure:
RIFF-based, Little-Endian
All text strings are UTF-8 encoded and null-terminated

Always traverse the RIFF structure properly, don't make any assumptions about the order or existence
of chunks. Chunks with the same ID/fourCC within a LIST block should be parsed/written in the order
of their appearance.

Hierarchy:

RIFF:SC2P
        Head
        LIST:Mult			// Multi entry (multis only)
                MulD
                LIST:Fltr
                        FltD
        LIST:PtLs			// Part List
                LIST:Part
                        ParD
                        Name		// Variable length UTF-8 string
                        AuxB
                        MMen
                        LIST:Fltr
                                FltD
                        LIST:Ctrl
                                CtrD
                                Name	// Variable length UTF-8 string
                        LIST:Layr
                                LayD	// Placeholder
                                NCen
        LIST:ZnLs			// Zone List
                LIST:Zone
                        ZonD
                        Name		// Variable length UTF-8 string
                        AuxB
                        AHDS
                        SLFO
                        MMen
                        NCen
                        HtPt
                        LIST:Fltr
                                FltD
        LIST:SmLs			// Sample list. All Child nodes increment the ID,
unrecognized or not. LIST:WAVE		// Embedded Samples in WAVE format data fmt
                        ..
                LIST:Smpl			// External Sample URL
                        SUrl		// system path to sample (Variable length UTF-8 string)
                        SmpD		// Unspecified


All endpoints of sample-ranges (such as LoopEnd, SampleStop..) are stored EXCLUDING the end-point.
Thus the last sample to be played for a zone is (SampleStop - 1). All undefined (UD) values should
be set to 0.

TODO Frequency values are stored as ...

*/

// "MMen"
struct RIFF_MM_ENTRY
{
    unsigned char Source1, Source2, Destination, Active, Curve, UD0[3];
    float Strength;
};

// "FltD"
struct RIFF_FILTER
{
    unsigned int Type, Flags;
    float P[12];
    unsigned char PFlags[12];
    int IP[4];
    float Mix;
};

enum RIFF_FILTER_Flags
{
    RFF_Bypass = 1 << 0,
};

// "FltB"
struct RIFF_FILTER_BUSSEX
{
    float PreGain, PostGain;
    unsigned char Output, UD[3];
};

// "Mrph" only present when morphEQ is used
struct RIFF_FILTER_MORPHEQ
{
    struct
    {
        float Freq[8];
        float Gain[8];
        float BW[8];
        unsigned int flags;
    } state[2];
};

enum RIFF_FILTER_MORPHEQ_Flags
{
    RFMF_Active = 1 << 0,
};

// Modulators (AHDS & SLFO)
// Write in this order: AEG, EG2, SLFO1, SLFO2, SLFO3
// Future versions may have freely assignable modulator types for each slot so the EG vs. SLFO order
// is important.

// "AHDS"
struct RIFF_AHDSR
{
    float Attack, Hold, Decay, Sustain, Release;
    float Shape[3];
};

// "SLFO"
struct RIFF_LFO
{
    float StepData[32];
    unsigned int Flags;
    float Rate, Smooth, Shuffle;
    char Repeat, TriggerMode, UD[2];
};

enum RIFF_LFO_Flags
{
    RLF_TempoSync = 1 << 0,
    RLF_CycleMode = 1 << 1,
    RLF_OnlyOnce = 1 << 2,
};

// "NCen"
struct RIFF_NC_ENTRY
{
    unsigned char Source, Low, High, UD0;
};

// "AuxB"
struct RIFF_AUX_BUSS
{
    float Level, Balance;
    char Output, OutMode, UD0[2];
};

// "HtPt"
struct RIFF_HITPOINT
{
    unsigned int StartSample;
    unsigned int EndSample;
    float Env;
    unsigned int Flags;
};

// "ZonD"
struct RIFF_ZONE
{
    unsigned char Part, Layer, Playmode, KeyRoot;
    unsigned int SampleID; // This ID assigns a sample from the LIST:SmLs sample list to the zone
                           // (starting at 0)
    unsigned int LoopStart, LoopEnd, SampleStart, SampleStop, LoopCrossfadeLength, Flags, UD0[5];
    char KeyLow, KeyHigh, KeyLowFade, KeyHighFade;
    char VelocityHigh, VelocityLow, VelocityLowFade, VelocityHighFade;
    char Transpose, PitchBendDepth, MuteGroup, N_Hitpoints;
    float FineTune, PitchCorrection, VelSense, Keytrack, PreFilterGain;
    float LagGenerator[2];
};

enum RIFF_ZONE_Flags
{
    RZF_Mute = 1 << 0,
    RZF_IgnorePartPolymode = 1 << 1,
    RZF_Reverse = 1 << 2,
};

enum RIFF_ZONE_Playmode
{
    RZP_Normal = 0,
    RZP_Loop = 1,
    RZP_LoopUntilRelease = 3,
    RZP_LoopBidirectional = 4,
    RZP_Shot = 5,
    RZP_Hitpoints = 6,
    RZP_OnRelease = 7,
};

// "Smpl"
// TODO Add more metadata
struct RIFF_SampleURL
{
    unsigned int FileSize;
};

// "CtrD"
struct RIFF_CONTROLLER
{
    float State;
    int Flags;
};

enum RIFF_CONTROLLER_Flags
{
    RCF_Bipolar = 1 << 0,
};

// "ZonD"
struct RIFF_PART
{
    char MIDIChannel, Transpose, Formant, PolyMode;
    char PolyLimit, PortamentoMode, VelSplitLayerCount, Undefined;
    float FineTune, Portamento, PreFilterGain;
    float VelSplitDistribution, VelSplitXFade;
    unsigned int Flags;
};

enum RIFF_Part_Flags
{
    RPF_XFadeEqualPower = 1 << 0, // 1 = Equal Power, 0 = Equal Gain
    RPF_DisplayMode1 = 1 << 1,    // 0 = Zones by Key, 1 = Zones by Pad
    RPF_DisplayMode2 = 1 << 2,    // Undefined
};

// "MulD"
struct RIFF_MULTI
{
};

enum RIFF_FILTER_Type
{
    RFT_None = 0,

    // "Filters"
    RFT_SBQ = 0x100,
    RTF_Surge = 0x110,
    RFT_Moog = 0x120,

    // Legacy filters from Version 1
    RFT_LP2A = 0x180,
    RFT_LP2B = 0x181,
    RFT_HP2A = 0x182,
    RFT_BP2 = 0x183,
    RFT_BP2B = 0x184,
    RFT_Peak = 0x185,
    RFT_Notch = 0x186,
    RFT_BP2Dual = 0x187,
    RFT_PeakDual = 0x188,

    // EQ
    RFT_EQ_2Band = 0x200,
    RFT_EQ_6BandGraphic = 0x210,
    RFT_MorphEQ = 0x240,

    // Dynamics
    RFT_Limiter = 0x300,
    RFT_Gate = 0x310,
    RFT_MicroGate = 0x311,

    // Non-linear
    RFT_Distortion = 0x400,
    RFT_DigiLofi = 0x410,
    RFT_Clipper = 0x420,
    RFT_Slewer = 0x430,

    // Modulation
    RFT_RingMod = 0x500,
    RFT_FreqShift = 0x510,
    RFT_PhaseMod = 0x520,

    // Misc
    RFT_Treemonster = 0x700,
    RFT_Comb1 = 0x710,
    RFT_StereoTools = 0x720,

    // "Effects"
    RFT_Delay = 0x800,
    RFT_Reverb = 0x810,
    RFT_Chorus = 0x820,
    RFT_Phaser = 0x830,
    RFT_Rotary = 0x840,
    RFT_FauxStereo = 0x850,
    RFT_FSFlange = 0x860,
    RFT_FSDelay = 0x861,

    // Oscillators
    RFT_SawOsc = 0x900,
    RFT_SinOsc = 0x910,
    RFT_PulseOsc = 0x930,
    RFT_PulseOscSync = 0x931,
};

enum RIFF_MM_Source
{
    RMS_None = 0,

    // Zone only
    RMS_Keytrack = 0x01,
    RMS_Velocity = 0x02,
    RMS_ReleaseVelocity = 0x03, // Unsupported
    RMS_Modulator1 = 0x10,      // Amp EG
    RMS_Modulator2 = 0x11,      // Filter EG
    RMS_Modulator3 = 0x12,      // SLFO1
    RMS_Modulator4 = 0x13,      // SLFO2
    RMS_Modulator5 = 0x14,      // SLFO3
    RMS_Gate = 0x20,
    RMS_Time = 0x21,
    RMS_TimeMinutes = 0x22,
    RMS_WithinLoop = 0x23,
    RMS_LoopPos = 0x24,
    RMS_SliceEnv = 0x25,
    RMS_Filter1ModOut = 0x28,
    RMS_Filter2ModOut = 0x29,
    RMS_Random = 0x30,
    RMS_RandomBP = 0x38,
    RMS_LagGenerator1 = 0x40,
    RMS_LagGenerator2 = 0x41,
    // Common
    RMS_PitchBend = 0x80,
    // RMS_PitchBendUp	= 0x81,	// Superfluous with curves
    // RMS_PitchBendDown	= 0x82, // Superfluous with curves
    RMS_ChAftertouch = 0x81,
    RMS_PolyAftertouch = 0x82, // Currently unsupported
    RMS_ModulationWheel = 0x83,
    RMS_PosBeat = 0x94,
    RMS_Pos2Beats = 0x95,
    RMS_PosBar = 0x96,
    RMS_Pos2Bars = 0x97,
    RMS_Pos4Bars = 0x98,
    RMS_Alternate = 0xA0,
    RMS_Noise = 0xA8,
    RMS_Ctrl1 = 0xB0, // RMS_CtrlX = RMS_Ctrl1 + X if X starts at 0
    // Constants
    RMS_One = 0xff,
};

enum RIFF_MM_Destination
{
    RMD_None = 0,
    // Zone destinations
    RMD_Pitch = 0x01,
    RMD_Rate = 0x02,
    RMD_LagGenerator1 = 0x08,
    RMD_LagGenerator2 = 0x09,
    RMD_SampleStart = 0x10,
    RMD_LoopStart = 0x11,
    RMD_LoopLength = 0x12,
    RMD_Amplitude = 0x20,
    RMD_AmplitudeAux1 = 0x21,
    RMD_AmplitudeAux2 = 0x22,
    RMD_Balance = 0x23,
    RMD_BalanceAux1 = 0x24,
    RMD_BalanceAux2 = 0x25,
    RMD_PreFilterGain = 0x26,
    RMD_Filter1Mix = 0x30,
    RMD_Filter1Param0 = 0x31,
    RMD_Filter2Mix = 0x40,
    RMD_Filter2Param0 = 0x41,
    RMD_AEGAttack = 0x70, // Modulators: 0x70 + 0x08*ID + # [0..7]
    RMD_AEGHold = 0x71,
    RMD_AEGDecay = 0x72,
    RMD_AEGSustain = 0x73,
    RMD_AEGRelease = 0x74,
    RMD_EG2Attack = 0x78,
    RMD_EG2Hold = 0x79,
    RMD_EG2Decay = 0x7A,
    RMD_EG2Sustain = 0x7B,
    RMD_EG2Release = 0x7C,
    RMD_LFO1Rate = 0x80,
    RMD_LFO2Rate = 0x88,
    RMD_LFO3Rate = 0x90,
    // Part destinations
    RMD_PartAmplitude = 0xB0,
    RMD_PartAmplitudeAux1 = 0xB1,
    RMD_PartAmplitudeAux2 = 0xB2,
    RMD_PartBalance = 0xB3,
    RMD_PartBalanceAux1 = 0xB4,
    RMD_PartBalanceAux2 = 0xB5,
    RMD_PartPreFilterGain = 0xB6,
    RMD_PartFilter1Mix = 0xC0,
    RMD_PartFilter1Param0 = 0xC1,
    RMD_PartFilter2Mix = 0xD0,
    RMD_PartFilter2Param0 = 0xD1,
};

#pragma pack(pop)
