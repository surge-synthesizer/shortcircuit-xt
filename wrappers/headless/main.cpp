#include <iomanip>
#include <iostream>
#include <memory>

#include "sampler.h"
#include "version.h"

void *hInstance = 0;
int main(int argc, char **argv)
{
    std::cout << "ShortCircuit3 Headless. " << ShortCircuit::Build::FullVersionStr << std::endl;

    auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr);
    sc3->set_samplerate(48000);
    if (!sc3)
        std::cout << "Couldn't make an SC3" << std::endl;
    else
        std::cout << "Made an SC3" << std::endl;

    // this is easy - we expose no automation parameters
    int np = sc3->auto_get_n_parameters();
    for (int i = 0; i < np; ++i)
    {
        std::cout << i << " " << sc3->auto_get_parameter_name(i) << " "
                  << sc3->auto_get_parameter_display(i) << " " << sc3->auto_get_parameter_value(i)
                  << std::endl;
    }

    if(true)
    {
        std::cout << "Loading harpsi.sf2" << std::endl;
        auto res = sc3->load_file("resources/test_samples/harpsi.sf2");
        std::cout << "RES is " << res << std::endl;
    }
    else
    {
        std::cout << "Loading Bad Pluck" << std::endl;
#if WINDOWS
        auto res = sc3->load_file("resources\\test_samples\\BadPluckSample.wav");
#else
        auto res = sc3->load_file("resources/test_samples/BadPluckSample.wav");
#endif

        std::cout << "RES is " << res << std::endl;
    }
    for( int n=0; n<127; ++n )
    {
        double rms = 0;

        for (int i = 0; i < 100; ++i)
        {
            if (i == 30)
                sc3->PlayNote(0, n, 120);
            if( i == 70 )
                sc3->ReleaseNote(0,n,0);

            sc3->process_audio();
            for (int k = 0; k < block_size; ++k)
            {
                rms +=
                    sc3->output[0][k] * sc3->output[0][k] + sc3->output[1][k] * sc3->output[1][k];
                // std::cout << sc3->output[0][k] << " " << sc3->output[1][k] << std::endl;
            }
        }
        rms = sqrt(rms) / block_size / 100;

        if( rms > 1e-10 )
            std::cout << n << " " << rms << std::endl;
    }
}
