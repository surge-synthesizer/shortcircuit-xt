#pragma once

const float env_phasemulti = 1000/44100.f;
const float lfo_range = 1000;

float lfo_phaseincrement(int samples, float rate);
float dB_to_scamp(float in);
double linear_to_dB(double in);
double dB_to_linear(double in);
float timecent_to_seconds(float in);
float seconds_to_envtime(float in);		// ff rev2
//float log2(float x) noexcept;
char* get_notename(char *s, int i_value);
float timecent_to_envtime(float in);
float timecent_to_envtime_GIGA(float in);
