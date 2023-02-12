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

#pragma once

const int max_meq_entries = 512;

struct meq_band
{
    float freq, gain, BW;
    bool active;
};

struct meq_snapshot
{
    meq_band bands[8];
    float gain;
    char name[32];
    int bank;
    int patch;
};

class morphEQ_loader
{
  public:
    morphEQ_loader();
    ~morphEQ_loader();
    void load(int bank, char *filename);
    meq_snapshot snapshot[max_meq_entries];
    float get_id(int bank, int patch);
    void set_id(int *bank, int *patch, float val);
    int n_loaded;
};
