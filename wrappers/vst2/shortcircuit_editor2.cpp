//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#include "shortcircuit_editor2.h"
#include "shortcircuit_vsti.h"
#include "interaction_parameters.h"
#include "sampler.h"
#include "configuration.h"

#include <string>
using std::string;
#include <vector>
using std::vector;

static int sc_instances = 0;
extern bool global_skip_zone_noteon_redraws;
extern bool global_skip_slice_noteon_redraws;

//-----------------------------------------------------------------------------

sc_editor2::sc_editor2 (sampler *sobj)
: AEffEditor (0) 
{
	this->sobj = sobj;	
}


sc_editor2::sc_editor2(AudioEffect *effect)
 : AEffEditor (effect) 
{	
	sobj = 0;	
}

//const int window_size_x = 1120;
//const int window_size_y = 704;
//static ERect editorrect = {0, 0, window_size_y, window_size_x};

bool sc_editor2::getRect (ERect **erect)
{		
	shortcircuit_vsti* sc_vsti = (shortcircuit_vsti*)(effect);
	if(!sc_vsti->initialized) sc_vsti->init();	// check if sampler object exists, otherwise init
	sobj = sc_vsti->sobj;
	assert(sobj);	
	
	string path = get_dir(1);

	TiXmlDocument doc;
	if (!doc.LoadFile(path + "layout.xml"))
	{
		path = get_dir(0);
		if(!doc.LoadFile(path + "layout.xml")) return false;
	}

	TiXmlElement *layout = doc.FirstChild("layout")->ToElement();
	if(!layout) return false;		
	
	editorrect.top = 0;
	editorrect.left = 0;
	editorrect.right = 200;
	editorrect.bottom = 32;	

	if(sobj->conf->keyboardmode != kbm_seperate_window)
	{
		int j;
		if(layout->QueryIntAttribute("sizeX",&j)==TIXML_SUCCESS)  editorrect.right = j;	
		if(layout->QueryIntAttribute("sizeY",&j)==TIXML_SUCCESS) editorrect.bottom = j;			
	}

	*erect = &editorrect;	
	return true;	
}

//-----------------------------------------------------------------------------

sc_editor2::~sc_editor2()
{
	sc_instances--;
}

//-----------------------------------------------------------------------------

string sc_editor2::get_dir(int id)
{
	CHAR path[1024];
	extern void* hInstance;
	GetModuleFileName((HMODULE)hInstance,path,1024);
	CHAR *end = strrchr(path,'\\');
	if(end) *end = 0;
	string scdata = path;

	switch(id)
	{
	case 0:	// default skin (fallback)
		return scdata + "\\gui\\default\\";
		break;
	case 1:	// user skin (make dynamic from config)
		return scdata + "\\gui\\" + sobj->conf->skindir + "\\";
		break;
	case 2:	
		return scdata + "\\database\\";
		break;
	};
	return path;
}

DWORD WINAPI global_refresh_db_workthread( LPVOID lpParam ) 
{	
	void *sy = lpParam;

	sc_editor2 *e = (sc_editor2*)sy;
	
	HRESULT hr = CoInitialize(NULL);	// init COM for this thread
	if(SUCCEEDED(hr))
	{
		// do refresh
		e->browserd.refresh();
		CoUninitialize();
	}
	// restore browser
	actiondata ad2;
	ad2.actiontype = vga_browser_is_refreshing;
	ad2.id = ip_browser;		
	ad2.subid = -1;	
	ad2.data.i[0] = 0;
	
	e->post_action_from_program(ad2);
	return 0;
}

bool sc_editor2::refresh_db()
{
	// tell the browser to disable itself
	actiondata ad2;
	ad2.actiontype = vga_browser_is_refreshing;
	ad2.id = ip_browser;		
	ad2.subid = -1;	
	ad2.data.i[0] = 1;
	post_action_from_program(ad2);
		
	DWORD dwThreadId;
	HANDLE hThread = CreateThread(
		NULL,                        // default security attributes 
		0,                           // use default stack size  
		global_refresh_db_workthread,                  // thread function 
		this,                // argument to thread function 
		0,                           // use default creation flags 
		&dwThreadId);                // returns the thread identifier 
	SetThreadPriority(hThread,THREAD_PRIORITY_BELOW_NORMAL);
		
	return true;
}

bool sc_editor2::open(void *ptr)
{						
	shortcircuit_vsti* sc_vsti = (shortcircuit_vsti*)(effect);
	if(!sc_vsti->initialized) sc_vsti->init();	// check if sampler object exists, otherwise init
	sobj = sc_vsti->sobj;
	assert(sobj);	

	keyboardmode = sobj->conf->keyboardmode;
	if(keyboardmode == kbm_autodetect)	// autodetect
	{
		keyboardmode = kbm_os;
	}

	string scdata = get_dir(2);;	
	browserd.set_paths(0,scdata + "samples\\");
	browserd.set_paths(1,scdata + "patches\\");	
	if(!browserd.init(scdata + "dircache.sctemp")) refresh_db();
	
	// TODO	 
	//browserd.build_samplelist(2,);	

	rootdir_default = get_dir(0);	
	rootdir_skin = get_dir(1);	
	create("layout.xml",false,0,ptr);	

	// tell the sampler to send init-data to the editor
	actiondata ad;
	ad.actiontype = vga_openeditor;
	post_action(ad,0);

	ad.actiontype = vga_browser_listptr;
	ad.id = ip_browser;
	ad.subid = -1;
	ad.data.ptr[0] = &browserd.categorylist[0];
	ad.data.ptr[1] = &browserd.patchlist[0];
	ad.data.i[4] = 0;
	post_action_from_program(ad);

	return true;
}

bool sc_editor2::onKeyDown (VstKeyCode &keyCode)
{	
	if(keyboardmode == kbm_os)
	{	
		take_focus();
	}
	else if(keyboardmode == kbm_vst)
	{
		bool ischar = true;
		vg_controlevent e;			
		e.eventdata[0] = keyCode.character;
		switch(keyCode.virt)
		{
		case VKEY_BACK:
			ischar = true;
			e.eventdata[0] = VK_BACK;
			break;
		case VKEY_LEFT:
		case VKEY_RIGHT:
		case VKEY_UP:
		case VKEY_DOWN:
		case VKEY_RETURN:
		case VKEY_HOME:
		case VKEY_END:
		case 0X14:	// caps lock
		case VKEY_PAGEUP:
		case VKEY_PAGEDOWN:			
		case VKEY_NUMLOCK:
		case VKEY_SCROLL:
		case VKEY_SHIFT:
		case VKEY_CONTROL:
		case VKEY_ALT:
		case VKEY_INSERT:
		case VKEY_DELETE:
			ischar = false;
			break;
		}

		if (GetKeyState(VK_SHIFT)) e.eventdata[0] = toupper(e.eventdata[0]);

		e.eventtype = vget_keypress;
		processevent(e);		
		if(ischar)
		{
			e.eventtype = vget_character;
			processevent(e);
		}
	}
	return 0;	
}

bool sc_editor2::onKeyUp (VstKeyCode &keyCode)
{
	if(keyboardmode == kbm_os)
	{	
		take_focus();
	}
	else if(keyboardmode == kbm_vst)
	{
		vg_controlevent e;	
		e.eventtype = vget_keyrelease;
		e.eventdata[0] = keyCode.character;
		processevent(e);
	}
	return 0;
	//return frame ? frame->onKeyUp (keyCode) : -1;
}

void sc_editor2::close()
{
	actiondata ad;
	ad.actiontype = vga_closeeditor;
	post_action(ad,0);

	destroy();
}
void sc_editor2::setParameter (long index, float value)
{

}

void sc_editor2::process_action_to_editor(actiondata ad)
{
	assert(sobj);
	if(ad.actiontype == vga_inject_database)
	{
		browserd.inject_newfile(ad.data.i[0],inject_file);

		// refresh browser
		actiondata ad2;
		ad2.actiontype = vga_browser_is_refreshing;
		ad2.id = ip_browser;		
		ad2.subid = -1;	
		ad2.data.i[0] = 0;
		post_action_from_program(ad2);
	}
	else if(ad.actiontype == vga_database_samplelist)
	{
		int slid = 2;
		browserd.build_samplelist(slid,(database_samplelist*)ad.data.ptr[0], ad.data.i[2]);
		if(browserd.currentmode == slid)
		{
			actiondata ad2;
			ad2.actiontype = vga_browser_listptr;
			ad2.id = ip_browser;		
			ad2.subid = -1;
			ad2.data.ptr[0] = &browserd.categorylist[slid];
			ad2.data.ptr[1] = &browserd.patchlist[slid];
			ad2.data.i[4] = slid;
			post_action_from_program(ad2);
		}
	}
}

void sc_editor2::post_action_to_program(actiondata ad)
{
	assert(sobj);
	if((ad.id == ip_browser_mode)&&(ad.actiontype == vga_intval))
	{
		actiondata ad2;
		ad2.actiontype = vga_browser_listptr;
		ad2.id = ip_browser;		
		ad2.subid = -1;
		ad2.data.ptr[0] = &browserd.categorylist[ad.data.i[0]&3];
		ad2.data.ptr[1] = &browserd.patchlist[ad.data.i[0]&3];
		ad2.data.i[4] = ad.data.i[0]&3;
		browserd.currentmode = ad.data.i[0]&3;
		post_action_from_program(ad2);
	}
	else if(ad.id == ip_config_refresh_db)
	{
		refresh_db();
	}
	else if(ad.actiontype == vga_exec_external)
	{		
		char tpath[256];
		extern void* hInstance;
		GetModuleFileName((HMODULE)hInstance,tpath,256);
		char *end = strrchr(tpath,'\\');			
		if(end)
		{
			end++;
			sprintf(end,(char*)ad.data.str);			
			//HWND help_hwnd = HtmlHelp(owner,helppath,HH_DISPLAY_TOC,NULL);			
			ShellExecute(hWnd,"open",tpath,0,0,SW_SHOWNORMAL);			
		}
	}
	else if(ad.actiontype == vga_url_external)
	{		
		ShellExecute(hWnd,"open",(char*)ad.data.str,0,0,SW_SHOWNORMAL);			
	}
	else if(ad.actiontype == vga_doc_external)
	{		
		char tmp[512];
		sprintf(tmp,"http://wiki.vemberaudio.se/shortcircuit:%s",(char*)ad.data.str);
		ShellExecute(hWnd,"open",tmp,0,0,SW_SHOWNORMAL);
	}	
	else
	{
		sobj->post_events_from_editor(ad);
	}
}

bool sc_editor2::take_focus()
{
	SetFocus(hWnd);
	return true;
}

// store as text-labels in xml
// functions:
// get id by label
// get label by id

int sc_editor2::param_get_n()
{
	return n_ip_entries;	
}
string sc_editor2::param_get_label_from_id(int id)
{	
	if((id>=0)&&(id<n_ip_entries)) return ip_data[id].label;
	return "-";
}
int sc_editor2::param_get_id_from_label(string s)
{
	for(int i=0; i<n_ip_entries; i++)
	{
		if(s.compare(ip_data[i].label) == 0) return i;
	}
	return 0;
}
int sc_editor2::param_get_n_subparams(int id)
{
	if((id>=0)&&(id<n_ip_entries)) return ip_data[id].n_subid;
	return 0;
}

// processes events for the end-user interface (intended to be small for easy overriding)
bool sc_editor2::processevent_user(vg_controlevent &e)
{	
	if(e.eventtype == vget_process_actionbuffer)
	{
		process_actions_from_program();
		return true;
	}

	bool bypassfocus=(e.eventtype == vget_dragfiles);
	
	int x = (int)e.x;
	int y = (int)e.y;
	int n = (int)children.size();

	if(!bypassfocus && (focus_id>0)&&(focus_id<n)/*&&(e.eventtype != vget_dragfiles)*/)
	{
		vg_rect r = children[focus_id]->get_rect();

		vg_controlevent e2 = e;
		e2.x -= (float) r.x;
		e2.y -= (float) r.y;								
		children[focus_id]->processevent(e2);				
	} else if((focus_id == layout_menu)&& !bypassfocus)
	{
		menu->processevent(e);
	}
	else
	{
		// check hover status
		if((hover_id >= 0) && (hover_id<n)) //  && !bypassfocus)
		{
			if(children[hover_id]->visible())
			{
				vg_rect r = children[hover_id]->get_rect();
				if(children[hover_id]->hidden || !((x >= r.x) && (x < r.x2) && (y >= r.y) && (y < r.y2)))
				{
					vg_controlevent e2 = e;
					e2.x -= (float) r.x;
					e2.y -= (float) r.y;
					e2.eventtype = vget_mouseleave;
					children[hover_id]->processevent(e2);
					hover_id = -1;				
				}	
			}
		}

		for(int i=(n-1); i>=0; i--)	// go backwards
		{			
			if(children[i]->visible())
			{
				vg_rect r = children[i]->get_rect();
				if(!children[i]->hidden && (x >= r.x) && (x < r.x2) && (y >= r.y) && (y < r.y2))
				{
					vg_controlevent e2 = e;
					e2.x -= (float) r.x;
					e2.y -= (float) r.y;				
					if(((e.eventtype == vget_mousemove)||(e.eventtype == vget_dragfiles)) && (i != hover_id))
					{						
						if((hover_id >= 0)&&(hover_id<n))
						{	
							vg_controlevent e4 = e;						
							e4.eventtype = vget_mouseleave;
							children[hover_id]->processevent(e4);
						}
						if((e.eventtype != vget_dragfiles) || children[i]->is_dragdrop_destination())	// only let dragfiles change focus to proper destinations
						{
							vg_controlevent e3 = e2;
							e3.eventtype = vget_mouseenter;
							children[i]->processevent(e3);
							hover_id = i;
						}
					}
					children[i]->processevent(e2);
					break;
				}
			}
		}
		if (!bypassfocus && (hover_id<0)) set_cursor(cursor_arrow);		
	}
	
	// if it is a text-edit control, don't let any kbdevents at all thru
	if((focus_id>=0) && (focus_id<children.size()) && children[focus_id]->steal_kbd_on_focus()) return true;
	
	// global keyboard commands
	actiondata ad;
	ad.id = 0;
	ad.subid = 0;					
	if(e.eventtype == vget_keypress)
	{			
		switch(e.eventdata[0])		
		{
		case VK_DOWN:
			{
				ad.actiontype = vga_select_zone_previous;
				post_action(ad,0);
				return true;
			}
		case VK_UP:
			{
				ad.actiontype = vga_select_zone_next;
				post_action(ad,0);
				return true;
			}
		case 'a':
		case 'A':
			{
				ad.actiontype = vga_audition_zone;
				ad.data.i[0] = true;
				post_action(ad,0);
				return true;
			}						
		case 'z':
		case 'Z':
			{
				ad.actiontype = vga_toggle_zoom;
				ad.id = ip_kgv_or_list;
				ad.subid = 0;
				post_action_from_program(ad);
				return true;	
			}
		}
	}
	else if(e.eventtype == vget_keyrelease)
	{
		switch(e.eventdata[0])
		{
		case 'a':
		case 'A':
			{
				ad.actiontype = vga_audition_zone;
				ad.data.i[0] = false;
				post_action(ad,0);
				return true;
			}
		}
	}	

	// global event when nonfocused
	if((focus_id < 0) && (e.eventtype == vget_keypress))
	{		
		ad.id = ip_browser;
		ad.subid = -1;
		switch(e.eventdata[0])		
		{
		case '8':
			{
				ad.actiontype = vga_browser_entry_prev;
				post_action_from_program(ad);
				return true;
			}
		case '2':
			{
				ad.actiontype = vga_browser_entry_next;
				post_action_from_program(ad);
				return true;
			}
		case '4':
			{
				ad.actiontype = vga_browser_category_prev;
				post_action_from_program(ad);
				return true;
			}
		case '6':
			{
				ad.actiontype = vga_browser_category_next;
				post_action_from_program(ad);
				return true;
			}
		case '1':
			{
				ad.actiontype = vga_browser_category_child;
				post_action_from_program(ad);
				return true;
			}
		case '7':
			{
				ad.actiontype = vga_browser_category_parent;
				post_action_from_program(ad);
				return true;
			}
		case '5':
			{
				ad.actiontype = vga_browser_entry_load;
				post_action_from_program(ad);
				return true;
			}					
		}
	}
	return false;
}

bool sort_dropfile_by_filename(dropfile c1, dropfile c2){
	/* Returns true if first string is less than second one. */
	return c1.path.compare(c2.path)<0;		
};

bool sort_dropfile_by_rootkey(dropfile c1, dropfile c2){	
	return c1.key_root < c2.key_root;
};

void vg_window::dropfiles_process(int mode, int key)
{		
	assert(key < 128);

	if (dropfiles.empty()) 
	{
		return;
	}
	
	vector <dropfile>::iterator i;

	if(mode == 2)
	{
		int vel = 0, n = dropfiles.size();
		int id=1;
		for(i =	dropfiles.begin(); i != dropfiles.end(); i++)
		{
			// prepare the data to a known state
			i->has_key = false;
			i->has_startend = false;
			i->key_root = key;
			i->key_hi = key;
			i->key_lo = key;		
			i->vel_lo = vel;
			vel = min(127,(id<<7)/n);
			if(id == n) vel = 128;
			i->vel_hi = min(127,vel-1);
			id++;
		}
		return;
	}

	for(i =	dropfiles.begin(); i != dropfiles.end(); i++)
	{
		// prepare the data to a known state
		i->has_key = false;
		i->has_startend = false;
		key = min(key, 127);
		i->key_root = key;
		i->key_hi = key;
		i->key_lo = key;		
		i->vel_lo = 0;
		i->vel_hi = 127;
		key++;
	}
		
	if(mode == 0)
	{
		bool has_kf=false;
		unsigned int identical_chars=256;
		unsigned int c;
		// get number of identical chars
		for(i =dropfiles.begin(); i != (dropfiles.end()-1); i++)
		{			
			c=0;
			while(1)
			{
				c++;
				if(c > i->path.size()) break;
				if(c > (i+1)->path.size()) break;				

				if(i->path[c] != (i+1)->path[c]) break;				
			}
			identical_chars = min(identical_chars, c);
		}		

		// see if the "key field" is found
		int kf = identical_chars;
		bool kf_found = true;
		bool kf_found_slot[3];	// the key field might be in the last two identical characters if the same key character is used for all samples
		kf_found_slot[0] = true;	
		kf_found_slot[1] = true;
		kf_found_slot[2] = true;		
		for(i =	dropfiles.begin(); i != dropfiles.end(); i++)
		{					
			char searchstr[2];
			if((kf < 2) || ((kf-1) >= i->path.size()))
			{
				kf_found = false;
				break;
			}		

			for(int j=0; j<3; j++)
			{
				searchstr[0] = i->path[kf-j];
				searchstr[1] = 0;	

				if(!searchstr[0])
				{					
					kf_found_slot[j] = false;
				}
				if(strcspn(searchstr,"abcdefghABCDEFGH")) kf_found_slot[j] = false;			
				
				switch(i->path[kf-j-1])	// keyfield must be preceded by space, underscore or similar
				{
				case ' ':
				case '_':
				case '-':
				case ',':
				case '.':
				case ':':
				case '|':
				case '[':
				case ']':
				case '(':
				case ')':
					break;
				default:
					kf_found_slot[j] = false;
					break;
				};				
			}						
		}

		if(kf_found)
		{
			if(kf_found_slot[0])
			{
				has_kf = true;
			}
			else if(kf_found_slot[1])
			{
				has_kf = true;
				kf = kf - 1;
			}
			else if(kf_found_slot[2])
			{
				has_kf = true;
				kf = kf - 2;
			}
		}

		/*while(!has_kf && (kf<256))
		{
			bool kf_found = true;
			for(i =	dropfiles.begin(); i != dropfiles.end(); i++)
			{	
				char oi[2];
				if(kf >= i->path.size())
				{
					kf_found = false;
					break;
				}
				oi[0] = i->path[kf];
				oi[1] = 0;

				if(!oi[0])
				{
					kf = 256;	// bail out
					kf_found = false;
				}
				if(strcspn(oi,"0123456789- ")) 
				else if(strcspn(oi,"abcdefghABCDEFGH")) kf_found = false;
				if(!kf_found) break;
			}
			if(kf_found) has_kf = true;
			else kf++;
		}*/

		// if keyfield is found, add it to the datastructure
		if(has_kf)
		{
			for(i =	dropfiles.begin(); i != dropfiles.end(); i++)
			{				
				int key=0;				
				switch(i->path[kf])
				{
				case 'c':
				case 'C':
					key = 0;
					break;
				case 'd':
				case 'D':
					key = 2;
					break;
				case 'e':
				case 'E':
					key = 4;
					break;
				case 'f':
				case 'F':
					key = 5;
					break;
				case 'g':
				case 'G':
					key = 7;
					break;
				case 'a':
				case 'A':
					key = 9;
					break;
				case 'b':
				case 'B':
				case 'h':
				case 'H':
					key = 11;
					break;
				};

				int of = 2;
				switch(i->path[kf+1])
				{
				case 'B':
				case 'b':					
					key--;
					break;
				case '#':					
					key++;
					break;																	
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					of = 1;
					break;
				};
				int octave = i->path[kf+of] - '0';				
				i->has_key = (octave>=0)&&(octave<=9);
				if(i->has_key) i->key_root = 24 + 12*octave + key;				
			}
			// stretch zones to fill void between them			
			std::sort(dropfiles.begin(),dropfiles.end(),sort_dropfile_by_filename);
			std::sort(dropfiles.begin(),dropfiles.end(),sort_dropfile_by_rootkey);
			
			for(unsigned int j=0; j<dropfiles.size(); j++)
			{
				i = dropfiles.begin() + j;
				int s = ((int)j)-1;
				int key = i->key_root;
				int prev_key = key-1;
				int next_key = key+1;

				// figure out which key the adjacent zones have (ignore zones with the same key as they could be velocity layers)
				while(s>=0)
				{
					if(dropfiles[s].key_root < key)
					{	
						prev_key = dropfiles[s].key_root;
						break;
					}
					s--;
				}
				s = (int)j+1;
				while(s<(int)dropfiles.size())
				{
					if(dropfiles[s].key_root > key)
					{	
						next_key = dropfiles[s].key_root;
						break;
					}
					s++;
				}
				i->key_lo = key + (prev_key - key)/2;
				i->key_hi = key + (next_key - key - 1)/2;				
				i->has_startend = true;
			}
		}
	}	
}
