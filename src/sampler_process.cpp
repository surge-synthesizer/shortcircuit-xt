#include "sampler.h"
#if !TARGET_HEADLESS
#include "shortcircuit_editor2.h"
#endif
#include "configuration.h"
#include "synthesis/mathtables.h"
#include "sampler_voice.h"
#include "synthesis/filter.h"
#include "synthesis/modmatrix.h"
#include <vt_dsp/basic_dsp.h>
#if !TARGET_HEADLESS
#include <vt_gui/vt_gui_controls.h>
#endif
#include "interaction_parameters.h"
#include "util/tools.h"

using std::max;
using std::min;

float *sampler::get_output_pointer(int id, int channel, int part)
{
    if (id == out_part)
        return output_part[(part << 1) + channel];
    else if (id >= out_fx1)
        return output_fx[(((id - out_fx1) & 0x7) << 1) + channel];
    // else return output_ptr[(((id-out_output1)&0x7)<<1)+channel];
    // HACK, disablad no-memcpy speedup
    else
        return output[(((id - out_output1) & 0x7) << 1) + channel];
}

void sampler::part_check_filtertypes(int p, int f)
{
    //	have the filtertypes changed?
    if (partv[p].last_ft[f] != parts[p].Filter[f].type)
    {
        spawn_filter_release(partv[p].pFilter[f]);

        partv[p].pFilter[f] = spawn_filter(
            parts[p].Filter[f].type,
            partv[p].mm->get_destination_ptr(f ? md_part_filter2prm0 : md_part_filter1prm0),
            parts[p].Filter[f].ip, 0, true);

        if (partv[p].pFilter[f])
            partv[p].pFilter[f]->init();
        partv[p].last_ft[f] = parts[p].Filter[f].type;
    }
}

void sampler::process_global_effects()
{
    for (int f = 0; f < n_sampler_effects; f++)
    {
        //	have the filtertype changed?
        if (multiv.last_ft[f] != multi.Filter[f].type)
        {
            spawn_filter_release(multiv.pFilter[f]);
            multiv.pFilter[f] =
                spawn_filter(multi.Filter[f].type, multi.Filter[f].p, multi.Filter[f].ip, 0, true);
            if (multiv.pFilter[f])
                multiv.pFilter[f]->init();
            multiv.last_ft[f] = multi.Filter[f].type;
        }

        if ((multiv.pFilter[f]) && (!multi.Filter[f].bypass))
        {
            _MM_ALIGN16 float tempbuf[2][block_size];

            float *L = output_fx[f << 1];
            float *R = output_fx[(f << 1) + 1];
            float *mainL = get_output_pointer(multi.filter_output[f], 0, 0);
            float *mainR = get_output_pointer(multi.filter_output[f], 1, 0);

            multiv.pregain.set_target_smoothed(db_to_linear(multi.filter_pregain[f]));
            multiv.postgain.set_target_smoothed(db_to_linear(multi.filter_postgain[f]));

            multiv.pFilter[f]->process_stereo(L, R, tempbuf[0], tempbuf[1], 0);
            // multiv.postgain.fade_2_blocks_to(L,tempbuf[0],R,tempbuf[1],L,R,block_size_quad);

            accumulate_block(tempbuf[0], mainL, block_size_quad);
            accumulate_block(tempbuf[1], mainR, block_size_quad);
        }
    }
}

void sampler::process_part(int p)
{
    float *mainL, *aux1L, *aux2L;
    float *mainR, *aux1R, *aux2R;
    mainL = get_output_pointer(parts[p].aux[0].output, 0, 0);
    mainR = get_output_pointer(parts[p].aux[0].output, 1, 0);
    aux1L = get_output_pointer(parts[p].aux[1].output, 0, 0);
    aux1R = get_output_pointer(parts[p].aux[1].output, 1, 0);
    aux2L = get_output_pointer(parts[p].aux[2].output, 0, 0);
    aux2R = get_output_pointer(parts[p].aux[2].output, 1, 0);

    float *L = output_part[(p << 1)];
    float *R = output_part[(p << 1) + 1];

    _MM_ALIGN16 float tempbuf[2][block_size], postfader_buf[2][block_size];

    // smooth controllers

    for (unsigned int i = 0; i < n_controllers; i++)
    {
        float b =
            fabs(controllers_target[p * n_controllers + i] - controllers[p * n_controllers + i]);
        float a = 0.1 * b;
        controllers[p * n_controllers + i] = (1 - a) * controllers[p * n_controllers + i] +
                                             a * controllers_target[p * n_controllers + i];
    }
    for (unsigned int i = 0; i < n_custom_controllers; i++)
    {
        float b = fabs(parts[p].userparameter[i] - parts[p].userparameter_smoothed[i]);
        float a = 0.1 * b;
        parts[p].userparameter_smoothed[i] =
            (1 - a) * parts[p].userparameter_smoothed[i] + a * parts[p].userparameter[i];
    }

    partv[p].mm->process_part();

    // update interpolators
    float amp = db_to_linear(partv[p].mm->get_destination_value(md_part_amplitude));
    float pan = partv[p].mm->get_destination_value(md_part_pan);
    partv[p].ampL.set_target_smoothed(megapanL(pan) * amp);
    partv[p].ampR.set_target_smoothed(megapanR(pan) * amp);
    amp = db_to_linear(partv[p].mm->get_destination_value(md_part_aux_level));
    pan = partv[p].mm->get_destination_value(md_part_aux_balance);
    partv[p].aux1L.set_target_smoothed(megapanL(pan) * amp);
    partv[p].aux1R.set_target_smoothed(megapanR(pan) * amp);
    amp = db_to_linear(partv[p].mm->get_destination_value(md_part_aux2_level));
    pan = partv[p].mm->get_destination_value(md_part_aux2_balance);
    partv[p].aux2L.set_target_smoothed(megapanL(pan) * amp);
    partv[p].aux2R.set_target_smoothed(megapanR(pan) * amp);
    partv[p].pfg.set_target(
        db_to_linear(partv[p].mm->get_destination_value(md_part_prefilter_gain)));
    partv[p].fmix1.set_target_smoothed(partv[p].mm->get_destination_value(md_part_filter1mix));
    partv[p].fmix2.set_target_smoothed(partv[p].mm->get_destination_value(md_part_filter2mix));

    // process filters
    part_check_filtertypes(p, 0);
    part_check_filtertypes(p, 1);

    partv[p].pfg.multiply_2_blocks(L, R, block_size_quad);
    if ((partv[p].pFilter[0]) && (!parts[p].Filter[0].bypass))
    {
        partv[p].pFilter[0]->process_stereo(L, R, tempbuf[0], tempbuf[1], 0);
        partv[p].fmix1.fade_2_blocks_to(L, tempbuf[0], R, tempbuf[1], L, R, block_size_quad);
    }
    if ((partv[p].pFilter[1]) && (!parts[p].Filter[1].bypass))
    {
        partv[p].pFilter[1]->process_stereo(L, R, tempbuf[0], tempbuf[1], 0);
        partv[p].fmix2.fade_2_blocks_to(L, tempbuf[0], R, tempbuf[1], L, R, block_size_quad);
    }

    // process output
    partv[p].ampL.multiply_block_to(L, postfader_buf[0], block_size_quad);
    partv[p].ampR.multiply_block_to(R, postfader_buf[1], block_size_quad);
    accumulate_block(postfader_buf[0], mainL, block_size_quad);
    accumulate_block(postfader_buf[1], mainR, block_size_quad);

    if (aux1L && aux1R && parts[p].aux[1].outmode)
    {
        if (parts[p].aux[1].outmode == 2)
        {
            partv[p].aux1L.MAC_block_to(postfader_buf[0], aux1L, block_size_quad);
            partv[p].aux1R.MAC_block_to(postfader_buf[1], aux1R, block_size_quad);
        }
        else
        {
            partv[p].aux1L.MAC_block_to(L, aux1L, block_size_quad);
            partv[p].aux1R.MAC_block_to(R, aux1R, block_size_quad);
        }
    }
    if (aux2L && aux2R && parts[p].aux[2].outmode)
    {
        if (parts[p].aux[2].outmode == 2)
        {
            partv[p].aux2L.MAC_block_to(postfader_buf[0], aux2L, block_size_quad);
            partv[p].aux2R.MAC_block_to(postfader_buf[1], aux2R, block_size_quad);
        }
        else
        {
            partv[p].aux2L.MAC_block_to(L, aux2L, block_size_quad);
            partv[p].aux2R.MAC_block_to(R, aux2R, block_size_quad);
        }
    }
}

void sampler::process_audio()
{
#ifdef SCPB
    holdengine |= (scpb_queue_patch > -1);
#endif
    if (holdengine)
    {
        for (unsigned int op = 0; op < (mNumOutputs); op++)
            clear_block(output[op], block_size_quad << 1);
        return;
    }
    process_editor_events();

    /*if (sample_replace_filename[0])
    {
            if(zone_exist(selected->get_active_id()) && (selected->get_active_type() == 1))
                    replace_zone(selected->get_active_id(),sample_replace_filename);
            sample_replace_filename[0] = 0;
    }*/

    if (sample_replace_filename[0])
    {
        if (zone_exist(selected->get_active_id()) && (selected->get_active_type() == 1))
            kill_notes(selected->get_active_id());
    }

    // memset(output,0,sizeof(output));
    for (unsigned int op = 0; op < (mNumOutputs << 1); op++)
        clear_block_antidenormalnoise(output_ptr[op], block_size_quad);
    for (unsigned int op = 0; op < (n_sampler_effects << 1); op++)
        clear_block(output_fx[op], block_size_quad);
    for (unsigned int op = 0; op < (n_sampler_parts << 1); op++)
        clear_block(output_part[op], block_size_quad);

    // clear buffers & process controls
    {
        std::lock_guard g(cs_patch);

        // render voices
        for (unsigned int v = 0; v < highest_voice_id; v++)
        {
            if (voice_state[v].active) // is bottleneck at idle (?)
            {
                float *outbuf[3][2];
                outbuf[0][0] =
                    get_output_pointer(voices[v]->zone->aux[0].output, 0, voices[v]->zone->part);
                outbuf[0][1] =
                    get_output_pointer(voices[v]->zone->aux[0].output, 1, voices[v]->zone->part);
                outbuf[1][0] =
                    get_output_pointer(voices[v]->zone->aux[1].output, 0, voices[v]->zone->part);
                outbuf[1][1] =
                    get_output_pointer(voices[v]->zone->aux[1].output, 1, voices[v]->zone->part);
                outbuf[2][0] =
                    get_output_pointer(voices[v]->zone->aux[2].output, 0, voices[v]->zone->part);
                outbuf[2][1] =
                    get_output_pointer(voices[v]->zone->aux[2].output, 1, voices[v]->zone->part);

                voice_state[v].active =
                    voices[v]->process_block(outbuf[0][0], outbuf[0][1], outbuf[1][0], outbuf[1][1],
                                             outbuf[2][0], outbuf[2][1]);

                if (!voice_state[v].active)
                {
                    polyphony--;
                    holdbuffer.remove(v);
#if TARGET_VST2
                    if (editor && editor->isOpen())
                        track_zone_triggered(voice_state[v].zone_id, false);
#endif
                }
            }
        }

        // process parts
        for (int p = 0; p < n_sampler_parts; p++)
        {
            process_part(p);
        }

        process_global_effects();

        // Process preview player
        if (mpPreview->mActive)
        {
            bool ContinuePreviewing =
                mpPreview->mpVoice->process_block(output[0], output[1], NULL, NULL, NULL, NULL);

            if (!ContinuePreviewing)
            {
                mpPreview->SetPlayingState(false);
            }
        }
    }

    processVUs();
    // post amplitude
    /*
    for(op=0; op<(output_pairs*2); op++)
    {
            vu_peak[op] = max(vu_peak[op],get_absmax(output[op],block_size_quad));
            for(i=0; i<block_size; i++)
            {
                    this->output[op][i] *= (float)this->headroom_linear;
            }
    }
    vu_time += block_size;
    */
}

void sampler::processVUs()
{
#if TARGET_VST2
    if (!(editor && editor->isOpen()))
        return;

    for (int op = 0; op < (mNumOutputs * 2); op++)
    {
        vu_peak[op] = max(vu_peak[op], get_absmax(output[op], block_size_quad));
    }

    VUidx--;
    if (VUidx < 1)
    {
        VUidx = VUrate;
        actiondata ad;
        ad.actiontype = vga_vudata;
        ad.id = ip_vumeter;
        ad.subid = -1;
        ad.data.i[0] = mNumOutputs;

        for (int op = 0; op < (mNumOutputs * 2); op += 2)
        {
            VUdata vu;
            vu.stereo = 1;
            vu.clip1 = (vu_peak[op] > 1.f) ? 1 : 0;
            vu.clip2 = (vu_peak[op + 1] > 1.f) ? 1 : 0;
            const __m128 m255 = _mm_set1_ps(255.f);

            int a = _mm_cvttss_si32(_mm_mul_ss(m255, _mm_sqrt_ss(_mm_load_ss(&vu_peak[op]))));
            vu.ch1 = limit_range(a, 0, 255);
            a = _mm_cvttss_si32(_mm_mul_ss(m255, _mm_sqrt_ss(_mm_load_ss(&vu_peak[op + 1]))));
            vu.ch2 = limit_range(a, 0, 255);
            vu_peak[op] = 0.f;
            vu_peak[op + 1] = 0.f;
            ad.data.i[1 + (op >> 1)] = *(int *)&vu;
        }
        post_events_to_editor(ad);
    }
#endif
}
