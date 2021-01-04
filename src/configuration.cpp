//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#include "configuration.h"
#include <regex>
#include <cstring>
#include <cstdlib>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <tinyxml/tinyxml.h>
#include <vt_util/vt_string.h>

#include "version.h"

bool global_use_alt_keyboardmethod = false;
bool global_skip_zone_noteon_redraws = false;
bool global_skip_slice_noteon_redraws = false;

configuration::configuration()
{
    memset(MIDIcontrol, 0, sizeof(midi_controller) * n_custom_controllers);

    headroom = 0;
    this->mono_outputs = 0;
    this->stereo_outputs = 8;
    keyboardmode = 0;
    mPreviewLevel = -12.f;
    mAutoPreview = true;
}

bool configuration::load(const fs::path &filename)
{
    auto fn = mConfFilename;
    if (!filename.empty())
    {   
        fn = filename;
        mConfFilename = filename;
    }

    TiXmlDocument doc(path_to_string(fn)); // TODO propagate fs::path down?

    doc.LoadFile();
    if (doc.Error())
        return false;

    TiXmlElement *conf = (TiXmlElement *)doc.FirstChild("configuration");

    int ti;
    if (conf->Attribute("outputs_stereo", &ti))
        this->stereo_outputs = ti;
    if (conf->Attribute("store_in_projdir", &ti))
        this->store_in_projdir = (ti != 0);
    if (conf->Attribute("skin"))
        skindir = conf->Attribute("skin");
    if (conf->Attribute("keyboardmode", &ti))
        keyboardmode = ti;
    if (conf->Attribute("autopreview", &ti))
        mAutoPreview = (ti != 0);
    if (conf->Attribute("previewlevel", &ti))
        mPreviewLevel = (float)ti;
    if (conf->Attribute("DumpOnExceptions", &ti))
        mUseMiniDumper = (ti != 0);

    for (int i = 0; i < 4; i++)
    {
        char tag[64];
        sprintf(tag, "pathlist%i", i);
        const char *s = conf->Attribute(tag);
        if (s)
            pathlist[i] = s;
    }

    TiXmlElement *sub;
    int i, j;
    sub = conf->FirstChild("control")->ToElement();
    while (sub)
    {
        sub->Attribute("i", &i);
        if (i < n_custom_controllers)
        {
            const char *tstr = sub->Attribute("type");
            if (tstr)
            {
                if (!stricmp(tstr, ("CC")))
                    MIDIcontrol[i].type = mct_cc;
                else if (!stricmp(tstr, ("RPN")))
                    MIDIcontrol[i].type = mct_rpn;
                else if (!stricmp(tstr, ("NRPN")))
                    MIDIcontrol[i].type = mct_nrpn;
                sub->Attribute("number", &j);
                MIDIcontrol[i].number = j;
                tstr = sub->Attribute("name");
                if (tstr)
                    vtCopyString(MIDIcontrol[i].name, tstr, 16);
            }
        }
        sub = sub->NextSibling("control")->ToElement();
    }

    return true;
}

bool configuration::save(const fs::path &filename)
{
    auto fn = mConfFilename;
    if (!filename.empty())
    {
        fn = filename;
        mConfFilename = filename;
    }
        
    TiXmlDeclaration decl("1.0", "UTF-8", "yes");

    TiXmlDocument doc(path_to_string(fn)); // TODO propagate fs::path down?

    TiXmlElement conf("configuration");
    conf.SetAttribute("version", ShortCircuit::Build::FullVersionStr);
    conf.SetAttribute("store_in_projdir", store_in_projdir ? 1 : 0);
    conf.SetAttribute("outputs_stereo", this->stereo_outputs);
    conf.SetAttribute("skin", skindir.c_str());
    conf.SetAttribute("keyboardmode", keyboardmode);
    conf.SetAttribute("previewlevel", (int)mPreviewLevel);
    conf.SetAttribute("autopreview", mAutoPreview ? 1 : 0);
    conf.SetAttribute("DumpOnExceptions", mUseMiniDumper ? 1 : 0);

    for (int i = 0; i < 4; i++)
    {
        char tag[64];
        sprintf(tag, "pathlist%i", i);
        conf.SetAttribute(tag, pathlist[i].c_str());
    }

    doc.InsertEndChild(decl);

    for (int i = 0; i < n_custom_controllers; i++)
    {
        TiXmlElement ctrl("control");
        ctrl.SetAttribute("i", i);
        switch (this->MIDIcontrol[i].type)
        {
        case mct_cc:
            ctrl.SetAttribute("type", "CC");
            break;
        case mct_rpn:
            ctrl.SetAttribute("type", "RPN");
            break;
        case mct_nrpn:
            ctrl.SetAttribute("type", "NRPN");
            break;
        default:
            ctrl.SetAttribute("type", "NONE");
            break;
        };
        ctrl.SetAttribute("number", MIDIcontrol[i].number);
        ctrl.SetAttribute("name", MIDIcontrol[i].name);
        conf.InsertEndChild(ctrl);
    }

    doc.InsertEndChild(conf);
    doc.SaveFile();
    return true;
}

fs::path configuration::resolve_path(const fs::path &in)
{
    auto s = path_to_string(in);
    auto r = std::regex_replace(s, std::regex("<relative>"), path_to_string(mRelative));
    return string_to_path(r);
}

void configuration::set_relative_path(const fs::path &in) 
{ 
    mRelative = in; 
}

void decode_path(const fs::path &in, fs::path *out, std::string *extension,
    std::string *name_only, fs::path *path_only, int *program_id, int *sample_id)
{
    if (path_only)
    {

        *path_only = in;       
        if (path_only->has_filename())
        {
            path_only->remove_filename();
        }
        // rem trailing separator if there
        auto s = path_to_string(*path_only);
        if (s.back() == static_cast<char>(fs::path::preferred_separator))
        {
            s.pop_back();
            *path_only = string_to_path(s);
        }
    }
    if (out)
        out->clear();
    if (extension)
        extension->clear();
    if (name_only)
        name_only->clear();
    if (sample_id)
        *sample_id = -1;
    if (program_id)
        *program_id = -1;

    auto tmp = path_to_string(in);
    
    // get and strip off program/sample id if available
    const char *separator = strrchr(tmp.c_str(), '|');
    if (separator)
    {
        std::string sidstr = separator + 1;

        if (sample_id)
            *sample_id = std::strtol(sidstr.c_str(), NULL, 10);

        tmp = tmp.substr(0, separator - tmp.c_str());
    }

    separator = strrchr(tmp.c_str(), '>');
    if (separator)
    {
        std::string pidstr = separator + 1;
        if (program_id)
            *program_id = strtol(pidstr.c_str(), NULL, 10);
        tmp = tmp.substr(0, separator - tmp.c_str());
    }
    // extract filename portion only
    auto p = string_to_path(tmp);
    auto no = path_to_string(p.filename());

    // extract extension and cvt to lower, also get name only without ext
    separator = strrchr(no.c_str(), '.');
    if (separator)
    {
        if (extension)
        {
            *extension = separator + 1;
            std::transform((*extension).begin(), (*extension).end(), (*extension).begin(),
                           ::tolower);
        }
        // strip it off
        no = no.substr(0, separator - no.c_str());    
    }

    if (name_only)
    {
        *name_only = no;
    }
    

    if (out)
        *out = string_to_path(tmp);

}
