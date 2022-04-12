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

#include "shortcircuit_vsti_edit.h"

#include "shortcircuit_editor2.h"

#include "sampler.h"

// extern bool oome;

//-------------------------------------------------------------------------------------------------------

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
    return new shortcircuit_vsti_edit(audioMaster);
}

//-----------------------------------------------------------------------------

shortcircuit_vsti_edit::shortcircuit_vsti_edit(audioMasterCallback audioMaster)
    : shortcircuit_vsti(audioMaster)
{
    setUniqueID('SCv2');
#ifndef _DEMO_
    programsAreChunks(true);
#endif
    setEditor(new sc_editor2(this));

    /*if (!editor)
            oome = true;*/
}

//-----------------------------------------------------------------------------

shortcircuit_vsti_edit::~shortcircuit_vsti_edit()
{
    // the editor gets deleted by the
    // AudioEffect base class
}

//-----------------------------------------------------------------------------

void shortcircuit_vsti_edit::setParameter(long index, float value)
{
    shortcircuit_vsti::setParameter(index, value);

    /*if (editor)
            ((AEffEditor*)editor)->setParameter (index, value);*/
}

//-----------------------------------------------------------------------------------------

#ifndef _DEMO_
VstInt32 shortcircuit_vsti_edit::getChunk(void **data, bool isPreset)
{
    if (!initialized)
        init();

    // return sobj->save_all_as_xml(data);
    return (VstInt32)sobj->SaveAllAsRIFF(data);
}

//-----------------------------------------------------------------------------------------

VstInt32 shortcircuit_vsti_edit::setChunk(void *data, VstInt32 byteSize, bool isPreset)
{
    if (!initialized)
        init();
    // sobj->load_all_from_xml(data,byteSize);
    if (sobj->LoadAllFromRIFF(data, byteSize))
        return 1;
    return 0;
}
#endif

//-----------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------