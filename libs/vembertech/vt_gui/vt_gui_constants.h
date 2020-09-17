#pragma once

enum 
{
	vget_mousedown=0,
	vget_mouseup,
	vget_mousemove,		
	vget_mouseenter,
	vget_mouseleave,
	vget_mousedblclick,
	vget_mousewheel,
	vget_character,
	vget_keypress,
	vget_keyrepeat,
	vget_keyrelease,	
	vget_hotkey,
	vget_idle,
	vget_process_actionbuffer,
	vget_dragfiles,
	vget_dropfiles,
	vget_deactivate,
	
	vgm_LMB=0x1,
	vgm_RMB=0x2,
	vgm_shift=0x4,
	vgm_control=0x8,
	vgm_MMB=0x10,
	vgm_MB4=0x20,
	vgm_MB5=0x40,
	vgm_alt=0x80,

	cursor_arrow=0,
	cursor_hand_point,
	cursor_hand_pan,
	cursor_wait,
	cursor_forbidden,
	cursor_move,
	cursor_text,
	cursor_sizeNS,
	cursor_sizeWE,
	cursor_sizeNESW,
	cursor_sizeNWSE,	
	cursor_copy,
	cursor_zoomin,
	cursor_zoomout,
	cursor_vmove,
	cursor_hmove,

	vg_align_top=-1,
	vg_align_left=-1,
	vg_align_center=0,
	vg_align_bottom=1,
	vg_align_right=1,

	vgvt_int=0,
	vgvt_bool,
	vgvt_float,	

	vga_nothing=0,
	vga_floatval,
	vga_intval,
	vga_intval_inc,
	vga_intval_dec,
	vga_boolval,
	vga_beginedit,
	vga_endedit,
	vga_load_dropfiles,
	vga_load_patch,
	vga_text,
	vga_click,
	vga_exec_external,
	vga_url_external,
	vga_doc_external,
	vga_disable_state,
	vga_stuck_state,	
	vga_hide,
	vga_label,
	vga_temposync,
	vga_filter,	
	vga_datamode,
	vga_menu,
	vga_entry_add,
	vga_entry_add_ival_from_self,
	vga_entry_add_ival_from_self_with_id,
	vga_entry_replace_label_on_id,
	vga_entry_setactive,
	vga_entry_clearall,		
	vga_select_zone_clear,
	vga_select_zone_primary,
	vga_select_zone_secondary,
	vga_select_zone_previous,
	vga_select_zone_next,
	vga_zone_playtrigger,	
	vga_deletezone,
	vga_createemptyzone,
	vga_clonezone,
	vga_clonezone_next,
	vga_movezonetopart,
	vga_movezonetolayer,
	vga_zonelist_clear,
	vga_zonelist_populate,
	vga_zonelist_done,	
	vga_zonelist_mode,
	vga_toggle_zoom,
	vga_note,
	vga_audition_zone,
	vga_request_refresh,
	vga_set_zone_keyspan,
	vga_set_zone_keyspan_clone,
	vga_openeditor,
	vga_closeeditor,
	vga_wavedisp_sample,	
	vga_wavedisp_multiselect,
	vga_wavedisp_plot,	
	vga_wavedisp_editpoint,
	vga_steplfo_repeat,
	vga_steplfo_shape,
	vga_steplfo_data,
	vga_steplfo_data_single,
	vga_browser_listptr,
	vga_browser_entry_next,
	vga_browser_entry_prev,
	vga_browser_entry_load,
	vga_browser_category_next,
	vga_browser_category_prev,
	vga_browser_category_parent,
	vga_browser_category_child,
	vga_browser_preview_start,
	vga_browser_preview_stop,
	vga_browser_is_refreshing,
	vga_inject_database,
	vga_database_samplelist,
	vga_save_patch,
	vga_save_multi,
	vga_vudata,

	// color palette entries
	col_standard_bg=0,
	col_standard_text,
	col_standard_bg_disabled,
	col_standard_text_disabled,
	col_selected_bg,
	col_selected_text,
	col_alt_bg,
	col_alt_text,
	col_frame,
	col_tintarea,	
	col_nowavebg,	
	col_slidertray1,
	col_slidertray2,
	col_misclabels,
	col_LFOgrid,
	col_LFOBG1,
	col_LFOBG2,
	col_LFObars,
	col_LFOcurve,
	col_list_bg,
	col_list_text,
	col_list_text_unassigned,
	col_list_text_keybed,
	col_vu_back,
	col_vu_front,
	col_vu_overload,
	kbm_autodetect=0,
	kbm_os,
	kbm_vst,
	kbm_globalhook,
	kbm_seperate_window,
};

union vg_pdata
{	
	int i;
	bool b;
	float f;
	void* ptr;
};

const int actiondata_maxstring = 13*4-1;
struct actiondata
{
	int actiontype;
	int id,subid;

	union
	{
		int i[13];		
		float f[13];
		char str[52];
		void* ptr[6];	// assume the pointer is 64-bit and always put the pointers in the first slots of the message
	} data;
};	// 64 bytes of whatever you like.. should be plenty

struct ad_zonedata
{
	int actiontype;
	int id,subid;
	
	int zid;
	
	char part,layer;
	char keylo,keyhi,keyroot,vello,velhi;
	char keylofade,keyhifade,vellofade,velhifade;
	char is_active,is_selected,mute;	
	char name[34];
};
