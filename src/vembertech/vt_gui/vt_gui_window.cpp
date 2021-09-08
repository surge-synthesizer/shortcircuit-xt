#include "vt_gui.h"
#include "vt_gui_controls.h"
#include <assert.h>
#include "resource.h"
#include <float.h>
#include <xmmintrin.h>
#include "cpuarch.h"
#include <vt_util/vt_lockfree.h>
#include <vt_util/vt_string.h>

#include <algorithm>
using std::swap;
using std::multimap;
using std::pair;

const int sizer_size = 5;

extern void* hInstance;

enum
{
	editor_tools_first=0,
	editor_pointer=editor_tools_first,
	editor_new1,
	editor_new2,
	editor_new3,
	editor_new4,
	editor_new5,
	editor_new6,
	editor_new7,
	editor_new8,
	editor_new9,
	editor_new10,
	editor_new11,
	editor_new12,
	editor_new13,
	editor_new14,
	editor_tools_last = editor_new14,	
	editor_alignV_top,
	editor_alignV_mid,
	editor_alignV_bottom,
	editor_alignH_left,
	editor_alignH_center,
	editor_alignH_right,
	editor_scaleV,
	editor_scaleH,	
	editor_delete,
	editor_rectX,
	editor_rectX2,
	editor_rectY,
	editor_rectY2,
	editor_cdata0,
	editor_cdata1,
	editor_cdata2,
	editor_cdata3,
	editor_cdata4,
	editor_cdata5,
	editor_cdata6,
	editor_cdata7,
	editor_filter_type,
	editor_filter_entry,
	editor_param_id,
	editor_param_subid,
	editor_offset,
	editor_offset_amount,
	/*editor_snap1,
	editor_snap4,
	editor_snap16,*/	
};

//
// vg_window	
//

int mask_from_win32(WPARAM m)
{
	return	((m&MK_LBUTTON)?vgm_LMB:0)	| ((m&MK_RBUTTON)?vgm_RMB:0) | ((m&MK_MBUTTON)?vgm_MMB:0) | ((m&MK_XBUTTON1)?vgm_MB4:0) | ((m&MK_XBUTTON2)?vgm_MB5:0) |
			((GetKeyState(VK_MENU) < 0)?vgm_alt:0)		| ((m&MK_SHIFT)?vgm_shift:0) | ((m&MK_CONTROL)?vgm_control:0);
}

HHOOK gkbdhook;
HWND kbd_hook_hWnd=0;

LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

// keyboard hooking
LRESULT __declspec(dllexport)__stdcall  CALLBACK hookKeyboardProc(int nCode,WPARAM wParam, LPARAM lParam)
{
    char ch;            
    if (kbd_hook_hWnd&&(HC_ACTION==nCode))
    {               		
		//HWND activehwnd = GetActiveWindow();
		vg_window *appwin = (vg_window*) GetWindowLongPtr(kbd_hook_hWnd, GWLP_USERDATA);
		if(appwin)
		{
			bool previous = ((DWORD)lParam & 0x40000000);
			bool transition = ((DWORD)lParam & 0x80000000);
			vg_controlevent e;
			ZeroMemory( &e, sizeof(e) );

			if(!transition && !previous)		// WM_KEYDOWN-ish
			{
				e.eventtype = vget_keypress;
			} else if(transition && !previous)
			{
				e.eventtype = vget_keyrelease;
			} else if(!transition && previous)
			{
				e.eventtype = vget_keyrepeat;
			}
			
			bool used = false;

			// translate key
			char tb[64],ks[256];
			GetKeyboardState((PBYTE)ks);
			e.buttonmask = (GetKeyState(VK_SHIFT)?vgm_shift:0) | (GetKeyState(VK_CONTROL)?vgm_control:0);				

			unsigned int	vkey = (unsigned int)wParam,
				scancode = (unsigned int)((lParam>>16) & 0xff);			

			if (ToAscii(vkey,scancode,(PBYTE)ks,(LPWORD)tb,0) == 1)
			{
				e.eventdata[0] = tb[0];								
				used = appwin->processevent(e);
				if((e.eventtype == vget_keypress)||(e.eventtype == vget_keyrepeat))
				{
					e.eventtype = vget_character;
					used = appwin->processevent(e);
				}
			}
			else
			{
				e.eventdata[0] = vkey;				
				used = appwin->processevent(e);
			}

			if (used) return 1;	// if found
			//LRESULT result = MsgProc(kbd_hook_hWnd,WM_KEYDOWN,wParam,lParam);
		}
    }

    LRESULT RetVal = CallNextHookEx( gkbdhook, nCode, wParam, lParam );
    return  RetVal;
}

LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{	
	if(msg == WM_NCCREATE)
	{
		LPCREATESTRUCT cs = (LPCREATESTRUCT) lParam;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) cs->lpCreateParams);		
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	
	vg_controlevent e;
	ZeroMemory( &e, sizeof(e) );	
	e.x = (float) GET_X_LPARAM(lParam);
	e.y = (float) GET_Y_LPARAM(lParam);

	vg_window *appwin = (vg_window*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if(!appwin) return DefWindowProc(hWnd, msg, wParam, lParam);

	switch( msg )
	{
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;
	case WM_DESTROY:
		//Cleanup();
		//PostQuitMessage( 0 );		
		return 0L;
	case WM_PAINT:
		appwin->draw((void*)hWnd);
		return 0L; 
	case WM_ACTIVATE:	
		if(wParam == WA_INACTIVE) 
		{
			e.eventtype = vget_deactivate;									
			appwin->processevent(e);
			return 0L;
		}
		else SetFocus(hWnd);
		return 0L;
	case WM_LBUTTONDOWN:
		{			
			SetFocus(hWnd);
			e.eventtype = vget_mousedown;			
			e.activebutton = 1;
			e.buttonmask = mask_from_win32(wParam);		
			appwin->processevent(e);
			return 0L;
		}
	case WM_LBUTTONUP:
		{
			e.eventtype = vget_mouseup;			
			e.activebutton = 1;
			e.buttonmask = mask_from_win32(wParam);
			appwin->processevent(e);
			return 0L;
		}
	case WM_LBUTTONDBLCLK:
		{
			e.eventtype = vget_mousedblclick;			
			e.activebutton = 1;
			e.buttonmask = mask_from_win32(wParam);
			appwin->processevent(e);
			return 0L;
		}
	case WM_RBUTTONDOWN:
		{
			SetFocus(hWnd);
			e.eventtype = vget_mousedown;			
			e.activebutton = 2;
			e.buttonmask = mask_from_win32(wParam);		
			appwin->processevent(e);
			return 0L;
		}
	case WM_RBUTTONUP:
		{
			e.eventtype = vget_mouseup;			
			e.activebutton = 2;
			e.buttonmask = mask_from_win32(wParam);
			appwin->processevent(e);
			return 0L;
		}
	case WM_RBUTTONDBLCLK:
		{
			e.eventtype = vget_mousedblclick;			
			e.activebutton = 2;
			e.buttonmask = mask_from_win32(wParam);
			appwin->processevent(e);
			return 0L;
		}
	case WM_MBUTTONDOWN:
		{
			SetFocus(hWnd);
			e.eventtype = vget_mousedown;			
			e.activebutton = 3;
			e.buttonmask = mask_from_win32(wParam);		
			appwin->processevent(e);
			return 0L;
		}
	case WM_MBUTTONUP:
		{
			e.eventtype = vget_mouseup;			
			e.activebutton = 3;
			e.buttonmask = mask_from_win32(wParam);
			appwin->processevent(e);
			return 0L;
		}
	case WM_MOUSEWHEEL:
		{
			e.eventtype = vget_mousewheel;
			e.buttonmask = mask_from_win32(wParam);
			e.eventdata[0] = GET_WHEEL_DELTA_WPARAM(wParam);
			
			// convert to client coords
			POINT p;
			p.x = GET_X_LPARAM(lParam);
			p.y = GET_Y_LPARAM(lParam);
			ScreenToClient(hWnd, &p); 
			e.x = (float)p.x;
			e.y = (float)p.y;	

			appwin->processevent(e);
			return 0L;
		}
	case WM_DROPFILES:		
		{						
			HDROP hDrop = (HDROP)wParam;			

			char szFn[1024];
			wchar_t wszFn[1024];
			UINT n = DragQueryFileW( hDrop, 0xFFFFFFFF,NULL,0);
			appwin->dropfiles.clear();			
			for(unsigned int i=0; i<n; i++)
			{
				if (DragQueryFileW((HDROP)wParam, i, wszFn, 1024))
				{					
					WideCharToMultiByte(CP_UTF8,0,wszFn,-1,szFn,1024,0,0);
					dropfile d;					
					d.path = szFn;
					TCHAR *lbl = strrchr(szFn,('\\'));
					if(lbl) d.label = lbl + 1;
					else d.label = szFn;
					d.database_id = -1;
					d.db_type = 0;					
					appwin->dropfiles.push_back(d);
				}
			}

			DragFinish(hDrop);
			//POINT p;
			//bool result = DragQueryPoint(hDrop,&p);
			//e.x = (float)p.x;
			//e.y = (float)p.y;
			POINT p;
			GetCursorPos(&p);
			ScreenToClient(hWnd, &p); 
			e.x = (float)p.x;
			e.y = (float)p.y;	

			e.eventtype = vget_dropfiles;			

			appwin->processevent(e);			
			return 0L;
		}
	case WM_MOUSEMOVE:
		{
		/*	TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hWnd;
			TrackMouseEvent(&tme);*/

			if(appwin->ignore_next_mousemove)
			{
				appwin->ignore_next_mousemove = false;
				return 0L;
			}

			e.eventtype = vget_mousemove;
			e.activebutton = 0;	
			e.buttonmask = mask_from_win32(wParam);
			appwin->processevent(e);
			return 0L;
		}		
	case WM_MOUSELEAVE:
		{
			e.eventtype = vget_mouseleave;
			e.activebutton = 0;	
			e.buttonmask = mask_from_win32(wParam);
			appwin->processevent(e);
			return 0L;
		}		
	case WM_KEYDOWN:
		{			
			if((appwin->keyboardmode == kbm_os)||(appwin->keyboardmode == kbm_seperate_window))
			{
				POINT p;
				GetCursorPos(&p);
				ScreenToClient(hWnd, &p); 
				e.x = (float)p.x;
				e.y = (float)p.y;									

				// translate key
				char tb[64],ks[256];
				GetKeyboardState((PBYTE)ks);
				e.buttonmask = (GetKeyState(VK_SHIFT)?vgm_shift:0) | (GetKeyState(VK_CONTROL)?vgm_control:0);				

				unsigned int	vkey = (unsigned int)wParam,
					scancode = (unsigned int)((lParam>>16) & 0xff);

				if (ToAscii(vkey,scancode,(PBYTE)ks,(LPWORD)tb,0) == 1)
				{
					e.eventdata[0] = tb[0];				
					e.eventtype = vget_keypress;
					if (lParam & 0x40000000) e.eventtype = vget_keyrepeat;	// if repeating, change eventtype											

					appwin->processevent(e);
				}
				else
				{
					e.eventdata[0] = vkey;
					e.eventtype = vget_keypress;
					appwin->processevent(e);
				}
				return 0L;
			}
		}
	case WM_KEYUP:
		{			
			if((appwin->keyboardmode == kbm_os)||(appwin->keyboardmode == kbm_seperate_window))
			{
				POINT p;
				GetCursorPos(&p);
				ScreenToClient(hWnd, &p); 
				e.x = (float)p.x;
				e.y = (float)p.y;						

				// translate key
				char tb[64],ks[256];
				GetKeyboardState((PBYTE)ks);
				unsigned int	vkey = (unsigned int)wParam,
					scancode = (unsigned int) ((lParam>>16) & 0xff);

				if (ToAscii(vkey,scancode,(PBYTE)ks,(LPWORD)tb,0) == 1)
				{
					e.eventdata[0] = tb[0];
					e.eventtype = vget_keyrelease;				
					e.buttonmask = (GetKeyState(VK_SHIFT)?vgm_shift:0) | (GetKeyState(VK_CONTROL)?vgm_control:0);				

					appwin->processevent(e);
				}
				return 0L;
			}
		}	
	case WM_CHAR:
		{
			if((appwin->keyboardmode == kbm_os)||(appwin->keyboardmode == kbm_seperate_window))
			{
				e.eventtype = vget_character;
				e.eventdata[0] = (int)wParam;
				e.buttonmask = (GetKeyState(VK_SHIFT)?vgm_shift:0) | (GetKeyState(VK_CONTROL)?vgm_control:0);

				POINT p;
				GetCursorPos(&p);
				ScreenToClient(hWnd, &p); 
				e.x = (float)p.x;
				e.y = (float)p.y;						

				appwin->processevent(e);
				return 0L;
			}
		}	
	case (WM_APP+1212):
		{
			e.eventtype = vget_process_actionbuffer;
			appwin->processevent(e);
			return 0L;
		}
	}
	return DefWindowProc( hWnd, msg, wParam, lParam );
}

vg_window::vg_window()
{
	keyboardmode = kbm_os;
	surf.imgdata = 0;	
	editwin = 0;
	owner = 0;
	menu = 0;
	initialized = false;
	ActionBuffer = 0;
	rootdir_default = ("");
	rootdir_skin = ("");
	syswindow = 0;	
	ignore_next_mousemove = false;

	unsigned int arch = determine_support();
	// detect 
	if(arch & ca_SSE2)
		sse_state_flag = 0x8040;
	else if (arch & ca_SSE) 
		sse_state_flag = 0x8000;	
	else sse_state_flag = 0;
}

//-----------------------------------------------------------------------------------------
void vg_window::fpuflags_set()
{	
#if !PPC
	old_sse_state = _mm_getcsr();
	_mm_setcsr(_mm_getcsr() | sse_state_flag);	
#ifdef _DEBUG		
	_MM_SET_EXCEPTION_STATE(_MM_EXCEPT_DENORM);

	old_fpu_state = _control87(0,0);
	int u = old_fpu_state & ~(/*_EM_INVALID | _EM_DENORMAL |*/ _EM_ZERODIVIDE | _EM_OVERFLOW /* | _EM_UNDERFLOW | _EM_INEXACT*/);
	_control87(u, _MCW_EM);	
	
#endif			
#endif
}
void vg_window::fpuflags_restore()
{	
#if !PPC
	_mm_setcsr(old_sse_state);
#ifdef _DEBUG
	_control87(old_fpu_state,_MCW_EM);	
#endif
#endif
}
vg_window::~vg_window()
{		
}

bool vg_window::create(std::string filename, bool is_editor, vg_window* owner, void *syswindow)
{			
	load_art();
	
	editmode = false;
	selected_id = -1;
	focus_id = -1;
	hover_id = -1;
	this->owner = owner;
	editmode_tool = 0;
	bgcolor = 0x00b0b0b0;
	this->is_editor = is_editor;
	
	ActionBuffer = new vt_LockFree(sizeof(actiondata),0x4000,1);

	layoutfile = rootdir_skin + filename;	
	if(!load_layout())
	{
		layoutfile = rootdir_default + filename;	
		load_layout();
	}

	int sizeX = surf.sizeX;
	int sizeY = surf.sizeY;
	for(int i=0; i<max_vgwindow_filters; i++) filters[i] = 0;

	surf.create(sizeX,sizeY,true);
	
	std::string cf = rootdir_default + ("arrowcopy.cur");
	hcursor_arrowcopy = LoadCursorFromFile(cf.c_str());
	cf = rootdir_default + ("arrowvmove.cur");
	hcursor_vmove = LoadCursorFromFile(cf.c_str());
	cf = rootdir_default + ("arrowhmove.cur");
	hcursor_hmove = LoadCursorFromFile(cf.c_str());
	cf = rootdir_default + ("zoomin.cur");
	hcursor_zoomin = LoadCursorFromFile(cf.c_str());
	cf = rootdir_default + ("zoomout.cur");
	hcursor_zoomout = LoadCursorFromFile(cf.c_str());		
	cf = rootdir_default + ("panhand.cur");
	hcursor_panhand = LoadCursorFromFile(cf.c_str());	

	menu = new vg_menu(this,layout_menu,0);
	menu->set_rect(vg_rect(0,0,sizeX,sizeY));
		
	redraw_all = true;	

	if(syswindow && (keyboardmode != kbm_seperate_window))	// host has already created a window
	{
		// Register the window class		
		WNDCLASSEX wc;
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_DBLCLKS | CS_GLOBALCLASS;
		wc.lpfnWndProc = MsgProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = GetModuleHandle(NULL);
		wc.hIcon = 0;
		wc.hCursor = 0;
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = ("vgwindow");
		wc.hIconSm = NULL;   
		RegisterClassEx( &wc );
		
		// Create the application's window
		hWnd = CreateWindowEx( 0, ("vgwindow"), ("Window"),
			WS_CHILD | WS_VISIBLE, 0, 0, sizeX, sizeY,
			(HWND)syswindow, NULL, wc.hInstance, this );	
		
		if (!hWnd) return FALSE;  
		this->syswindow = (HWND)syswindow;
	}
	else
	{
		// Register the window class
		WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC|CS_DBLCLKS, MsgProc, 0L, 0L,
			GetModuleHandle(NULL), NULL, LoadCursor(NULL,IDC_ARROW), NULL, NULL,
			is_editor?("vgwindow_e"):("vgwindow"), NULL };
		RegisterClassEx( &wc );

		RECT r = {0,0,sizeX,sizeY};
		DWORD WS = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
		if(AdjustWindowRectEx(&r,WS,false,is_editor?WS_EX_TOOLWINDOW:0))
		{
			sizeX = r.right-r.left;
			sizeY = r.bottom-r.top;
		}
		// Create the application's window
		hWnd = CreateWindowEx( is_editor?WS_EX_TOOLWINDOW:0, is_editor?("vgwindow_e"):("vgwindow"), is_editor?("vember GUI editor"):("shortcircuit2"),
			WS, is_editor?1150:100, 100, sizeX, sizeY,
			owner?owner->hWnd:GetDesktopWindow(), NULL, wc.hInstance, this );	

		if (!hWnd) return FALSE;   	

		// Show the window and paint its contents.  
		ShowWindow(hWnd, SW_SHOWDEFAULT); 
		UpdateWindow(hWnd);
	}		
		
	if(keyboardmode == kbm_globalhook) 
	{	
		kbd_hook_hWnd = hWnd;	
		gkbdhook = SetWindowsHookEx(WH_KEYBOARD,(HOOKPROC)hookKeyboardProc,(HINSTANCE)hInstance,0);
	}

	DragAcceptFiles(hWnd,true);

	initialized = true;

	return true;
}

void vg_window::destroy()
{		
	if(keyboardmode == kbm_globalhook) UnhookWindowsHookEx(gkbdhook);
	if(keyboardmode == kbm_globalhook) kbd_hook_hWnd = 0;

	close_toolbox();

	DestroyCursor(hcursor_arrowcopy);
	DestroyCursor(hcursor_vmove);
	DestroyCursor(hcursor_hmove);
	DestroyCursor(hcursor_zoomin);
	DestroyCursor(hcursor_zoomout);	
	DestroyCursor(hcursor_panhand);		

	delete ActionBuffer;
	ActionBuffer = 0;

	for(unsigned int i=0; i<children.size(); i++)
	{		
		delete children[i];
	}
	children.clear();

	for(unsigned int i=0; i<artdata_loaded.size(); i++)
	{
		artdata_loaded[i]->destroy();
		delete artdata_loaded[i];
	}
	artdata_loaded.clear();

	for(unsigned int i=0; i<artfonts.size(); i++)
	{		
		delete artfonts[i];
	}
	artfonts.clear();	

	DestroyWindow(hWnd);
	/*DeleteObject(hbmp);
	// NOTE: this->surf does not need to be destroyed since it's freed by DeleteObject. so just set the pointer to 0
	surf.imgdata = 0;*/
	surf.destroy();

	UnregisterClass(is_editor?("vgwindow_e"):("vgwindow"),GetModuleHandle(NULL));		

	if(background.is_initiated()) background.destroy();

	for(int i=0; i<8; i++) { if(altbg[i].is_initiated()) altbg[i].destroy(); }
	delete menu;
}

void vg_window::draw_sizer(vg_rect r)
{	
	unsigned int scol = 0xffffffff;
	
	int sxa = surf.sizeXA;
	if(r.x2 > r.x)
	{
		for(int y = r.y; y<r.y2; y++)	
		{
			surf.imgdata[r.x + y*sxa] = scol;
			surf.imgdata[r.x2 - 1 + y*sxa] = scol;
		}
	}
	if(r.y2 > r.y)
	{
		for(int x = r.x; x<r.x2; x++)
		{
			surf.imgdata[x + (r.y)*sxa] = scol;
			surf.imgdata[x + (r.y2-1)*sxa] = scol;
		}
	}

	int ss = max(2,min(sizer_size, min(r.get_h(),r.get_w())));

	// whole rect (can be handy)
	sizer_rects[0] = r;
	
	// mid
	sizer_rects[5].set(r.x+ss,r.y+ss,r.x2-ss,r.y2-ss);	

	// corners
	sizer_rects[7].set(r.x,r.y,r.x+ss,r.y+ss);	
	sizer_rects[9].set(r.x2-ss,r.y,r.x2,r.y+ss);	
	sizer_rects[1].set(r.x,r.y2-ss,r.x+ss,r.y2);	
	sizer_rects[3].set(r.x2-ss,r.y2-ss,r.x2,r.y2);	

	// edges
	sizer_rects[8].set(r.x+ss,r.y,r.x2-ss,r.y+ss);
	sizer_rects[2].set(r.x+ss,r.y2-ss,r.x2-ss,r.y2);	
	sizer_rects[4].set(r.x,r.y+ss,r.x+ss,r.y2-ss);
	sizer_rects[6].set(r.x2-ss,r.y+ss,r.x2,r.y2-ss);
		
	for(int i=1; i<9; i++) sizer_rects[i].bound(get_rect()); 	

	surf.fill_rect(sizer_rects[7],scol);	
	surf.fill_rect(sizer_rects[9],scol);
	surf.fill_rect(sizer_rects[1],scol);
	surf.fill_rect(sizer_rects[3],scol);
}

void vg_window::draw_child(int id, bool redraw, HDC hdc)
{
	if(!((id >= 0)&&(id < (int)children.size()))) return;
	
	// fitler controls (multipage etc)
	if((children[id]->filter && !((1<<filters[children[id]->filter]) & children[id]->filter_entry))) return;
	if(children[id]->hidden && !editmode)
	{
		dirtykids_csec.enter();
		if(dirtykids.find(id) == dirtykids.end()) 
		{
			dirtykids_csec.leave();
			return;
		}
		dirtykids_csec.leave();
				
		// hidden control ise dirty, draw background
		int subpage = filters[1]&7;
		vg_bitmap b;
		b.surf = 0;
		if(altbg[subpage].is_initiated()) b.surf = &altbg[subpage];
		else if(background.is_initiated()) b.surf = &background;

		if(b.surf)
		{						
			b.r = children[id]->get_rect();
			surf.blitGDI(b.r, b, hdc);
		}
		else
		{
			surf.clear(bgcolor);			
		}
		return;
	}
	
	if(redraw || children[id]->draw_needed) 
	{
		HDC DC = CreateCompatibleDC(hdc);
		children[id]->surf.set_HDC(DC);		
		children[id]->draw();
		children[id]->surf.set_HDC(0);
		if (DC) DeleteDC(DC);
	} 	

	// blit the child bitmap to the window bitmap		
	vg_rect r = children[id]->get_rect();
	surf.blitGDI(r,&children[id]->surf, hdc);

	if(0)
	{
		// for debugging
		int rndcol = 0x20000000 | 0x00ffffff & (rand() | (rand() << 16));
		//children[id]->surf.fill_rect(vg_rect(0,0,surf.sizeX,surf.sizeY),rndcol);
		surf.fill_rect(r,rndcol);

		char txt[256];
		sprintf(txt,"%i",children[id]->debug_drawcount++);
		surf.font = this->artfonts[0];
		surf.draw_text(r,txt,0xffff0000);
	}	

	//debug
	/*vg_rect debugr = r;
	debugr.y2 = debugr.y + 1;
	surf.fill_rect(debugr,rand()|0xff000000);*/

	if(editmode)
	{		
		if((selected_id == id) || (multiselector.size() && (multiselector.find(id) != multiselector.end())))
		{
			surf.fill_rect(r,0x400000ff);
		}

		if(selected_id == id) draw_sizer(r);
	}
}

void vg_window::draw(void *hwnd, int ctrlid)
{
	// put this in an idle loop later
	//process_actions_from_program();	
	
	PAINTSTRUCT ps; 
    HDC hdc; 

	HWND hWnd = (HWND)hwnd;
	hdc = BeginPaint(hWnd, &ps); 		
	HGDIOBJ hOldObj;
	HDC hdcWinBuf = CreateCompatibleDC (hdc);
	hOldObj = SelectObject (hdcWinBuf, surf.hbmp);

	// draw controls to bitmap		
			
	if(ctrlid >= 0)
	{
		draw_child(ctrlid,true,hdcWinBuf);
		dirtykids_csec.enter();
		dirtykids.erase(ctrlid);
		dirtykids_csec.leave();
	}
	else if(redraw_all || redraw_all_controls)
	{
		int subpage = filters[1]&7;
		if(altbg[subpage].is_initiated())
		{			
			surf.blitGDI(vg_rect(0,0,surf.sizeX,surf.sizeY),&altbg[subpage], hdcWinBuf);			
		}
		else if(background.is_initiated())
		{
			surf.blitGDI(vg_rect(0,0,surf.sizeX,surf.sizeY),&background, hdcWinBuf);
		}
		else
		{
			surf.clear(bgcolor);
		}
		
		for(unsigned int i=0; i<children.size(); i++)
		{			
			draw_child(i,redraw_all_controls,hdcWinBuf);
		}
		redraw_all = false;
		redraw_all_controls = false;

		dirtykids_csec.enter(); dirtykids.clear(); dirtykids_csec.leave();
	}
	else
	{		
		dirtykids_csec.enter();
		std::set<int>::iterator iter;		
		dirtykids.begin();
		iter=dirtykids.begin();
		while(iter != dirtykids.end())
		{
			draw_child(*iter,false,hdcWinBuf);
			iter++;
		}	
		dirtykids.clear();
		dirtykids_csec.leave();
	}	

	if(editmode)
	{		
		//draw grid

		/*int gridsize = 8;
		int sxa = surf.sizeXA;
		for(int y = 0; y<surf.sizeY; y+=gridsize)	
		{
			for(int x = 0; x<surf.sizeX; x+=gridsize)
			{
				surf.imgdata[(x) + (y)*sxa] = 0x00828282;
			}
		}*/

		if(select_rect_visible)
		{
			vg_rect t = select_rect;
			t.flip_if_needed();
			surf.fill_rect(t,0x200000ff);
		}

		if(editwin)
		{
			editwin->toolbox_selectedstate(selected_id >= 0, multiselector.size()>1,editmode_tool);
			if(selected_id>=0)
			{
				vg_control *ct = children[selected_id];

				vg_rect r = ct->get_rect();
				actiondata ad;
				ad.actiontype = vga_text;
				
				char *c = (char*)ad.data.str;
				ad.id = editor_rectX;
				sprintf(c,"%i",r.x);
				editwin->post_action_to_control(ad);				
				ad.id = editor_rectX2;
				sprintf(c,"%i",r.x2);
				editwin->post_action_to_control(ad);
				ad.id = editor_rectY;
				sprintf(c,"%i",r.y);
				editwin->post_action_to_control(ad);				
				ad.id = editor_rectY2;
				sprintf(c,"%i",r.y2);
				editwin->post_action_to_control(ad);								
			}
		}
	}
	
	if(focus_id == layout_menu)
	{
		vg_menu* m = (vg_menu*)menu;
		if (m->draw_needed) m->draw();
		for(unsigned int i=0; i<m->columns.size(); i++)
		{
			vg_bitmap b;
			b.surf = &m->surf;
			b.r = m->columns[i].r;
			b.r.bound(vg_rect(0,0,surf.sizeX,surf.sizeY));
			//surf.blit_alphablendGDI(b.r,b,hdcWinBuf);
			surf.blitGDI(b.r,b,hdcWinBuf);
			//surf.blit(b.r,b);
		}
		
		//surf.blit_alphablend(menu->get_rect(),&menu->surf);		
	}
	RECT UR;		
	// paint bitmap to window
	if(ctrlid >= 0)
	{
		vg_rect R = children[ctrlid]->get_rect();
		BitBlt (hdc, R.x, R.y, min(R.x2,surf.sizeXA), min(R.y2,surf.sizeYA), (HDC)hdcWinBuf, R.x, R.y, SRCCOPY);
	}
	else if(GetUpdateRect((HWND)hwnd,&UR,false))	
		BitBlt (hdc, UR.left, UR.top, min(UR.right,surf.sizeXA), min(UR.bottom,surf.sizeYA), (HDC)hdcWinBuf, 0, 0, SRCCOPY);
	else 
		BitBlt (hdc, 0, 0, surf.sizeXA, surf.sizeYA, (HDC)hdcWinBuf, 0, 0, SRCCOPY);
	SelectObject (hdcWinBuf, hOldObj);
	DeleteDC (hdcWinBuf);

	EndPaint(hWnd, &ps); 
}

void vg_window::set_selected_id(int s)
{
	selected_id = s;
	int n = 0;
	actiondata ad;	
	char *c = ad.data.str;
	size_t csize = sizeof(ad.data); 

	if((selected_id>=0)&&(selected_id<(int)children.size()))
	{
		vg_control *ct = children[selected_id];
		n = ct->get_n_parameters();				

		for(int i=0; i<8; i++)
		{
			ad.id = editor_cdata0+i;
			ad.actiontype = vga_text;
			_snprintf_s(c,csize,_TRUNCATE,"%s",ct->get_parameter_text(i).c_str());
			editwin->post_action_to_control(ad);
			ad.actiontype = vga_label;
			_snprintf_s(c,csize,_TRUNCATE,"%s",ct->get_parameter_name(i).c_str());
			editwin->post_action_to_control(ad);
			ad.actiontype = vga_disable_state;
			ad.data.i[0] = false;
			editwin->post_action_to_control(ad);
		}

		ad.id = editor_filter_type;
		sprintf_s(c,csize,"%i",ct->filter);
		ad.actiontype = vga_text;
		editwin->post_action_to_control(ad);	
		ad.actiontype = vga_disable_state;
		ad.data.i[0] = false;
		editwin->post_action_to_control(ad);
		ad.id = editor_filter_entry;
		//sprintf_s(c,csize,"%i",ct->filter_entry);
		hexlist_from_bitmask(c,ct->filter_entry);
		ad.actiontype = vga_text;
		editwin->post_action_to_control(ad);	
		ad.actiontype = vga_disable_state;
		ad.data.i[0] = false;
		editwin->post_action_to_control(ad);		
		
		ad.id = editor_offset;
		sprintf_s(c,csize,"%i",ct->offset);
		ad.actiontype = vga_text;
		editwin->post_action_to_control(ad);	
		ad.actiontype = vga_disable_state;
		ad.data.i[0] = false;
		editwin->post_action_to_control(ad);
		ad.id = editor_offset_amount;
		sprintf_s(c,csize,"%i",ct->offset_amount);
		ad.actiontype = vga_text;
		editwin->post_action_to_control(ad);	
		ad.actiontype = vga_disable_state;
		ad.data.i[0] = false;
		editwin->post_action_to_control(ad);

		// add param_id list etc etc
		ad.id = editor_param_id;
		ad.actiontype = vga_entry_clearall;
		editwin->post_action_to_control(ad);		
		for(int i=0; i<param_get_n(); i++)
		{
			actiondata ad2;	
			ad2.id = editor_param_id;
			ad2.actiontype = vga_intval;
			ad2.data.i[0] = i;
			ad.actiontype = vga_entry_add;
			ad.data.ptr[0] = &ad2;
			std::string s = param_get_label_from_id(i);
			vtCopyString((char*)&ad.data.str[8],s.c_str(),32);
			editwin->post_action_to_control(ad);
		}	
		ad.actiontype = vga_intval;
		ad.id = editor_param_id;
		ad.data.i[0] = ct->parameter_id;
		editwin->post_action_to_control(ad);
		
		// & subid
		ad.id = editor_param_subid;
		ad.actiontype = vga_entry_clearall;
		editwin->post_action_to_control(ad);
		for(int i=0; i<param_get_n_subparams(ct->parameter_id); i++)
		{
			actiondata ad2;
			ad2.id = editor_param_subid;
			ad2.actiontype = vga_intval;
			ad2.data.i[0] = i;
			ad.actiontype = vga_entry_add;
			ad.data.ptr[0] = &ad2;			
			sprintf((char*)&ad.data.str[8],"%i",i+1);
			editwin->post_action_to_control(ad);
		}					
		ad.actiontype = vga_intval;
		ad.id = editor_param_subid;
		ad.data.i[0] = ct->parameter_subid;
		editwin->post_action_to_control(ad);		
	} 
	else
	{
		ad.id = editor_filter_type;
		sprintf_s(c,csize,"-");
		ad.actiontype = vga_text;
		editwin->post_action_to_control(ad);	
		ad.actiontype = vga_disable_state;
		ad.data.i[0] = true;
		editwin->post_action_to_control(ad);
		ad.id = editor_filter_entry;
		sprintf_s(c,csize,"-");
		ad.actiontype = vga_text;
		editwin->post_action_to_control(ad);	
		ad.actiontype = vga_disable_state;
		ad.data.i[0] = true;
		editwin->post_action_to_control(ad);
		ad.id = editor_offset;
		sprintf_s(c,csize,"-");
		ad.actiontype = vga_text;
		editwin->post_action_to_control(ad);	
		ad.actiontype = vga_disable_state;
		ad.data.i[0] = true;
		editwin->post_action_to_control(ad);
		ad.id = editor_offset_amount;
		sprintf_s(c,csize,"-");
		ad.actiontype = vga_text;
		editwin->post_action_to_control(ad);	
		ad.actiontype = vga_disable_state;
		ad.data.i[0] = true;
		editwin->post_action_to_control(ad);
	}
	for(int i=n; i<8; i++)
	{
		ad.id = editor_cdata0+i;
		ad.actiontype = vga_text;
		sprintf_s(c,csize,"-");
		editwin->post_action_to_control(ad);
		ad.actiontype = vga_label;					
		editwin->post_action_to_control(ad);
		ad.actiontype = vga_disable_state;
		ad.data.i[0] = true;
		editwin->post_action_to_control(ad);
	}	
}

bool vg_window::processevent(vg_controlevent &e)
{	
	fpuflags_set();
	bool used = true;
	
	vg_point p;
	p.x = (int)e.x;
	p.y = (int)e.y;

	// check for editor hotkey (ctrl+shift+E?)
	if(is_editor)
	{
		editmode = false;		
	}
	else
	{
		if(e.eventtype == vget_keypress)
		{			
			if((e.buttonmask & vgm_control)&&(e.eventdata[0] == VK_F12))
			{
				editmode = !editmode;
				redraw_all = true;

				if(editmode) open_toolbox();
				else close_toolbox();
			}
		}
	}

	if (editmode)
	{		
		if (e.eventtype == vget_mouseup)
		{
			editmode_drag = 0;
			select_rect_visible = false;	
			redraw_all = true;
		}		

		if(e.eventtype == vget_keypress)
		{
			int nudge = 1;
			if(e.buttonmask & vgm_shift) nudge = 8;

			switch(e.eventdata[0])
			{
			case VK_UP:					
				nudge_selected_control(0,-nudge);
				break;
			case VK_DOWN:
				nudge_selected_control(0,nudge);
				break;
			case VK_LEFT:
				nudge_selected_control(-nudge,0);
				break;
			case VK_RIGHT:
				nudge_selected_control(nudge,0);
				break;
			case 's':
			case 'S':
				save_layout();
				break;
			case 'l':
			case 'L':
				load_layout();
				break;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				{
					int id = e.eventdata[0] - '1';
					editmode_tool = id;
					redraw_all = true;				
				}					
			case VK_DELETE:
				{
					delete_child(-1);					
					redraw_all = true;
					break;
				}			
			case VK_PRIOR:
				{					
					if((selected_id > 0)&&(selected_id < (int)children.size()))
					{
						swap(children[selected_id],children[selected_id-1]);						
						redraw_all = true;
						set_selected_id(selected_id-1);
					}
				}				
				break;
			case VK_NEXT:			
				{					
					if((selected_id >= 0)&&(selected_id < ((int)children.size()-1)))
					{
						swap(children[selected_id],children[selected_id+1]);
						redraw_all = true;
						set_selected_id(selected_id+1);
					}
				}				
				break;
			}
		} 
		else if((e.eventtype == vget_mousedown) && (e.activebutton == vgm_LMB))
		{
			if(editmode_tool>0)
			{
				int id = min(editmode_tool,n_vgct-1);
				int newid = (int)children.size();
				vg_control *t = spawn_control(vgct_names[id],0,this,newid);
				t->set_rect(vg_rect(p.x,p.y,p.x+t->get_min_w(),p.y+t->get_min_h()));	
				set_selected_id(newid);
				multiselector.clear();
				children.push_back(t);
				editmode_tool = 0;
				editmode_drag = 3;	// continue dragging size
				editmode_drag_origin = p;
				editmode_cursor_origin = p;				
				redraw_all = true;
			}
			else
			{						
				bool cont = true;
				bool shifted = ((e.buttonmask & (vgm_control|vgm_shift)) != 0);

				select_rect_visible = false;					
				redraw_all = true;			

				for(int i=1; i<10; i++)
				{
					if((sizer_rects[i].point_within(p)) && (selected_id >= 0))
					{
						editmode_drag = i;
						editmode_drag_origin = p;
						editmode_cursor_origin = p;

						if((i == 3)||(i == 6)||(i == 9)) editmode_drag_origin.x -= sizer_rects[0].x2;
						else editmode_drag_origin.x -= sizer_rects[0].x;

						if((i == 1)||(i == 2)||(i == 3)) editmode_drag_origin.y -= sizer_rects[0].y2;
						else editmode_drag_origin.y -= sizer_rects[0].y;				

						cont = false;
						break;
					}
				}

				if(cont)
				{									
					if(!shifted) 
					{
						multiselector.clear();
						set_selected_id(-1);
					}
					int x = (int)e.x;
					int y = (int)e.y;
					select_rect.x = x;
					select_rect.y = y;	

					int n = (int)children.size();
					for(int i=(n-1); i>=0; i--)	// go backwards
					{
						if(children[i]->visible())
						{
							vg_rect r = children[i]->get_rect();
							if((x >= r.x) && (x < r.x2) && (y >= r.y) && (y < r.y2))
							{
								if(shifted) multiselector.insert(i);
								else 
								{
									set_selected_id(i);
									multiselector.insert(i);
								}
								break;
							}
						}
					}					
					redraw_all = true;	
					editmode_drag = 0;
				}
			}
		}	
		else if((e.eventtype == vget_mousemove) && (selected_id >= 0))
		{					
			if(sizer_rects[2].point_within(p)) set_cursor(cursor_sizeNS);
			else if(sizer_rects[8].point_within(p)) set_cursor(cursor_sizeNS);
			else if(sizer_rects[4].point_within(p)) set_cursor(cursor_sizeWE);
			else if(sizer_rects[6].point_within(p)) set_cursor(cursor_sizeWE);			
			else if(sizer_rects[1].point_within(p)) set_cursor(cursor_sizeNESW);
			else if(sizer_rects[9].point_within(p)) set_cursor(cursor_sizeNESW);			
			else if(sizer_rects[3].point_within(p)) set_cursor(cursor_sizeNWSE);
			else if(sizer_rects[7].point_within(p)) set_cursor(cursor_sizeNWSE);			
			else if(sizer_rects[5].point_within(p)) set_cursor(cursor_move);	
			else set_cursor(cursor_arrow); 

			if(editmode_drag == 5)
			{
				vg_rect cr = children[selected_id]->get_rect();								
				
				if(e.buttonmask & vgm_shift)
				{
					if(abs(p.x-editmode_cursor_origin.x) < abs(p.y-editmode_cursor_origin.y))
					{
						cr.move_constrained(editmode_cursor_origin.x - editmode_drag_origin.x,p.y - editmode_drag_origin.y,get_rect());
					}
					else
					{
						cr.move_constrained(p.x - editmode_drag_origin.x,editmode_cursor_origin.y - editmode_drag_origin.y,get_rect());
					}
				}
				else
				{
					cr.move_constrained(p.x - editmode_drag_origin.x,p.y - editmode_drag_origin.y,get_rect());
				}
				children[selected_id]->set_rect(cr);
				redraw_all = true;
				set_cursor(cursor_move);	
			} 
			else if(editmode_drag > 0)
			{								
				vg_rect cr = children[selected_id]->get_rect();								
				int i = editmode_drag;
				if((i == 1)||(i == 4)||(i == 7)) cr.x = min(p.x,cr.x2-children[selected_id]->get_min_w());
				if((i == 3)||(i == 6)||(i == 9)) cr.x2 = max(p.x,cr.x+children[selected_id]->get_min_w());				
				if((i == 7)||(i == 8)||(i == 9)) cr.y = min(p.y,cr.y2-children[selected_id]->get_min_h());
				if((i == 1)||(i == 2)||(i == 3)) cr.y2 = max(p.y,cr.y+children[selected_id]->get_min_h());

				cr.bound(get_rect());
				children[selected_id]->set_rect(cr);
				redraw_all = true;
				//set_cursor(cursor_sizeWE);
			}										
		}		
		if((e.eventtype == vget_mousemove) && (e.buttonmask & vgm_LMB) && !editmode_drag)
		{
			select_rect.x2 = p.x;
			select_rect.y2 = p.y;
			select_rect_visible = true;				
			redraw_all = true;

			// process selection rectangle
			vg_rect sel = select_rect;
			sel.flip_if_needed();
			bool picky = select_rect.x2 < select_rect.x;
			multiselector.clear();		
			for(unsigned int i=0; i<children.size(); i++)
			{
				if(children[i]->visible())
				{
					bool b;
					if(picky) b = sel.does_encase(children[i]->get_rect());
					else b = sel.does_intersect(children[i]->get_rect());
					if(b)
					{					
						multiselector.insert(i);
					}
				}
			}			
			if((selected_id<0) && (multiselector.size()>0)) set_selected_id(*multiselector.begin());
		}
	}
	else if(e.eventtype == vget_mouseleave)
	{		
		// only affects hovered control
		if((hover_id >= 0)&&(hover_id<(int)children.size()))
		{
			vg_rect r = children[hover_id]->get_rect();
			vg_controlevent e2 = e;
			e2.x -= (float) r.x;
			e2.y -= (float) r.y;	
			e2.eventtype = vget_mouseleave;			
			children[hover_id]->processevent(e2);
			hover_id = -1;										
		}		
		set_cursor(cursor_arrow);		
	}
	else if(e.eventtype == vget_deactivate)
	{		
		if(focus_id >= 0)
		{
			drop_focus(focus_id);
		}
		hover_id = -1;
		focus_id = -1;
		set_cursor(cursor_arrow);
		show_mousepointer(true);
	}
	else used = processevent_user(e);
	
	if (redraw_all_controls || redraw_all) 
	{
		RedrawWindow(hWnd,0,0,RDW_INVALIDATE);
	}	
	else if(!dirtykids.empty())
	{
		dirtykids_csec.enter();
		if(dirtykids.size() < 2)
		{						
			std::set<int>::iterator iter = dirtykids.begin();
			if(!((*iter >= 0)&&(*iter < (int)children.size()))) RedrawWindow(hWnd,0,0,RDW_INVALIDATE);
			else
			{
				vg_rect vgr = children[*iter]->get_rect();
				RECT R;
				R.left = vgr.x;
				R.right = vgr.x2;
				R.top = vgr.y;
				R.bottom = vgr.y2;
				RedrawWindow(hWnd,&R,0,RDW_INVALIDATE);
				//draw((void*)hWnd,*iter);
			}			
		}		
		else
			RedrawWindow(hWnd,0,0,RDW_INVALIDATE);
		dirtykids_csec.leave();
	}
	fpuflags_restore();
	return used;
}

// processes events for the end-user interface (intended to be small for easy overriding)
bool vg_window::processevent_user(vg_controlevent &e)
{	
	if(e.eventtype == vget_process_actionbuffer)
	{
		process_actions_from_program();
		return true;
	}	
	
	int x = (int)e.x;
	int y = (int)e.y;
	int n = (int)children.size();

	if((focus_id>0)&&(focus_id<n))
	{
		vg_rect r = children[focus_id]->get_rect();

		vg_controlevent e2 = e;
		e2.x -= (float) r.x;
		e2.y -= (float) r.y;								
		children[focus_id]->processevent(e2);				
	} else if((focus_id == layout_menu))
	{
		menu->processevent(e);
	}
	else
	{
		// check hover status
		if((hover_id >= 0) && (hover_id<n))
		{
			if(children[hover_id]->visible())
			{
				vg_rect r = children[hover_id]->get_rect();
				if(!((x >= r.x) && (x < r.x2) && (y >= r.y) && (y < r.y2)))
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
				if((x >= r.x) && (x < r.x2) && (y >= r.y) && (y < r.y2))
				{
					vg_controlevent e2 = e;
					e2.x -= (float) r.x;
					e2.y -= (float) r.y;				
					if((e.eventtype == vget_mousemove) && (i != hover_id))
					{
						if((hover_id >= 0)&&(hover_id<n))
						{	
							vg_controlevent e4 = e;						
							e4.eventtype = vget_mouseleave;
							children[hover_id]->processevent(e4);
						}
						vg_controlevent e3 = e2;
						e3.eventtype = vget_mouseenter;
						children[i]->processevent(e3);
						hover_id = i;
					}
					children[i]->processevent(e2);
					break;
				}
			}
		}
		if (hover_id<0) set_cursor(cursor_arrow);		
	}		
	return false;
}

void vg_window::nudge_selected_control(int x, int y)
{
	if (editmode)
	{
		if(multiselector.size()>0)
		{
			std::set<int>::iterator iter;
			
			for (iter = multiselector.begin(); iter != multiselector.end(); iter++)
			{
				int i = *iter;

				if((i>=0)&&(i<(int)children.size()))
				{
					vg_rect cr = children[i]->get_rect();				
					cr.move_constrained(cr.x + x, cr.y + y, get_rect());				
					children[i]->set_rect(cr);								
				}				
			}
			redraw_all = true;
		}
		else if(selected_id >= 0)
		{
			vg_rect cr = children[selected_id]->get_rect();				
			cr.move_constrained(cr.x + x, cr.y + y, get_rect());				
			children[selected_id]->set_rect(cr);			
			redraw_all = true;
		}		
	}
}

bool vg_window::take_focus(int id)
{
	SetCapture(hWnd);
	if(focus_id<0) 
	{
		focus_id = id;
		return true;
	}
	return false;
}
void vg_window::drop_focus(int id)
{
	if(focus_id == id) focus_id = -1;
	ReleaseCapture();
}

void vg_window::set_child_dirty(int id, bool do_redraw_all)
{
	if(do_redraw_all) 
		redraw_all = true;			
	else		
	{
		dirtykids_csec.enter();
		dirtykids.insert(id);				
		dirtykids_csec.leave();
	}	
}

vg_rect vg_window::get_rect()
{
	vg_rect r(0,0,surf.sizeX,surf.sizeY);
	return r;
}

vg_font* vg_window::get_font(unsigned int id)
{
	if (artfonts.empty()) return 0;
	if(id>=artfonts.size()) return artfonts[0];
	else return artfonts[id];
}

const int ff_revision = 1;

void vg_window::save_layout()
{	
	TiXmlDeclaration decl("1.0","UTF-8","yes");
	TiXmlDocument doc;

	TiXmlElement layout("layout");
	layout.SetAttribute("revision",ff_revision);
	layout.SetAttribute("sizeX",surf.sizeX);
	layout.SetAttribute("sizeY",surf.sizeY);
	if(background_filename.size()) layout.SetAttribute("background",background_filename.c_str());

	for(int i=0; i<7; i++)
	{
		char atr[64];
		sprintf(atr,"background%i",i);		
		if(altbg_filename[i].size()) layout.SetAttribute(atr,altbg_filename[i].c_str());
	}

	unsigned int n = (int)children.size();
	for(unsigned int i=0; i<n; i++)
	{
		TiXmlElement control("control");
		control.SetAttribute("type",children[i]->serialize_tag());		
		control.SetAttribute("parameter",param_get_label_from_id(children[i]->parameter_id));
		if (children[i]->parameter_subid) control.SetAttribute("parameter_subid",children[i]->parameter_subid);
		if (children[i]->filter) 
		{
			control.SetAttribute("filter",children[i]->filter);
			//control.SetAttribute("filter_entry",children[i]->filter_entry);
			char tmp[64];
			hexlist_from_bitmask(tmp,children[i]->filter_entry);
			control.SetAttribute("filter_entry",tmp);
			control.SetAttribute("offset",children[i]->offset);
			control.SetAttribute("offset_amount",children[i]->offset_amount);
		}
		TiXmlElement rect("rect");
		vg_rect r = children[i]->get_rect();
		rect.SetAttribute("x",r.x);
		rect.SetAttribute("y",r.y);
		rect.SetAttribute("x2",r.x2);
		rect.SetAttribute("y2",r.y2);
		control.InsertEndChild(rect);
		
		TiXmlElement data("data");
		children[i]->serialize_data(&data);
		control.InsertEndChild(data);

		layout.InsertEndChild(control);
	}	

	doc.InsertEndChild(decl);
	doc.InsertEndChild(layout);		
	
//	doc.InsertEndChild();
	doc.SaveFile(layoutfile.c_str());
	
}
bool vg_window::load_layout()
{
	int j;	
	
	TiXmlDocument doc;
	if (!doc.LoadFile(layoutfile.c_str())) return false;

	TiXmlElement *layout = doc.FirstChild("layout")->ToElement();
	if(!layout) return false;		

	int revision = ff_revision;
	layout->QueryIntAttribute("revision",&revision);	

	if(layout->QueryIntAttribute("sizeX",&j)==TIXML_SUCCESS) 
	{
		surf.sizeX = j;
		surf.sizeXA = j;
	}

	if(layout->QueryIntAttribute("sizeY",&j)==TIXML_SUCCESS) 
	{
		surf.sizeY = j;
		surf.sizeYA = j;
	}

	const char *bgfile = layout->Attribute("background");
	if(bgfile)
	{
		if (!background.create(rootdir_skin + bgfile)) background.create(rootdir_default + bgfile);
		background_filename = bgfile;
	}

	for(int i=0; i<7; i++)
	{
		char atr[64];
		sprintf(atr,"background%i",i);
		bgfile = layout->Attribute(atr);
		if(bgfile)
		{		
			if(!altbg[i].create(rootdir_skin + bgfile)) altbg[i].create(rootdir_default + bgfile);
			altbg_filename[i] = bgfile;
		}
	}

	TiXmlElement *control = layout->FirstChild("control")->ToElement();	

	while(control)
	{		
		TiXmlElement *rect = control->FirstChild("rect")->ToElement();
		TiXmlElement *data = control->FirstChild("data")->ToElement();
		std::string type = control->Attribute("type");
		if(rect)
		{
			int x=0,x2=0,y=0,y2=0;
			if (rect->QueryIntAttribute("x",&j)==TIXML_SUCCESS) x = j;
			if (rect->QueryIntAttribute("x2",&j)==TIXML_SUCCESS) x2 = j;
			if (rect->QueryIntAttribute("y",&j)==TIXML_SUCCESS) y = j;
			if (rect->QueryIntAttribute("y2",&j)==TIXML_SUCCESS) y2 = j;
			
			int newid = (int)children.size();
			//vg_control *t = new vg_control(this,newid,data);
			vg_control *t = spawn_control(type,data,this,newid);
			if(t)
			{
				t->set_rect(vg_rect(x,y,x2,y2));	
				selected_id = newid;

				const char *c = control->Attribute("parameter",&j);
				if(c) t->parameter_id = param_get_id_from_label(c);
				if((t->parameter_id < 0)&&(control->QueryIntAttribute("parameter",&j)==TIXML_SUCCESS)) t->parameter_id = j;
				if (control->QueryIntAttribute("parameter_subid",&j)==TIXML_SUCCESS) t->parameter_subid = j;
				if (control->QueryIntAttribute("filter",&j)==TIXML_SUCCESS) t->filter = j;
				const char *s = control->Attribute("filter_entry");
				if(s) t->filter_entry = bitmask_from_hexlist(s);
				
				if (control->QueryIntAttribute("offset",&j)==TIXML_SUCCESS) t->offset = j;
				if (control->QueryIntAttribute("offset_amount",&j)==TIXML_SUCCESS) t->offset_amount = j;			

				children.push_back(t);
			}
		}

		control = control->NextSibling("control")->ToElement();
	}	
	build_children_by_pid();
	
	return true;
}

vg_bitmap vg_window::get_art(unsigned int bank_id, unsigned int style_id, unsigned int entry_id)
{
	TiXmlElement *bank = artdoc.FirstChild("bank")->ToElement();
	while(bank)
	{
		int j,b_id=0;
		if (bank->QueryIntAttribute("i",&j)==TIXML_SUCCESS) b_id = j;
		if(b_id == bank_id)
		{
			int bs = 0;

			TiXmlElement *style = bank->FirstChild("style")->ToElement();	

			while(style)
			{
				if(bs == style_id)
				{
					int bss = 0;
					TiXmlElement *surface = style->FirstChild("surface")->ToElement();	
					while(surface)
					{			
						surface->Attribute("loadedid",&j);
						vg_surface *s = artdata_loaded[j];

/*						surface->Attribute("pointer",&j);
						s = (vg_surface*) j;	*/
						
						if(s)
						{																				
							int bssb = 0;
							TiXmlElement *bitmap = surface->FirstChild("bitmap")->ToElement();	
							while(bitmap)
							{
								if(bssb == entry_id)
								{
									int x=0,x2=0,y=0,y2=0;
									if (bitmap->QueryIntAttribute("x",&j)==TIXML_SUCCESS) x = j;
									if (bitmap->QueryIntAttribute("x2",&j)==TIXML_SUCCESS) x2 = j;
									if (bitmap->QueryIntAttribute("y",&j)==TIXML_SUCCESS) y = j;
									if (bitmap->QueryIntAttribute("y2",&j)==TIXML_SUCCESS) y2 = j;
									vg_bitmap nb;
									nb.r.set(x,y,x2,y2);
									nb.surf = s;
									return nb;	
								}
								bssb++;
								bitmap = bitmap->NextSibling("bitmap")->ToElement();				
							}					
						}
						bss++;
						surface = surface->NextSibling("surface")->ToElement();						
					}
					break;
				}
				bs++;
				style = style->NextSibling("style")->ToElement();				
			}	
			break;
		}
		bank = bank->NextSibling("bank")->ToElement();	
	}
	vg_bitmap none;
	none.r.set(0,0,0,0);
	none.surf = 0;
	return none;
}

void vg_window::load_art()
{	
	std::string filename = rootdir_skin + "art.xml";
		
	if (!artdoc.LoadFile(filename))
	{
		std::string filename = rootdir_default + "art.xml";
		artdoc.LoadFile(filename);
	}
	int j;
	
	TiXmlElement *bank = artdoc.FirstChild("bank")->ToElement();
	while(bank)
	{
		int b_id=0;
		if (bank->QueryIntAttribute("i",&j)==TIXML_SUCCESS) b_id = j;

		int bs = 0;

		TiXmlElement *style = bank->FirstChild("style")->ToElement();	

		while(style)
		{
			int bss = 0;

			TiXmlElement *surface = style->FirstChild("surface")->ToElement();	
			while(surface)
			{				
				const char *fn = surface->Attribute("filename");
				if(fn)
				{
					vg_surface *s = new vg_surface();
					if(s->read_png(rootdir_skin + fn) == -1) s->read_png(rootdir_default + fn);
					surface->SetAttribute("loadedid",(int)artdata_loaded.size());
					artdata_loaded.push_back(s);					
					//surface->SetAttribute("pointer",(int)(s));
				}
				bss++;
				surface = surface->NextSibling("surface")->ToElement();
			}
			bs++;
			style = style->NextSibling("style")->ToElement();
		}	
		bank = bank->NextSibling("bank")->ToElement();
	}

	TiXmlElement *font = artdoc.FirstChild("font")->ToElement();
	while(font)
	{
		const char *fn = font->Attribute("filename");

		vg_font *nf = new vg_font();
		if(!nf->load(rootdir_skin + fn)) nf->load(rootdir_default + fn);
		artfonts.push_back(nf);

		font = font->NextSibling("font")->ToElement();
	}	
	
	TiXmlElement *color = artdoc.FirstChild("color")->ToElement();
	while(color)
	{				
		int id;
		if ((color->QueryIntAttribute("i",&id)==TIXML_SUCCESS) && (id >= 0) && (id<256))
		{		
			const char *val = color->Attribute("val");
			if (val) colorpalette[id] = hex2color(val);
		}
		color = color->NextSibling("color")->ToElement();
	}	
}

void vg_window::move_mousepointer(int x, int y)
{
#if WINDOWS	
	POINT p;
	p.x = x;
	p.y = y;
	ClientToScreen(hWnd,&p);
	ignore_next_mousemove = true;
	SetCursorPos(p.x,p.y);
#endif
}

void vg_window::show_mousepointer(bool s)
{
#if WINDOWS	
	if(s) 
	{
		// show cursor
		ShowCursor(true);
	}
	else
	{
		// hide (make sure it actually dissapears)
		int c = ShowCursor(false);
		while(c >= 0)
		{
			c = ShowCursor(false);
		}
	}
#endif
}

void vg_window::set_cursor(int a)
{
#if WINDOWS
	switch(a)
	{
	default:
	case cursor_arrow:
		SetCursor(LoadCursor(NULL,IDC_ARROW));
		break;
	case cursor_hand_point:
		SetCursor(LoadCursor(NULL,IDC_HAND));
		break;		
	case cursor_hand_pan:
		if(hcursor_panhand) SetCursor (hcursor_panhand);
		break;
	case cursor_wait:
		SetCursor(LoadCursor(NULL,IDC_WAIT));
		break;
	case cursor_forbidden:
		SetCursor(LoadCursor(NULL,IDC_NO));
		break;
	case cursor_move:
		SetCursor(LoadCursor(NULL,IDC_SIZEALL));
		break;
	case cursor_copy:		
		if(hcursor_arrowcopy) SetCursor (hcursor_arrowcopy);		
		break;
	case cursor_zoomin:
		if(hcursor_zoomin) SetCursor (hcursor_zoomin);
		break;
	case cursor_zoomout:
		if(hcursor_zoomout) SetCursor (hcursor_zoomout);
		break;
	case cursor_vmove:
		if(hcursor_vmove) SetCursor (hcursor_vmove);
		break;
	case cursor_hmove:
		if(hcursor_hmove) SetCursor (hcursor_hmove);
		break;
	case cursor_text:
		SetCursor(LoadCursor(NULL,IDC_IBEAM));
		break;
	case cursor_sizeNS:
		SetCursor(LoadCursor(NULL,IDC_SIZENS));
		break;	
	case cursor_sizeWE:
		SetCursor(LoadCursor(NULL,IDC_SIZEWE));
		break;	
	case cursor_sizeNESW:
		SetCursor(LoadCursor(NULL,IDC_SIZENESW));
		break;	
	case cursor_sizeNWSE:
		SetCursor(LoadCursor(NULL,IDC_SIZENWSE));
		break;	
	}
#endif
}

void vg_window::toolbox_selectedstate(bool selected, bool multiselector, int currenttool)
{
	actiondata ad;	
	ad.actiontype = vga_disable_state;
	ad.data.i[0] = !(selected || multiselector);
	
	ad.id = editor_delete;	post_action_to_control(ad);
	ad.id = editor_rectX;	post_action_to_control(ad);
	ad.id = editor_rectX2;	post_action_to_control(ad);
	ad.id = editor_rectY;	post_action_to_control(ad);
	ad.id = editor_rectY2;	post_action_to_control(ad);	
	ad.id = editor_param_id;	post_action_to_control(ad);	
	ad.id = editor_param_subid;	post_action_to_control(ad);	

	ad.data.i[0] = !multiselector;
	ad.id = editor_alignV_top;		post_action_to_control(ad);
	ad.id = editor_alignV_mid;		post_action_to_control(ad);
	ad.id = editor_alignV_bottom;	post_action_to_control(ad);	
	ad.id = editor_alignH_left;		post_action_to_control(ad);
	ad.id = editor_alignH_center;	post_action_to_control(ad);
	ad.id = editor_alignH_right;	post_action_to_control(ad);	
	ad.id = editor_scaleV;			post_action_to_control(ad);
	ad.id = editor_scaleH;			post_action_to_control(ad);	
	
	ad.actiontype = vga_stuck_state;
	ad.data.i[0] = false;	
	for(int i=editor_tools_first; i<=editor_tools_last; i++) 
	{
		ad.id = i;
		post_action_to_control(ad);
	}
	ad.id = currenttool;
	ad.data.i[0] = true;	
	post_action_to_control(ad);			
	
	if (redraw_all_controls || redraw_all || (dirtykids.size() > 0)) 
	{
		RedrawWindow(hWnd,0,0,RDW_INVALIDATE);
	}	
}

bool vg_window::delete_child(int id)		// only works one at a time (since the ids will change)
{
	if(id<0) // delete all selected children
	{
		if(multiselector.size()>0)
		{
			if((selected_id >= 0) && (multiselector.find(selected_id) != multiselector.end())) multiselector.insert(selected_id);			
			int n = (int)children.size();
			for(int i=0; i<n; i++)
			{
				if (multiselector.find(i) != multiselector.end())
				{
					delete children[i];
					children[i] = 0;
				}
			}

			bool go_on=true;
			unsigned int i=0;
			while(i < children.size())
			{
				if (children[i] == 0) children.erase(children.begin() + i);
				else i++;
			}
			
			/*vector<vg_control*>::iterator iter;
			iter=children.begin();
			while((iter != children.end()))
			{
				if(*iter == 0) children.erase(iter);
				else iter++;
			}	*/
		}
		else 
		{
			if(selected_id >= 0) 
			{
				delete_child(selected_id);
			}
		}
		multiselector.clear();
		selected_id = -1;
		return true;
	}
	else
	{
		vg_control *t = children[id];
		delete t;						
		children.erase(children.begin() + id);	
		return true;
	}
	return false;
}

void vg_window::align_selected(int mode)
{
	if(multiselector.size()>1)
	{
		std::set<int>::iterator iter;

		bool first=true;
		vg_rect bounds;
		int setw=0,seth=0;

		if((selected_id>=0)&&(selected_id<(int)children.size())) 
		{
			vg_rect cr = children[selected_id]->get_rect();	
			seth = cr.get_h();
			setw = cr.get_w();
			bounds = cr;
		}
		else
		{
			// RUN 1: determine max & min
			for (iter = multiselector.begin(); iter != multiselector.end(); iter++)
			{
				int i = *iter;
				assert((i>=0)&&(i<(int)children.size()));

				vg_rect cr = children[i]->get_rect();														
				if(first){
					first = false;
					bounds = cr;
				}
				else
				{
					bounds.x = min(bounds.x,cr.x);
					bounds.y = min(bounds.y,cr.y);
					bounds.x2 = max(bounds.x2,cr.x2);
					bounds.y2 = max(bounds.y2,cr.y2);
				}				
			}		
		}

		// RUN 2: perform alignment
		for (iter = multiselector.begin(); iter != multiselector.end(); iter++)
		{
			int i = *iter;
			assert((i>=0)&&(i<(int)children.size()));

			vg_rect cr = children[i]->get_rect();
			switch(mode)
			{
			case editor_alignV_top:
				cr.move(cr.x,bounds.y);
				break;
			case editor_alignV_mid:		
				cr.move(cr.x,bounds.y+((bounds.get_h()-cr.get_h())>>1));
				break;
			case editor_alignV_bottom:	
				cr.move(cr.x,bounds.y2-cr.get_h());
				break;
			case editor_alignH_left:
				cr.move(bounds.x,cr.y);
				break;
			case editor_alignH_center:				
				cr.move(bounds.x+((bounds.get_w()-cr.get_w())>>1),cr.y);
				break;
			case editor_alignH_right:
				cr.move(bounds.x2-cr.get_w(),cr.y);
				break;
			case editor_scaleV:
				{
					int t = (seth - cr.get_h()) >> 1;
					cr.y -= t;
					cr.y2 = cr.y + seth;					
					break;
				}
			case editor_scaleH:				
				{
					int t = (setw - cr.get_w()) >> 1;
					cr.x -= t;
					cr.x2 = cr.x + setw;					
					break;
				}
			};
			children[i]->set_rect(cr);
		}
	}
}

void vg_window::events_from_toolbox(actiondata ad)
{
	if(ad.actiontype == vga_click)
	{
		if((ad.id >= editor_tools_first)&&(ad.id<=editor_tools_last))
		{
			editmode_tool = ad.id - editor_tools_first;
			redraw_all = true;	
		}
		else
		{
			switch(ad.id)
			{
			case editor_delete:
				delete_child(-1);			
				redraw_all = true;						
				break;
			case editor_alignV_top:
			case editor_alignV_mid:
			case editor_alignV_bottom:	
			case editor_alignH_left:
			case editor_alignH_center:
			case editor_alignH_right:
			case editor_scaleV:
			case editor_scaleH:			
				align_selected(ad.id);	
				redraw_all = true;	
				break;				
			};
		}
	} else if (ad.actiontype == vga_text)
	{
		int val = atol((char*)ad.data.str);

		switch(ad.id)
		{		
		case editor_rectX:
		case editor_rectX2:
		case editor_rectY:
		case editor_rectY2:
			{					 
				for (std::set<int>::iterator iter = multiselector.begin(); iter != multiselector.end(); iter++)
				{
					int i = *iter; assert((i>=0)&&(i<(int)children.size()));
					vg_rect cr = children[i]->get_rect();					

					switch(ad.id)
					{
					case editor_rectX:
						cr.x = val;
						break;
					case editor_rectX2:
						cr.x2 = val;
						break;
					case editor_rectY:
						cr.y = val;
						break;
					case editor_rectY2:
						cr.y2 = val;
						break;
					};

					children[i]->set_rect(cr);
					redraw_all = true;	
				}
			}
			break;
		case editor_cdata0:
		case editor_cdata1:
		case editor_cdata2:
		case editor_cdata3:
		case editor_cdata4:
		case editor_cdata5:
		case editor_cdata6:
		case editor_cdata7:
			{
				int pid = ad.id - editor_cdata0;
				for (std::set<int>::iterator iter = multiselector.begin(); iter != multiselector.end(); iter++)
				{
					int i = *iter; assert((i>=0)&&(i<(int)children.size()));
					children[i]->set_parameter_text(pid,(char*)ad.data.str);					
				}
			redraw_all = true;
			break;
			}
		case editor_filter_type:
			{				
				for (std::set<int>::iterator iter = multiselector.begin(); iter != multiselector.end(); iter++)
				{
					int i = *iter; assert((i>=0)&&(i<(int)children.size()));
					children[i]->filter = val;
				}
				redraw_all = true;
				break;
			}
		case editor_filter_entry:
			{		
				val = bitmask_from_hexlist((char*)ad.data.str);
				for (std::set<int>::iterator iter = multiselector.begin(); iter != multiselector.end(); iter++)
				{
					int i = *iter; assert((i>=0)&&(i<(int)children.size()));
					children[i]->filter_entry = val;
				}
				redraw_all = true;
				break;
			}
		case editor_offset:
			{				
				for (std::set<int>::iterator iter = multiselector.begin(); iter != multiselector.end(); iter++)
				{
					int i = *iter; assert((i>=0)&&(i<(int)children.size()));
					children[i]->offset = val;
				}
				redraw_all = true;
				break;
			}
		case editor_offset_amount:
			{				
				for (std::set<int>::iterator iter = multiselector.begin(); iter != multiselector.end(); iter++)
				{
					int i = *iter; assert((i>=0)&&(i<(int)children.size()));
					children[i]->offset_amount = val;
				}
				redraw_all = true;
				break;
			}
		}
	} 
	else if (ad.actiontype == vga_intval)
	{
		switch(ad.id)
		{
		case editor_param_id:
			for (std::set<int>::iterator iter = multiselector.begin(); iter != multiselector.end(); iter++)
			{
				int i = *iter; assert((i>=0)&&(i<(int)children.size()));
				children[i]->parameter_id = ad.data.i[0];
			}
			redraw_all = true;
			set_selected_id(selected_id);
			break;			
		case editor_param_subid:
			for (std::set<int>::iterator iter = multiselector.begin(); iter != multiselector.end(); iter++)
			{
				int i = *iter; assert((i>=0)&&(i<(int)children.size()));
				children[i]->parameter_subid = ad.data.i[0];
			}
			redraw_all = true;
			set_selected_id(selected_id);
			break;			
		};
	}
	
	if (redraw_all_controls || redraw_all || (dirtykids.size() > 0)) 
	{
		RedrawWindow(hWnd,0,0,RDW_INVALIDATE);
	}	
}

void vg_window::post_action_from_program(actiondata ad)
{
	if(!ActionBuffer) return;
	ActionBuffer->WriteBlock(&ad);		// thread-safe buffer

	// do we really need threadsafeness at this time? YES!	
	// above all, it makes sure it doesn't end up in a TimeCritical thread

	if(initialized) PostMessage(hWnd,WM_APP+1212,0,0);	
}

void vg_window::build_children_by_pid()
{
	typedef pair <int, vg_control*> child_id_pair;

	children_by_pid.clear();
	for(unsigned int i=0; i<children.size(); i++)
	{
		children_by_pid.insert(child_id_pair(children[i]->parameter_id,children[i]));
		//children[i]		
	}
}

void vg_window::process_actions_from_program()
{				
	if (!initialized) return;
	// precalculate subid
	for(unsigned int i=0; i<children.size(); i++)
	{
		children[i]->parameter_subid_tempoverride = children[i]->parameter_subid;
		if(children[i]->offset) 
		{
			children[i]->parameter_subid_tempoverride += children[i]->offset_amount * filters[children[i]->offset];
		}
	}
	int last_pid = -1;
	multimap <int, vg_control*> ::const_iterator iter,lower,upper;

	//for(UINT j=0; j<actionbuffer_copy.size(); j++)
	actiondata *ad;
	while(ad = (actiondata*)ActionBuffer->ReadBlock())
	{				
		//int pid = actionbuffer_copy[j].id;		
		int pid = ad->id;
		if(pid == -1)
		{
			process_action_to_editor(*ad);
		}
		else 
		{
			if(pid != last_pid)	// only lookup if different than last time
			{
				lower = children_by_pid.lower_bound(pid);
				upper = children_by_pid.upper_bound(pid);
				last_pid = pid;
			}
			for (iter = lower; iter != upper; iter++)
			{
				if((iter->second->parameter_subid_tempoverride == ad->subid)||(ad->subid == -1))
				{
					iter->second->post_action(*ad);
				}
			}
		}
	}	
}

void vg_window::post_action_to_control(actiondata ad)
{		
	if((ad.id >= 0) && (ad.id < (int)children.size()))
	{
		children[ad.id]->post_action(ad);
	}
}
void vg_window::post_action(actiondata ad, vg_control *c)
{
	if(ad.actiontype == vga_menu)
	{
		menu->post_action(ad);
	} else if(ad.actiontype == vga_filter)
	{
		if(ad.data.i[0] < max_vgwindow_filters)
		{
			filters[ad.data.i[0]] = ad.data.i[1];
			if(ad.data.i[2] > 0)
			{
				// signal editor to refresh its state
				ad.actiontype = vga_request_refresh;
				post_action_to_program(ad);			
			}
			else redraw_all = true;
		}		
		
	}
	else if(ad.actiontype == vga_save_patch)
	{
		dialog_save_part();
	}
	else if(ad.actiontype == vga_save_multi)
	{
		dialog_save_multi();
	}
	else if(is_editor && owner)
	{
		if(c)
		{
			ad.id = c->parameter_id;
			ad.subid = c->parameter_subid;
			if(c->offset) ad.subid += c->offset_amount * filters[c->offset];
		}
		owner->events_from_toolbox(ad);
	}
	else 
	{
		if((ad.actiontype == vga_deletezone) && !dialog_confirm(L"Delete zones?",L"This operation will remove the selected zone(s) from the part.\n\nAre you sure?",0)) return;
		
		if(c)
		{
			ad.id = c->parameter_id;
			ad.subid = c->parameter_subid;
			if(c->offset) ad.subid += c->offset_amount * filters[c->offset];
		}
		post_action_to_program(ad);
	}
}

void vg_window::open_toolbox()
{
	if(is_editor) return;
	if(!editwin)
	{
		editwin = new vg_window();
		editwin->rootdir_default = rootdir_default;
		editwin->rootdir_skin = rootdir_skin;
		editwin->create("..\\editor.xml",true,this);				
		SetFocus(hWnd);
	}
}

void vg_window::close_toolbox()
{
	if(is_editor) return;
	if(editwin) 
	{
		editwin->destroy();
		delete editwin;
		editwin = 0;
	}
}

int vg_window::get_syscolor(int id)
{
	return colorpalette[id&0xff];
}

bool vg_window::dialog_confirm(std::wstring label, std::wstring text, int mode)
{	
	UINT flags = 0;
	switch(mode)
	{
	default:
	case 0: // confirm
		flags = MB_ICONQUESTION | MB_OKCANCEL;
		break;
	case 1: // inform
		flags = MB_ICONINFORMATION | MB_OK;
		break;
	case 2: // error
		flags = MB_ICONERROR | MB_OK;
		break;
	case 3: // yesno
		flags = MB_ICONERROR | MB_YESNO;
		break;
	}
	int result = MessageBoxW(hWnd,text.c_str(),label.c_str(),flags);
	return (result == IDOK)||(result == IDYES);
}

void vg_window::dialog_save_part()
{	
	savepart_fname = dialog_save(L"Save patch",L"",1);
	if(savepart_fname.empty()) return;
	
	actiondata ad;
	memset(&ad,0,sizeof(actiondata));
	ad.actiontype = vga_save_patch;	
	/*ad.data.i[0] = dialog_confirm(L"Copy samples with patch?",L"Do you wish to make a copy of the samples to the location of the patch?\n\nChoosing 'No' will make the patch reference the contained files at their original location.",3);
	if (ad.data.i[0]) 
		ad.data.i[1] = dialog_confirm(L"Update references?",L"Do you wish to update the in-memory references to the newly created copies?",3);
	else ad.data.i[1] = 0;*/
	post_action_to_program(ad);
}

void vg_window::dialog_save_multi()
{	
	savepart_fname = dialog_save(L"Save multi",L"",1);
	if(savepart_fname.empty()) return;
	
	actiondata ad;
	memset(&ad,0,sizeof(actiondata));
	ad.actiontype = vga_save_multi;	
	post_action_to_program(ad);
}

std::wstring vg_window::dialog_save(std::wstring label, std::wstring startdir, int type)
{
	OPENFILENAMEW ofn;       // common dialog box structure
	WCHAR szFile[512];       // buffer for file name
	szFile[0] = 0;		

	// Initialize OPENFILENAME
	ZeroMemory(szFile, sizeof(szFile));
	ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
	ofn.lStructSize = sizeof(OPENFILENAMEW); 			
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);				
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = startdir.c_str();
	
	if(type == 1)
	{
		ofn.lpstrFilter = L"shortcircuit2 patch (*.sc2p)\0*.sc2p\0";	
		ofn.lpstrDefExt = L"sc2p";
		ofn.lpstrTitle = L"Save patch";
	}
	else
	{
		ofn.lpstrFilter = L"shortcircuit2 multi (*.sc2m)\0*.sc2m\0";
		ofn.lpstrDefExt = L"sc2m";
		ofn.lpstrTitle = L"Save multi";
	}

	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

	if (GetSaveFileNameW(&ofn))		
	{		
		return ofn.lpstrFile;
	}	

	return L"";
}