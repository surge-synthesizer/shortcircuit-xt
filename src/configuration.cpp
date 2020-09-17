//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#include "configuration.h"
#include <stdio.h>
#include <string.h>
#include <tinyxml/tinyxml.h>
#include <assert.h>
#include "versionno.h"
#include <vt_util/vt_string.h>


bool global_use_alt_keyboardmethod = false;
bool global_skip_zone_noteon_redraws = false;
bool global_skip_slice_noteon_redraws = false;

bool stringreplace(char *string,char *find, char *replace)
{
	if (strlen(replace) <= 0 ) return false;
	char source[256];
	strcpy(source,string);
	char *begin,*end;
	begin = strstr(string,find);	
	if(find[0] && begin)
	{
		end = strstr(source,find) + strlen(find);
		strcpy(begin,replace);
		begin += strlen(replace);
		strcpy(begin,end);
		return true;
	}
	return false;
}

bool stringreplaceW(wchar_t *string, wchar_t *find, const wchar_t *replace)
{
	if (wcslen(replace) <= 0 ) return false;
	wchar_t source[256];
	wcscpy(source,string);
	wchar_t *begin,*end;
	begin = wcsstr(string,find);	
	if(find[0] && begin)
	{
		end = wcsstr(source,find) + wcslen(find);
		wcscpy(begin,replace);
		begin += wcslen(replace);
		wcscpy(begin,end);
		return true;
	}
	return false;
}

configuration::configuration()
{	
	relative = L"";
	conf_filename = L"";

	memset(MIDIcontrol,0,sizeof(midi_controller)*n_custom_controllers);
	
	headroom = 0;
	this->mono_outputs = 0;
	this->stereo_outputs = 8;
	keyboardmode = 0;
	mPreviewLevel = -12.f;
	mAutoPreview = true;
}

bool configuration::load(std::wstring filename)
{
	if(filename.empty()) 
	{
		filename = conf_filename;
	}
	else 
	{
		conf_filename = filename;		
	}

	wstringCharReadout(filename, filenameUTF8, 256);
	
	TiXmlDocument doc(filenameUTF8);

	doc.LoadFile();
	if (doc.Error()) return false;
	
	TiXmlElement *conf = (TiXmlElement*)doc.FirstChild("configuration");
			
	int ti;
	if(conf->Attribute("outputs_stereo",&ti))	this->stereo_outputs = ti;	
	if(conf->Attribute("store_in_projdir",&ti))	this->store_in_projdir = (ti != 0);	
	if(conf->Attribute("skin")) skindir = conf->Attribute("skin");
	if(conf->Attribute("keyboardmode",&ti))	keyboardmode = ti;	
	if(conf->Attribute("autopreview",&ti))	mAutoPreview = (ti != 0);	
	if(conf->Attribute("previewlevel",&ti))	mPreviewLevel = (float)ti;	
	if(conf->Attribute("DumpOnExceptions",&ti))	mUseMiniDumper = (ti != 0);		

	for(int i=0; i<4; i++)
	{
		char tag[64];
		sprintf(tag,"pathlist%i",i);
		const char *s = conf->Attribute(tag);
		if(s) pathlist[i] = s;
	}

	TiXmlElement *sub;
	int i,j;
	sub = conf->FirstChild("control")->ToElement();
	while (sub){
		sub->Attribute("i",&i);
		if(i<n_custom_controllers)
		{
			const char *tstr = sub->Attribute("type");
			if (tstr){
				if(!stricmp(tstr,("CC"))) MIDIcontrol[i].type = mct_cc;
				else if(!stricmp(tstr,("RPN"))) MIDIcontrol[i].type = mct_rpn;
				else if(!stricmp(tstr,("NRPN"))) MIDIcontrol[i].type = mct_nrpn;			
				sub->Attribute("number",&j);	MIDIcontrol[i].number = j;
				tstr = sub->Attribute("name");	
				if (tstr) vtCopyString(MIDIcontrol[i].name,tstr,16);
			}
		}
		sub = sub->NextSibling("control")->ToElement();
	}
	
	return true;
}

bool configuration::save(std::wstring filename)
{
	if(filename.empty()) 
	{
		filename = conf_filename;
	}
	else 
	{
		conf_filename = filename;		
	}	
	TiXmlDeclaration decl("1.0","UTF-8","yes");
	
	wstringCharReadout(filename,filenameUTF8,256)
	TiXmlDocument doc(filenameUTF8);

	TiXmlElement conf("configuration");
	conf.SetAttribute("version",STRSCVERNICE);			
	conf.SetAttribute("store_in_projdir",store_in_projdir?1:0);	
	conf.SetAttribute("outputs_stereo",this->stereo_outputs);				
	conf.SetAttribute("skin",skindir.c_str());
	conf.SetAttribute("keyboardmode",keyboardmode);	
	conf.SetAttribute("previewlevel",(int)mPreviewLevel);	
	conf.SetAttribute("autopreview",mAutoPreview ? 1 : 0);
	conf.SetAttribute("DumpOnExceptions",mUseMiniDumper ? 1 : 0);	

	for(int i=0; i<4; i++)
	{
		char tag[64];
		sprintf(tag,"pathlist%i",i);
		conf.SetAttribute(tag,pathlist[i].c_str());
	}

	doc.InsertEndChild(decl);
	
	for(int i=0; i<n_custom_controllers; i++)
	{
		TiXmlElement ctrl("control");
		ctrl.SetAttribute("i",i);
		switch(this->MIDIcontrol[i].type)
		{	
		case mct_cc:
			ctrl.SetAttribute("type","CC");
			break;
		case mct_rpn:
			ctrl.SetAttribute("type","RPN");
			break;
		case mct_nrpn:
			ctrl.SetAttribute("type","NRPN");
			break;
		default:
			ctrl.SetAttribute("type","NONE");
			break;		
		};
		ctrl.SetAttribute("number",MIDIcontrol[i].number);
		ctrl.SetAttribute("name",MIDIcontrol[i].name);
		conf.InsertEndChild(ctrl);
	}

	doc.InsertEndChild(conf);
	doc.SaveFile();	
	return true;
}

void configuration::decode_pathW(std::wstring in, wchar_t *out, wchar_t *extension, int *program_id, int *sample_id)
{	
	assert(out);
	assert(extension);
	wcsncpy(out,in.c_str(),pathlength);

	stringreplaceW(out, L"<relative>", relative.c_str());
	
	// get program/sample id if available
	if (sample_id) *sample_id = -1;
	if (program_id) *program_id = -1;
	wchar_t *separator = wcsrchr(out,L'|');
	if(separator)
	{
		wchar_t sidstr[64];
		wcscpy(sidstr,separator+1);		
		if (sample_id) *sample_id = wcstol(sidstr, NULL, 10);
		*separator = 0;
	}
	separator = wcsrchr(out,L'>');
	if(separator)
	{
		wchar_t pidstr[64];
		wcscpy(pidstr,separator+1);		
		if (program_id) *program_id = wcstol(pidstr, NULL, 10);
		*separator = 0;
	}

	separator = wcsrchr(out,L'.');
	if(separator)
	{		
		wcsncpy(extension,separator+1,64);
	}
}
