#include "generator.h"
#include "globals.h"
#include "resampling.h"
#include "tools.h"
#include "sampler_voice.h"
#include <vt_dsp/basic_dsp.h>

extern float SincTableF32[(FIRipol_M+1)*FIRipol_N];
extern float SincOffsetF32[(FIRipol_M)*FIRipol_N];
extern short SincTableI16[(FIRipol_M+1)*FIRipol_N];
extern short SincOffsetI16[(FIRipol_M)*FIRipol_N];	

const float I16InvScale = (1.f/(16384.f*32768.f));
const __m128 I16InvScale_m128 = _mm_set1_ps(I16InvScale);

static int SSE_VERSION = 2;

template<bool,bool,int,int> void GeneratorSample(
	GeneratorState* __restrict GD, 
	GeneratorIO * __restrict IO );

GeneratorFPtr GetFPtrGeneratorSample(bool Stereo, bool Float, int LoopMode)
{
	if(Stereo)
	{
		if(Float)
		{
			switch(LoopMode)
			{
				case 0: return GeneratorSample<1,1,0,1>;
				case 1: return GeneratorSample<1,1,1,1>;
				case 2: return GeneratorSample<1,1,2,1>;
				case 3: return GeneratorSample<1,1,3,1>;
			}
		}
		else
		{
			if(SSE_VERSION >= 2)
			{
				switch(LoopMode)
				{
					case 0: return GeneratorSample<1,0,0,2>;
					case 1: return GeneratorSample<1,0,1,2>;
					case 2: return GeneratorSample<1,0,2,2>;
					case 3: return GeneratorSample<1,0,3,2>;
				}
			}
			else
			{
				switch(LoopMode)
				{
					case 0: return GeneratorSample<1,0,0,1>;
					case 1: return GeneratorSample<1,0,1,1>;
					case 2: return GeneratorSample<1,0,2,1>;
					case 3: return GeneratorSample<1,0,3,1>;
				}
			}
		}
	}
	else
	{
		if(Float)
		{
			switch(LoopMode)
			{
				case 0: return GeneratorSample<0,1,0,1>;
				case 1: return GeneratorSample<0,1,1,1>;
				case 2: return GeneratorSample<0,1,2,1>;
				case 3: return GeneratorSample<0,1,3,1>;
			}
		}
		else
		{
			if(SSE_VERSION >= 2)
			{
				switch(LoopMode)
				{
					case 0: return GeneratorSample<0,0,0,2>;
					case 1: return GeneratorSample<0,0,1,2>;
					case 2: return GeneratorSample<0,0,2,2>;
					case 3: return GeneratorSample<0,0,3,2>;
				}
			}
			else
			{
				switch(LoopMode)
				{
					case 0: return GeneratorSample<0,0,0,1>;
					case 1: return GeneratorSample<0,0,1,1>;
					case 2: return GeneratorSample<0,0,2,1>;
					case 3: return GeneratorSample<0,0,3,1>;
				}
			}
		}
	}
	return 0;	
}

template<bool stereo, bool fp, int playmode, int SSE>
void GeneratorSample(
	GeneratorState* __restrict GD,
	GeneratorIO* __restrict IO )
{	
	int SamplePos = GD->SamplePos;
	int SampleSubPos = GD->SampleSubPos;			
	int LowerBound = GD->LowerBound;
	int UpperBound = GD->UpperBound;
	int IsFinished = GD->IsFinished;
	int WaveSize = IO->WaveSize;
	int LoopOffset = Max(1,UpperBound - LowerBound);
	int Ratio = GD->Ratio;
	int RatioSign = Sign(Ratio);
	Ratio = abs(Ratio);
	int Direction = GD->Direction * RatioSign;
	short * __restrict SampleDataL;	
	short * __restrict SampleDataR;
	float * __restrict SampleDataFL;	
	float * __restrict SampleDataFR;
	float * __restrict OutputL;
	float * __restrict OutputR;
	
	if(fp) SampleDataFL = (float*) IO->SampleDataL;
	else SampleDataL = (short*) IO->SampleDataL;
	OutputL = IO->OutputL;	
	if(stereo)
	{		
		if(fp) SampleDataFR = (float*) IO->SampleDataR;
		SampleDataR = (short*) IO->SampleDataR;
		OutputR = IO->OutputR;
	}
	int NSamples = GD->BlockSize;	

	for(int i=0; i<NSamples; i++)
	{				
		// 2. Resample		
		unsigned int m0 =	((SampleSubPos>>12)&0xff0);
		if(fp)
		{
			// float32 path (SSE)
			__m128 lipol0, tmp[4], sL4, sR4;			
			lipol0 = _mm_setzero_ps();
			lipol0 = _mm_cvtsi32_ss(lipol0, SampleSubPos&0xffff );
			lipol0 = _mm_shuffle_ps(lipol0,lipol0,_MM_SHUFFLE(0,0,0,0));
			tmp[0] = _mm_add_ps(_mm_mul_ps(*((__m128*)&SincOffsetF32[m0]),lipol0),*((__m128*)&SincTableF32[m0]));
			tmp[1] = _mm_add_ps(_mm_mul_ps(*((__m128*)&SincOffsetF32[m0 + 4]),lipol0),*((__m128*)&SincTableF32[m0 + 4]));
			tmp[2] = _mm_add_ps(_mm_mul_ps(*((__m128*)&SincOffsetF32[m0 + 8]),lipol0),*((__m128*)&SincTableF32[m0 + 8]));
			tmp[3] = _mm_add_ps(_mm_mul_ps(*((__m128*)&SincOffsetF32[m0 + 12]),lipol0),*((__m128*)&SincTableF32[m0 + 12]));
			sL4 = _mm_mul_ps(tmp[0],_mm_loadu_ps(&SampleDataFL[SamplePos]));
			sL4 = _mm_add_ps(sL4, _mm_mul_ps(tmp[1],_mm_loadu_ps(&SampleDataFL[SamplePos+4])));
			sL4 = _mm_add_ps(sL4, _mm_mul_ps(tmp[2],_mm_loadu_ps(&SampleDataFL[SamplePos+8])));
			sL4 = _mm_add_ps(sL4, _mm_mul_ps(tmp[3],_mm_loadu_ps(&SampleDataFL[SamplePos+12])));
			sL4 = sum_ps_to_ss(sL4);
			_mm_store_ss(&OutputL[i],sL4);
			if(stereo)
			{
				sR4 = _mm_mul_ps(tmp[0],_mm_loadu_ps(&SampleDataFR[SamplePos]));
				sR4 = _mm_add_ps(sR4, _mm_mul_ps(tmp[1],_mm_loadu_ps(&SampleDataFR[SamplePos+4])));
				sR4 = _mm_add_ps(sR4, _mm_mul_ps(tmp[2],_mm_loadu_ps(&SampleDataFR[SamplePos+8])));
				sR4 = _mm_add_ps(sR4, _mm_mul_ps(tmp[3],_mm_loadu_ps(&SampleDataFR[SamplePos+12])));				
				sR4 = sum_ps_to_ss(sR4);
				_mm_store_ss(&OutputR[i],sR4);
			}
		}
		else
		{
			// int16
			if(SSE<2)
			{
#ifdef INCLUDE_BEFORE_SSE2
				// MMX path	92 cycles/sample (stereo)
				__m64 lipol0, tmp[4], sL4[4], sR4[4];		
				__m128 fL,fR;
				lipol0 = _mm_set1_pi16(SampleSubPos&0xffff);		
				tmp[0] = _mm_add_pi16(_mm_mulhi_pi16(*((__m64*)&SincOffsetI16[m0]),lipol0),*((__m64*)&SincTableI16[m0]));
				tmp[1] = _mm_add_pi16(_mm_mulhi_pi16(*((__m64*)&SincOffsetI16[m0 + 4]),lipol0),*((__m64*)&SincTableI16[m0 + 4]));
				tmp[2] = _mm_add_pi16(_mm_mulhi_pi16(*((__m64*)&SincOffsetI16[m0 + 8]),lipol0),*((__m64*)&SincTableI16[m0 + 8]));
				tmp[3] = _mm_add_pi16(_mm_mulhi_pi16(*((__m64*)&SincOffsetI16[m0 + 12]),lipol0),*((__m64*)&SincTableI16[m0 + 12]));
				sL4[0] = _mm_madd_pi16 (tmp[0],*(__m64*)&SampleDataL[SamplePos]);
				sL4[1] = _mm_madd_pi16 (tmp[1],*(__m64*)&SampleDataL[SamplePos+4]);
				sL4[2] = _mm_madd_pi16 (tmp[2],*(__m64*)&SampleDataL[SamplePos+8]);
				sL4[3] = _mm_madd_pi16 (tmp[3],*(__m64*)&SampleDataL[SamplePos+12]);
				sL4[0] = _mm_add_pi32(_mm_add_pi32(sL4[0],sL4[1]),_mm_add_pi32(sL4[2],sL4[3]));
				int *sL4ptr = (int*)&sL4;
				int rL = sL4ptr[0] + sL4ptr[1];
				fL = _mm_mul_ss(_mm_cvtsi32_ss(fL,rL), I16InvScale_m128);
				_mm_store_ss(&OutputL[i],fL);

				if(stereo)
				{
					sR4[0] = _mm_madd_pi16 (tmp[0],*(__m64*)&SampleDataR[SamplePos]);
					sR4[1] = _mm_madd_pi16 (tmp[1],*(__m64*)&SampleDataR[SamplePos+4]);
					sR4[2] = _mm_madd_pi16 (tmp[2],*(__m64*)&SampleDataR[SamplePos+8]);
					sR4[3] = _mm_madd_pi16 (tmp[3],*(__m64*)&SampleDataR[SamplePos+12]);
					sR4[0] = _mm_add_pi32(_mm_add_pi32(sR4[0],sR4[1]),_mm_add_pi32(sR4[2],sR4[3]));
					int *sR4ptr = (int*)&sR4;
					int rR = sR4ptr[0] + sR4ptr[1];
					fR = _mm_mul_ss(_mm_cvtsi32_ss(fR,rR), I16InvScale_m128);
					_mm_store_ss(&OutputR[i],fR);
				}
#endif
			}
			else
			{
				// SSE2 path
				__m128i lipol0, tmp, sL8A, sR8A, tmp2, sL8B, sR8B;
				__m128 fL,fR;
				lipol0 = _mm_set1_epi16(SampleSubPos&0xffff);

				tmp = _mm_add_epi16 (_mm_mulhi_epi16(*((__m128i*)&SincOffsetI16[m0]),lipol0),*((__m128i*)&SincTableI16[m0]));		
				sL8A = _mm_madd_epi16 (tmp,_mm_loadu_si128((__m128i*)&SampleDataL[SamplePos]));		
				if(stereo) sR8A = _mm_madd_epi16 (tmp,_mm_loadu_si128((__m128i*)&SampleDataR[SamplePos]));		
				tmp2 = _mm_add_epi16 (_mm_mulhi_epi16(*((__m128i*)&SincOffsetI16[m0+8]),lipol0),*((__m128i*)&SincTableI16[m0+8]));
				sL8B = _mm_madd_epi16 (tmp2,_mm_loadu_si128((__m128i*)&SampleDataL[SamplePos+8]));
				if(stereo) sR8B = _mm_madd_epi16 (tmp2,_mm_loadu_si128((__m128i*)&SampleDataR[SamplePos+8]));		
				sL8A = _mm_add_epi32(sL8A,sL8B);		
				if(stereo) sR8A = _mm_add_epi32(sR8A,sR8B);

				int l alignas(16) [4],r alignas(16) [4];
				_mm_store_si128((__m128i*)&l,sL8A);	
				if(stereo) _mm_store_si128((__m128i*)&r,sR8A);
				l[0] = (l[0] + l[1]) + (l[2] + l[3]);				
				if(stereo) r[0] = (r[0] + r[1]) + (r[2] + r[3]);
				fL = _mm_mul_ss(_mm_cvtsi32_ss(fL,l[0]), I16InvScale_m128);	
				if(stereo) fR = _mm_mul_ss(_mm_cvtsi32_ss(fR,r[0]), I16InvScale_m128);
				_mm_store_ss(&OutputL[i],fL);
				if(stereo) _mm_store_ss(&OutputR[i],fR);	
			}
		}

		// 3. Forward sample position
		SampleSubPos += Ratio * Direction;
		int incr = SampleSubPos>>24;
		SamplePos += incr;
		SampleSubPos = SampleSubPos - (incr<<24);

		//if (SamplePos > UpperBound) SamplePos = UpperBound;
		switch(playmode)
		{
		case GSM_Normal:		
			{				
				// Denna koden gör att sample-pekaren stannar när den kommer till start/stop
				// Det lämnar en DC-offset, men det är mindre störigt än ett klick
				// Med überreleasen så verkar det inte bli något problem.
				int zero = 0;				
				int one = 1;
#if SUPPORTS_ASM
				__asm
				{				
					; load
					mov eax, SamplePos
					mov ecx, UpperBound					
					mov edx, IsFinished
					
					; enforce upper bound
					cmp eax, ecx
					cmovge eax, ecx					
					cmovge edx, one					
					mov IsFinished, edx
					mov edx, SampleSubPos
					mov ecx, LowerBound		; start loading ahead
					cmovge edx, zero										
					
					; enforce lower bound
					cmp eax, ecx
					cmovl eax, ecx
					cmovl edx, zero
					
					; store
					mov SamplePos, eax				
					mov SampleSubPos, edx
				}
#endif
			}
			break;
		case GSM_Loop:		
			{			
#if SUPPORTS_ASM
				__asm
				{					
					; load
					mov eax, SamplePos
					mov ecx, eax
					mov edx, eax

					sub ecx, LoopOffset
					add edx, LoopOffset

					cmp Direction, 0			; välj path beroende på direction (som är toklätt att predicta)
					jg upper
;lower										
					cmp eax, LowerBound					
					cmovl eax, edx
					jmp store
upper:					
					cmp eax, UpperBound
					cmovg eax, ecx					
store:					
					; make sure its within wavesize
					cmp eax, WaveSize 
					cmova eax, UpperBound

					mov SamplePos, eax													
				}
#endif
			}
			break;
		case GSM_Bidirectional:		
			{				
				// Låter bäst då man INTE justerar SamplePos & SubPos för att den passerat Bounds

				int fram = 1, bak = -1;
#if SUPPORTS_ASM
				__asm
				{					
					; load					
					mov ecx, Direction					
					mov eax, SamplePos					

					; Lower bound					
					cmp eax, LowerBound
					cmovl ecx, fram

					; Upper bound					
					cmp eax, UpperBound
					cmovg ecx, bak

					; Store					
					mov Direction, ecx
				}
#endif
				SamplePos = limit_range(SamplePos,0,WaveSize);
			}
			break;
		case GSM_Shot:
			{								
				if(!within_range(LowerBound,SamplePos,UpperBound))
				{										
					/*sampler_voice *v = (sampler_voice*)IO->VoicePtr;									
					if (v) v->uberrelease();*/
					IsFinished = 1;
					SamplePos = limit_range(SamplePos,LowerBound,UpperBound);
				}
			}
		}
	}	
	
#if BEFORE_SSE2
	if(!fp && (SSE<2)) _mm_empty();
#endif

	GD->Direction = Direction*RatioSign;
	GD->SamplePos = SamplePos;
	GD->SampleSubPos = SampleSubPos;	
	GD->IsFinished = IsFinished;
}

/*
template<bool stereo, bool fp, int SSE>
void GeneratorStretch( DualGeneratorData * __restrict GD )
{
	bool Fading = false;
	if(Fading)
	{

	}
	else
	{
		GD->A.
		GeneratorSample<stereo, fp, GSM_Normal, SSE>(GD->A);
	}	
}*/

MultiGenerator::MultiGenerator()
{
	
}

void MultiGenerator::InitIO(
	bool Stereo,
	bool Float,
	void* SampleL,
	void* SampleR,
	int WaveSize,
	float* OutL,
	float* OutR,
	void* VoicePtr)
{
	mIO.OutputL = OutL;
	mIO.OutputR = OutR;
	mIO.SampleDataL = SampleL;
	mIO.SampleDataR = SampleR;
	mIO.WaveSize = WaveSize;
	mIO.VoicePtr = VoicePtr;
	mStereo = Stereo;
	mFloat = Float;
}

void MultiGenerator::Process()
{
	// 1.) Init new Generators to appear for this block

	// 2.) Run Generators for block
	// (could run 2, 1, 2 to make the voices last longer?)

	// 3.) Increment TimePos

	// Use a shared polyphase filter for all the oversampled Generators
	// as long as the envelope is done inside the generator this is no problem.
	// the polyphase can be skipped after no generators used it for n samples
}

void MultiGenerator::InitGenerator(
	int PitchRatio,
	int StartSample,
	int EndSample)
{
	int Voice = 0;

	mState[Voice].BlockSize = block_size;
	mState[Voice].Direction = 1;
	mState[Voice].LowerBound = StartSample;
	mState[Voice].UpperBound = EndSample;
	mState[Voice].Ratio = PitchRatio;
}