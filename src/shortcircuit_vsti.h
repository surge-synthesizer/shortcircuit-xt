//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <audioeffectx.h>

#include "mdump.h"

class sampler;

//-------------------------------------------------------------------------------------------------------

enum
{
	kAmplitude=0,
	kCutoff,
	kReso,
	kAEG_attack,
	kAEG_decay,
	kAEG_sustain,
	kAEG_release,
	kNumparams
};

//-------------------------------------------------------------------------------------------------------
class shortcircuit_vsti : public AudioEffectX
{
public:
	shortcircuit_vsti (audioMasterCallback audioMaster);
	~shortcircuit_vsti ();

	virtual long vendorSpecific(long lArg1, long lArg2, void* ptrArg, float floatArg);

	// Processes	
	template<bool replacing> void ProcessInternal(float **inputs, float **outputs, VstInt32 sampleFrames);
	virtual void process (float **inputs, float **outputs, VstInt32 sampleFrames);
	virtual void processReplacing (float **inputs, float **outputs, VstInt32 sampleFrames);
	virtual VstInt32 processEvents (VstEvents* ev);


	// Host->Client calls
	virtual void setProgramName (char *name);
	virtual void getProgramName (char *name);
	virtual void resume();
	virtual VstInt32 fxIdle();
	virtual void suspend();
	virtual VstInt32 stopProcess();
	virtual void setParameter (VstInt32 index, float value);
	virtual float getParameter (VstInt32 index);
	virtual void getParameterLabel (VstInt32 index, char *label);
	virtual void getParameterDisplay (VstInt32 index, char *text);
	virtual void getParameterName (VstInt32 index, char *text);
	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual VstInt32 getVendorVersion () { return 1000; }		
	virtual VstInt32 canDo (char* text);	
	virtual VstPlugCategory getPlugCategory () { return kPlugCategSynth; }	
	virtual bool hasMidiProgramsChanged (VstInt32 channel) { bool t = updateMidiKeyName; updateMidiKeyName = false; return t; }
	virtual VstInt32 getMidiProgramName (VstInt32 channel, MidiProgramName* midiProgramName);
	virtual VstInt32 getCurrentMidiProgram (VstInt32 channel, MidiProgramName* currentProgram);
	virtual bool getMidiKeyName (VstInt32 channel, MidiKeyName* keyName);
	virtual bool getOutputProperties (VstInt32 index, VstPinProperties * properties);
	virtual void setSampleRate(float sampleRate);
	virtual bool keysRequired () { return true; }	

protected:
	// internal calls		
	void handleEvent(VstEvent*);	
	int oldblokkosize;
	int blockpos;
	void fpuflags_set();
	void fpuflags_restore();	

public:
	// data	
	sampler *sobj;
	VstEvent eventbuffer[1024];
	unsigned int events_this_block, events_processed;

	bool initialized;
	void init();
	float fGain;
	bool supports_fxIdle;
	float parameters[kNumparams];
	char programName[32];
	bool updateMidiKeyName;
	int sse_state_flag,old_sse_state,old_fpu_state;
	bool mInProcess;
	int mNumOutputs;	

	MiniDumper* mpMiniDumper;
};

