//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#error THIS FILE IS UNUSED AND SHOULD NOT COMPILE

#if 0

#if WINDOWS

// #include "stdafx.h"
#include "configuration_dialog.h"
#include "globals.h"
#include "miniedit_dialog.h"
#include "resource2.h"
#include <atldlgs.h>


class CConfigDialog : public CDialogImpl<CConfigDialog>
{
public:
	configuration *conf;
	enum { IDD = IDD_CONFIG_DIALOG };

	BEGIN_MSG_MAP(CConfigDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER_EX(IDOK,BN_CLICKED,OnOK)
		COMMAND_HANDLER_EX(IDCANCEL,BN_CLICKED,OnCancel)				
		NOTIFY_HANDLER(IDC_CLIST, NM_CLICK, OnNMClickClist)
		NOTIFY_HANDLER(IDC_CLIST, NM_DBLCLK, OnNMClickClist)		
		NOTIFY_HANDLER(IDC_DIRLIST, NM_CLICK, OnNMClickDlist)
		NOTIFY_HANDLER(IDC_DIRLIST, NM_DBLCLK, OnNMClickDlist)		
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, 
	LPARAM lParam, BOOL& bHandled)
	{			
		// Do some initialization code		
		CListViewCtrl dlist;	
		dlist.Attach(GetDlgItem(IDC_DIRLIST));		
		dlist.AddColumn("path",0);
		dlist.AddColumn("alias",1);
		dlist.AddColumn("",2);
		dlist.SetColumnWidth(0,250);
		dlist.SetColumnWidth(1,100);
		dlist.SetColumnWidth(2,20);
		int dl_ord[3] = {1,0,2};
		dlist.SetColumnOrderArray(3,dl_ord);

		dlist.AddItem(0,0,conf->homepath);
		dlist.SetItemText(0,1,"home");
		dlist.SetItemText(0,2,"..");

		int i;		
		for(i=0; i<5; i++)
		{			
			char title[26];
			sprintf(title,"path%i",i+1);
			dlist.AddItem(i+1,0,conf->path[i]);
			dlist.SetItemText(i+1,1,title);
			dlist.SetItemText(i+1,2,"..");
		}
				
		dlist.AddItem(6,0,conf->editor);
		dlist.SetItemText(6,1,"external editor");
		dlist.SetItemText(6,2,"..");

		dlist.Detach();		

		CListViewCtrl clist;
		clist.Attach(GetDlgItem(IDC_CLIST));		
		clist.AddColumn("name",0);		
		clist.AddColumn("type",1);
		clist.AddColumn("number",2);		
		clist.AddColumn("id",3);
		int cl_ord[4] = {3,1,2,0};
		clist.SetColumnOrderArray(4,cl_ord);
		clist.SetColumnWidth(0,200);
		clist.SetColumnWidth(1,45);
		clist.SetColumnWidth(2,55);
		clist.SetColumnWidth(3,35);	
		
		for(i=0; i<num_custom_controllers; i++)
		{			
			char nid[16],num[16];
			sprintf(nid,"c%i",i+1);
			sprintf(num,"%i",conf->MIDIcontrol[i].number);
			clist.AddItem(i,0,conf->MIDIcontrol[i].name);					
			clist.SetItemText(i,1,ct_titles[conf->MIDIcontrol[i].type]);
			clist.SetItemText(i,2,num);
			clist.SetItemText(i,3,nid);			
		}	

		clist.Detach();	

		WTL::CComboBox combo;		
		combo.Attach(GetDlgItem(IDC_MONOOUT));		
		int zero = combo.AddString("0"); 
		combo.AddString("2");
		combo.AddString("4");
		combo.AddString("6");
		combo.AddString("8");
		combo.AddString("10");
		combo.AddString("12");
		combo.AddString("14");
		combo.AddString("16");
		combo.SetCurSel((conf->mono_outputs>>1)+zero);		
		combo.Detach();
		
		combo.Attach(GetDlgItem(IDC_STEREOOUT));
		int one = combo.AddString("1");
		combo.AddString("2");
		combo.AddString("3");
		combo.AddString("4");
		combo.AddString("5");
		combo.AddString("6");
		combo.AddString("7");
		combo.AddString("8");
		combo.SetCurSel(conf->stereo_outputs-1+one);
		combo.Detach();		
		
		combo.Attach(GetDlgItem(IDC_HEADROOM));
		combo.AddString("0 dB");
		combo.AddString("6 dB");
		combo.AddString("12 dB");
		combo.AddString("18 dB");
		combo.AddString("24 dB");
		combo.SetCurSel(conf->headroom/6);
		combo.Detach();

		WTL::CButton check;		
		check.Attach(GetDlgItem(IDC_CHECK1));
		check.SetCheck((conf->flags & 1)?BST_CHECKED:BST_UNCHECKED);
		check.Detach();
		check.Attach(GetDlgItem(IDC_CHECK2));
		check.SetCheck((conf->flags & 2)?BST_CHECKED:BST_UNCHECKED);
		check.Detach();
		check.Attach(GetDlgItem(IDC_CHECK4));
		check.SetCheck((conf->flags & 4)?BST_CHECKED:BST_UNCHECKED);
		check.Detach();
		check.Attach(GetDlgItem(IDC_CHECK8));
		check.SetCheck((conf->flags & 8)?BST_CHECKED:BST_UNCHECKED);
		check.Detach();
		
		/*WTL::CEdit edit;
		edit.Attach(GetDlgItem(IDC_OVERRIDE));						
		char textdata[256];
		sprintf(textdata,"%i",conf->flags);
		edit.AppendText(textdata);		
		edit.Detach();		*/
		
		return 1;
	}

	void CConfigDialog::OnOK ( UINT uCode, int nID, HWND hWndCtl )
	{		
		WTL::CComboBox combo;		
		combo.Attach(GetDlgItem(IDC_MONOOUT));				
		conf->mono_outputs = combo.GetCurSel()*2;
		combo.Detach();

		combo.Attach(GetDlgItem(IDC_STEREOOUT));				
		conf->stereo_outputs = combo.GetCurSel()+1;
		combo.Detach();

		combo.Attach(GetDlgItem(IDC_HEADROOM));				
		conf->headroom = combo.GetCurSel()*6;
		combo.Detach();

/*		WTL::CEdit edit;
		edit.Attach(GetDlgItem(IDC_OVERRIDE));				
		char textdata[256];
		edit.GetLine(0,textdata,256);
		conf->flags = atoi(textdata);
		edit.Detach();*/

		int flags = 0;
		WTL::CButton check;		
		check.Attach(GetDlgItem(IDC_CHECK1));
		flags |= (check.GetCheck() == BST_CHECKED)?1:0;
		check.Detach();
		check.Attach(GetDlgItem(IDC_CHECK2));
		flags |= (check.GetCheck() == BST_CHECKED)?2:0;
		check.Detach();
		check.Attach(GetDlgItem(IDC_CHECK4));
		flags |= (check.GetCheck() == BST_CHECKED)?4:0;
		check.Detach();
		check.Attach(GetDlgItem(IDC_CHECK8));
		flags |= (check.GetCheck() == BST_CHECKED)?8:0;
		check.Detach();
		conf->flags = flags;

		// add: save conf to make changes permanent
		conf->save(0);
		EndDialog(nID);
	}

	void CConfigDialog::OnCancel ( UINT uCode, int nID, HWND hWndCtl )
	{		
		// add: reload conf to get original settings
		conf->load(0);
		EndDialog(nID);
	}
	LRESULT OnNMClickClist(int /*idCtrl*/, LPNMHDR pData, BOOL& /*bHandled*/);
	LRESULT OnNMClickDlist(int /*idCtrl*/, LPNMHDR pData, BOOL& /*bHandled*/);
};

int spawn_config_dialog(HWND hwnd, configuration *conf)
{	
	CConfigDialog dlgConfig;	
	dlgConfig.conf = conf;
	dlgConfig.DoModal(hwnd, NULL);
	return 0; 
}

LRESULT CConfigDialog::OnNMClickClist(int idCtrl, LPNMHDR nData, BOOL& bHandled)
{
	// TODO: Add your control notification handler code here
	LPNMITEMACTIVATE pData = (LPNMITEMACTIVATE)nData;
	CListViewCtrl clist;
	clist.Attach(GetDlgItem(IDC_CLIST));	
	LVHITTESTINFO hi;		
	hi.pt.x = pData->ptAction.x;	
	hi.pt.y = pData->ptAction.y;	
	clist.SubItemHitTest(&hi);
	if((hi.flags | LVHT_ONITEM)&&(hi.iItem >= 0)&&(hi.iItem < num_custom_controllers))
	{
		switch(hi.iSubItem)
		{
		case 0:	// name
			{				
				CMiniEditDialog me;	
				me.SetText(conf->MIDIcontrol[hi.iItem].name);
				me.DoModal(::GetActiveWindow(), NULL);
				if(me.updated)				
				{
					strncpy(conf->MIDIcontrol[hi.iItem].name,me.textdata,16);
					clist.SetItemText(hi.iItem,hi.iSubItem,conf->MIDIcontrol[hi.iItem].name);
				}
				break;
			}
		case 1:	// type
			conf->MIDIcontrol[hi.iItem].type = (midi_controller_type)((conf->MIDIcontrol[hi.iItem].type + 1) % n_ctypes);
			clist.SetItemText(hi.iItem,hi.iSubItem,ct_titles[conf->MIDIcontrol[hi.iItem].type]);
			if (conf->MIDIcontrol[hi.iItem].type == mct_cc)
			{
				conf->MIDIcontrol[hi.iItem].number = min(conf->MIDIcontrol[hi.iItem].number,127);
				char num[16];
				sprintf(num,"%i",conf->MIDIcontrol[hi.iItem].number);					
				clist.SetItemText(hi.iItem,2,num);
			}
			break;
		case 2:	// number	
			{				
				CMiniEditDialog me;	
				me.SetValue(conf->MIDIcontrol[hi.iItem].number);
				if (conf->MIDIcontrol[hi.iItem].type == mct_cc)
					me.irange = 128;
				else
					me.irange = 32768;

				me.DoModal(::GetActiveWindow(), NULL);
				if(me.updated)				
				{
					conf->MIDIcontrol[hi.iItem].number = me.ivalue;
					char num[16];
					sprintf(num,"%i",conf->MIDIcontrol[hi.iItem].number);					
					clist.SetItemText(hi.iItem,hi.iSubItem,num);
				}
				break;
			}
			break;
		};
		//clist.SetItemText(hi.iItem,hi.iSubItem,"foo");
	}	
	clist.SelectItem(-1);
	clist.Detach();	
	bHandled = true;
	return 0;
}

LRESULT CConfigDialog::OnNMClickDlist(int idCtrl, LPNMHDR nData, BOOL& bHandled)
{
	// TODO: Add your control notification handler code here
	LPNMITEMACTIVATE pData = (LPNMITEMACTIVATE)nData;
	CListViewCtrl clist;
	clist.Attach(GetDlgItem(IDC_DIRLIST));	
	LVHITTESTINFO hi;		
	hi.pt.x = pData->ptAction.x;	
	hi.pt.y = pData->ptAction.y;		
	clist.SubItemHitTest(&hi);
	if((hi.flags | LVHT_ONITEM)&&(hi.iItem >= 0)&&(hi.iItem < 7))
	{
		int a = hi.iItem;
		char *s;
		if(a==0) s = conf->homepath;
		else if (a==6) s = conf->editor;
		else s = conf->path[a-1];
		

		switch(hi.iSubItem)
		{
		case 0:
			{
				CMiniEditDialog me;	
				me.SetText(s);
				me.DoModal(::GetActiveWindow(), NULL);
				if(me.updated)				
				{
					strncpy(s,me.textdata,256);
					clist.SetItemText(hi.iItem,0,s);
				}
				break;
			}
		case 2:
			{
				// l�gg in switch f�r om det �r external editor
				if (a==6)
				{
					WTL::CFileDialog fdiag(true,0,0,0,"Windows Executable (*.exe)\0*.exe\0",::GetActiveWindow());					
					fdiag.DoModal();
					char fname[1024];					
					if (fdiag.m_szFileName[0])
					{
						strncpy(s,fdiag.m_szFileName,256);
						strlwr(s);
						clist.SetItemText(hi.iItem,0,s);
					}
				}
				else
				{
					WTL::CFolderDialog dirdiag(::GetActiveWindow());				
					dirdiag.DoModal();
					char *path = (char*)dirdiag.GetFolderPath();
					if (path[0])
					{
						strncpy(s,path,256);
						strlwr(s);
						clist.SetItemText(hi.iItem,0,s);
					}	
				}
			}
		};	
	}	
	clist.SelectItem(-1);
	clist.Detach();	
	bHandled = true;
	return 0;
}
#endif
#endif
