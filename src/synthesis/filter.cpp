//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

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
    _mm_free(f);
    return true;
}

filter *spawn_filter(int id, float *fp, int *ip, void *loader, bool stereo)
{
    filter *t = 0;
    switch (id)
    {
    case ft_e_delay:
        t = (filter *)_mm_malloc(sizeof(dualdelay), 16);
        new (t) dualdelay(fp, ip);
        break;
    case ft_e_reverb:
        t = (filter *)_mm_malloc(sizeof(reverb), 16);
        new (t) reverb(fp, ip);
        break;
    case ft_e_chorus:
        t = (filter *)_mm_malloc(sizeof(chorus), 16);
        new (t) chorus(fp, ip);
        break;
    case ft_e_phaser:
        t = (filter *)_mm_malloc(sizeof(phaser), 16);
        new (t) phaser(fp, ip);
        break;
    case ft_e_rotary:
        t = (filter *)_mm_malloc(sizeof(rotary_speaker), 16);
        new (t) rotary_speaker(fp, ip);
        break;
    case ft_e_fauxstereo:
        t = (filter *)_mm_malloc(sizeof(fauxstereo), 16);
        new (t) fauxstereo(fp, ip);
        break;
    case ft_e_fsflange:
        t = (filter *)_mm_malloc(sizeof(fs_flange), 16);
        new (t) fs_flange(fp, ip);
        break;
    case ft_e_fsdelay:
        t = (filter *)_mm_malloc(sizeof(freqshiftdelay), 16);
        new (t) freqshiftdelay(fp, ip);
        break;
    /*case ft_biquadLP2B:
            t = (filter*) _mm_malloc(sizeof(LP2B),16);
            new(t) LP2B(fp);
            break;*/
    case ft_SuperSVF:
        t = (filter *)_mm_malloc(sizeof(SuperSVF), 16);
        new (t) SuperSVF(fp, ip);
        break;
    case ft_biquadSBQ:
        t = (filter *)_mm_malloc(sizeof(superbiquad), 16);
        new (t) superbiquad(fp, ip, 0);
        break;
    case ft_moogLP4sat:
        t = (filter *)_mm_malloc(sizeof(LP4M_sat), 16);
        new (t) LP4M_sat(fp, ip);
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
        t = (filter *)_mm_malloc(sizeof(EQ2BP_A), 16);
        new (t) EQ2BP_A(fp, ip);
        break;
    case ft_eq_6band:
        t = (filter *)_mm_malloc(sizeof(EQ6B), 16);
        new (t) EQ6B(fp);
        break;
        /*	case ft_morpheq:
                        t = (filter*) _mm_malloc(sizeof(morphEQ),16);
                        new(t) morphEQ(fp,loader,ip);
                        break;*/
    case ft_comb1:
        t = (filter *)_mm_malloc(sizeof(COMB1), 16);
        new (t) COMB1(fp);
        break;
    // case ft_comb2:
    //	t = new COMB2(fp);
    //	break;
    case ft_fx_slewer:
        t = (filter *)_mm_malloc(sizeof(fslewer), 16);
        new (t) fslewer(fp);
        break;
    case ft_fx_treemonster:
        t = (filter *)_mm_malloc(sizeof(treemonster), 16);
        new (t) treemonster(fp, ip);
        break;
    case ft_fx_stereotools:
        t = (filter *)_mm_malloc(sizeof(stereotools), 16);
        new (t) stereotools(fp, ip);
        break;
    case ft_fx_limiter:
        t = (filter *)_mm_malloc(sizeof(limiter), 16);
        new (t) limiter(fp, ip);
        break;
    case ft_fx_bitfucker:
        t = (filter *)_mm_malloc(sizeof(BF), 16);
        new (t) BF(fp);
        break;
    case ft_fx_distortion1:
        t = (filter *)_mm_malloc(sizeof(fdistortion), 16);
        new (t) fdistortion(fp);
        break;
    // case ft_fx_exciter:
    //	t = new fexciter(fp);
    //	break;
    case ft_fx_clipper:
        t = (filter *)_mm_malloc(sizeof(clipper), 16);
        new (t) clipper(fp);
        break;
    case ft_fx_gate:
        t = (filter *)_mm_malloc(sizeof(gate), 16);
        new (t) gate(fp);
        break;
    case ft_fx_microgate:
        t = (filter *)_mm_malloc(sizeof(microgate), 16);
        new (t) microgate(fp);
        break;
    case ft_fx_ringmod:
        t = (filter *)_mm_malloc(sizeof(RING), 16);
        new (t) RING(fp);
        break;
    case ft_fx_freqshift:
        t = (filter *)_mm_malloc(sizeof(FREQSHIFT), 16);
        new (t) FREQSHIFT(fp, ip);
        break;
    case ft_fx_phasemod:
        t = (filter *)_mm_malloc(sizeof(PMOD), 16);
        new (t) PMOD(fp);
        break;
    case ft_osc_pulse:
        t = (filter *)_mm_malloc(sizeof(osc_pulse), 16);
        new (t) osc_pulse(fp);
        break;
    case ft_osc_pulse_sync:
        t = (filter *)_mm_malloc(sizeof(osc_pulse_sync), 16);
        new (t) osc_pulse_sync(fp);
        break;
    case ft_osc_saw:
        t = (filter *)_mm_malloc(sizeof(osc_saw), 16);
        new (t) osc_saw(fp, ip);
        break;
    case ft_osc_sin:
        t = (filter *)_mm_malloc(sizeof(osc_sin), 16);
        new (t) osc_sin(fp);
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