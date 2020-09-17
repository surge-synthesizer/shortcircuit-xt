#pragma once

struct GeneratorState
{
	int Direction;
	int SamplePos;
	int SampleSubPos;	
	int LowerBound;			// inclusive
	int UpperBound;			// inclusive	
	int Ratio;
	int BlockSize;		
	int IsFinished;
};

typedef void (*ReleaseCallback)();

struct GeneratorIO
{
	float* restrict OutputL;
	float* restrict OutputR;
	void* restrict SampleDataL;
	void* restrict SampleDataR;
	int WaveSize;
	void* VoicePtr;
};


// borde bo i samplingen istället
/*struct GeneratorGrain
{
	int SampleTime;
	int FadeInTime;
};*/

enum GeneratorSampleModes
{
	GSM_Normal = 0,	// can stop voice
	GSM_Loop = 1,
	GSM_Bidirectional = 2,
	GSM_Shot = 3,  // can stop voice
};

typedef void (*GeneratorFPtr)(GeneratorState * restrict, GeneratorIO * restrict);

GeneratorFPtr GetFPtrGeneratorSample(bool Stereo, bool Float, int LoopMode);

//GeneratorFPtr GetFPtrGeneratorStretching(bool Stereo, bool Float);

const int MultiGeneratorLayers = 8;

class MultiGenerator
{
public:
	MultiGenerator();
	
	void InitIO(
		bool Stereo,
		bool Float,
		void* SampleL,
		void* SampleR,
		int WaveSize,
		float* OutL,
		float* OutR,
		void* VoicePtr);

	void Process();

	void InitGenerator(
		int PitchRatio,
		int StartSample,
		int EndSample);

protected:
	GeneratorIO mIO;
	GeneratorState mState[MultiGeneratorLayers];
	GeneratorFPtr fptrGenerator[MultiGeneratorLayers];
	int mTimeRatio;
	bool mStereo;
	bool mFloat;
};