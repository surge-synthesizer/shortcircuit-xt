//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "synthesis/filter.h"
//#include <process.h>
#include "multiselect.h"
#include <aeffeditor.h>
#include <vt_gui/browserdata.h>
#include <vt_gui/vt_gui.h>

class sampler;

class sc_editor2 : public vg_window, public AEffEditor
{
  public:
    sc_editor2(AudioEffect *effect);
    sc_editor2(sampler *sobj);
    virtual ~sc_editor2();
    virtual bool getRect(ERect **erect);

    virtual bool onKeyDown(VstKeyCode &keyCode);
    virtual bool onKeyUp(VstKeyCode &keyCode);
    virtual bool take_focus();

    // midi learn methods
    // void sendMidiMsg(char type, char data1, char data2);
    // void track_zone_triggered(int,bool);

    // static UINT_PTR CALLBACK OFN_sample_replace( HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM
    // lParam ); void wait_for_fsys_changes();
    browserdata browserd;
    std::wstring inject_file;

    virtual bool open(void *ptr);
    virtual void close();
    virtual bool isOpen() { return initialized; }

  protected:
    void *hThread;

    virtual void setParameter(long index, float value);
    virtual void post_action_to_program(actiondata ad);
    virtual void process_action_to_editor(actiondata ad);
    // virtual void post_action_from_program(actiondata ad);
    virtual bool processevent_user(vg_controlevent &e);

    virtual int param_get_n();
    virtual std::string param_get_label_from_id(int id);
    virtual int param_get_id_from_label(std::string s);
    virtual int param_get_n_subparams(int id);

    std::string get_dir(int id);
    bool refresh_db();

    sampler *sobj;
    multiselect *sel;
    ERect editorrect;
};