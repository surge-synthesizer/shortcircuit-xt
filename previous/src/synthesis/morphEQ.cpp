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

#include "morphEQ.h"

#include "tinyxml/tinyxml.h"
#include "util/scxtstring.h"

morphEQ_loader::morphEQ_loader() { n_loaded = 0; }

morphEQ_loader::~morphEQ_loader() {}

float morphEQ_loader::get_id(int bank, int patch)
{
    for (int i = 0; i < n_loaded; i++)
    {
        if ((snapshot[i].bank == bank) && (snapshot[i].patch == patch))
        {
            return (float)i;
        }
    }
    return 0;
}

void morphEQ_loader::set_id(int *bank, int *patch, float val)
{
    assert(bank);
    assert(patch);
    int iv = (int)val;
    if ((iv < 0) || (iv > n_loaded))
    {
        *bank = 0;
        *patch = 0;
        return;
    }

    *bank = snapshot[iv].bank;
    *patch = snapshot[iv].patch;
}
void morphEQ_loader::load(int bank, char *filename)
{
    assert(bank < max_meq_entries);
    int patch = 0;

    TiXmlDocument doc(filename);
    doc.LoadFile();

    TiXmlElement *meq = doc.FirstChildElement("morphEQ");
    if (!meq)
        return;

    TiXmlElement *sh = meq->FirstChildElement("snapshot");
    if (!sh)
        return;

    while (sh && (n_loaded < max_meq_entries))
    {
        meq_snapshot *s = &snapshot[n_loaded];
        int ival = 0;
        double dval = 0;
        const char *shname = sh->Attribute("name");
        strncpy_0term(s->name, shname, sizeof(s->name));

        sh->Attribute("gain", &dval);
        s->gain = (float)dval;
        s->bank = bank;
        s->patch = patch++;

        TiXmlElement *band = sh->FirstChildElement("band");

        for (int a = 0; a < 8; a++)
        {
            // set default values
            s->bands[a].active = false;
            s->bands[a].freq = 0;
            s->bands[a].gain = 0;
            s->bands[a].BW = 1;
        }

        s->bands[0].freq = -8;
        s->bands[1].freq = -2.5;
        s->bands[2].freq = -1;
        s->bands[3].freq = 0;
        s->bands[4].freq = 1;
        s->bands[5].freq = 2.5;
        s->bands[6].freq = 4.0;
        s->bands[7].freq = 8;

        while (band)
        {
            int b_id;
            band->Attribute("i", &b_id);
            if ((b_id >= 0) && (b_id < 8))
            {
                band->Attribute("freq", &dval);
                s->bands[b_id].freq = (float)dval;
                band->Attribute("gain", &dval);
                s->bands[b_id].gain = (float)dval;
                band->Attribute("BW", &dval);
                s->bands[b_id].BW = (float)dval;
                s->bands[b_id].active = true;
            }
            band = band->NextSibling("band")->ToElement();
        }

        sh = sh->NextSibling("snapshot")->ToElement();
        n_loaded++;
    }

    // would be ok sort here
}