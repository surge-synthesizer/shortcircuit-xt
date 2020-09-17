#include "sampler.h"
#include "configuration.h"

enum
{
	auto_p_gain=0,
	auto_p_pan,
	auto_p_a1_gain,
	auto_p_a1_pan,
	auto_p_a2_gain,
	auto_p_a2_pan,
	auto_p_c1,
	auto_p_c_last = auto_p_c1+n_custom_controllers,	
};

unsigned int sampler::auto_get_n_parameters()
{
	return 1;
}
char* sampler::auto_get_parameter_name(unsigned int)
{
	return "";
}
char* sampler::auto_get_parameter_display(unsigned int)
{
	return "";
}
float sampler::auto_get_parameter_value(unsigned int)
{
	return 0.f;
}
void sampler::auto_set_parameter_value(unsigned int,float)
{
}