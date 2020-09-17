//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#ifndef __SHORTCIRCUIT_VSTI_H
#include "shortcircuit_vsti.h"
#endif

#include <float.h>
#include <fpieee.h>
#include <excpt.h>
#include "shortcircuit_editor2.h"
#include "cpuarch.h"
#include "sampler.h"
#include "configuration.h"
#include <vt_util/vt_string.h>

//#include "mdump.h"

//MiniDumper mdumper("shortcircuit");

int SSE_VERSION;

static int InstanceCount = 0;

//-------------------------------------------------------------------------------------------------------
shortcircuit_vsti::shortcircuit_vsti (audioMasterCallback audioMaster)
	: AudioEffectX (audioMaster, 1, n_automation_parameters)	// 1 program, 1 parameter only	
{				
	// load configuration
	configuration conf;		
	wchar_t path[1024];
	extern void* hInstance;
	GetModuleFileNameW((HMODULE)hInstance,path,1024);
	wchar_t *end = wcsrchr(path,L'\\');
	wcscpy(end,L"\\shortcircuitV2-conf.xml");
	conf.load(path);
	mNumOutputs = conf.stereo_outputs;			

	// Init minidumper (add yes/no choice to options)
	if ((InstanceCount == 0) && conf.mUseMiniDumper)
		mpMiniDumper = new MiniDumper("Shortcircuit");
	else 
		mpMiniDumper = 0;
	InstanceCount++;
	
	setNumInputs (0);		// stereo in
	setNumOutputs(mNumOutputs*2);		// multi out
	isSynth();
	setUniqueID ('SCv2');	// identify

	mInProcess = false;

	//#ifndef _DEMO_

	programsAreChunks(true);
	//#endif
	//canMono ();				// makes sense to feed both inputs with the same signal
	canProcessReplacing ();	// supports both accumulating and replacing output
//	supports_fxIdle = needIdle();
	strcpy (programName, "Default");	// default program name	
	//write_log("vsti constructor");
	initialized = false;	
	// NULL objects	
	sobj = NULL;	
	events_this_block = 0;
	events_processed = 0;
	oldblokkosize = 0;
	updateMidiKeyName = true;

#if PPC
	long cpuAttributes;
	Boolean hasAltiVec = false;
	OSErr err = Gestalt( gestaltPowerPCProcessorFeatures, &cpuAttributes );
	if( noErr == err )
		hasAltiVec = ( 1 << gestaltPowerPCHasVectorInstructions) & cpuAttributes;

	//	if (!hasAltiVec)  throw hissy fit
#else
	unsigned int arch = determine_support();
	// detect 
	if(arch & ca_SSE3)
	{
		sse_state_flag = 0x8040;
		SSE_VERSION = 3;
	}
	else if(arch & ca_SSE2)
	{
		sse_state_flag = 0x8040;
		SSE_VERSION = 2;
	}
	else if (arch & ca_SSE) 
	{
		sse_state_flag = 0x8000;
		SSE_VERSION = 1;
#if (_M_IX86_FP >= 2)
	MessageBox(::GetActiveWindow(),"This binary requires a CPU supporting the SSE2 instruction set.\n\n"
		"Please rerun the installer and choose the P3/K7 binary when asked",
		"System requirements not met",MB_OK | MB_ICONERROR);
#endif
	}
	else 
	{
		SSE_VERSION = 0;
		MessageBox(::GetActiveWindow(),"This plugin requires a CPU supporting the SSE instruction set.","System requirements not met",MB_OK | MB_ICONERROR);
		//oome = true;
	}

	if(!(arch & ca_CMOV))
	{
		MessageBox(::GetActiveWindow(),"This plugin requires a CPU supporting the CMOV instruction.","System requirements not met",MB_OK | MB_ICONERROR);
	}

#endif
}

//-------------------------------------------------------------------------------------------------------
shortcircuit_vsti::~shortcircuit_vsti ()
{		
	//write_log("vsti deconstructor");
	
	// delete objects	
	if (sobj) 
	{
		sobj->~sampler();		
		_mm_free(sobj);
	}	

	delete mpMiniDumper;
}

//-------------------------------------------------------------------------------------------------------

long shortcircuit_vsti::vendorSpecific(long lArg1, long lArg2, void* ptrArg, float floatArg)
{
  if(editor && lArg1 == 'stCA' && lArg2 == 'Whee') //support mouse wheel!
    //return editor->onWheel(floatArg) == true ? 1 : 0;
	return ((sc_editor2*)editor)->take_focus();
  else
    return AudioEffectX::vendorSpecific(lArg1, lArg2, ptrArg, floatArg);
}

//-------------------------------------------------------------------------------------------------------
void shortcircuit_vsti::suspend()
{
	//write_log("suspend");
	if (initialized) 
	{
		sobj->AllNotesOff();
		sobj->AudioHalted = true;
	}
}

VstInt32 shortcircuit_vsti::stopProcess()
{
	if (initialized) sobj->AllNotesOff();
	return 1;
}
//-------------------------------------------------------------------------------------------------------

void shortcircuit_vsti::init()
{
	//write_log("init");
	
	sobj = (sampler*) _mm_malloc(sizeof(sampler),16);	
	new(sobj) sampler(editor, mNumOutputs);		
	
	assert(sobj);
	sobj->set_samplerate(this->getSampleRate());
	/*char tmp[4096];
	if (getChunkFile (tmp))
	{
		char *r = strrchr(tmp,'\\');
		if(r) *r = 0;
		vtCopyString(sobj->conf->projdir,tmp,255);		
		sobj->conf->projdir[255] = 0;
	}*/
	blockpos = 0;
	initialized = true;	
}

//-------------------------------------------------------------------------------------------------------
void shortcircuit_vsti::resume()
{
	//write_log("resume");
	if (!initialized){
		init();			
	}

	sobj->set_samplerate(this->getSampleRate());
	sobj->AudioHalted = false;

	//wantEvents ();		
	AudioEffectX::resume();
}

//------------------------------------------------------------------------

VstInt32 shortcircuit_vsti::canDo (char* text)
{
	if (!strcmp (text, "receiveVstEvents"))
		return 1;

	if (!strcmp (text, "receiveVstMidiEvent"))
		return 1;

	if (!strcmp (text, "midiProgramNames"))
		return 1;	
	
	return -1;
}

//------------------------------------------------------------------------

VstInt32 shortcircuit_vsti::processEvents (VstEvents* ev)
{
	assert(!mInProcess);
	if(initialized){				
		this->events_this_block = min(ev->numEvents,1024);
		this->events_processed = 0;		
		
		unsigned int i;
		for(i=0; i<this->events_this_block; i++)
		{
			this->eventbuffer[i] = *(ev->events[i]);
		}			
	}
	return 1; // want more
}

//------------------------------------------------------------------------

void shortcircuit_vsti::handleEvent(VstEvent* ev){
	if (!initialized) return;
	if (ev->type == kVstMidiType){		
		VstMidiEvent* event = (VstMidiEvent*)ev;
		char* midiData = event->midiData;
		long status = midiData[0] & 0xf0; // ignoring channel
		long channel = midiData[0] & 0x0f; // ignoring channel
		if (status == 0x90 || status == 0x80) // we only look at notes
		{
			long newnote = midiData[1] & 0x7f;
			long velo = midiData[2] & 0x7f;
			if ((status == 0x80)||(velo == 0))
			{				
				sobj->ReleaseNote((char)channel,(char)newnote,(char)velo);
			}
			else
			{
				sobj->PlayNote((char)channel,(char)newnote,(char)velo,false,event->detune);								
			}
		}
		else if (status == 0xe0)		// pitch bend
		{
			long value = (midiData[1] & 0x7f) + ((midiData[2] & 0x7f) << 7);
			sobj->PitchBend((char)channel,value-8192);
		} 
		else if (status == 0xB0)		// controller
		{			
			if (midiData[1] == 0x7b || midiData[1] == 0x7e) 
			{
				sobj->AllNotesOff();	// all notes off
			}
			else 
			{
				sobj->ChannelController((char)channel,midiData[1] & 0x7f,midiData[2] & 0x7f);
			}

		}
		else if (status == 0xD0)		// channel aftertouch
		{			
			sobj->ChannelAftertouch((char)channel,midiData[1] & 0x7f);
		} else if (( status == 0xfc )||( status == 0xff  )) //  MIDI STOP or reset
		{			
			sobj->AllNotesOff();		
		} 

		// lägg till allnotesoff etc.. sno från pdroidmk2

		// send a copy to the editor for MIDI-learn purposes
		//if (editor)	((shortcircuit_editor*)editor)->sendMidiMsg(midiData[0],midiData[1],midiData[2]);
	}
}

//-------------------------------------------------------------------------------------------------------
void shortcircuit_vsti::setProgramName (char *name)
{
	strcpy (programName, name);
}

//-----------------------------------------------------------------------------------------
void shortcircuit_vsti::getProgramName (char *name)
{	
	strcpy (name, programName);
}

//-----------------------------------------------------------------------------------------
void shortcircuit_vsti::setParameter (VstInt32 index, float value)
{
	assert(!mInProcess);
	if (!initialized) return;	
	sobj->auto_set_parameter_value(index,value);
}

//-----------------------------------------------------------------------------------------
float shortcircuit_vsti::getParameter (VstInt32 index)
{
	if (!initialized) return 0;	
	return sobj->auto_get_parameter_value(index);
}

//-----------------------------------------------------------------------------------------
void shortcircuit_vsti::getParameterName (VstInt32 index, char *label)
{
	if (!initialized) return;	
	vtCopyString(label,sobj->auto_get_parameter_name(index),32);
}

//-----------------------------------------------------------------------------------------
void shortcircuit_vsti::getParameterDisplay (VstInt32 index, char *text)
{
	if (!initialized) return;	
	vtCopyString(text,sobj->auto_get_parameter_display(index),32);
}

//-----------------------------------------------------------------------------------------
void shortcircuit_vsti::getParameterLabel(VstInt32 index, char *label)
{
	strcpy (label, "");
}

//------------------------------------------------------------------------
bool shortcircuit_vsti::getEffectName (char* name)
{
    strcpy( name, "ShortCircuit3" );
	return true;
}

//------------------------------------------------------------------------
bool shortcircuit_vsti::getProductString (char* text)
{
    strcpy (text, "ShortCircuit3");
	return true;
}

//------------------------------------------------------------------------
bool shortcircuit_vsti::getVendorString (char* text)
{
	strcpy (text, "Surge Synth Team");
	return true;
}

//-----------------------------------------------------------------------------------------
void shortcircuit_vsti::fpuflags_set()
{	
#if !PPC
	old_sse_state = _mm_getcsr();
	_mm_setcsr(_mm_getcsr() | sse_state_flag);	
#ifdef _DEBUG		
	_MM_SET_EXCEPTION_STATE(_MM_EXCEPT_DENORM);

	old_fpu_state = _control87(0,0);
	int u = old_fpu_state & ~(/*_EM_INVALID | _EM_DENORMAL |*/ _EM_ZERODIVIDE | _EM_OVERFLOW /* | _EM_UNDERFLOW | _EM_INEXACT*/);
	_control87(u, _MCW_EM);	
	
#endif			
#endif
}

//-----------------------------------------------------------------------------------------

void shortcircuit_vsti::fpuflags_restore()
{	
#if !PPC
	_mm_setcsr(old_sse_state);
#ifdef _DEBUG
	_control87(old_fpu_state,_MCW_EM);	
#endif
#endif
}

//-----------------------------------------------------------------------------------------

template<bool replacing> 
void shortcircuit_vsti::ProcessInternal(float **inputs, float **outputs, VstInt32 sampleFrames)
{      		
	assert(initialized);
	if (!initialized) return;

#ifdef DEBUG
	assert(!mInProcess);
	mInProcess = true;
#endif	

	fpuflags_set();		

	// do each buffer
	VstTimeInfo *timeinfo = getTimeInfo(kVstPpqPosValid | kVstTempoValid | kVstTransportPlaying);
	if (timeinfo)
	{
		if(timeinfo->flags & kVstTempoValid)	sobj->time_data.tempo = timeinfo->tempo;	
		if(timeinfo->flags & kVstTransportPlaying)
		{
			if(timeinfo->flags & kVstPpqPosValid)	sobj->time_data.ppqPos = timeinfo->ppqPos;
		}
	}	

	int i=0;	
	
	while(i<sampleFrames)
	{		
		if(blockpos == 0)
		{			
			// move clock			
			float ipart;
			timedata *td = &(sobj->time_data);
			td->ppqPos += (double) block_size * sobj->time_data.tempo/(60. * sampleRate);			
			td->pos_in_beat = modf((float)td->ppqPos,&ipart);
			td->pos_in_2beats = modf((float)td->ppqPos*0.5f,&ipart);			
			td->pos_in_bar = modf((float)td->ppqPos*0.25f,&ipart);
			td->pos_in_2bars = modf((float)td->ppqPos*0.125f,&ipart);
			td->pos_in_4bars = modf((float)td->ppqPos*0.0625f,&ipart);

			// process events for the current block
			while(events_processed < events_this_block)
			{
				if (i >= eventbuffer[events_processed].deltaFrames)
				{
					handleEvent(&eventbuffer[events_processed]);
					events_processed++;
				}
				else
					break;
			}

			// run sampler engine for the block
			sobj->process_audio();
		}		

		int ns = min(sampleFrames-i, block_size-blockpos);

		assert(ns > 0);
		assert(ns <= block_size);

		for(unsigned int outp=0; outp<(mNumOutputs<<1); outp++)		
		{			
			if (replacing)
			{
				memcpy(&outputs[outp][i], &sobj->output[outp][blockpos], ns*sizeof(float));
			}
			else
			{
				for(int k=0; k<ns; k++)
				{
					outputs[outp][i+k] += sobj->output[outp][blockpos+k];
				}
			}				
		}				
		blockpos+=ns;
		i+=ns;			

		blockpos = blockpos & (block_size-1);
	}

	assert(i == sampleFrames);

	// process out old events that didn't finish last block
	while(events_processed < events_this_block)
	{
		handleEvent(&eventbuffer[events_processed]);
		events_processed++;
	}
	fpuflags_restore();

	// catch silent block hack
	assert(outputs[0][3] != 0.f);

#ifdef DEBUG
	mInProcess = false;
#endif
}

//-----------------------------------------------------------------------------------------

void shortcircuit_vsti::processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames)
{
	ProcessInternal<true>(inputs, outputs, sampleFrames);
}

//-----------------------------------------------------------------------------------------

void shortcircuit_vsti::process(float **inputs, float **outputs, VstInt32 sampleFrames)
{
	ProcessInternal<false>(inputs, outputs, sampleFrames);
}

//-----------------------------------------------------------------------------------------

VstInt32 shortcircuit_vsti::fxIdle()
{
	if (sobj && supports_fxIdle) sobj->idle();
	return 1;
}

//-----------------------------------------------------------------------------------------


bool shortcircuit_vsti::getOutputProperties (VstInt32 index, VstPinProperties* properties)
{	
	if (index < (mNumOutputs*2))
	{
		sprintf (properties->label, "sc %i-%i", (index&0xfe) + 1, (index&0xfe) + 2);
		properties->flags = kVstPinIsStereo | kVstPinIsActive;
		return true;
	} 
	return false;
}

//-----------------------------------------------------------------------------------------


VstInt32 shortcircuit_vsti::getMidiProgramName (VstInt32 channel, MidiProgramName* midiProgramName)
{
	sprintf(midiProgramName->name, "default");
	midiProgramName->midiBankLsb = -1;
	midiProgramName->midiBankMsb = -1;
	midiProgramName->midiProgram = 0;
	midiProgramName->parentCategoryIndex = -1;
	midiProgramName->flags = 0;
	return 1;
}

VstInt32 shortcircuit_vsti::getCurrentMidiProgram (VstInt32 channel, MidiProgramName* currentProgram)
{
	sprintf(currentProgram->name, "default");
	currentProgram->midiBankLsb = -1;
	currentProgram->midiBankMsb = -1;
	currentProgram->midiProgram = 0;
	currentProgram->parentCategoryIndex = -1;
	currentProgram->flags = 0;
	return 0;
}

void shortcircuit_vsti::setSampleRate (float sampleRate)
{
	AudioEffectX::setSampleRate (sampleRate);

	if (sobj) sobj->set_samplerate(sampleRate);
}

//-----------------------------------------------------------------------------------------

bool shortcircuit_vsti::getMidiKeyName (VstInt32 channel, MidiKeyName* keyName)
{ 			
	if(!sobj) return false;
	
	sobj->get_key_name(keyName->keyName, channel, keyName->thisKeyNumber);
	return true; 
}

