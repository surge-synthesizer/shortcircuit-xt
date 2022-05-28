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

#error THIS FILE IS UNUSED AND SHOULD NOT COMPILE

#ifndef __scpb_vsti_H
#include "scpb_vsti.h"
#endif

#include "infrastructure/logfile.h"
#include "scpb_editor.h"
#include <float.h>
#include <math.h>

//#include "mdump.h"

// MiniDumper mdumper("shortcircuit");
int output_pairs;
int outputs_mono;

extern bool oome;

//-------------------------------------------------------------------------------------------------------
scpb_vsti::scpb_vsti(audioMasterCallback audioMaster)
    : AudioEffectX(audioMaster, 1, kNumparams) // 1 program, 1 parameter only
{
    output_pairs = 1;
    outputs_mono = 0;

    setNumInputs(0);  // stereo in
    setNumOutputs(2); // multi out
    isSynth();
    setUniqueID('scpb'); // identify

    //#ifndef _DEMO_

    programsAreChunks(true);
    //#endif
    canProcessReplacing(); // supports both accumulating and replacing output
    supports_fxIdle = needIdle();
    strcpy(programName, "Default"); // default program name
    // write_log("vsti constructor");
    initialized = false;
    // NULL objects
    sobj = NULL;
    newdata = 0;
    events_this_block = 0;
    events_processed = 0;
    oldblokkosize = 0;
    updateMidiKeyName = false;

    editor = new scpb_editor(this);
    if (!editor)
        oome = true;
}

//-------------------------------------------------------------------------------------------------------
scpb_vsti::~scpb_vsti()
{
    // write_log("vsti deconstructor");

    // delete objects
    delete sobj;
}

//-------------------------------------------------------------------------------------------------------

long scpb_vsti::vendorSpecific(long lArg1, long lArg2, void *ptrArg, float floatArg)
{
    if (editor && lArg1 == 'stCA' && lArg2 == 'Whee') // support mouse wheel!
        return editor->onWheel(floatArg) == true ? 1 : 0;
    else
        return AudioEffectX::vendorSpecific(lArg1, lArg2, ptrArg, floatArg);
}

//-------------------------------------------------------------------------------------------------------
void scpb_vsti::suspend()
{
    // write_log("suspend");
    if (initialized)
        sobj->all_notes_off();
}

long scpb_vsti::stopProcess()
{
    if (initialized)
        sobj->all_notes_off();
    return 1;
}
//-------------------------------------------------------------------------------------------------------

void scpb_vsti::init()
{
    // write_log("init");
    sobj = new scpb_sampler(editor, this); //,this);
    assert(sobj);
    sobj->set_samplerate(this->getSampleRate());
    updateMidiKeyName = true;
    blockpos = 0;
    initialized = true;
}

//-------------------------------------------------------------------------------------------------------
void scpb_vsti::resume()
{
    // write_log("resume");
    if (!initialized)
    {
        init();
    }

    sobj->set_samplerate(this->getSampleRate());

    wantEvents();
}

struct chunkstorage
{
    float params[16];
    int fxbypass, transpose, polylimit;
    char fname[256];
};

long scpb_vsti::getChunk(void **data, bool isPreset)
{
    if (!initialized)
        init();

    chunkstorage *cs = new chunkstorage();
    if (!cs)
        return 0;
    *data = cs;
    for (int i = 0; i < 16; i++)
        cs->params[i] = sobj->controllers_target[c_custom0 + i];
    cs->polylimit = sobj->polyphony_cap;
    cs->fxbypass = sobj->fxbypass;
    cs->transpose = sobj->scpbtranspose;

    if (sobj->currentpatch > -1)
    {
        char libpath[1024];
        extern void *hInstance;
        GetModuleFileName((HMODULE)hInstance, libpath, 1024);
        char *end = strrchr(libpath, '\\');
        if (!end)
            return 0;
        strcpy(end, "\\swissarmylib\\");
        int pn = strlen(libpath); // number of characters in path
        const char *relative = sobj->patchlist.at(sobj->currentpatch).path.c_str();
        if (!relative)
            return 0;
        relative += pn;
        strncpy(cs->fname, relative, 256);
    }
    else
        strcpy(cs->fname, "nofile");
    // strncpy(cs->category,sobj->categories.at(sobj->currentcategory).c_str(),256);

    // return sobj->save_all_as_xml(data);
    return sizeof(chunkstorage);
}

//-----------------------------------------------------------------------------------------

long scpb_vsti::setChunk(void *data, long byteSize, bool isPreset)
{
    if (!initialized)
        init();
    assert(byteSize >= sizeof(chunkstorage));

    chunkstorage *cs = (chunkstorage *)data;

    char libpath[1024];
    extern void *hInstance;
    GetModuleFileName((HMODULE)hInstance, libpath, 1024);
    char *end = strrchr(libpath, '\\');
    if (!end)
        return 0;
    strcpy(end, "\\swissarmylib\\");

    string fullpath = libpath;
    fullpath += cs->fname;
    int n = sobj->patchlist.size();
    bool changed = false;
    for (int i = 0; i < n; i++)
    {
        int c = fullpath.compare(sobj->patchlist.at(i).path);
        if (c == 0)
        {
            for (int j = 0; j < 16; j++)
                sobj->controllers_target[c_custom0 + j] = cs->params[j];
            sobj->polyphony_cap = cs->polylimit;
            sobj->fxbypass = cs->fxbypass;
            sobj->scpbtranspose = cs->transpose;
            sobj->changepatch(i, false);
            return 1;
        }
    }

    // add warning not-found lalala here

    return 1;
}

//------------------------------------------------------------------------

long scpb_vsti::canDo(char *text)
{
    if (!strcmp(text, "receiveVstEvents"))
        return 1;

    if (!strcmp(text, "receiveVstMidiEvent"))
        return 1;

    if (!strcmp(text, "midiProgramNames"))
        return 1;

    return -1;
}

//------------------------------------------------------------------------

long scpb_vsti::processEvents(VstEvents *ev)
{
    if (initialized)
    {
        this->events_this_block = min(ev->numEvents, 1024);
        this->events_processed = 0;

        int i;
        for (i = 0; i < this->events_this_block; i++)
        {
            this->eventbuffer[i] = *(ev->events[i]);
        }
    }
    return 1; // want more
}

//------------------------------------------------------------------------

void scpb_vsti::handleEvent(VstEvent *ev)
{
    if (!initialized)
        return;
    if (ev->type == kVstMidiType)
    {
        VstMidiEvent *event = (VstMidiEvent *)ev;
        char *midiData = event->midiData;
        long status = midiData[0] & 0xf0; // ignoring channel
        long channel = midiData[0] & 0x0f;
        if (status == 0x90 || status == 0x80) // we only look at notes
        {
            long newnote = (midiData[1] & 0x7f) + sobj->scpbtranspose;
            if ((newnote > 0) && (newnote < 128))
            {
                long velo = midiData[2] & 0x7f;
                if ((status == 0x80) || (velo == 0))
                {
                    sobj->release_note((char)channel, (char)newnote, (char)velo);
                }
                else
                {
                    sobj->play_note((char)channel, (char)newnote, (char)velo);
                }
            }
        }
        else if (status == 0xe0) // pitch bend
        {
            long value = (midiData[1] & 0x7f) + ((midiData[2] & 0x7f) << 7);
            sobj->pitch_bend((char)channel, value - 8192);
        }
        else if (status == 0xB0) // controller
        {
            if (midiData[1] == 0x7b || midiData[1] == 0x7e)
                sobj->all_notes_off(); // all notes off
            else
            {
                int rid =
                    sobj->channel_controller((char)channel, midiData[1] & 0x7f, midiData[2] & 0x7f);
                // if((rid>=0)&&(rid<8))
                {
                    float v = midiData[2] & 0x7f;
                    v /= 127.f;
                    setParameterAutomated(rid, v);
                }
            }
        }
        else if (status == 0xC0) // program change
        {
            sobj->programchange(midiData[1] & 0x7f);
        }
        else if (status == 0xD0) // channel aftertouch
        {
            sobj->channel_aftertouch((char)channel, midiData[1] & 0x7f);
        }
        else if ((status == 0xfc) || (status == 0xff)) //  MIDI STOP or reset
        {
            sobj->all_notes_off();
        }

        // l�gg till allnotesoff etc.. sno fr�n pdroidmk2
    }
}

//-------------------------------------------------------------------------------------------------------
void scpb_vsti::setProgramName(char *name) { strcpy(programName, name); }

//-----------------------------------------------------------------------------------------
void scpb_vsti::getProgramName(char *name) { strcpy(name, programName); }

//-----------------------------------------------------------------------------------------
void scpb_vsti::setParameter(long index, float value)
{
    if (!initialized)
        return;

    if ((index >= kCtrl0) && (index <= kCtrl_last))
    {
        sobj->set_ccontrol(index, value);
    }
    else
    {
        switch (index)
        {
        case kAmplitude:
            sobj->controllers_target[c_custom0 + index] = value;
            break;
        case kAEG_A:
        case kAEG_D:
        case kAEG_R:
            // sobj->adsr[(index-kAEG_A)&3] = 2.f*value - 1;
            // sobj->automation[(index-kAEG_A)&3] = 13*sobj->adsr[(index-kAEG_A)&3];
            sobj->controllers_target[c_custom0 + index] = 13 * (2.f * value - 1);
            break;
        case kAEG_S:
            // sobj->adsr[(index-kAEG_A)&3] = 2.f*value - 1;
            // sobj->automation[(index-kAEG_A)&3] = 1*sobj->adsr[(index-kAEG_A)&3];
            sobj->controllers_target[c_custom0 + index] = 2.f * value - 1;
            break;
        };
    }

    // if(index<num_automation_parameters) sobj->automation[index] = value;

    if (editor)
        ((scpb_editor *)editor)->setParameter(index, value);
}

//-----------------------------------------------------------------------------------------
float scpb_vsti::getParameter(long index)
{
    if (!initialized)
        init();
    if ((index < kAEG_A))
    {
        return sobj->get_ccontrol(index);
    }
    else
    {
        switch (index)
        {
        case kAmplitude:
            return sobj->controllers_target[c_custom0 + index];
            break;
        case kAEG_S:
            return 0.5 + 0.5 * sobj->controllers_target[c_custom0 + index];
        case kAEG_A:
        case kAEG_D:
        case kAEG_R:
            return 0.5 + 0.5 * sobj->controllers_target[c_custom0 + index] * (1.f / 13.f);
            // return 0.5 + 0.5*sobj->controllers_target[8 + ((index-kAEG_A)&3)];
            // return 0.5 + 0.5*sobj->adsr[(index-kAEG_A)&3];
        };
    }
    return 0;
}

float scpb_vsti::getParameterLag(long index)
{
    if (!initialized)
        return 0;
    if ((index >= 0) && (index <= 16))
    {
        return sobj->get_ccontrol_lag(index - kCtrl0);
    }
    else if (index < num_automation_parameters)
        return sobj->automation[index];
    return 0;
}

//-----------------------------------------------------------------------------------------
void scpb_vsti::getParameterName(long index, char *label)
{
    if ((index >= kCtrl0) && (index <= kCtrl_last))
    {
        if (!sobj)
            init();
        sprintf(label, "%s", sobj->customcontrollers[index - kCtrl0].c_str());
        return;
    }

    switch (index)
    {
    case kAmplitude:
        sprintf(label, "Amplitude");
        break;
    case kAEG_A:
        sprintf(label, "Attack");
        break;
    case kAEG_D:
        sprintf(label, "Decay");
        break;
    case kAEG_S:
        sprintf(label, "Sustain");
        break;
    case kAEG_R:
        sprintf(label, "Release");
        break;
    default:
        sprintf(label, "");
        break;
    };
}

//-----------------------------------------------------------------------------------------
void scpb_vsti::getParameterDisplay(long index, char *text)
{
    if (!initialized)
        return;
    if ((index >= 0) && (index <= 16))
    {
        sprintf(text, "%f", sobj->get_ccontrol(index));
        return;
    }
    else
    {
        switch (index)
        {
        case kAmplitude:
            sprintf(text, "%f", sobj->mastergain);
            return;
        case kAEG_A:
        case kAEG_D:
        case kAEG_S:
        case kAEG_R:
            sprintf(text, "%f", sobj->adsr[(index - kAEG_A) & 3]);
            return;
        };
    }
    sprintf(text, "-");
}

bool scpb_vsti::isParameterBipolar(long index)
{
    if (!initialized)
        return false;
    if ((index >= kCtrl0) && (index <= kCtrl_last))
    {
        return sobj->is_ccontrol_bipolar(index - kCtrl0);
    }
    else
    {
        switch (index)
        {
        case kAmplitude:
            return false;
        case kAEG_A:
        case kAEG_D:
        case kAEG_S:
        case kAEG_R:
            return true;
        };
    }
    return false;
}

//-----------------------------------------------------------------------------------------
void scpb_vsti::getParameterLabel(long index, char *label) { strcpy(label, ""); }

//------------------------------------------------------------------------
bool scpb_vsti::getEffectName(char *name)
{
    // strcpy (name, "shortcircuit player");
    strcpy(name, "Swiss Army Synth");

    return true;
}

//------------------------------------------------------------------------
bool scpb_vsti::getProductString(char *text)
{
    // strcpy (text, "shortcircuit player");
    strcpy(text, "Swiss Army Synth");

    return true;
}

//------------------------------------------------------------------------
bool scpb_vsti::getVendorString(char *text)
{
    strcpy(text, "vember|audio & PowerFX");
    return true;
}
//-----------------------------------------------------------------------------------------
template <bool replacing>
void scpb_vsti::processT(float **inputs, float **outputs, long sampleFrames)
{
    if (!initialized)
        return;

    // do each buffer
    VstTimeInfo *timeinfo = getTimeInfo(kVstPpqPosValid | kVstTempoValid | kVstTransportPlaying);
    if (timeinfo->flags & kVstTempoValid)
        sobj->time_data.tempo = timeinfo->tempo;
    if (timeinfo->flags & kVstTransportPlaying)
    {
        if (timeinfo->flags & kVstPpqPosValid)
            sobj->time_data.ppqPos = timeinfo->ppqPos;
    }

    int i;
    for (i = 0; i < sampleFrames; i++)
    {
        if (blockpos == 0)
        {
            // move clock
            float ipart;
            timedata *td = &(sobj->time_data);
            td->ppqPos += (double)BLOCK_SIZE * sobj->time_data.tempo / (60. * sampleRate);
            td->pos_in_beat = modf(td->ppqPos, &ipart);
            td->pos_in_2beats = modf(td->ppqPos * 0.5, &ipart);
            td->pos_in_bar = modf(td->ppqPos * 0.25, &ipart);
            td->pos_in_2bars = modf(td->ppqPos * 0.125, &ipart);
            td->pos_in_4bars = modf(td->ppqPos * 0.0625, &ipart);

            // process events for the current block
            while (events_processed < events_this_block)
            {
                if (i >= eventbuffer[events_processed].deltaFrames)
                {
                    handleEvent(&eventbuffer[events_processed]);
                    events_processed++;
                }
                else
                    break;
            }

            // run sampler engine for the current block
            sobj->process_audio();
        }
        int outp;
        for (outp = 0; outp < (2 * output_pairs); outp++)
        {
            if (replacing)
                outputs[outp][i] = sobj->output[outp][blockpos]; // replacing
            else
                outputs[outp][i] += sobj->output[outp][blockpos]; // adding
        }
        for (outp = 0; outp < outputs_mono; outp++)
        {
            if (replacing)
                outputs[2 * output_pairs + outp][i] =
                    sobj->output[outputs_mono_index + outp][blockpos]; // replacing
            else
                outputs[2 * output_pairs + outp][i] +=
                    sobj->output[outputs_mono_index + outp][blockpos]; // adding
        }
        blockpos++;
        if (blockpos >= BLOCK_SIZE)
            blockpos = 0;
    }

    // process out old events that didn't finish last block
    while (events_processed < events_this_block)
    {
        handleEvent(&eventbuffer[events_processed]);
        events_processed++;
    }
}

//-----------------------------------------------------------------------------------------
void scpb_vsti::process(float **inputs, float **outputs, long sampleFrames)
{
    processT<false>(inputs, outputs, sampleFrames);
}

//-----------------------------------------------------------------------------------------
void scpb_vsti::processReplacing(float **inputs, float **outputs, long sampleFrames)
{
    processT<true>(inputs, outputs, sampleFrames);
}

//-----------------------------------------------------------------------------------------

long scpb_vsti::fxIdle()
{
    if (sobj && supports_fxIdle)
        sobj->idle();
    return 1;
}

//-----------------------------------------------------------------------------------------

bool scpb_vsti::getOutputProperties(long index, VstPinProperties *properties)
{
    if (index < (output_pairs * 2))
    {
        sprintf(properties->label, "out %i-%i", (index & 0xfe) + 1, (index & 0xfe) + 2);
        properties->flags = kVstPinIsStereo | kVstPinIsActive;
        return true;
    }
    else if (index < (outputs_mono + output_pairs * 2))
    {
        int mono = index - output_pairs * 2;
        sprintf(properties->label, "out %im", mono + 1);
        properties->flags = kVstPinIsActive;
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------------------

long scpb_vsti::getMidiProgramName(long channel, MidiProgramName *midiProgramName)
{
    if (!sobj)
        init();

    if (channel)
        return 0;

    sprintf(midiProgramName->name, "default");

    int id = midiProgramName->thisProgramIndex;
    for (int c = 0; c < sobj->patchlist.size(); c++)
    {
        midiProgramName->midiProgram = id & 0x7F;
        midiProgramName->midiBankLsb = (id >> 7) & 0x7F;
        midiProgramName->midiBankMsb = id >> 14;
        midiProgramName->parentCategoryIndex = -1;
        // midiProgramName->parentCategoryIndex = sobj->patchlist.at(id).category;
        midiProgramName->flags = kMidiIsOmni;

        strncpy(midiProgramName->name, sobj->patchlist.at(id).name.c_str(), 64);

        return sobj->patchlist.size();
    }

    return sobj->patchlist.size();
}

long scpb_vsti::getCurrentMidiProgram(long channel, MidiProgramName *currentProgram)
{
    if (!sobj)
        init();
    if (sobj->currentpatch < 0)
        return 0;

    strncpy(currentProgram->name, sobj->patchlist.at(sobj->currentpatch).name.c_str(), 64);
    currentProgram->midiBankLsb = (sobj->currentpatch >> 7) & 0x7F;
    currentProgram->midiBankMsb = sobj->currentpatch >> 14;
    currentProgram->midiProgram = sobj->currentpatch & 0x7F;
    currentProgram->parentCategoryIndex = -1;
    // currentProgram->parentCategoryIndex = sobj->patchlist.at(sobj->currentpatch).category;
    currentProgram->flags = kMidiIsOmni;
    return (sobj->currentpatch);
}

long scpb_vsti::getMidiProgramCategory(long channel, MidiProgramCategory *category)
{
    return 0;
    /*if(!sobj) init();
    if(channel) return 0;

    int cid = category->thisCategoryIndex;

    category_entry *e = sobj->find_category(cid);
    if(e)
    {
            if(e->parent_ptr) category->parentCategoryIndex = e->parent_ptr->id;
            else category->parentCategoryIndex = -1;
            strncpy(category->name,e->name.c_str(),64);
            return sobj->n_categories;
    }
    return sobj->n_categories;*/
}

void scpb_vsti::setSampleRate(float sampleRate)
{
    AudioEffectX::setSampleRate(sampleRate);

    if (sobj)
        sobj->set_samplerate(sampleRate);
}
/*
long scpb_vsti::getNumCategories ()
{
        if (sobj)
        {
                return sobj->n_categories();
        }
        return 0;
}

//-----------------------------------------------------------------------------------------

bool scpb_vsti::getMidiKeyName (long channel, MidiKeyName* keyName)
{
        return false;
        if((keyName->thisKeyNumber&1)==0)
                strcpy(keyName->keyName, "even");
        else
                strcpy(keyName->keyName, "odd");

        write_log("keyname query");
        return true;
}

*/