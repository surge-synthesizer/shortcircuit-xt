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

#include "filter_defs.h"
#include "sampler_state.h"
#include <algorithm>
#include "util/scxtstring.h"
//#include <new.h>		// needed for "placement new" to work

extern float SincTableF32[(FIRipol_M + 1) * FIRipol_N];
extern float SincOffsetF32[(FIRipol_M)*FIRipol_N];

/*	spawner			*/

bool spawn_filter_release(filter *f)
{
    if (!f)
        return false;
    f->~filter();
    free(f);
    return true;
}

template <typename T, bool takesIP = true> void spawn_internal(filter *&t, float *fp, int *ip)
{
    // static_assert(sizeof(T) < 1024 * 128);
#if MAC
    t = (filter *)malloc(sizeof(T));
#else
#if WIN
    t = (filter *)_aligned_alloc(16, sizeof(T));
#else
    t = (filter *)std::aligned_alloc(16, sizeof(T));
#endif
#endif
    if constexpr (takesIP)
        new (t) T(fp, ip);
    else
        new (t) T(fp);
}

template <> void spawn_internal<superbiquad, true>(filter *&t, float *fp, int *ip)
{
#if MAC
    t = (filter *)malloc(sizeof(superbiquad));
#else
#if WIN
    t = (filter *)_aligned_alloc(16, sizeof(superbiquad));
#else
    t = (filter *)std::aligned_alloc(16, sizeof(superbiquad));
#endif
#endif
    new (t) superbiquad(fp, ip, 0);
}

filter *spawn_filter(int id, float *fp, int *ip, void *loader, bool stereo)
{
    filter *t = 0;
    switch (id)
    {
    case ft_e_delay:
        spawn_internal<dualdelay>(t, fp, ip);
        break;
    case ft_e_reverb:
        spawn_internal<reverb>(t, fp, ip);
        break;
    case ft_e_chorus:
        spawn_internal<chorus>(t, fp, ip);
        break;
    case ft_e_phaser:
        spawn_internal<phaser>(t, fp, ip);
        break;
    case ft_e_rotary:
        spawn_internal<rotary_speaker>(t, fp, ip);
        break;
    case ft_e_fauxstereo:
        spawn_internal<fauxstereo>(t, fp, ip);
        break;
    case ft_e_fsflange:
        spawn_internal<fs_flange>(t, fp, ip);
        break;
    case ft_e_fsdelay:
        spawn_internal<freqshiftdelay>(t, fp, ip);
        break;
    /*case ft_biquadLP2B:
            t = (filter*) _mm_malloc(sizeof(LP2B),16);
            new(t) LP2B(fp);
            break;*/
    case ft_SuperSVF:
        spawn_internal<SuperSVF>(t, fp, ip);
        break;
    case ft_biquadSBQ:
        spawn_internal<superbiquad>(t, fp, ip);
        break;
    case ft_moogLP4sat:
        spawn_internal<LP4M_sat>(t, fp, ip);
        break;
        /*case ft_biquadHP2:
                t = (filter*) _mm_malloc(sizeof(superbiquad),16);
                new(t) superbiquad(fp,ip,1);
                break;*/
        /*	case ft_biquadBP2B:
                        t = (filter*) _mm_malloc(sizeof(BP2B),16);
                        new(t) BP2B(fp);
                        break;
                case ft_biquad_peak:
                        t = (filter*) _mm_malloc(sizeof(PKA),16);
                        new(t) PKA(fp);
                        break;
                case ft_biquad_notch:
                        t = (filter*) _mm_malloc(sizeof(NOTCH),16);
                        new(t) NOTCH(fp);
                        break;
                case ft_biquadBP2_dual:
                        t = (filter*) _mm_malloc(sizeof(BP2AD),16);
                        new(t) BP2AD(fp);
                        break;
                case ft_biquad_peak_dual:
                        t = (filter*) _mm_malloc(sizeof(PKAD),16);
                        new(t) PKAD(fp);
                        break;
                case ft_biquadLPHP_serial:
                        t = (filter*) _mm_malloc(sizeof(LPHP_ser),16);
                        new(t) LPHP_ser(fp);
                        break;
                case ft_biquadLPHP_parallel:
                        t = (filter*) _mm_malloc(sizeof(LPHP_par),16);
                        new(t) LPHP_par(fp);
                        break;*/
    // case ft_biquadLP2HP2_morph:
    //	t = new LP2HP2_morph(fp);
    //	break;
    case ft_eq_2band_parametric_A:
        spawn_internal<EQ2BP_A>(t, fp, ip);
        break;
    case ft_eq_6band:
        spawn_internal<EQ6B, false>(t, fp, ip);
        break;
        /*	case ft_morpheq:
                        t = (filter*) _mm_malloc(sizeof(morphEQ),16);
                        new(t) morphEQ(fp,loader,ip);
                        break;*/
    case ft_comb1:
        spawn_internal<COMB1, false>(t, fp, ip);
        break;
    // case ft_comb2:
    //	t = new COMB2(fp);
    //	break;
    case ft_fx_slewer:
        spawn_internal<fslewer, false>(t, fp, ip);
        break;
    case ft_fx_treemonster:
        spawn_internal<treemonster>(t, fp, ip);
        break;
    case ft_fx_stereotools:
        spawn_internal<stereotools>(t, fp, ip);
        break;
    case ft_fx_limiter:
        spawn_internal<limiter>(t, fp, ip);
        break;
    case ft_fx_bitfucker:
        spawn_internal<BF, false>(t, fp, ip);
        break;
    case ft_fx_distortion1:
        spawn_internal<fdistortion, false>(t, fp, ip);
        break;
    // case ft_fx_exciter:
    //	t = new fexciter(fp);
    //	break;
    case ft_fx_clipper:
        spawn_internal<clipper, false>(t, fp, ip);
        break;
    case ft_fx_gate:
        spawn_internal<gate, false>(t, fp, ip);
        break;
    case ft_fx_microgate:
        spawn_internal<microgate, false>(t, fp, ip);
        break;
    case ft_fx_ringmod:
        spawn_internal<RING, false>(t, fp, ip);
        break;
    case ft_fx_freqshift:
        spawn_internal<FREQSHIFT>(t, fp, ip);
        break;
    case ft_fx_phasemod:
        spawn_internal<PMOD, false>(t, fp, ip);
        break;
    case ft_osc_pulse:
        spawn_internal<osc_pulse, false>(t, fp, ip);
        break;
    case ft_osc_pulse_sync:
        spawn_internal<osc_pulse_sync, false>(t, fp, ip);
        break;
    case ft_osc_saw:
        spawn_internal<osc_saw>(t, fp, ip);
        break;
    case ft_osc_sin:
        spawn_internal<osc_sin, false>(t, fp, ip);
        break;
    };

    assert(!id || t);
    // Make sure that no filtertype other than 0 return a NULL pointer

    return t;
}

/* base class		*/

filter::filter(float *params, void *loader, bool stereo, int *iparams)
{
    this->param = params; // DO NOT BE FS without the post-mod matrix honeycomb
    this->iparam = iparams;
    parameter_count = 0;
    modulation_output = 0.f;
    this->loader = loader;
    this->is_stereo = stereo;
    int i;
    for (i = 0; i < max_fparams; i++)
    {
        strncpy_0term(ctrllabel[i], ("---"), 32);
        strncpy_0term(ctrlmode_desc[i], ("f,0,0.01,1,0,"), 32);
    }

#ifdef _DEBUG
    debugstr[0] = 0;
#endif
}

int filter::get_parameter_ctrlmode(int p_id)
{
    if (p_id < 0)
        return 0;
    if (p_id > parameter_count)
        return 0;
    return ctrlmode[p_id];
}

char *filter::get_parameter_label(int p_id)
{
    if (p_id < 0)
        return 0;
    if (p_id > max_fparams)
        return 0;
    return ctrllabel[p_id];
}

char *filter::get_parameter_ctrlmode_descriptor(int p_id)
{
    if (p_id < 0)
        return 0;
    if (p_id > std::min(max_fparams, parameter_count))
        return 0;
    return ctrlmode_desc[p_id];
}