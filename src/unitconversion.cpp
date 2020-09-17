#include "unitconversion.h"

#include <stdio.h>
#include <algorithm>
#include "tools.h"

using namespace std;

float lfo_phaseincrement(int samples, float rate)
{
	rate = 1 - rate;
	return samples*env_phasemulti/( 1 + lfo_range * rate * rate * rate );
}

float dB_to_scamp(float in)			// ff rev2
{
	float v = powf(10,-0.05f*in);
	v = max(0.f,v);
	v = min(1.f,v);
	return v;	
}

double linear_to_dB(double in)	
{
	return 20*log10(in);	
}

double dB_to_linear(double in)	
{
	return pow((double)10,0.05*in);			
}

float timecent_to_seconds(float in)
{
	return powf(2,in/1200);
}

float seconds_to_envtime(float in)		// ff rev2
{
	float v = powf(in/30.f,1.f/3.f);
	v = max(0.f,v);
	v = min(1.f,v);
	return v;
}

float log2(float x)
{
	return log(x)/log(2.f);
}

char* get_notename(char *s, int i_value)
{
	int octave = (i_value/12)-2;
	char notenames[12][3] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
	sprintf(s,"%s%i",notenames[i_value%12],octave);
	return s;
}

float timecent_to_envtime(float in)
{
	//return seconds_to_envtime(timecent_to_seconds(in));
	return (in / 1200.f);
}

float timecent_to_envtime_GIGA(float in)
{
	//return seconds_to_envtime(timecent_to_seconds(in));
	return in * (1.f/(1200.f*65536.f));
}