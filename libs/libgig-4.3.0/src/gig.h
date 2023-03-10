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

#ifndef __GIG_H__
#define __GIG_H__

#include "DLS.h"
#include <vector>
#include <array> // since C++11

#ifndef __has_feature
# define __has_feature(x) 0
#endif
#ifndef HAVE_RTTI
# if __GXX_RTTI || __has_feature(cxx_rtti) || _CPPRTTI
#  define HAVE_RTTI 1
# else
#  define HAVE_RTTI 0
# endif
#endif
#if HAVE_RTTI
# include <typeinfo>
#else
# warning No RTTI available!
#endif

#if WORDS_BIGENDIAN
# define LIST_TYPE_3PRG	0x33707267
# define LIST_TYPE_3EWL	0x3365776C
# define LIST_TYPE_3GRI	0x33677269
# define LIST_TYPE_3GNL	0x33676E6C
# define LIST_TYPE_3LS  0x334c5320 // own gig format extension
# define LIST_TYPE_RTIS 0x52544953 // own gig format extension
# define LIST_TYPE_3DNM 0x33646e6d
# define CHUNK_ID_3GIX	0x33676978
# define CHUNK_ID_3EWA	0x33657761
# define CHUNK_ID_3LNK	0x336C6E6B
# define CHUNK_ID_3EWG	0x33657767
# define CHUNK_ID_EWAV	0x65776176
# define CHUNK_ID_3GNM	0x33676E6D
# define CHUNK_ID_EINF	0x65696E66
# define CHUNK_ID_3CRC	0x33637263
# define CHUNK_ID_SCRI  0x53637269 // own gig format extension
# define CHUNK_ID_LSNM  0x4c534e4d // own gig format extension
# define CHUNK_ID_SCSL  0x5343534c // own gig format extension
# define CHUNK_ID_SCPV  0x53435056 // own gig format extension
# define CHUNK_ID_LSDE  0x4c534445 // own gig format extension
# define CHUNK_ID_3DDP  0x33646470
#else  // little endian
# define LIST_TYPE_3PRG	0x67727033
# define LIST_TYPE_3EWL	0x6C776533
# define LIST_TYPE_3GRI	0x69726733
# define LIST_TYPE_3GNL	0x6C6E6733
# define LIST_TYPE_3LS  0x20534c33 // own gig format extension
# define LIST_TYPE_RTIS 0x53495452 // own gig format extension
# define LIST_TYPE_3DNM 0x6d6e6433
# define CHUNK_ID_3GIX	0x78696733
# define CHUNK_ID_3EWA	0x61776533
# define CHUNK_ID_3LNK	0x6B6E6C33
# define CHUNK_ID_3EWG	0x67776533
# define CHUNK_ID_EWAV	0x76617765
# define CHUNK_ID_3GNM	0x6D6E6733
# define CHUNK_ID_EINF	0x666E6965
# define CHUNK_ID_3CRC	0x63726333
# define CHUNK_ID_SCRI  0x69726353 // own gig format extension
# define CHUNK_ID_LSNM  0x4d4e534c // own gig format extension
# define CHUNK_ID_SCSL  0x4c534353 // own gig format extension
# define CHUNK_ID_SCPV  0x56504353 // own gig format extension
# define CHUNK_ID_LSDE  0x4544534c // own gig format extension
# define CHUNK_ID_3DDP  0x70646433
#endif // WORDS_BIGENDIAN

#ifndef GIG_DECLARE_ENUM
# define GIG_DECLARE_ENUM(type, ...) enum type { __VA_ARGS__ }
#endif

// just symbol prototyping (since Serialization.h not included by default here)
namespace Serialization { class Archive; }

/** Gigasampler/GigaStudio specific classes and definitions */
namespace gig {

    typedef std::string String;
    typedef RIFF::progress_t progress_t;
    typedef RIFF::file_offset_t file_offset_t;

    /** Lower and upper limit of a range. */
    struct range_t {
        uint8_t low;  ///< Low value of range.
        uint8_t high; ///< High value of range.
    };

    /** Pointer address and size of a buffer. */
    struct buffer_t {
        void*         pStart;            ///< Points to the beginning of the buffer.
        file_offset_t Size;              ///< Size of the actual data in the buffer in bytes.
        file_offset_t NullExtensionSize; ///< The buffer might be bigger than the actual data, if that's the case that unused space at the end of the buffer is filled with NULLs and NullExtensionSize reflects that unused buffer space in bytes. Those NULL extensions are mandatory for differential algorithms that have to take the following data words into account, thus have to access past the buffer's boundary. If you don't know what I'm talking about, just forget this variable. :)
        buffer_t() {
            pStart            = NULL;
            Size              = 0;
            NullExtensionSize = 0;
        }
    };

    /** Standard types of sample loops.
     *
     * @see enumCount(), enumKey(), enumKeys(), enumValue()
     */
    GIG_DECLARE_ENUM(loop_type_t,
        loop_type_normal        = 0x00000000,  /**< Loop forward (normal) */
        loop_type_bidirectional = 0x00000001,  /**< Alternating loop (forward/backward, also known as Ping Pong) */
        loop_type_backward      = 0x00000002   /**< Loop backward (reverse) */
    );

    /** Society of Motion Pictures and Television E time format.
     *
     * @see enumCount(), enumKey(), enumKeys(), enumValue()
     */
    GIG_DECLARE_ENUM(smpte_format_t,
        smpte_format_no_offset          = 0x00000000,  /**< no SMPTE offset */
        smpte_format_24_frames          = 0x00000018,  /**< 24 frames per second */
        smpte_format_25_frames          = 0x00000019,  /**< 25 frames per second */
        smpte_format_30_frames_dropping = 0x0000001D,  /**< 30 frames per second with frame dropping (30 drop) */
        smpte_format_30_frames          = 0x0000001E   /**< 30 frames per second */
    );

    /** Defines the shape of a function graph.
     *
     * @see enumCount(), enumKey(), enumKeys(), enumValue()
     */
    GIG_DECLARE_ENUM(curve_type_t,
        curve_type_nonlinear = 0,          /**< Non-linear curve type. */
        curve_type_linear    = 1,          /**< Linear curve type. */
        curve_type_special   = 2,          /**< Special curve type. */
        curve_type_unknown   = 0xffffffff  /**< Unknown curve type. */
    );

    /** Defines the wave form type used by an LFO (gig format extension).
     *
     * This is a gig format extension. The original Gigasampler/GigaStudio
     * software always used a sine (sinus) wave form for all its 3 LFOs, so this
     * was not configurable in the original gig format. Accordingly setting any
     * other wave form than sine (sinus) will be ignored by the original
     * Gigasampler/GigaStudio software.
     *
     * @see enumCount(), enumKey(), enumKeys(), enumValue()
     */
    GIG_DECLARE_ENUM(lfo_wave_t,
        lfo_wave_sine     = 0,  /**< Sine (sinus) wave form (this is the default wave form). */
        lfo_wave_triangle = 1,  /**< Triangle wave form. */
        lfo_wave_saw      = 2,  /**< Saw (up) wave form (saw down wave form can be achieved by flipping the phase). */
        lfo_wave_square   = 3,  /**< Square wave form. */
    );

    /** Dimensions allow to bypass one of the following controllers.
     *
     * @see enumCount(), enumKey(), enumKeys(), enumValue()
     */
    GIG_DECLARE_ENUM(dim_bypass_ctrl_t,
        dim_bypass_ctrl_none, /**< No controller bypass. */
        dim_bypass_ctrl_94,   /**< Effect 4 Depth (MIDI Controller 94) */
        dim_bypass_ctrl_95    /**< Effect 5 Depth (MIDI Controller 95) */
    );

    /** Defines how LFO3 is controlled by.
     *
     * @see enumCount(), enumKey(), enumKeys(), enumValue()
     */
    GIG_DECLARE_ENUM(lfo3_ctrl_t,
        lfo3_ctrl_internal            = 0x00, /**< Only internally controlled. */
        lfo3_ctrl_modwheel            = 0x01, /**< Only controlled by external modulation wheel. */
        lfo3_ctrl_aftertouch          = 0x02, /**< Only controlled by aftertouch controller. */
        lfo3_ctrl_internal_modwheel   = 0x03, /**< Controlled internally and by external modulation wheel. */
        lfo3_ctrl_internal_aftertouch = 0x04  /**< Controlled internally and by aftertouch controller. */
    );

    /** Defines how LFO2 is controlled by.
     *
     * @see enumCount(), enumKey(), enumKeys(), enumValue()
     */
    GIG_DECLARE_ENUM(lfo2_ctrl_t,
        lfo2_ctrl_internal            = 0x00, /**< Only internally controlled. */
        lfo2_ctrl_modwheel            = 0x01, /**< Only controlled by external modulation wheel. */
        lfo2_ctrl_foot                = 0x02, /**< Only controlled by external foot controller. */
        lfo2_ctrl_internal_modwheel   = 0x03, /**< Controlled internally and by external modulation wheel. */
        lfo2_ctrl_internal_foot       = 0x04  /**< Controlled internally and by external foot controller. */
    );

    /** Defines how LFO1 is controlled by.
     *
     * @see enumCount(), enumKey(), enumKeys(), enumValue()
     */
    GIG_DECLARE_ENUM(lfo1_ctrl_t,
        lfo1_ctrl_internal            = 0x00, /**< Only internally controlled. */
        lfo1_ctrl_modwheel            = 0x01, /**< Only controlled by external modulation wheel. */
        lfo1_ctrl_breath              = 0x02, /**< Only controlled by external breath controller. */
        lfo1_ctrl_internal_modwheel   = 0x03, /**< Controlled internally and by external modulation wheel. */
        lfo1_ctrl_internal_breath     = 0x04  /**< Controlled internally and by external breath controller. */
    );

    /** Defines how the filter cutoff frequency is controlled by.
     *
     * @see enumCount(), enumKey(), enumKeys(), enumValue()
     */
    GIG_DECLARE_ENUM(vcf_cutoff_ctrl_t,
        vcf_cutoff_ctrl_none         = 0x00,  /**< No MIDI controller assigned for filter cutoff frequency. */
        vcf_cutoff_ctrl_none2        = 0x01,  /**< The difference between none and none2 is unknown */
        vcf_cutoff_ctrl_modwheel     = 0x81,  /**< Modulation Wheel (MIDI Controller 1) */
        vcf_cutoff_ctrl_effect1      = 0x8c,  /**< Effect Controller 1 (Coarse, MIDI Controller 12) */
        vcf_cutoff_ctrl_effect2      = 0x8d,  /**< Effect Controller 2 (Coarse, MIDI Controller 13) */
        vcf_cutoff_ctrl_breath       = 0x82,  /**< Breath Controller (Coarse, MIDI Controller 2) */
        vcf_cutoff_ctrl_foot         = 0x84,  /**< Foot Pedal (Coarse, MIDI Controller 4) */
        vcf_cutoff_ctrl_sustainpedal = 0xc0,  /**< Sustain Pedal (MIDI Controller 64) */
        vcf_cutoff_ctrl_softpedal    = 0xc3,  /**< Soft Pedal (MIDI Controller 67) */
        vcf_cutoff_ctrl_genpurpose7  = 0xd2,  /**< General Purpose Controller 7 (Button, MIDI Controller 82) */
        vcf_cutoff_ctrl_genpurpose8  = 0xd3,  /**< General Purpose Controller 8 (Button, MIDI Controller 83) */
        vcf_cutoff_ctrl_aftertouch   = 0x80   /**< Key Pressure */
    );

    /** Defines how the filter resonance is controlled by.
     *
     * @see enumCount(), enumKey(), enumKeys(), enumValue()
     */
    GIG_DECLARE_ENUM(vcf_res_ctrl_t,
        vcf_res_ctrl_none        = 0xffffffff,  /**< No MIDI controller assigned for filter resonance. */
        vcf_res_ctrl_genpurpose3 = 0,           /**< General Purpose Controller 3 (Slider, MIDI Controller 18) */
        vcf_res_ctrl_genpurpose4 = 1,           /**< General Purpose Controller 4 (Slider, MIDI Controller 19) */
        vcf_res_ctrl_genpurpose5 = 2,           /**< General Purpose Controller 5 (Button, MIDI Controller 80) */
        vcf_res_ctrl_genpurpose6 = 3            /**< General Purpose Controller 6 (Button, MIDI Controller 81) */
    );

    /**
     * Defines a controller that has a certain contrained influence on a
     * particular synthesis parameter (used to define attenuation controller,
     * EG1 controller and EG2 controller).
     *
     * You should use the respective <i>typedef</i> (means either
     * attenuation_ctrl_t, eg1_ctrl_t or eg2_ctrl_t) in your code!
     */
    struct leverage_ctrl_t {
        /** Defines possible controllers.
         *
         * @see enumCount(), enumKey(), enumKeys(), enumValue()
         */
        GIG_DECLARE_ENUM(type_t,
            type_none              = 0x00, /**< No controller defined */
            type_channelaftertouch = 0x2f, /**< Channel Key Pressure */
            type_velocity          = 0xff, /**< Key Velocity */
            type_controlchange     = 0xfe  /**< Ordinary MIDI control change controller, see field 'controller_number' */
        );

        type_t type;              ///< Controller type
        uint   controller_number; ///< MIDI controller number if this controller is a control change controller, 0 otherwise

        void serialize(Serialization::Archive* archive);
    };

    /**
     * Defines controller influencing attenuation.
     *
     * @see leverage_ctrl_t
     */
    typedef leverage_ctrl_t attenuation_ctrl_t;

    /**
     * Defines controller influencing envelope generator 1.
     *
     * @see leverage_ctrl_t
     */
    typedef leverage_ctrl_t eg1_ctrl_t;

    /**
     * Defines controller influencing envelope generator 2.
     *
     * @see leverage_ctrl_t
     */
    typedef leverage_ctrl_t eg2_ctrl_t;

    /**
     * Defines the type of dimension, that is how the dimension zones (and
     * thus how the dimension regions are selected by. The number of
     * dimension zones is always a power of two. All dimensions can have up
     * to 32 zones (except the layer dimension with only up to 8 zones and
     * the samplechannel dimension which currently allows only 2 zones).
     *
     * @see enumCount(), enumKey(), enumKeys(), enumValue()
     */
    GIG_DECLARE_ENUM(dimension_t,
        dimension_none              = 0x00, /**< Dimension not in use. */
        dimension_samplechannel     = 0x80, /**< If used sample has more than one channel (thus is not mono). */
        dimension_layer             = 0x81, /**< For layering of up to 8 instruments (and eventually crossfading of 2 or 4 layers). */
        dimension_velocity          = 0x82, /**< Key Velocity (this is the only dimension in gig2 where the ranges can exactly be defined). */
        dimension_channelaftertouch = 0x83, /**< Channel Key Pressure */
        dimension_releasetrigger    = 0x84, /**< Special dimension for triggering samples on releasing a key. */
        dimension_keyboard          = 0x85, /**< Dimension for keyswitching */
        dimension_roundrobin        = 0x86, /**< Different samples triggered each time a note is played, dimension regions selected in sequence */
        dimension_random            = 0x87, /**< Different samples triggered each time a note is played, random order */
        dimension_smartmidi         = 0x88, /**< For MIDI tools like legato and repetition mode */
        dimension_roundrobinkeyboard = 0x89, /**< Different samples triggered each time a note is played, any key advances the counter */
        dimension_modwheel          = 0x01, /**< Modulation Wheel (MIDI Controller 1) */
        dimension_breath            = 0x02, /**< Breath Controller (Coarse, MIDI Controller 2) */
        dimension_foot              = 0x04, /**< Foot Pedal (Coarse, MIDI Controller 4) */
        dimension_portamentotime    = 0x05, /**< Portamento Time (Coarse, MIDI Controller 5) */
        dimension_effect1           = 0x0c, /**< Effect Controller 1 (Coarse, MIDI Controller 12) */
        dimension_effect2           = 0x0d, /**< Effect Controller 2 (Coarse, MIDI Controller 13) */
        dimension_genpurpose1       = 0x10, /**< General Purpose Controller 1 (Slider, MIDI Controller 16) */
        dimension_genpurpose2       = 0x11, /**< General Purpose Controller 2 (Slider, MIDI Controller 17) */
        dimension_genpurpose3       = 0x12, /**< General Purpose Controller 3 (Slider, MIDI Controller 18) */
        dimension_genpurpose4       = 0x13, /**< General Purpose Controller 4 (Slider, MIDI Controller 19) */
        dimension_sustainpedal      = 0x40, /**< Sustain Pedal (MIDI Controller 64) */
        dimension_portamento        = 0x41, /**< Portamento (MIDI Controller 65) */
        dimension_sostenutopedal    = 0x42, /**< Sostenuto Pedal (MIDI Controller 66) */
        dimension_softpedal         = 0x43, /**< Soft Pedal (MIDI Controller 67) */
        dimension_genpurpose5       = 0x30, /**< General Purpose Controller 5 (Button, MIDI Controller 80) */
        dimension_genpurpose6       = 0x31, /**< General Purpose Controller 6 (Button, MIDI Controller 81) */
        dimension_genpurpose7       = 0x32, /**< General Purpose Controller 7 (Button, MIDI Controller 82) */
        dimension_genpurpose8       = 0x33, /**< General Purpose Controller 8 (Button, MIDI Controller 83) */
        dimension_effect1depth      = 0x5b, /**< Effect 1 Depth (MIDI Controller 91) */
        dimension_effect2depth      = 0x5c, /**< Effect 2 Depth (MIDI Controller 92) */
        dimension_effect3depth      = 0x5d, /**< Effect 3 Depth (MIDI Controller 93) */
        dimension_effect4depth      = 0x5e, /**< Effect 4 Depth (MIDI Controller 94) */
        dimension_effect5depth      = 0x5f  /**< Effect 5 Depth (MIDI Controller 95) */
    );

    /**
     * Intended for internal usage: will be used to convert a dimension value
     * into the corresponding dimension bit number.
     *
     * @see enumCount(), enumKey(), enumKeys(), enumValue()
     */
    GIG_DECLARE_ENUM(split_type_t,
        split_type_normal,         /**< dimension value between 0-127 */
        split_type_bit             /**< dimension values are already the sought bit number */
    );

    /** General dimension definition. */
    struct dimension_def_t {
        dimension_t  dimension;  ///< Specifies which source (usually a MIDI controller) is associated with the dimension.
        uint8_t      bits;       ///< Number of "bits" (1 bit = 2 splits/zones, 2 bit = 4 splits/zones, 3 bit = 8 splits/zones,...).
        uint8_t      zones;      ///< Number of zones the dimension has.
        split_type_t split_type; ///< Intended for internal usage: will be used to convert a dimension value into the corresponding dimension bit number.
        float        zone_size;  ///< Intended for internal usage: reflects the size of each zone (128/zones) for normal split types only, 0 otherwise.
    };

    /** Audio filter types.
     *
     * The first 5 filter types are the ones which exist in GigaStudio, and
     * which are very accurately modeled on LinuxSampler side such that they
     * would sound with LinuxSampler exactly as with GigaStudio.
     *
     * The other filter types listed here are extensions to the gig format and
     * are LinuxSampler specific filter type implementations. Note that none of
     * these are duplicates of the GigaStudio filter types. For instance
     * @c vcf_type_lowpass (GigaStudio) and @c vcf_type_lowpass_2p
     * (LinuxSampler) are both lowpass filters with 2 poles, however they do
     * sound differently.
     *
     * @see enumCount(), enumKey(), enumKeys(), enumValue()
     */
    GIG_DECLARE_ENUM(vcf_type_t,
        vcf_type_lowpass       = 0x00, /**< Standard lowpass filter type (GigaStudio). */
        vcf_type_lowpassturbo  = 0xff, /**< More poles than normal lowpass (GigaStudio). */
        vcf_type_bandpass      = 0x01, /**< Bandpass filter type (GigaStudio). */
        vcf_type_highpass      = 0x02, /**< Highpass filter type (GigaStudio). */
        vcf_type_bandreject    = 0x03, /**< Band reject filter type (GigaStudio). */
        vcf_type_lowpass_1p    = 0x11, /**< [gig extension]: 1-pole lowpass filter type (LinuxSampler). */
        vcf_type_lowpass_2p    = 0x12, /**< [gig extension]: 2-pole lowpass filter type (LinuxSampler). */
        vcf_type_lowpass_4p    = 0x14, /**< [gig extension]: 4-pole lowpass filter type (LinuxSampler). */
        vcf_type_lowpass_6p    = 0x16, /**< [gig extension]: 6-pole lowpass filter type (LinuxSampler). */
        vcf_type_highpass_1p   = 0x21, /**< [gig extension]: 1-pole highpass filter type (LinuxSampler). */
        vcf_type_highpass_2p   = 0x22, /**< [gig extension]: 2-pole highpass filter type (LinuxSampler). */
        vcf_type_highpass_4p   = 0x24, /**< [gig extension]: 4-pole highpass filter type (LinuxSampler). */
        vcf_type_highpass_6p   = 0x26, /**< [gig extension]: 6-pole highpass filter type (LinuxSampler). */
        vcf_type_bandpass_2p   = 0x32, /**< [gig extension]: 2-pole bandpass filter type (LinuxSampler). */
        vcf_type_bandreject_2p = 0x42  /**< [gig extension]: 2-pole bandreject filter type (LinuxSampler). */
    );

    /**
     * Defines the envelope of a crossfade.
     *
     * Note: The default value for crossfade points is 0,0,0,0. Layers with
     * such a default value should be treated as if they would not have a
     * crossfade.
     */
    struct crossfade_t {
        #if WORDS_BIGENDIAN
        uint8_t out_end;    ///< End postition of fade out.
        uint8_t out_start;  ///< Start position of fade out.
        uint8_t in_end;     ///< End position of fade in.
        uint8_t in_start;   ///< Start position of fade in.
        #else // little endian
        uint8_t in_start;   ///< Start position of fade in.
        uint8_t in_end;     ///< End position of fade in.
        uint8_t out_start;  ///< Start position of fade out.
        uint8_t out_end;    ///< End postition of fade out.
        #endif // WORDS_BIGENDIAN

        void serialize(Serialization::Archive* archive);
    };

    /** Reflects the current playback state for a sample. */
    struct playback_state_t {
        file_offset_t position;          ///< Current position within the sample.
        bool          reverse;           ///< If playback direction is currently backwards (in case there is a pingpong or reverse loop defined).
        file_offset_t loop_cycles_left;  ///< How many times the loop has still to be passed, this value will be decremented with each loop cycle.
    };

    /**
     * Defines behavior options for envelope generators (gig format extension).
     *
     * These options allow to override the precise default behavior of the
     * envelope generators' state machines.
     *
     * @b Note: These EG options are an extension to the original gig file
     * format, so these options are not available with the original
     * Gigasampler/GigaStudio software! Currently only LinuxSampler and gigedit
     * support these EG options!
     *
     * Adding these options to the original gig file format was necessary,
     * because the precise state machine behavior of envelope generators of the
     * gig format (and thus the default EG behavior if not explicitly overridden
     * here) deviates from common, expected behavior of envelope generators in
     * general, if i.e. compared with EGs of hardware synthesizers. For example
     * with the gig format, the attack and decay stages will be aborted as soon
     * as a note-off is received. Most other EG implementations in the industry
     * however always run the attack and decay stages to their full duration,
     * even if an early note-off arrives. The latter behavior is intentionally
     * implemented in most other products, because it is required to resemble
     * percussive sounds in a realistic manner.
     */
    struct eg_opt_t {
        bool AttackCancel;     ///< Whether the "attack" stage is cancelled when receiving a note-off (default: @c true).
        bool AttackHoldCancel; ///< Whether the "attack hold" stage is cancelled when receiving a note-off (default: @c true).
        bool Decay1Cancel;     ///< Whether the "decay 1" stage is cancelled when receiving a note-off (default: @c true).
        bool Decay2Cancel;     ///< Whether the "decay 2" stage is cancelled when receiving a note-off (default: @c true).
        bool ReleaseCancel;    ///< Whether the "release" stage is cancelled when receiving a note-on (default: @c true).

        eg_opt_t();
        void serialize(Serialization::Archive* archive);
    };

    /** @brief Defines behaviour of release triggered sample(s) on sustain pedal up event.
     *
     * This option defines whether a sustain pedal up event (CC#64) would cause
     * release triggered samples to be played (if any).
     *
     * @b Note: This option is an extension to the original gig file format,
     * so this option is not available with the original Gigasampler/GigaStudio
     * software! Currently only LinuxSampler and gigedit support this option!
     *
     * By default (which equals the original Gigasampler/GigaStudio behaviour)
     * no release triggered samples are played if the sustain pedal is released.
     * So usually in the gig format release triggered samples are only played
     * on MIDI note-off events.
     *
     * @see enumCount(), enumKey(), enumKeys(), enumValue()
     */
    GIG_DECLARE_ENUM(sust_rel_trg_t,
        sust_rel_trg_none        = 0x00, /**< No release triggered sample(s) are played on sustain pedal up (default). */
        sust_rel_trg_maxvelocity = 0x01, /**< Play release trigger sample(s) on sustain pedal up, and simply use 127 as MIDI velocity for playback. */
        sust_rel_trg_keyvelocity = 0x02  /**< Play release trigger sample(s) on sustain pedal up, and use the key`s last MIDI note-on velocity for playback. */
    );

    // just symbol prototyping
    class File;
    class Instrument;
    class Sample;
    class Region;
    class Group;
    class Script;
    class ScriptGroup;

    /** @brief Encapsulates articulation informations of a dimension region.
     *
     * This is the most important data object of the Gigasampler / GigaStudio
     * format. A DimensionRegion provides the link to the sample to be played
     * and all required articulation informations to be interpreted for playing
     * back the sample and processing it appropriately by the sampler software.
     * Every Region of a Gigasampler Instrument has at least one dimension
     * region (exactly then when the Region has no dimension defined). Many
     * Regions though provide more than one DimensionRegion, which reflect
     * different playing "cases". For example a different sample might be played
     * if a certain pedal is pressed down, or if the note was triggered with
     * different velocity.
     *
     * One instance of a DimensionRegion reflects exactly one particular case
     * while playing an instrument (for instance "note between C3 and E3 was
     * triggered AND note on velocity was between 20 and 42 AND modulation wheel
     * controller is between 80 and 127). The DimensionRegion defines what to do
     * under that one particular case, that is which sample to play back and how
     * to play that sample back exactly and how to process it. So a
     * DimensionRegion object is always linked to exactly one sample. It may
     * however also link to no sample at all, for defining a "silence" case
     * where nothing shall be played (for example when note on velocity was
     * below 6).
     *
     * Note that a DimensionRegion object only defines "what to do", but it does
     * not define "when to do it". To actually resolve which DimensionRegion to
     * pick under which situation, you need to refer to the DimensionRegions'
     * parent Region object. The Region object contains the necessary
     * "Dimension" definitions, which in turn define which DimensionRegion is
     * associated with which playing case exactly.
     *
     * The Gigasampler/GigaStudio format defines 3 Envelope Generators and 3
     * Low Frequency Oscillators:
     *
     *  - EG1 and LFO1, both controlling sample amplitude
     *  - EG2 and LFO2, both controlling filter cutoff frequency
     *  - EG3 and LFO3, both controlling sample pitch
     *
     * Since the gig format was designed as extension to the DLS file format,
     * this class is derived from the DLS::Sampler class. So also refer to
     * DLS::Sampler for additional informations, class attributes and methods.
     */
    class DimensionRegion : protected DLS::Sampler {
        public:
            uint8_t            VelocityUpperLimit;            ///< Defines the upper velocity value limit of a velocity split (only if an user defined limit was set, thus a value not equal to 128/NumberOfSplits, else this value is 0). Only for gig2, for gig3 and above the DimensionUpperLimits are used instead.
            Sample*            pSample;                       ///< Points to the Sample which is assigned to the dimension region.
            // Sample Amplitude EG/LFO
            uint16_t           EG1PreAttack;                  ///< Preattack value of the sample amplitude EG (0 - 1000 permille).
            double             EG1Attack;                     ///< Attack time of the sample amplitude EG (0.000 - 60.000s).
            double             EG1Decay1;                     ///< Decay time of the sample amplitude EG (0.000 - 60.000s).
            double             EG1Decay2;                     ///< Only if <i>EG1InfiniteSustain == false</i>: 2nd decay stage time of the sample amplitude EG (0.000 - 60.000s).
            bool               EG1InfiniteSustain;            ///< If <i>true</i>, instead of going into Decay2 phase, Decay1 level will be hold until note will be released.
            uint16_t           EG1Sustain;                    ///< Sustain value of the sample amplitude EG (0 - 1000 permille).
            double             EG1Release;                    ///< Release time of the sample amplitude EG (0.000 - 60.000s).
            bool               EG1Hold;                       ///< If <i>true</i>, Decay1 stage should be postponed until the sample reached the sample loop start.
            eg1_ctrl_t         EG1Controller;                 ///< MIDI Controller which has influence on sample amplitude EG parameters (attack, decay, release).
            bool               EG1ControllerInvert;           ///< Invert values coming from defined EG1 controller.
            uint8_t            EG1ControllerAttackInfluence;  ///< Amount EG1 Controller has influence on the EG1 Attack time (0 - 3, where 0 means off).
            uint8_t            EG1ControllerDecayInfluence;   ///< Amount EG1 Controller has influence on the EG1 Decay time (0 - 3, where 0 means off).
            uint8_t            EG1ControllerReleaseInfluence; ///< Amount EG1 Controller has influence on the EG1 Release time (0 - 3, where 0 means off).
            lfo_wave_t         LFO1WaveForm;                  ///< [gig extension]: The fundamental wave form to be used by the amplitude LFO, e.g. sine, triangle, saw, square (default: sine).
            double             LFO1Frequency;                 ///< Frequency of the sample amplitude LFO (0.10 - 10.00 Hz).
            double             LFO1Phase;                     ///< [gig extension]: Phase displacement of the amplitude LFO's wave form (0.0° - 360.0°).
            uint16_t           LFO1InternalDepth;             ///< Firm pitch of the sample amplitude LFO (0 - 1200 cents).
            uint16_t           LFO1ControlDepth;              ///< Controller depth influencing sample amplitude LFO pitch (0 - 1200 cents).
            lfo1_ctrl_t        LFO1Controller;                ///< MIDI Controller which controls sample amplitude LFO.
            bool               LFO1FlipPhase;                 ///< Inverts the polarity of the sample amplitude LFO wave, so it flips the wave form vertically.
            bool               LFO1Sync;                      ///< If set to <i>true</i> only one LFO should be used for all voices.
            // Filter Cutoff Frequency EG/LFO
            uint16_t           EG2PreAttack;                  ///< Preattack value of the filter cutoff EG (0 - 1000 permille).
            double             EG2Attack;                     ///< Attack time of the filter cutoff EG (0.000 - 60.000s).
            double             EG2Decay1;                     ///< Decay time of the filter cutoff EG (0.000 - 60.000s).
            double             EG2Decay2;                     ///< Only if <i>EG2InfiniteSustain == false</i>: 2nd stage decay time of the filter cutoff EG (0.000 - 60.000s).
            bool               EG2InfiniteSustain;            ///< If <i>true</i>, instead of going into Decay2 phase, Decay1 level will be hold until note will be released.
            uint16_t           EG2Sustain;                    ///< Sustain value of the filter cutoff EG (0 - 1000 permille).
            double             EG2Release;                    ///< Release time of the filter cutoff EG (0.000 - 60.000s).
            eg2_ctrl_t         EG2Controller;                 ///< MIDI Controller which has influence on filter cutoff EG parameters (attack, decay, release).
            bool               EG2ControllerInvert;           ///< Invert values coming from defined EG2 controller.
            uint8_t            EG2ControllerAttackInfluence;  ///< Amount EG2 Controller has influence on the EG2 Attack time (0 - 3, where 0 means off).
            uint8_t            EG2ControllerDecayInfluence;   ///< Amount EG2 Controller has influence on the EG2 Decay time (0 - 3, where 0 means off).
            uint8_t            EG2ControllerReleaseInfluence; ///< Amount EG2 Controller has influence on the EG2 Release time (0 - 3, where 0 means off).
            lfo_wave_t         LFO2WaveForm;                  ///< [gig extension]: The fundamental wave form to be used by the filter cutoff LFO, e.g. sine, triangle, saw, square (default: sine).
            double             LFO2Frequency;                 ///< Frequency of the filter cutoff LFO (0.10 - 10.00 Hz).
            double             LFO2Phase;                     ///< [gig extension]: Phase displacement of the filter cutoff LFO's wave form (0.0° - 360.0°).
            uint16_t           LFO2InternalDepth;             ///< Firm pitch of the filter cutoff LFO (0 - 1200 cents).
            uint16_t           LFO2ControlDepth;              ///< Controller depth influencing filter cutoff LFO pitch (0 - 1200).
            lfo2_ctrl_t        LFO2Controller;                ///< MIDI Controlle which controls the filter cutoff LFO.
            bool               LFO2FlipPhase;                 ///< Inverts the polarity of the filter cutoff LFO wave, so it flips the wave form vertically.
            bool               LFO2Sync;                      ///< If set to <i>true</i> only one LFO should be used for all voices.
            // Sample Pitch EG/LFO
            double             EG3Attack;                     ///< Attack time of the sample pitch EG (0.000 - 10.000s).
            int16_t            EG3Depth;                      ///< Depth of the sample pitch EG (-1200 - +1200).
            lfo_wave_t         LFO3WaveForm;                  ///< [gig extension]: The fundamental wave form to be used by the pitch LFO, e.g. sine, triangle, saw, square (default: sine).
            double             LFO3Frequency;                 ///< Frequency of the sample pitch LFO (0.10 - 10.00 Hz).
            double             LFO3Phase;                     ///< [gig extension]: Phase displacement of the pitch LFO's wave form (0.0° - 360.0°).
            int16_t            LFO3InternalDepth;             ///< Firm depth of the sample pitch LFO (-1200 - +1200 cents).
            int16_t            LFO3ControlDepth;              ///< Controller depth of the sample pitch LFO (-1200 - +1200 cents).
            lfo3_ctrl_t        LFO3Controller;                ///< MIDI Controller which controls the sample pitch LFO.
            bool               LFO3FlipPhase;                 ///< [gig extension]: Inverts the polarity of the pitch LFO wave, so it flips the wave form vertically (@b NOTE: this setting for LFO3 is a gig format extension; flipping the polarity was only available for LFO1 and LFO2 in the original Gigasampler/GigaStudio software).
            bool               LFO3Sync;                      ///< If set to <i>true</i> only one LFO should be used for all voices.
            // Filter
            bool               VCFEnabled;                    ///< If filter should be used.
            vcf_type_t         VCFType;                       ///< Defines the general filter characteristic (lowpass, highpass, bandpass, etc.).
            vcf_cutoff_ctrl_t  VCFCutoffController;           ///< Specifies which external controller has influence on the filter cutoff frequency. @deprecated Don't alter directly, use SetVCFCutoffController() instead!
            bool               VCFCutoffControllerInvert;     ///< Inverts values coming from the defined cutoff controller
            uint8_t            VCFCutoff;                     ///< Max. cutoff frequency.
            curve_type_t       VCFVelocityCurve;              ///< Defines a transformation curve for the incoming velocity values, affecting the VCF. @deprecated Don't alter directly, use SetVCFVelocityCurve() instead!
            uint8_t            VCFVelocityScale;              ///< (0-127) Amount velocity controls VCF cutoff frequency (only if no other VCF cutoff controller is defined, otherwise this is the minimum cutoff). @deprecated Don't alter directly, use SetVCFVelocityScale() instead!
            uint8_t            VCFVelocityDynamicRange;       ///< 0x04 = lowest, 0x00 = highest . @deprecated Don't alter directly, use SetVCFVelocityDynamicRange() instead!
            uint8_t            VCFResonance;                  ///< Firm internal filter resonance weight.
            bool               VCFResonanceDynamic;           ///< If <i>true</i>: Increases the resonance Q according to changes of controllers that actually control the VCF cutoff frequency (EG2, ext. VCF MIDI controller).
            vcf_res_ctrl_t     VCFResonanceController;        ///< Specifies which external controller has influence on the filter resonance Q.
            bool               VCFKeyboardTracking;           ///< If <i>true</i>: VCF cutoff frequence will be dependend to the note key position relative to the defined breakpoint value.
            uint8_t            VCFKeyboardTrackingBreakpoint; ///< See VCFKeyboardTracking (0 - 127).
            // Key Velocity Transformations
            curve_type_t       VelocityResponseCurve;         ///< Defines a transformation curve to the incoming velocity values affecting amplitude (usually you don't have to interpret this parameter, use GetVelocityAttenuation() instead). @deprecated Don't alter directly, use SetVelocityResponseCurve() instead!
            uint8_t            VelocityResponseDepth;         ///< Dynamic range of velocity affecting amplitude (0 - 4) (usually you don't have to interpret this parameter, use GetVelocityAttenuation() instead). @deprecated Don't alter directly, use SetVelocityResponseDepth() instead!
            uint8_t            VelocityResponseCurveScaling;  ///< 0 - 127 (usually you don't have to interpret this parameter, use GetVelocityAttenuation() instead). @deprecated Don't alter directly, use SetVelocityResponseCurveScaling() instead!
            curve_type_t       ReleaseVelocityResponseCurve;  ///< Defines a transformation curve to the incoming release veloctiy values affecting envelope times. @deprecated Don't alter directly, use SetReleaseVelocityResponseCurve() instead!
            uint8_t            ReleaseVelocityResponseDepth;  ///< Dynamic range of release velocity affecting envelope time (0 - 4). @deprecated Don't alter directly, use SetReleaseVelocityResponseDepth() instead!
            uint8_t            ReleaseTriggerDecay;           ///< 0 - 8
            // Mix / Layer
            crossfade_t        Crossfade;
            bool               PitchTrack;                    ///< If <i>true</i>: sample will be pitched according to the key position (this will be disabled for drums for example).
            dim_bypass_ctrl_t  DimensionBypass;               ///< If defined, the MIDI controller can switch on/off the dimension in realtime.
            int8_t             Pan;                           ///< Panorama / Balance (-64..0..63 <-> left..middle..right)
            bool               SelfMask;                      ///< If <i>true</i>: high velocity notes will stop low velocity notes at the same note, with that you can save voices that wouldn't be audible anyway.
            attenuation_ctrl_t AttenuationController;         ///< MIDI Controller which has influence on the volume level of the sample (or entire sample group).
            bool               InvertAttenuationController;   ///< Inverts the values coming from the defined Attenuation Controller.
            uint8_t            AttenuationControllerThreshold;///< 0-127
            uint8_t            ChannelOffset;                 ///< Audio output where the audio signal of the dimension region should be routed to (0 - 9).
            bool               SustainDefeat;                 ///< If <i>true</i>: Sustain pedal will not hold a note.
            bool               MSDecode;                      ///< Gigastudio flag: defines if Mid Side Recordings should be decoded.
            uint16_t           SampleStartOffset;             ///< Number of samples the sample start should be moved (0 - 2000).
            double             SampleAttenuation;             ///< Sample volume (calculated from DLS::Sampler::Gain)
            uint8_t            DimensionUpperLimits[8];       ///< gig3: defines the upper limit of the dimension values for this dimension region. In case you wondered why this is defined on DimensionRegion level and not on Region level: the zone sizes (upper limits) of the velocity dimension can indeed differ in the individual dimension regions, depending on which zones of the other dimension types are currently selected. So this is exceptional for the velocity dimension only. All other dimension types have the same dimension zone sizes for every single DimensionRegion (of the sample Region).
            eg_opt_t           EG1Options;                    ///< [gig extension]: Behavior options which should be used for envelope generator 1 (volume amplitude EG).
            eg_opt_t           EG2Options;                    ///< [gig extension]: Behavior options which should be used for envelope generator 2 (filter cutoff EG).
            sust_rel_trg_t     SustainReleaseTrigger;         ///< [gig extension]: Whether a sustain pedal up event shall play release trigger sample.
            bool               NoNoteOffReleaseTrigger;       ///< [gig extension]: If @c true then don't play a release trigger sample on MIDI note-off events.

            // derived attributes from DLS::Sampler
            using DLS::Sampler::UnityNote;
            using DLS::Sampler::FineTune;
            using DLS::Sampler::Gain;
            using DLS::Sampler::SampleLoops;
            using DLS::Sampler::pSampleLoops;

            // own methods
            double GetVelocityAttenuation(uint8_t MIDIKeyVelocity);
            double GetVelocityRelease(uint8_t MIDIKeyVelocity);
            double GetVelocityCutoff(uint8_t MIDIKeyVelocity);
            void SetVelocityResponseCurve(curve_type_t curve);
            void SetVelocityResponseDepth(uint8_t depth);
            void SetVelocityResponseCurveScaling(uint8_t scaling);
            void SetReleaseVelocityResponseCurve(curve_type_t curve);
            void SetReleaseVelocityResponseDepth(uint8_t depth);
            void SetVCFCutoffController(vcf_cutoff_ctrl_t controller);
            void SetVCFVelocityCurve(curve_type_t curve);
            void SetVCFVelocityDynamicRange(uint8_t range);
            void SetVCFVelocityScale(uint8_t scaling);
            Region* GetParent() const;
            // derived methods
            using DLS::Sampler::AddSampleLoop;
            using DLS::Sampler::DeleteSampleLoop;
            // overridden methods
            virtual void SetGain(int32_t gain) OVERRIDE;
            virtual void UpdateChunks(progress_t* pProgress) OVERRIDE;
            virtual void CopyAssign(const DimensionRegion* orig);
        protected:
            uint8_t* VelocityTable; ///< For velocity dimensions with custom defined zone ranges only: used for fast converting from velocity MIDI value to dimension bit number.
            DimensionRegion(Region* pParent, RIFF::List* _3ewl);
            DimensionRegion(RIFF::List* _3ewl, const DimensionRegion& src);
           ~DimensionRegion();
            void CopyAssign(const DimensionRegion* orig, const std::map<Sample*,Sample*>* mSamples);
            void serialize(Serialization::Archive* archive);
            friend class Region;
            friend class Serialization::Archive;
        private:
            typedef enum { ///< Used to decode attenuation, EG1 and EG2 controller
                // official leverage controllers as they were defined in the original Gigasampler/GigaStudio format:
                _lev_ctrl_none              = 0x00,
                _lev_ctrl_modwheel          = 0x03, ///< Modulation Wheel (MIDI Controller 1)
                _lev_ctrl_breath            = 0x05, ///< Breath Controller (Coarse, MIDI Controller 2)
                _lev_ctrl_foot              = 0x07, ///< Foot Pedal (Coarse, MIDI Controller 4)
                _lev_ctrl_effect1           = 0x0d, ///< Effect Controller 1 (Coarse, MIDI Controller 12)
                _lev_ctrl_effect2           = 0x0f, ///< Effect Controller 2 (Coarse, MIDI Controller 13)
                _lev_ctrl_genpurpose1       = 0x11, ///< General Purpose Controller 1 (Slider, MIDI Controller 16)
                _lev_ctrl_genpurpose2       = 0x13, ///< General Purpose Controller 2 (Slider, MIDI Controller 17)
                _lev_ctrl_genpurpose3       = 0x15, ///< General Purpose Controller 3 (Slider, MIDI Controller 18)
                _lev_ctrl_genpurpose4       = 0x17, ///< General Purpose Controller 4 (Slider, MIDI Controller 19)
                _lev_ctrl_portamentotime    = 0x0b, ///< Portamento Time (Coarse, MIDI Controller 5)
                _lev_ctrl_sustainpedal      = 0x01, ///< Sustain Pedal (MIDI Controller 64)
                _lev_ctrl_portamento        = 0x19, ///< Portamento (MIDI Controller 65)
                _lev_ctrl_sostenutopedal    = 0x1b, ///< Sostenuto Pedal (MIDI Controller 66)
                _lev_ctrl_softpedal         = 0x09, ///< Soft Pedal (MIDI Controller 67)
                _lev_ctrl_genpurpose5       = 0x1d, ///< General Purpose Controller 5 (Button, MIDI Controller 80)
                _lev_ctrl_genpurpose6       = 0x1f, ///< General Purpose Controller 6 (Button, MIDI Controller 81)
                _lev_ctrl_genpurpose7       = 0x21, ///< General Purpose Controller 7 (Button, MIDI Controller 82)
                _lev_ctrl_genpurpose8       = 0x23, ///< General Purpose Controller 8 (Button, MIDI Controller 83)
                _lev_ctrl_effect1depth      = 0x25, ///< Effect 1 Depth (MIDI Controller 91)
                _lev_ctrl_effect2depth      = 0x27, ///< Effect 2 Depth (MIDI Controller 92)
                _lev_ctrl_effect3depth      = 0x29, ///< Effect 3 Depth (MIDI Controller 93)
                _lev_ctrl_effect4depth      = 0x2b, ///< Effect 4 Depth (MIDI Controller 94)
                _lev_ctrl_effect5depth      = 0x2d, ///< Effect 5 Depth (MIDI Controller 95)
                _lev_ctrl_channelaftertouch = 0x2f, ///< Channel Key Pressure
                _lev_ctrl_velocity          = 0xff, ///< Key Velocity

                // format extension (these controllers are so far only supported by LinuxSampler & gigedit) they will *NOT* work with Gigasampler/GigaStudio !
                // (the assigned values here are their official MIDI CC number plus the highest bit set):
                _lev_ctrl_CC3_EXT           = 0x83, ///< MIDI Controller 3 [gig format extension]

                _lev_ctrl_CC6_EXT           = 0x86, ///< Data Entry MSB (MIDI Controller 6) [gig format extension]
                _lev_ctrl_CC7_EXT           = 0x87, ///< Channel Volume (MIDI Controller 7) [gig format extension]
                _lev_ctrl_CC8_EXT           = 0x88, ///< Balance (MIDI Controller 8) [gig format extension]
                _lev_ctrl_CC9_EXT           = 0x89, ///< MIDI Controller 9 [gig format extension]
                _lev_ctrl_CC10_EXT          = 0x8a, ///< Pan (MIDI Controller 10) [gig format extension]
                _lev_ctrl_CC11_EXT          = 0x8b, ///< Expression Controller (MIDI Controller 11) [gig format extension]

                _lev_ctrl_CC14_EXT          = 0x8e, ///< MIDI Controller 14 [gig format extension]
                _lev_ctrl_CC15_EXT          = 0x8f, ///< MIDI Controller 15 [gig format extension]

                _lev_ctrl_CC20_EXT          = 0x94, ///< MIDI Controller 20 [gig format extension]
                _lev_ctrl_CC21_EXT          = 0x95, ///< MIDI Controller 21 [gig format extension]
                _lev_ctrl_CC22_EXT          = 0x96, ///< MIDI Controller 22 [gig format extension]
                _lev_ctrl_CC23_EXT          = 0x97, ///< MIDI Controller 23 [gig format extension]
                _lev_ctrl_CC24_EXT          = 0x98, ///< MIDI Controller 24 [gig format extension]
                _lev_ctrl_CC25_EXT          = 0x99, ///< MIDI Controller 25 [gig format extension]
                _lev_ctrl_CC26_EXT          = 0x9a, ///< MIDI Controller 26 [gig format extension]
                _lev_ctrl_CC27_EXT          = 0x9b, ///< MIDI Controller 27 [gig format extension]
                _lev_ctrl_CC28_EXT          = 0x9c, ///< MIDI Controller 28 [gig format extension]
                _lev_ctrl_CC29_EXT          = 0x9d, ///< MIDI Controller 29 [gig format extension]
                _lev_ctrl_CC30_EXT          = 0x9e, ///< MIDI Controller 30 [gig format extension]
                _lev_ctrl_CC31_EXT          = 0x9f, ///< MIDI Controller 31 [gig format extension]

                _lev_ctrl_CC68_EXT          = 0xc4, ///< Legato Footswitch (MIDI Controller 68) [gig format extension]
                _lev_ctrl_CC69_EXT          = 0xc5, ///< Hold 2 (MIDI Controller 69) [gig format extension]
                _lev_ctrl_CC70_EXT          = 0xc6, ///< Sound Ctrl. 1 - Sound Variation (MIDI Controller 70) [gig format extension]
                _lev_ctrl_CC71_EXT          = 0xc7, ///< Sound Ctrl. 2 - Timbre (MIDI Controller 71) [gig format extension]
                _lev_ctrl_CC72_EXT          = 0xc8, ///< Sound Ctrl. 3 - Release Time (MIDI Controller 72) [gig format extension]
                _lev_ctrl_CC73_EXT          = 0xc9, ///< Sound Ctrl. 4 - Attack Time (MIDI Controller 73) [gig format extension]
                _lev_ctrl_CC74_EXT          = 0xca, ///< Sound Ctrl. 5 - Brightness (MIDI Controller 74) [gig format extension]
                _lev_ctrl_CC75_EXT          = 0xcb, ///< Sound Ctrl. 6 - Decay Time (MIDI Controller 75) [gig format extension]
                _lev_ctrl_CC76_EXT          = 0xcc, ///< Sound Ctrl. 7 - Vibrato Rate (MIDI Controller 76) [gig format extension]
                _lev_ctrl_CC77_EXT          = 0xcd, ///< Sound Ctrl. 8 - Vibrato Depth (MIDI Controller 77) [gig format extension]
                _lev_ctrl_CC78_EXT          = 0xce, ///< Sound Ctrl. 9 - Vibrato Delay (MIDI Controller 78) [gig format extension]
                _lev_ctrl_CC79_EXT          = 0xcf, ///< Sound Ctrl. 10 (MIDI Controller 79) [gig format extension]

                _lev_ctrl_CC84_EXT          = 0xd4, ///< Portamento Control (MIDI Controller 84) [gig format extension]
                _lev_ctrl_CC85_EXT          = 0xd5, ///< MIDI Controller 85 [gig format extension]
                _lev_ctrl_CC86_EXT          = 0xd6, ///< MIDI Controller 86 [gig format extension]
                _lev_ctrl_CC87_EXT          = 0xd7, ///< MIDI Controller 87 [gig format extension]

                _lev_ctrl_CC89_EXT          = 0xd9, ///< MIDI Controller 89 [gig format extension]
                _lev_ctrl_CC90_EXT          = 0xda, ///< MIDI Controller 90 [gig format extension]

                _lev_ctrl_CC96_EXT          = 0xe0, ///< Data Increment (MIDI Controller 96) [gig format extension]
                _lev_ctrl_CC97_EXT          = 0xe1, ///< Data Decrement (MIDI Controller 97) [gig format extension]

                _lev_ctrl_CC102_EXT         = 0xe6, ///< MIDI Controller 102 [gig format extension]
                _lev_ctrl_CC103_EXT         = 0xe7, ///< MIDI Controller 103 [gig format extension]
                _lev_ctrl_CC104_EXT         = 0xe8, ///< MIDI Controller 104 [gig format extension]
                _lev_ctrl_CC105_EXT         = 0xe9, ///< MIDI Controller 105 [gig format extension]
                _lev_ctrl_CC106_EXT         = 0xea, ///< MIDI Controller 106 [gig format extension]
                _lev_ctrl_CC107_EXT         = 0xeb, ///< MIDI Controller 107 [gig format extension]
                _lev_ctrl_CC108_EXT         = 0xec, ///< MIDI Controller 108 [gig format extension]
                _lev_ctrl_CC109_EXT         = 0xed, ///< MIDI Controller 109 [gig format extension]
                _lev_ctrl_CC110_EXT         = 0xee, ///< MIDI Controller 110 [gig format extension]
                _lev_ctrl_CC111_EXT         = 0xef, ///< MIDI Controller 111 [gig format extension]
                _lev_ctrl_CC112_EXT         = 0xf0, ///< MIDI Controller 112 [gig format extension]
                _lev_ctrl_CC113_EXT         = 0xf1, ///< MIDI Controller 113 [gig format extension]
                _lev_ctrl_CC114_EXT         = 0xf2, ///< MIDI Controller 114 [gig format extension]
                _lev_ctrl_CC115_EXT         = 0xf3, ///< MIDI Controller 115 [gig format extension]
                _lev_ctrl_CC116_EXT         = 0xf4, ///< MIDI Controller 116 [gig format extension]
                _lev_ctrl_CC117_EXT         = 0xf5, ///< MIDI Controller 117 [gig format extension]
                _lev_ctrl_CC118_EXT         = 0xf6, ///< MIDI Controller 118 [gig format extension]
                _lev_ctrl_CC119_EXT         = 0xf7  ///< MIDI Controller 119 [gig format extension]
            } _lev_ctrl_t;
            typedef std::map<uint32_t, double*> VelocityTableMap;

            static size_t            Instances;                  ///< Number of DimensionRegion instances.
            static VelocityTableMap* pVelocityTables;            ///< Contains the tables corresponding to the various velocity parameters (VelocityResponseCurve and VelocityResponseDepth).
            double*                  pVelocityAttenuationTable;  ///< Points to the velocity table corresponding to the velocity parameters of this DimensionRegion.
            double*                  pVelocityReleaseTable;      ///< Points to the velocity table corresponding to the release velocity parameters of this DimensionRegion
            double*                  pVelocityCutoffTable;       ///< Points to the velocity table corresponding to the filter velocity parameters of this DimensionRegion
            Region*                  pRegion;

            leverage_ctrl_t DecodeLeverageController(_lev_ctrl_t EncodedController);
            _lev_ctrl_t     EncodeLeverageController(leverage_ctrl_t DecodedController);
            double* GetReleaseVelocityTable(curve_type_t releaseVelocityResponseCurve, uint8_t releaseVelocityResponseDepth);
            double* GetCutoffVelocityTable(curve_type_t vcfVelocityCurve, uint8_t vcfVelocityDynamicRange, uint8_t vcfVelocityScale, vcf_cutoff_ctrl_t vcfCutoffController);
            double* GetVelocityTable(curve_type_t curveType, uint8_t depth, uint8_t scaling);
            double* CreateVelocityTable(curve_type_t curveType, uint8_t depth, uint8_t scaling);
            bool UsesAnyGigFormatExtension() const;
    };

    /** @brief Encapsulates sample waves of Gigasampler/GigaStudio files used for playback.
     *
     * This class provides access to the actual audio sample data of a
     * Gigasampler/GigaStudio file. Along to the actual sample data, it also
     * provides access to the sample's meta informations like bit depth,
     * sample rate, encoding type, but also loop informations. The latter may be
     * used by instruments for resembling sounds with arbitary note lengths.
     *
     * In case you created a new sample with File::AddSample(), you should
     * first update all attributes with the desired meta informations
     * (amount of channels, bit depth, sample rate, etc.), then call
     * Resize() with the desired sample size, followed by File::Save(), this
     * will create the mandatory RIFF chunk which will hold the sample wave
     * data and / or resize the file so you will be able to Write() the
     * sample data directly to disk.
     *
     * @e Caution: for gig synthesis, most looping relevant information are
     * retrieved from the respective DimensionRegon instead from the Sample
     * itself. This was made for allowing different loop definitions for the
     * same sample under different conditions.
     *
     * Since the gig format was designed as extension to the DLS file format,
     * this class is derived from the DLS::Sample class. So also refer to
     * DLS::Sample for additional informations, class attributes and methods.
     */
    class Sample : public DLS::Sample {
        public:
            uint32_t       Manufacturer;      ///< Specifies the MIDI Manufacturer's Association (MMA) Manufacturer code for the sampler intended to receive this file's waveform. If no particular manufacturer is to be specified, a value of 0 should be used.
            uint32_t       Product;           ///< Specifies the MIDI model ID defined by the manufacturer corresponding to the Manufacturer field. If no particular manufacturer's product is to be specified, a value of 0 should be used.
            uint32_t       SamplePeriod;      ///< Specifies the duration of time that passes during the playback of one sample in nanoseconds (normally equal to 1 / Samples Per Second, where Samples Per Second is the value found in the format chunk), don't bother to update this attribute, it won't be saved.
            uint32_t       MIDIUnityNote;     ///< Specifies the musical note at which the sample will be played at it's original sample rate.
            uint32_t       FineTune;          ///< Specifies the fraction of a semitone up from the specified MIDI unity note field. A value of 0x80000000 means 1/2 semitone (50 cents) and a value of 0x00000000 means no fine tuning between semitones.
            smpte_format_t SMPTEFormat;       ///< Specifies the Society of Motion Pictures and Television E time format used in the following <i>SMPTEOffset</i> field. If a value of 0 is set, <i>SMPTEOffset</i> should also be set to 0.
            uint32_t       SMPTEOffset;       ///< The SMPTE Offset value specifies the time offset to be used for the synchronization / calibration to the first sample in the waveform. This value uses a format of 0xhhmmssff where hh is a signed value that specifies the number of hours (-23 to 23), mm is an unsigned value that specifies the number of minutes (0 to 59), ss is an unsigned value that specifies the number of seconds (0 to 59) and ff is an unsigned value that specifies the number of frames (0 to -1).
            uint32_t       Loops;             ///< @e Caution: Use the respective field in the DimensionRegion instead of this one! (Intended purpose: Number of defined sample loops. So far only seen single loops in gig files - please report if you encounter more!)
            uint32_t       LoopID;            ///< Specifies the unique ID that corresponds to one of the defined cue points in the cue point list (only if Loops > 0), as the Gigasampler format only allows one loop definition at the moment, this attribute isn't really useful for anything.
            loop_type_t    LoopType;          ///< @e Caution: Use the respective field in the DimensionRegion instead of this one! (Intended purpose: The type field defines how the waveform samples will be looped.)
            uint32_t       LoopStart;         ///< @e Caution: Use the respective field in the DimensionRegion instead of this one! (Intended purpose: The start value specifies the offset [in sample points] in the waveform data of the first sample to be played in the loop [only if Loops > 0].)
            uint32_t       LoopEnd;           ///< @e Caution: Use the respective field in the DimensionRegion instead of this one! (Intended purpose: The end value specifies the offset [in sample points] in the waveform data which represents the end of the loop [only if Loops > 0].)
            uint32_t       LoopSize;          ///< @e Caution: Use the respective fields in the DimensionRegion instead of this one! (Intended purpose: Length of the looping area [in sample points] which is equivalent to @code LoopEnd - LoopStart @endcode.)
            uint32_t       LoopFraction;      ///< The fractional value specifies a fraction of a sample at which to loop. This allows a loop to be fine tuned at a resolution greater than one sample. A value of 0 means no fraction, a value of 0x80000000 means 1/2 of a sample length. 0xFFFFFFFF is the smallest fraction of a sample that can be represented.
            uint32_t       LoopPlayCount;     ///< Number of times the loop should be played (a value of 0 = infinite).
            bool           Compressed;        ///< If the sample wave is compressed (probably just interesting for instrument and sample editors, as this library already handles the decompression in it's sample access methods anyway).
            uint32_t       TruncatedBits;     ///< For 24-bit compressed samples only: number of bits truncated during compression (0, 4 or 6)
            bool           Dithered;          ///< For 24-bit compressed samples only: if dithering was used during compression with bit reduction

            // own methods
            buffer_t      LoadSampleData();
            buffer_t      LoadSampleData(file_offset_t SampleCount);
            buffer_t      LoadSampleDataWithNullSamplesExtension(uint NullSamplesCount);
            buffer_t      LoadSampleDataWithNullSamplesExtension(file_offset_t SampleCount, uint NullSamplesCount);
            buffer_t      GetCache();
            // own static methods
            static buffer_t CreateDecompressionBuffer(file_offset_t MaxReadSize);
            static void     DestroyDecompressionBuffer(buffer_t& DecompressionBuffer);
            // overridden methods
            void          ReleaseSampleData();
            void          Resize(file_offset_t NewSize);
            file_offset_t SetPos(file_offset_t SampleCount, RIFF::stream_whence_t Whence = RIFF::stream_start);
            file_offset_t GetPos() const;
            file_offset_t Read(void* pBuffer, file_offset_t SampleCount, buffer_t* pExternalDecompressionBuffer = NULL);
            file_offset_t ReadAndLoop(void* pBuffer, file_offset_t SampleCount, playback_state_t* pPlaybackState, DimensionRegion* pDimRgn, buffer_t* pExternalDecompressionBuffer = NULL);
            file_offset_t Write(void* pBuffer, file_offset_t SampleCount);
            Group*        GetGroup() const;
            virtual void  UpdateChunks(progress_t* pProgress) OVERRIDE;
            void CopyAssignMeta(const Sample* orig);
            void CopyAssignWave(const Sample* orig);
            uint32_t GetWaveDataCRC32Checksum();
            bool VerifyWaveData(uint32_t* pActually = NULL);
        protected:
            static size_t        Instances;               ///< Number of instances of class Sample.
            static buffer_t      InternalDecompressionBuffer; ///< Buffer used for decompression as well as for truncation of 24 Bit -> 16 Bit samples.
            Group*               pGroup;                  ///< pointer to the Group this sample belongs to (always not-NULL)
            file_offset_t        FrameOffset;             ///< Current offset (sample points) in current sample frame (for decompression only).
            file_offset_t*       FrameTable;              ///< For positioning within compressed samples only: stores the offset values for each frame.
            file_offset_t        SamplePos;               ///< For compressed samples only: stores the current position (in sample points).
            file_offset_t        SamplesInLastFrame;      ///< For compressed samples only: length of the last sample frame.
            file_offset_t        WorstCaseFrameSize;      ///< For compressed samples only: size (in bytes) of the largest possible sample frame.
            file_offset_t        SamplesPerFrame;         ///< For compressed samples only: number of samples in a full sample frame.
            buffer_t             RAMCache;                ///< Buffers samples (already uncompressed) in RAM.
            unsigned long        FileNo;                  ///< File number (> 0 when sample is stored in an extension file, 0 when it's in the gig)
            RIFF::Chunk*         pCk3gix;
            RIFF::Chunk*         pCkSmpl;
            uint32_t             crc;                     ///< Reflects CRC-32 checksum of the raw sample data at the last time when the sample's raw wave form data has been modified consciously by the user by calling Write().

            Sample(File* pFile, RIFF::List* waveList, file_offset_t WavePoolOffset, unsigned long fileNo = 0, int index = -1);
           ~Sample();
            uint32_t CalculateWaveDataChecksum();

            // Guess size (in bytes) of a compressed sample
            inline file_offset_t GuessSize(file_offset_t samples) {
                // 16 bit: assume all frames are compressed - 1 byte
                // per sample and 5 bytes header per 2048 samples

                // 24 bit: assume next best compression rate - 1.5
                // bytes per sample and 13 bytes header per 256
                // samples
                const file_offset_t size =
                    BitDepth == 24 ? samples + (samples >> 1) + (samples >> 8) * 13
                                   : samples + (samples >> 10) * 5;
                // Double for stereo and add one worst case sample
                // frame
                return (Channels == 2 ? size << 1 : size) + WorstCaseFrameSize;
            }

            // Worst case amount of sample points that can be read with the
            // given decompression buffer.
            inline file_offset_t WorstCaseMaxSamples(buffer_t* pDecompressionBuffer) {
                return (file_offset_t) ((float)pDecompressionBuffer->Size / (float)WorstCaseFrameSize * (float)SamplesPerFrame);
            }
        private:
            void ScanCompressedSample();
            friend class File;
            friend class Region;
            friend class Group; // allow to modify protected member pGroup
    };

    // TODO: <3dnl> list not used yet - not important though (just contains optional descriptions for the dimensions)
    /** @brief Defines Region information of a Gigasampler/GigaStudio instrument.
     *
     * A Region reflects a consecutive area (key range) on the keyboard. The
     * individual regions in the gig format may not overlap with other regions
     * (of the same instrument that is). Further, in the gig format a Region is
     * merely a container for DimensionRegions (a.k.a. "Cases"). The Region
     * itself does not provide the sample mapping or articulation informations
     * used, even though the data structures of regions indeed provide such
     * informations. The latter is however just of historical nature, because
     * the gig file format was derived from the DLS file format.
     *
     * Each Region consists of at least one or more DimensionRegions. The actual
     * amount of DimensionRegions depends on which kind of "dimensions" are
     * defined for this region, and on the split / zone amount for each of those
     * dimensions.
     *
     * Since the gig format was designed as extension to the DLS file format,
     * this class is derived from the DLS::Region class. So also refer to
     * DLS::Region for additional informations, class attributes and methods.
     */
    class Region : public DLS::Region {
        public:
            unsigned int            Dimensions;               ///< Number of defined dimensions, do not alter!
            dimension_def_t         pDimensionDefinitions[8]; ///< Defines the five (gig2) or eight (gig3) possible dimensions (the dimension's controller and number of bits/splits). Use AddDimension() and DeleteDimension() to create a new dimension or delete an existing one.
            uint32_t                DimensionRegions;         ///< Total number of DimensionRegions this Region contains, do not alter!
            DimensionRegion*        pDimensionRegions[256];   ///< Pointer array to the 32 (gig2) or 256 (gig3) possible dimension regions (reflects NULL for dimension regions not in use). Avoid to access the array directly and better use GetDimensionRegionByValue() instead, but of course in some cases it makes sense to use the array (e.g. iterating through all DimensionRegions). Use AddDimension() and DeleteDimension() to create a new dimension or delete an existing one (which will create or delete the respective dimension region(s) automatically).
            unsigned int            Layers;                   ///< Amount of defined layers (1 - 32). A value of 1 actually means no layering, a value > 1 means there is Layer dimension. The same information can of course also be obtained by accessing pDimensionDefinitions. Do not alter this value!

            // own methods
            DimensionRegion* GetDimensionRegionByValue(const uint DimValues[8]);
            DimensionRegion* GetDimensionRegionByBit(const uint8_t DimBits[8]);
            int              GetDimensionRegionIndexByValue(const uint DimValues[8]);
            Sample*          GetSample();
            void             AddDimension(dimension_def_t* pDimDef);
            void             DeleteDimension(dimension_def_t* pDimDef);
            dimension_def_t* GetDimensionDefinition(dimension_t type);
            void             DeleteDimensionZone(dimension_t type, int zone);
            void             SplitDimensionZone(dimension_t type, int zone);
            void             SetDimensionType(dimension_t oldType, dimension_t newType);
            // overridden methods
            virtual void     SetKeyRange(uint16_t Low, uint16_t High) OVERRIDE;
            virtual void     UpdateChunks(progress_t* pProgress) OVERRIDE;
            virtual void     CopyAssign(const Region* orig);
        protected:
            Region(Instrument* pInstrument, RIFF::List* rgnList);
            void LoadDimensionRegions(RIFF::List* rgn);
            void UpdateVelocityTable();
            Sample* GetSampleFromWavePool(unsigned int WavePoolTableIndex, progress_t* pProgress = NULL);
            void CopyAssign(const Region* orig, const std::map<Sample*,Sample*>* mSamples);
            DimensionRegion* GetDimensionRegionByBit(const std::map<dimension_t,int>& DimCase);
           ~Region();
            friend class Instrument;
        private:
            bool UsesAnyGigFormatExtension() const;
    };

    /** @brief Abstract base class for all MIDI rules.
     *
     * Note: Instead of using MIDI rules, we recommend you using real-time
     * instrument scripts instead. Read about the reasons below.
     *
     * MIDI Rules (also called "iMIDI rules" or "intelligent MIDI rules") were
     * introduced with GigaStudio 4 as an attempt to increase the power of
     * potential user controls over sounds. At that point other samplers already
     * supported certain powerful user control features, which were not possible
     * with GigaStudio yet. For example triggering new notes by MIDI CC
     * controller.
     *
     * Such extended features however were usually implemented by other samplers
     * by requiring the sound designer to write an instrument script which the
     * designer would then bundle with the respective instrument file. Such
     * scripts are essentially text files, using a very specific programming 
     * language for the purpose of controlling the sampler in real-time. Since
     * however musicians are not typically keen to writing such cumbersome
     * script files, the GigaStudio designers decided to implement such extended
     * features completely without instrument scripts. Instead they created a
     * set of rules, which could be defined and altered conveniently by mouse
     * clicks in GSt's instrument editor application. The downside of this
     * overall approach however, was that those MIDI rules were very limited in
     * practice. As sound designer you easily came across the possiblities such
     * MIDI rules were able to offer.
     *
     * Due to such severe use case constraints, support for MIDI rules is quite
     * limited in libgig. At the moment only the "Control Trigger", "Alternator"
     * and the "Legato" MIDI rules are supported by libgig. Consequently the
     * graphical instrument editor application gigedit just supports the
     * "Control Trigger" and "Legato" MIDI rules, and LinuxSampler even does not
     * support any MIDI rule type at all and LinuxSampler probably will not
     * support MIDI rules in future either.
     *
     * Instead of using MIDI rules, we introduced real-time instrument scripts
     * as extension to the original GigaStudio file format. This script based
     * solution is much more powerful than MIDI rules and is already supported
     * by libgig, gigedit and LinuxSampler.
     *
     * @deprecated Just provided for backward compatiblity, use Script for new
     *             instruments instead.
     */
    class MidiRule {
        public:
            virtual ~MidiRule() { }
        protected:
            virtual void UpdateChunks(uint8_t* pData) const = 0;
            friend class Instrument;
    };

    /** @brief MIDI rule for triggering notes by control change events.
     *
     * A "Control Trigger MIDI rule" allows to trigger new notes by sending MIDI
     * control change events to the sampler.
     *
     * Note: "Control Trigger" MIDI rules are only supported by gigedit, but not
     * by LinuxSampler. We recommend you using real-time instrument scripts
     * instead. Read more about the details and reasons for this in the
     * description of the MidiRule base class.
     *
     * @deprecated Just provided for backward compatiblity, use Script for new
     *             instruments instead. See description of MidiRule for details.
     */
    class MidiRuleCtrlTrigger : public MidiRule {
        public:
            uint8_t ControllerNumber;   ///< MIDI controller number.
            uint8_t Triggers;           ///< Number of triggers.
            struct trigger_t {
                uint8_t TriggerPoint;   ///< The CC value to pass for the note to be triggered.
                bool    Descending;     ///< If the change in CC value should be downwards.
                uint8_t VelSensitivity; ///< How sensitive the velocity should be to the speed of the controller change.
                uint8_t Key;            ///< Key to trigger.
                bool    NoteOff;        ///< If a note off should be triggered instead of a note on.
                uint8_t Velocity;       ///< Velocity of the note to trigger. 255 means that velocity should depend on the speed of the controller change.
                bool    OverridePedal;  ///< If a note off should be triggered even if the sustain pedal is down.
            } pTriggers[32];

        protected:
            MidiRuleCtrlTrigger(RIFF::Chunk* _3ewg);
            MidiRuleCtrlTrigger();
            void UpdateChunks(uint8_t* pData) const OVERRIDE;
            friend class Instrument;
    };

    /** @brief MIDI rule for instruments with legato samples.
     *
     * A "Legato MIDI rule" allows playing instruments resembling the legato
     * playing technique. In the past such legato articulations were tried to be
     * simulated by pitching the samples of the instrument. However since
     * usually a high amount of pitch is needed for legatos, this always sounded
     * very artificial and unrealistic. The "Legato MIDI rule" thus uses another
     * approach. Instead of pitching the samples, it allows the sound designer
     * to bundle separate, additional samples for the individual legato
     * situations and the legato rules defined which samples to be played in
     * which situation.
     *
     * Note: "Legato MIDI rules" are only supported by gigedit, but not
     * by LinuxSampler. We recommend you using real-time instrument scripts
     * instead. Read more about the details and reasons for this in the
     * description of the MidiRule base class.
     *
     * @deprecated Just provided for backward compatiblity, use Script for new
     *             instruments instead. See description of MidiRule for details.
     */
    class MidiRuleLegato : public MidiRule {
        public:
            uint8_t LegatoSamples;     ///< Number of legato samples per key in each direction (always 12)
            bool BypassUseController;  ///< If a controller should be used to bypass the sustain note
            uint8_t BypassKey;         ///< Key to be used to bypass the sustain note
            uint8_t BypassController;  ///< Controller to be used to bypass the sustain note
            uint16_t ThresholdTime;    ///< Maximum time (ms) between two notes that should be played legato
            uint16_t ReleaseTime;      ///< Release time
            range_t KeyRange;          ///< Key range for legato notes
            uint8_t ReleaseTriggerKey; ///< Key triggering release samples
            uint8_t AltSustain1Key;    ///< Key triggering alternate sustain samples
            uint8_t AltSustain2Key;    ///< Key triggering a second set of alternate sustain samples

        protected:
            MidiRuleLegato(RIFF::Chunk* _3ewg);
            MidiRuleLegato();
            void UpdateChunks(uint8_t* pData) const OVERRIDE;
            friend class Instrument;
    };

    /** @brief MIDI rule to automatically cycle through specified sequences of different articulations.
     *
     * The instrument must be using the smartmidi dimension.
     *
     * Note: "Alternator" MIDI rules are neither supported by gigedit nor by
     * LinuxSampler. We recommend you using real-time instrument scripts
     * instead. Read more about the details and reasons for this in the
     * description of the MidiRule base class.
     *
     * @deprecated Just provided for backward compatiblity, use Script for new
     *             instruments instead. See description of MidiRule for details.
     */
    class MidiRuleAlternator : public MidiRule {
        public:
            uint8_t Articulations;     ///< Number of articulations in the instrument
            String pArticulations[32]; ///< Names of the articulations

            range_t PlayRange;         ///< Key range of the playable keys in the instrument

            uint8_t Patterns;          ///< Number of alternator patterns
            struct pattern_t {
                String Name;           ///< Name of the pattern
                int Size;              ///< Number of steps in the pattern
                const uint8_t& operator[](int i) const { /// Articulation to play
                    return data[i];
                }
                uint8_t& operator[](int i) {
                    return data[i];
                }
            private:
                uint8_t data[32];
            } pPatterns[32];           ///< A pattern is a sequence of articulation numbers

            typedef enum {
                selector_none,
                selector_key_switch,
                selector_controller
            } selector_t;
            selector_t Selector;       ///< Method by which pattern is chosen
            range_t KeySwitchRange;    ///< Key range for key switch selector
            uint8_t Controller;        ///< CC number for controller selector

            bool Polyphonic;           ///< If alternator should step forward only when all notes are off
            bool Chained;              ///< If all patterns should be chained together

        protected:
            MidiRuleAlternator(RIFF::Chunk* _3ewg);
            MidiRuleAlternator();
            void UpdateChunks(uint8_t* pData) const OVERRIDE;
            friend class Instrument;
    };

    /** @brief A MIDI rule not yet implemented by libgig.
     *
     * This class is currently used as a place holder by libgig for MIDI rule
     * types which are not supported by libgig yet.
     *
     * Note: Support for missing MIDI rule types are probably never added to
     * libgig. We recommend you using real-time instrument scripts instead.
     * Read more about the details and reasons for this in the description of
     * the MidiRule base class.
     *
     * @deprecated Just provided for backward compatiblity, use Script for new
     *             instruments instead. See description of MidiRule for details.
     */
    class MidiRuleUnknown : public MidiRule {
        protected:
            MidiRuleUnknown() { }
            void UpdateChunks(uint8_t* pData) const OVERRIDE { }
            friend class Instrument;
    };

    /** @brief Real-time instrument script (gig format extension).
     *
     * Real-time instrument scripts are user supplied small programs which can
     * be used by instrument designers to create custom behaviors and features
     * not available in the stock sampler engine. Features which might be very
     * exotic or specific for the respective instrument.
     *
     * This is an extension of the GigaStudio format, thus a feature which was
     * not available in the GigaStudio 4 software. It is currently only
     * supported by LinuxSampler and gigedit. Scripts will not load with the
     * original GigaStudio software.
     *
     * You find more informations about Instrument Scripts on the LinuxSampler
     * documentation site:
     *
     * - <a href="http://doc.linuxsampler.org/Instrument_Scripts/">About Instrument Scripts in General</a>
     * - <a href="http://doc.linuxsampler.org/Instrument_Scripts/NKSP_Language">Introduction to the NKSP Script Language</a>
     * - <a href="http://doc.linuxsampler.org/Instrument_Scripts/NKSP_Language/Reference/">NKSP Reference Manual</a>
     * - <a href="http://doc.linuxsampler.org/Gigedit/Managing_Scripts">Using Instrument Scripts with Gigedit</a>
     */
    class Script : protected DLS::Storage {
        public:
            enum Encoding_t {
                ENCODING_ASCII = 0 ///< Standard 8 bit US ASCII character encoding (default).
            };
            enum Compression_t {
                COMPRESSION_NONE = 0 ///< Is not compressed at all (default).
            };
            enum Language_t {
                LANGUAGE_NKSP = 0 ///< NKSP stands for "Is Not KSP" (default). Refer to the <a href="http://doc.linuxsampler.org/Instrument_Scripts/NKSP_Language/Reference/">NKSP Reference Manual</a> for details about this script language. 
            };

            String         Name;        ///< Arbitrary name of the script, which may be displayed i.e. in an instrument editor.
            Compression_t  Compression; ///< Whether the script was/should be compressed, and if so, which compression algorithm shall be used.
            Encoding_t     Encoding;    ///< Format the script's source code text is encoded with.
            Language_t     Language;    ///< Programming language and dialect the script is written in.
            bool           Bypass;      ///< Global bypass: if enabled, this script shall not be executed by the sampler for any instrument.
            uint8_t        Uuid[16];    ///< Persistent Universally Unique Identifier of this script, which remains identical after any changes to this script.

            String GetScriptAsText();
            void   SetScriptAsText(const String& text);
            void   SetGroup(ScriptGroup* pGroup);
            ScriptGroup* GetGroup() const;
            void   CopyAssign(const Script* orig);
        protected:
            Script(ScriptGroup* group, RIFF::Chunk* ckScri);
            virtual ~Script();
            void UpdateChunks(progress_t* pProgress) OVERRIDE;
            void DeleteChunks() OVERRIDE;
            void RemoveAllScriptReferences();
            void GenerateUuid();
            friend class ScriptGroup;
            friend class Instrument;
        private:
            ScriptGroup*          pGroup;
            RIFF::Chunk*          pChunk; ///< 'Scri' chunk
            std::vector<uint8_t>  data;
            uint32_t              crc; ///< CRC-32 checksum of the raw script data
    };

    /** @brief Group of instrument scripts (gig format extension).
     *
     * This class is simply used to sort a bunch of real-time instrument scripts
     * into individual groups. This allows instrument designers and script
     * developers to keep scripts in a certain order while working with a larger
     * amount of scripts in an instrument editor.
     *
     * This is an extension of the GigaStudio format, thus a feature which was
     * not available in the GigaStudio 4 software. It is currently only
     * supported by LinuxSampler and gigedit.
     */
    class ScriptGroup : protected DLS::Storage {
        public:
            String   Name; ///< Name of this script group. For example to be displayed in an instrument editor.

            Script*  GetScript(uint index);
            Script*  AddScript();
            void     DeleteScript(Script* pScript);
        protected:
            ScriptGroup(File* file, RIFF::List* lstRTIS);
            virtual ~ScriptGroup();
            void LoadScripts();
            virtual void UpdateChunks(progress_t* pProgress) OVERRIDE;
            virtual void DeleteChunks() OVERRIDE;
            friend class Script;
            friend class File;
        private:
            File*                pFile;
            RIFF::List*          pList; ///< 'RTIS' list chunk
            std::list<Script*>*  pScripts;
    };

    /** @brief Provides access to a Gigasampler/GigaStudio instrument.
     *
     * This class provides access to Gigasampler/GigaStudio instruments
     * contained in .gig files. A gig instrument is merely a set of keyboard
     * ranges (called Region), plus some additional global informations about
     * the instrument. The major part of the actual instrument definition used
     * for the synthesis of the instrument is contained in the respective Region
     * object (or actually in the respective DimensionRegion object being, see
     * description of Region for details).
     *
     * Since the gig format was designed as extension to the DLS file format,
     * this class is derived from the DLS::Instrument class. So also refer to
     * DLS::Instrument for additional informations, class attributes and
     * methods.
     */
    class Instrument : protected DLS::Instrument {
        public:
            // derived attributes from DLS::Resource
            using DLS::Resource::pInfo;
            using DLS::Resource::pDLSID;
            // derived attributes from DLS::Instrument
            using DLS::Instrument::IsDrum;
            using DLS::Instrument::MIDIBank;
            using DLS::Instrument::MIDIBankCoarse;
            using DLS::Instrument::MIDIBankFine;
            using DLS::Instrument::MIDIProgram;
            using DLS::Instrument::Regions;
            // own attributes
            int32_t   Attenuation;       ///< in dB
            uint16_t  EffectSend;
            int16_t   FineTune;          ///< in cents
            uint16_t  PitchbendRange;    ///< Number of semitones pitchbend controller can pitch (default is 2).
            bool      PianoReleaseMode;
            range_t   DimensionKeyRange; ///< 0-127 (where 0 means C1 and 127 means G9)


            // derived methods from DLS::Resource
            using DLS::Resource::GetParent;
            // overridden methods
            Region*   GetFirstRegion();
            Region*   GetNextRegion();
            Region*   AddRegion();
            void      DeleteRegion(Region* pRegion);
            void      MoveTo(Instrument* dst);
            virtual void UpdateChunks(progress_t* pProgress) OVERRIDE;
            virtual void CopyAssign(const Instrument* orig);
            // own methods
            Region*   GetRegion(unsigned int Key);
            MidiRule* GetMidiRule(int i);
            MidiRuleCtrlTrigger* AddMidiRuleCtrlTrigger();
            MidiRuleLegato*      AddMidiRuleLegato();
            MidiRuleAlternator*  AddMidiRuleAlternator();
            void      DeleteMidiRule(int i);
            // real-time instrument script methods
            Script*   GetScriptOfSlot(uint index);
            void      AddScriptSlot(Script* pScript, bool bypass = false);
            void      SwapScriptSlots(uint index1, uint index2);
            void      RemoveScriptSlot(uint index);
            void      RemoveScript(Script* pScript);
            uint      ScriptSlotCount() const;
            bool      IsScriptSlotBypassed(uint index);
            void      SetScriptSlotBypassed(uint index, bool bBypass);
            bool      IsScriptPatchVariableSet(int slot, String variable);
            std::map<String,String> GetScriptPatchVariables(int slot);
            String    GetScriptPatchVariable(int slot, String variable);
            void      SetScriptPatchVariable(int slot, String variable, String value);
            void      UnsetScriptPatchVariable(int slot = -1, String variable = "");
        protected:
            Region*   RegionKeyTable[128]; ///< fast lookup for the corresponding Region of a MIDI key

            Instrument(File* pFile, RIFF::List* insList, progress_t* pProgress = NULL);
           ~Instrument();
            void CopyAssign(const Instrument* orig, const std::map<Sample*,Sample*>* mSamples);
            void UpdateRegionKeyTable();
            void LoadScripts();
            void UpdateScriptFileOffsets();
            friend class File;
            friend class Region; // so Region can call UpdateRegionKeyTable()
        private:
            struct _ScriptPooolEntry {
                uint32_t fileOffset;
                bool     bypass;
            };
            struct _ScriptPooolRef {
                Script*  script;
                bool     bypass;
            };
            typedef std::array<uint8_t,16> _UUID;
            typedef std::map<String,String> _PatchVars;
            typedef std::map<int,_PatchVars> _VarsBySlot;
            typedef std::map<_UUID,_VarsBySlot> _VarsByScript;
            MidiRule** pMidiRules;
            std::vector<_ScriptPooolEntry> scriptPoolFileOffsets;
            std::vector<_ScriptPooolRef>* pScriptRefs;
            _VarsByScript scriptVars;

            _VarsByScript stripScriptVars();
            bool ReferencesScriptWithUuid(const _UUID& uuid);
            bool UsesAnyGigFormatExtension() const;
    };

    /** @brief Group of Gigasampler samples
     *
     * Groups help to organize a huge collection of Gigasampler samples.
     * Groups are not concerned at all for the synthesis, but they help
     * sound library developers when working on complex instruments with an
     * instrument editor (as long as that instrument editor supports it ;-).
     *
     * A sample is always assigned to exactly one Group. This also means
     * there is always at least one Group in a .gig file, no matter if you
     * created one yet or not.
     */
    class Group : public DLS::Storage {
        public:
            String Name; ///< Stores the name of this Group.

            Sample* GetFirstSample();
            Sample* GetNextSample();
            void AddSample(Sample* pSample);
        protected:
            Group(File* file, RIFF::Chunk* ck3gnm);
            virtual ~Group();
            virtual void UpdateChunks(progress_t* pProgress) OVERRIDE;
            virtual void DeleteChunks() OVERRIDE;
            void MoveAll();
            friend class File;
        private:
            File*        pFile;
            RIFF::Chunk* pNameChunk; ///< '3gnm' chunk
    };

    /** @brief Provides convenient access to Gigasampler/GigaStudio .gig files.
     *
     * This is the entry class for accesing a Gigasampler/GigaStudio (.gig) file
     * with libgig. It allows you to open existing .gig files, modifying them
     * and saving them persistently either under the same file name or under a
     * different location.
     *
     * A .gig file is merely a monolithic file. That means samples and the
     * defintion of the virtual instruments are contained in the same file. A
     * .gig file contains an arbitrary amount of samples, and an arbitrary
     * amount of instruments which are referencing those samples. It is also
     * possible to store samples in .gig files not being referenced by any
     * instrument. This is not an error from the file format's point of view and
     * it is actually often used in practice during the design phase of new gig
     * instruments.
     *
     * So on toplevel of the gig file format you have:
     *
     * - A set of samples (see Sample).
     * - A set of virtual instruments (see Instrument).
     *
     * And as extension to the original GigaStudio format, we added:
     *
     * - Real-time instrument scripts (see Script).
     *
     * Note that the latter however is only supported by libgig, gigedit and
     * LinuxSampler. Scripts are not supported by the original GigaStudio
     * software.
     *
     * All released Gigasampler/GigaStudio file format versions are supported
     * (so from first Gigasampler version up to including GigaStudio 4).
     *
     * Since the gig format was designed as extension to the DLS file format,
     * this class is derived from the DLS::File class. So also refer to
     * DLS::File for additional informations, class attributes and methods.
     */
    class File : protected DLS::File {
        public:
            static const DLS::version_t VERSION_2;
            static const DLS::version_t VERSION_3;
            static const DLS::version_t VERSION_4;

            // derived attributes from DLS::Resource
            using DLS::Resource::pInfo;
            using DLS::Resource::pDLSID;
            // derived attributes from DLS::File
            using DLS::File::pVersion;
            using DLS::File::Instruments;

            // derived methods from DLS::Resource
            using DLS::Resource::GetParent;
            // derived methods from DLS::File
            using DLS::File::Save;
            using DLS::File::GetFileName;
            using DLS::File::SetFileName;
            using DLS::File::GetRiffFile;
            // overridden  methods
            File();
            File(RIFF::File* pRIFF);
            Sample*     GetFirstSample(progress_t* pProgress = NULL); ///< Returns a pointer to the first <i>Sample</i> object of the file, <i>NULL</i> otherwise.
            Sample*     GetNextSample();      ///< Returns a pointer to the next <i>Sample</i> object of the file, <i>NULL</i> otherwise.
            Sample*     GetSample(uint index);
            Sample*     AddSample();
            size_t      CountSamples();
            void        DeleteSample(Sample* pSample);
            Instrument* GetFirstInstrument(); ///< Returns a pointer to the first <i>Instrument</i> object of the file, <i>NULL</i> otherwise.
            Instrument* GetNextInstrument();  ///< Returns a pointer to the next <i>Instrument</i> object of the file, <i>NULL</i> otherwise.
            Instrument* GetInstrument(uint index, progress_t* pProgress = NULL);
            Instrument* AddInstrument();
            Instrument* AddDuplicateInstrument(const Instrument* orig);
            size_t      CountInstruments();
            void        DeleteInstrument(Instrument* pInstrument);
            Group*      GetFirstGroup(); ///< Returns a pointer to the first <i>Group</i> object of the file, <i>NULL</i> otherwise.
            Group*      GetNextGroup();  ///< Returns a pointer to the next <i>Group</i> object of the file, <i>NULL</i> otherwise.
            Group*      GetGroup(uint index);
            Group*      GetGroup(String name);
            Group*      AddGroup();
            void        DeleteGroup(Group* pGroup);
            void        DeleteGroupOnly(Group* pGroup);
            void        SetAutoLoad(bool b);
            bool        GetAutoLoad();
            void        AddContentOf(File* pFile);
            ScriptGroup* GetScriptGroup(uint index);
            ScriptGroup* GetScriptGroup(const String& name);
            ScriptGroup* AddScriptGroup();
            void        DeleteScriptGroup(ScriptGroup* pGroup);
            virtual    ~File();
            virtual void UpdateChunks(progress_t* pProgress) OVERRIDE;
        protected:
            // overridden protected methods from DLS::File
            virtual void LoadSamples() OVERRIDE;
            virtual void LoadInstruments() OVERRIDE;
            virtual void LoadGroups();
            virtual void UpdateFileOffsets() OVERRIDE;
            // own protected methods
            virtual void LoadSamples(progress_t* pProgress);
            virtual void LoadInstruments(progress_t* pProgress);
            virtual void LoadScriptGroups();
            void SetSampleChecksum(Sample* pSample, uint32_t crc);
            uint32_t GetSampleChecksum(Sample* pSample);
            uint32_t GetSampleChecksumByIndex(int index);
            bool VerifySampleChecksumTable();
            bool RebuildSampleChecksumTable();
            int  GetWaveTableIndexOf(gig::Sample* pSample);
            friend class Region;
            friend class Sample;
            friend class Instrument;
            friend class Group; // so Group can access protected member pRIFF
            friend class ScriptGroup; // so ScriptGroup can access protected member pRIFF
        private:
            std::list<Group*>*          pGroups;
            std::list<Group*>::iterator GroupsIterator;
            bool                        bAutoLoad;
            std::list<ScriptGroup*>*    pScriptGroups;

            bool UsesAnyGigFormatExtension() const;
    };

    /**
     * Will be thrown whenever a gig specific error occurs while trying to
     * access a Gigasampler File. Note: In your application you should
     * better catch for RIFF::Exception rather than this one, except you
     * explicitly want to catch and handle gig::Exception, DLS::Exception
     * and RIFF::Exception independently, which usually shouldn't be
     * necessary though.
     */
    class Exception : public DLS::Exception {
        public:
            Exception(String format, ...);
            Exception(String format, va_list arg);
            void PrintMessage();
        protected:
            Exception();
    };

#if HAVE_RTTI
    size_t enumCount(const std::type_info& type);
    const char* enumKey(const std::type_info& type, size_t value);
    bool        enumKey(const std::type_info& type, String key);
    const char** enumKeys(const std::type_info& type);
#endif // HAVE_RTTI
    size_t enumCount(String typeName);
    const char* enumKey(String typeName, size_t value);
    bool        enumKey(String typeName, String key);
    const char** enumKeys(String typeName);
    size_t enumValue(String key);

    String libraryName();
    String libraryVersion();

} // namespace gig

#endif // __GIG_H__
