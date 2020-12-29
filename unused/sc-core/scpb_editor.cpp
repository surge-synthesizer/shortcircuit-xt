//-------------------------------------------------------------------------------------------------------
//	Copyright 2005 Claes Johanson & vember|audio
//-------------------------------------------------------------------------------------------------------

#error THIS FILE IS UNUSED AND SHOULD NOT COMPILE

#if 0

#include "scpb_editor.h"
#include "../common/vstcontrols.h"
#include "Ckeygroupview.h"
#include "cparamedit.h"
#include "gui_display.h"
#include "gui_patchpicker.h"
#include "gui_slider.h"
#include "resource.h"
#include "scpb_sampler.h"
#include "scpb_vsti.h"
#include <assert.h>
#include <wx/dnd.h>

//#include <commctrl.h>

CBitmap* bmp_bg=0;
CBitmap* hfader_bg_black=0;
CBitmap* hfader_bg_black_bp=0;
CBitmap* hfader_handle_black=0;
CBitmap* vfader_bg_white=0;
CBitmap* vfader_bg_white_bp=0;
CBitmap* vfader_handle_black=0;

const int window_size_x = 649, window_size_y = 446;
const int leftpane = 157;
const int rightpane = 492;
const int yofs = 10;

const int picker_offset = 1<<8;
enum
{	
	tag_patch_picker = picker_offset,
	tag_poly,
	tag_transpose,
	tag_fxbypass,
};

scpb_editor::scpb_editor(AudioEffect *effect) : AEffGUIEditor(effect)
{
	frame = 0;
	
	// init the size of the plugin
	rect.left   = 0;
	rect.top    = 0;
	rect.right  = window_size_x;
	rect.bottom = window_size_y;	
	editor_open = false;
	
	//ToolTipWnd = 0;

	for(int i=0; i<kNumparams; i++) sliders[i]=0;
	kgv = 0;
	poly = 0;
	pp = 0;
	disp_title = 0;
	disp_other = 0;
	is_loading = false;
}

scpb_editor::~scpb_editor()
{
	
}

void scpb_editor::idle()
{	
	assert(effect);
	if(!((scpb_vsti*)effect)->supports_fxIdle)
	{
		sobj->idle();
	}

	if (frame)
	{
		if (triggered_since_last_idle && kgv) 
		{
			((CKeyGroupView*)kgv)->refreshKBD();
			triggered_since_last_idle = false;
		}

		if (sobj)
		{
			((CParamEdit*)poly)->setPoly(sobj->polyphony);
		}						
	}
	
	/*if(frame && effect && !frame->getEditView())
	{
		for(int i=0; i<kNumparams; i++)
		{
			if (sliders[i])
			{				
				((gui_slider*)sliders[i])->setDisplayValue(((scpb_vsti*)effect)->getParameterLag(i));				
			}
		}
	}*/
	AEffGUIEditor::idle();	
}

void scpb_editor::open_editor()
{	
	assert(frame);
	if (editor_open) close_editor();	

	float moverate = 1.0f;
	for(int i=0; i<kNumparams; i++)
	{
		//int xb = (i/4);
//		if(xb == 2) xb-=2;
//		else if(xb<2) xb++;
//		int subpos = i&3;
		int xpos = 0;
		int ypos = 0;
		int style = kHorizontal;

		switch(i)
		{
		case kAEG_A:
		case kAEG_D:
		case kAEG_S:
		case kAEG_R:
		case kAmplitude:
			ypos += 229;
			xpos += 4 + (i-kAEG_A) * 25;
			if(i==kAmplitude) xpos += 11;
			style = kVertical;
			moverate = 0.5f;
			break;
		/*case kAmplitude:
			ypos += 364;
			break;*/
		default:
			ypos += i*25;			
			break;
		};
		if (((scpb_vsti*)effect)->isParameterBipolar(i)) style |= kBipolar;
		gui_slider *slider = new gui_slider(CPoint(rightpane+10+xpos,22+ypos),style,this,i,false);		
		slider->setMin(0.f);
		slider->setMax(1.f);
		slider->setMoveRate(moverate);

		frame->addView(slider);
		sliders[i] = slider;
	}

	CRect patchdisp(0,72,rightpane,341);

	pp = new gui_patchpicker(patchdisp,this,tag_patch_picker,&sobj->patchlist,&sobj->categorylist);
	frame->addView(pp);

	CRect kgvrect(0,0,rightpane,104);
	kgvrect.offset(0,342);
	kgv = new CKeyGroupView(kgvrect,0,0,sobj,1);
	frame->addView(kgv);

	CRect peditrect(565,380,625,394);
	/*CColor segerblue = {93,143,226,0};
	CColor segerdarkblue = {75,105,157,0};*/
	//CColor segerblue = {215,107,107,0};
	CColor segerblue = {255,255,255,0};
	CColor segerdarkblue = {75,105,157,0};	

	CRect disparea(0,0,495,71);
	CRect dtr(disparea);
	dtr.bottom = 46;
	dtr.right -= 100;
	dtr.inset(6,6);
	disp_title = new gui_display(dtr);
	frame->addView(disp_title);
	dtr.top = dtr.bottom+1;
	dtr.bottom = disparea.bottom;
	disp_other = new gui_display(dtr);
	frame->addView(disp_other);
	//disp_other = 0


	const int poff = 18;
	peditrect.offset(0,3);

	CParamEdit *ply = new CParamEdit(peditrect,this,tag_poly);	
	poly = ply;
	ply->setLabel("poly");
	ply->setLabelPlacement(lp_left);
	ply->setControlMode(cm_polyphony);
	ply->setPoly(sobj->polyphony);
	ply->setBackColor(segerblue);	
	ply->setFontColor(kBlackCColor);
	ply->setBgcolor(segerblue);
	ply->setLineColor(segerblue);
	ply->setIntValue(sobj->polyphony_cap);
	frame->addView(poly);

	peditrect.offset(0,poff);
	CParamEdit *transpose = new CParamEdit(peditrect,this,tag_transpose);		
	transpose->setLabel("transpose");
	transpose->setLabelPlacement(lp_left);
	transpose->setControlMode(cm_integer);	
	transpose->setIntMin(-36);
	transpose->setIntMax(36);
	transpose->setIntStepSize(12);
	transpose->setBackColor(segerblue);		
	transpose->setFontColor(kBlackCColor);
	transpose->setBgcolor(segerblue);
	transpose->setLineColor(segerblue);
	transpose->setIntValue(sobj->scpbtranspose);
	frame->addView(transpose);

	peditrect.offset(0,poff);
	CParamEdit *fbxypass = new CParamEdit(peditrect,this,tag_fxbypass);		
	fbxypass->setLabel("FX bypass");
	fbxypass->setLabelPlacement(lp_left);
	fbxypass->setControlMode(cm_noyes);		
	fbxypass->setBackColor(segerblue);	
	fbxypass->setFontColor(kBlackCColor);
	fbxypass->setBgcolor(segerblue);
	fbxypass->setLineColor(segerblue);
	fbxypass->setIntValue(sobj->fxbypass);
	frame->addView(fbxypass);

	patchChanged();	

	editor_open = true;
	queue_refresh = false;		
	frame->setDirty();
}

void scpb_editor::track_zone_triggered(int zone)
{
	triggered_since_last_idle = true;
}

void scpb_editor::isLoading(bool b)
{
	is_loading = b;
	/*if(pp)
	{
		pp->isLoading(b);
	}*/
	/*if(disp_title && b)
	{
		disp_title->setLabel("Loading...",24,-1);
		disp_title->setDirty();
	}*/
}

void scpb_editor::patchChanged()
{
	if(!frame) return;
	if(!sobj) return;

	isLoading(false);

	if(pp)
	{
		pp->setCategory(sobj->currentcategory);
		pp->setPatch(sobj->currentpatch);
	}
	if(kgv)
	{
		((CKeyGroupView*)kgv)->refreshALL();
	}
	if(disp_title && sobj->getPatchTitle())
	{
		disp_title->setLabel(sobj->getPatchTitle(),24,-1);
	}
	if(disp_other)
	{
		string somedata;
		if((sobj->currentpatch>=0)&&(sobj->currentpatch < sobj->patchlist.size())) 
		{
			//somedata += "path:";
			somedata += sobj->patchlist.at(sobj->currentpatch).path;
		}
		disp_other->setLabel(somedata,10,-1);
	}
	for(int i=0; i<kNumparams; i++)
	{
		if(sliders[i])
		{
			char txt[256];
			effect->getParameterName(i,txt);			
			bool bipolar = ((scpb_vsti*)effect)->isParameterBipolar(i);
			((gui_slider*)sliders[i])->setBipolar(bipolar);
			
			if(bipolar) ((gui_slider*)sliders[i])->setDefaultValue(0.5f);
			else ((gui_slider*)sliders[i])->setDefaultValue(0.5f);
			((gui_slider*)sliders[kAmplitude])->setDefaultValue(1.0f);

			((gui_slider*)sliders[i])->setValue(((scpb_vsti*)effect)->getParameter(i));			
			((gui_slider*)sliders[i])->setLabel(txt);
		}
	}	
}

void scpb_editor::close_editor()
{
	frame->removeAll(true);
	editor_open = false;	
}

long scpb_editor::open(void *ptr)
{
	// !!! always call this !!!
	AEffGUIEditor::open (ptr);
	
	if (bmp_bg) bmp_bg->remember();
	else bmp_bg = new CBitmap(IDB_BG);	
	if (hfader_bg_black) hfader_bg_black->remember();
	else hfader_bg_black = new CBitmap(IDB_FADERH_BG_BLACK);
	if (hfader_bg_black_bp) hfader_bg_black_bp->remember();
	else hfader_bg_black_bp = new CBitmap(IDB_FADERH_BG_BLACK_BP);	
	if (hfader_handle_black) hfader_handle_black->remember();
	else hfader_handle_black = new CBitmap(IDB_FADERH_HANDLE_BLACK);
	if (vfader_bg_white) vfader_bg_white->remember();
	else vfader_bg_white = new CBitmap(IDB_FADERV_BG);
	if (vfader_bg_white_bp) vfader_bg_white_bp->remember();
	else vfader_bg_white_bp = new CBitmap(IDB_FADERV_BG_BP);
	if (vfader_handle_black) vfader_handle_black->remember();
	else vfader_handle_black = new CBitmap(IDB_FADERV_HANDLE_BLACK);

	CRect wsize (0, 0, window_size_x, window_size_y);
	frame = new CFrame(wsize, ptr, this);
	frame->setBackground(bmp_bg);
	assert(frame);	
	scpb_vsti *plug = (scpb_vsti*)effect;
	if(!plug->initialized) plug->init();
	// TODO add plug here 
	sobj = (scpb_sampler*)plug->sobj;				
	
	open_editor();

	return true;
}

void scpb_editor::close()
{
	if(frame){	
		close_editor();
		delete frame;
		frame = 0;
	}
	
	if (bmp_bg->forget()) bmp_bg = 0;
	if (hfader_bg_black->forget()) hfader_bg_black  = 0;
	if (hfader_bg_black_bp->forget()) hfader_bg_black_bp  = 0;
	if (hfader_handle_black->forget()) hfader_handle_black  = 0;
	if (vfader_bg_white->forget()) vfader_bg_white  = 0;
	if (vfader_bg_white_bp->forget()) vfader_bg_white_bp  = 0;
	if (vfader_handle_black->forget()) vfader_handle_black  = 0;
}
void scpb_editor::setParameter (long index, float value)
{
	if (!frame) return;
	if (!(index<kNumparams)) return;
	if (index<0) return;
	if(sliders[index])
	{
		sliders[index]->setValue(value);	
		sliders[index]->setDirty();
	}
}

long scpb_editor::controlModifierClicked (VSTGUI::CDrawContext *pContext, VSTGUI::CControl *control, long button)
{ 
	if (!frame) return 0;	
	if (!sobj) return 0;		
	long tag = control->getTag();
	if(tag < picker_offset)
	{
		if (button == -1) button = pContext->getMouseButtons ();
		if (!(button & (kRButton)))
			return 0;
		
		CRect menuRect;			
		CPoint where; pContext->getMouseLocation(where);
		CPoint fwhere = where;
		frame->localToFrame(fwhere);
		menuRect.offset (fwhere.h, fwhere.v);
		COptionMenu *contextMenu = new COptionMenu(menuRect, 0, 0, 0, 0, kNoDrawStyle);			
		contextMenu->addEntry("Assign MIDI controller (learn)");
		getFrame()->addView (contextMenu); // add to frame
		contextMenu->setDirty ();
		contextMenu->mouse (pContext, where); // <-- modal menu loop is here				

		long sel_id = contextMenu->getLastResult();		

		if(sel_id == 0) sobj->learn_tag = tag;

		getFrame()->removeView (contextMenu, true); // remove from frame and forget		
		return 1;
	}

	return 0; 
}

void scpb_editor::do_dnd()
{
	/*IDataObject idsdf;
	DoDragDrop(idsdf);	*/
}

void scpb_editor::valueChanged (CDrawContext* context, CControl* control)
{	
	assert(sobj);
	assert(frame);
	if (!frame) return;		
	if (!sobj) return;		
	long tag = control->getTag();	

	if(tag >= picker_offset)
	{
		switch(tag)
		{
		case tag_transpose:
			sobj->scpbtranspose = limit_range(((CParamEdit*)control)->getIntValue(),-36,36);
			sobj->all_notes_off();
			break;
		case tag_fxbypass:
			sobj->fxbypass = limit_range(((CParamEdit*)control)->getIntValue(),0,1);
			break;
		case tag_poly:			
			sobj->polyphony_cap = limit_range(((CParamEdit*)control)->getIntValue(),1,256);
			break;
		case tag_patch_picker:
			{
				if(((gui_patchpicker*)control)->queue_id>=0)
				{
					sobj->changepatch(((gui_patchpicker*)control)->queue_id);
					((gui_patchpicker*)control)->queue_id = -1;
				} 
				//else sobj->browsepatches(1,(control->getValue()>0.f)?1:-1);
			}			
			break;
		};
	}
	else
	{
		effect->setParameterAutomated(tag,control->getValue());
	}

	//control->setDirty();	
}

#endif