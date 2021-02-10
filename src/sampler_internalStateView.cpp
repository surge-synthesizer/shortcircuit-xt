/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#include <sstream>

#include "sampler.h"
#include "sample.h"
#include "synthesis/filter.h"

struct indenter {
    int indent = 0;
    struct g {
        g(indenter *p_) : p(p_) {
            p->indent ++;
        }
        ~g() { p->indent --; }
        indenter *p;
    };
    g up() { return g(this); }
};

std::ostream& operator<<(std::ostream& os, const indenter& p) {
    for( int i=0; i<p.indent; ++i )
        os << "   ";
    return os;
}


#define SHOW( a, b ) oss << pfx << #a << ": " << b -> a << "\n"
#define SHOWT( a, b, t ) oss << pfx << #a << ": " << (t)( b -> a ) << "\n"

std::string sampler::generateInternalStateView() const {
    std::ostringstream oss;

    indenter pfx;

    oss << pfx << "sc3:\n";

    auto g1 = pfx.up();
    oss << pfx << "samples:\n";
    for(auto i=0; i<max_samples; ++i )
    {
        auto g2 = pfx.up();
        auto s = samples[i];
        if(s)
        {
            oss << pfx << "- sample:\n";
            auto g3 = pfx.up();
            oss << pfx << "id: " << i << "\n";
            SHOWT( channels, s, int );
            SHOW( sample_length, s );
            SHOW( sample_rate, s );
            SHOW( grains_initialized, s );
            if( s->grains_initialized )
                SHOW( num_grains, s );
            SHOW( name, s );

            fs::path p;
            s->get_filename(&p);
            oss << pfx << "filename: " << path_to_string( p ) << "\n";

            oss << pfx << "meta: {";
            std::string comma = "";
#define M(a) oss << comma << #a << ": " << s->meta. a; comma = ", ";
#define MC(a) oss << comma << #a << ": " << (int)s->meta. a; comma = ", ";
#define MB(a) oss << comma << #a << ": " << ( s->meta. a ? "true" : "false" ); comma = ", ";
            MC(key_low);
            MC(key_high);
            MC(key_root);
            MC(vel_low);
            MC(vel_high);
            MC(playmode);
            M(detune);
            M(loop_start);
            M(loop_end);
            MB(rootkey_present);
            MB(key_present);
            MB(vel_present);
            MB(loop_present);
            MB(playmode_present);
            M(n_slices);
            M(n_beats);
#undef M
#undef MB
#undef MC
            oss << "}\n";
        }
    }

    oss << pfx << "zones:\n";
    for(auto i=0; i<max_zones; ++i )
    {
        auto g2 = pfx.up();
        if (zone_exists[i])
        {
            auto z = &zones[i];
            oss << pfx << "- zone:\n";
            auto g3 = pfx.up();
            oss << pfx << "id: " << i << "\n";
            SHOW(name,z);
            SHOW(sample_id,z);
            SHOW(mute_group,z);
            SHOW(ignore_part_polymode,z);
            SHOW(mute,z);
            SHOW(reverse,z);
            oss << pfx << "lag_generator: [" << z->lag_generator[0] << ", " << z->lag_generator[1] << "]\n";
            SHOW(key_root,z);
            SHOW(key_low,z);
            SHOW(key_high,z);
            SHOW(velocity_high,z);
            SHOW(velocity_low,z);
            SHOW(key_low_fade,z);
            SHOW(key_high_fade,z);
            SHOW(velocity_low_fade,z);
            SHOW(velocity_high_fade,z);
            SHOW(part,z);
            SHOW(layer,z);
            SHOW(transpose,z);
            SHOW(finetune,z);
            SHOW(pitchcorrection,z);
            SHOW(playmode,z);
            SHOW(loop_start,z);
            SHOW(loop_end,z);
            SHOW(sample_start,z);
            SHOW(sample_stop,z);
            SHOW(loop_crossfade_length,z);
            SHOW(pre_filter_gain,z);
            SHOW(pitch_bend_depth,z);
            SHOW(velsense,z);
            SHOW(keytrack,z);

            oss << pfx << "filter:\n";
            {
                auto g3 = pfx.up();
                oss << pfx << "coming_soon: true\n";
            }

            oss << pfx << "aeg:\n";
            {
                auto g3 = pfx.up();
                oss << pfx << "coming_soon: true\n";
            }

            oss << pfx << "feg:\n";
            {
                auto g3 = pfx.up();
                oss << pfx << "coming_soon: true\n";
            }

            oss << pfx << "lfo:\n";
            {
                auto g3 = pfx.up();
                oss << pfx << "coming_soon: true\n";
            }

            oss << pfx << "mm_entry:\n";
            {
                auto g3 = pfx.up();
                oss << pfx << "coming_soon: true\n";
            }

            oss << pfx << "nc_entry:\n";
            {
                auto g3 = pfx.up();
                oss << pfx << "coming_soon: true\n";
            }


            oss << pfx << "hitpoint:\n";
            {
                auto g3 = pfx.up();
                oss << pfx << "coming_soon: true\n";
            }

            oss << pfx << "aux:\n";
            {
                auto g3 = pfx.up();
                oss << pfx << "coming_soon: true\n";
            }

            oss << pfx << "lag_generator:\n";
            {
                auto g3 = pfx.up();
                oss << pfx << "coming_soon: true\n";
            }

        }
    }
    oss << pfx << "parts:\n";
    for( int i=0; i<n_sampler_parts; ++i )
    {
        auto g2 = pfx.up();
        auto p = &parts[i];
        oss << pfx << "- part:\n";
        auto g3 = pfx.up();
        SHOW(name, p );
        SHOW(polylimit,p);
        SHOW(MIDIchannel,p);
    }

    oss << pfx << "effects:\n";
    bool anyfx = false;
    for( int i=0; i<n_sampler_effects; ++i ){
        auto f = multiv.pFilter[i];
        if( f )
        {
            anyfx = true;
            auto g2 = pfx.up();
            oss << pfx << "- effect:\n";
            auto g3 = pfx.up();
            oss <<pfx << "id: " << i << "\n";
            oss << pfx << "name: " << f->get_filtername() << "\n";
        }
    }
    if( ! anyfx )
    {
        auto g2 = pfx.up();
        oss << pfx << "disabled: true\n";
    }

    return oss.str();
}