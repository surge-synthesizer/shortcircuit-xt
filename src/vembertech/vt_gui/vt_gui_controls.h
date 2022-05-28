#include "vt_gui.h"
#include "globals.h"

// if you change order here, update the art.xml file to match
enum vg_control_types
{
    vgct_none = 0,
    vgct_button,
    vgct_slider,
    vgct_textedit,
    vgct_optionmenu,
    vgct_list,
    vgct_multibutton,
    vgct_paramedit,
    vgct_waveedit,
    vgct_steplfo,
    vgct_browser,
    vgct_textview,
    vgct_VU,
    n_vgct,
    vgct_menu,
};

const char vgct_names[n_vgct][16] = {
    "none",      "button",   "slider",  "textedit", "optionmenu", "list", "multibutton",
    "paramedit", "waveedit", "steplfo", "browser",  "textview",   "VU"};

unsigned int hex2color(std::string colstring);

std::string dmode_float_to_str(float x, int dmode, std::string unit);
std::string dmode_int_to_str(int x, int dmode, std::string unit);

bool dmode_str_to_float(std::string str, float &x, int dmode);
bool dmode_str_to_int(std::string str, int &x, int dmode);

struct vg_menudata;
struct vg_menudata_entry;

struct vg_menudata_entry
{
    std::string label;
    vg_menudata *submenu;
    actiondata ad;
    vg_rect r; // rectangle used temporarily by vg_menu (other classes shouldn't rely on it)
};

struct vg_menudata
{
    std::vector<vg_menudata_entry> entries;
};

struct vg_menucolumn
{
    vg_rect r;
    int hover_id;
    vg_point origin;
    vg_menudata *menu;
};

class vg_menu : public vg_control
{
  public:
    vg_menu(vg_window *o, int id, TiXmlElement *e);
    // virtual ~vg_menu();

    virtual void draw();
    virtual void processevent(vg_controlevent &e);
    virtual std::string serialize_tag() { return "none"; }
    virtual int get_min_w() { return 50; }
    virtual int get_min_h() { return 50; }
    void set_style(int s);

    virtual void post_action(actiondata ad);

    virtual int get_n_parameters();
    virtual int get_parameter_type(int id);
    virtual std::string get_parameter_name(int id);
    virtual std::string get_parameter_text(int id);
    virtual void set_parameter_text(int id, std::string text, bool no_refresh = false);

    bool clicked_entry(vg_point p, int &c, int &e);

    std::string text, oldtext, label;
    int style, color;
    bool down, hover, disabled, moved;
    // vg_menudata *menu;
    std::vector<vg_menucolumn> columns;
};

class vg_optionmenu : public vg_control
{
  public:
    vg_optionmenu(vg_window *o, int id, TiXmlElement *e);
    // virtual ~vg_optionmenu();

    virtual void draw();
    virtual void processevent(vg_controlevent &e);
    virtual std::string serialize_tag() { return vgct_names[vgct_optionmenu]; }
    virtual int get_min_w() { return buttonbmp[0].r.get_w(); }
    virtual int get_min_h() { return buttonbmp[0].r.get_h(); }
    void set_style(int s);

    virtual void post_action(actiondata ad);

    virtual int get_n_parameters();
    virtual int get_parameter_type(int id);
    virtual std::string get_parameter_name(int id);
    virtual std::string get_parameter_text(int id);
    virtual void set_parameter_text(int id, std::string text, bool no_refresh = false);

    std::string label, action;
    int style, action_p[2], action_i, value_int;
    unsigned int color;
    bool down, hover, disabled, stuck;
    vg_bitmap buttonbmp[5];
    vg_menudata md;
};

class vg_button : public vg_control
{
  public:
    vg_button(vg_window *o, int id, TiXmlElement *e);
    // virtual ~vg_button();

    virtual void draw();
    virtual void processevent(vg_controlevent &e);
    virtual std::string serialize_tag() { return vgct_names[vgct_button]; }
    virtual int get_min_w() { return buttonbmp[0].r.get_w(); }
    virtual int get_min_h() { return buttonbmp[0].r.get_h(); }
    void set_style(int s);

    virtual void post_action(actiondata ad);

    virtual int get_n_parameters();
    virtual int get_parameter_type(int id);
    virtual std::string get_parameter_name(int id);
    virtual std::string get_parameter_text(int id);
    virtual void set_parameter_text(int id, std::string text, bool no_refresh = false);

    std::string label, action;
    int style, action_p[2], action_i;
    std::string action_text;
    unsigned int color;
    bool down, hover, disabled, stuck;
    vg_bitmap buttonbmp[4];
};

class vg_multibutton : public vg_button
{
  public:
    vg_multibutton(vg_window *o, int id, TiXmlElement *e);
    //~vg_multibutton();

    virtual void draw();
    virtual void processevent(vg_controlevent &e);
    virtual std::string serialize_tag() { return vgct_names[vgct_multibutton]; }
    virtual void post_action(actiondata ad);

    int sub_buttons, sb_width, sb_height, sub_rows, sub_columns;
    int sel_id, hover_id;
};
// (std::string based multibutton thing)
// should be used for channel-selection, pages etc
// comma-seperated labels (samma params som button annars)

class vg_paramedit : public vg_control
{
  public:
    vg_paramedit(vg_window *o, int id, TiXmlElement *e);
    // virtual ~vg_paramedit();

    virtual void draw();
    virtual void processevent(vg_controlevent &e);
    virtual std::string serialize_tag() { return vgct_names[vgct_paramedit]; }
    virtual int get_min_w() { return buttonbmp[0].r.get_w(); }
    virtual int get_min_h() { return buttonbmp[0].r.get_h(); }
    void set_style(int s);

    virtual void post_action(actiondata ad);

    virtual int get_n_parameters();
    virtual int get_parameter_type(int id);
    virtual std::string get_parameter_name(int id);
    virtual std::string get_parameter_text(int id);
    virtual void set_parameter_text(int id, std::string text, bool no_refresh = false);

    void interpret_textedit();

    std::string label, text, oldtext;
    int style, value_int, labelcolor, labelsplit;
    float value_float;
    float f_movespeed;
    float f_min, f_max, f_default;
    int i_min, i_max, dmode, i_default;
    std::string unit;

    bool down, textedit, hover, disabled;
    bool floatmode;
    bool temposync;
    int cursor;
    vg_fpoint lastmouseloc;
    vg_rect textrect;
    vg_bitmap buttonbmp[4];
};

// display/editor parameter display that mostly work as a display for other parameters (by
// intercepting their events) make sure an edited value is sent to all controls associated with that
// param id/subid removes the need to have displays on sliders etc..
/*class vg_paramdisplay : public vg_control
{
};*/

class vg_slider : public vg_control
{
  public:
    vg_slider(vg_window *o, int id, TiXmlElement *e);
    // virtual ~vg_slider();

    virtual void draw();
    virtual void processevent(vg_controlevent &e);
    virtual std::string serialize_tag() { return vgct_names[vgct_slider]; }
    virtual int get_min_w() { return 15; }
    virtual int get_min_h() { return 15; }
    void set_style(int s);

    virtual void post_action(actiondata ad);

    virtual int get_n_parameters();
    virtual int get_parameter_type(int id);
    virtual std::string get_parameter_name(int id);
    virtual std::string get_parameter_text(int id);
    virtual void set_parameter_text(int id, std::string text, bool no_refresh = false);

    int style;
    bool vertical;
    int col[2], headid;
    bool down, hover, disabled;

    vg_fpoint lastmouseloc;
    // vg_surface img;
    vg_bitmap head[2][5];
    vg_rect labelrect, headrect;

    bool floatmode;
    bool temposync;
    float value_float;
    int value_int;
    float f_movespeed;
    float f_min, f_max, f_default;
    int i_min, i_max, dmode, i_default;
    std::string unit;
};

class vg_textedit : public vg_control
{
  public:
    vg_textedit(vg_window *o, int id, TiXmlElement *e);
    // virtual ~vg_textedit();

    virtual void draw();
    virtual void processevent(vg_controlevent &e);
    virtual std::string serialize_tag() { return vgct_names[vgct_textedit]; }
    virtual int get_min_w() { return buttonbmp[0].r.get_w(); }
    virtual int get_min_h() { return buttonbmp[0].r.get_h(); }
    void set_style(int s);

    virtual void post_action(actiondata ad);

    virtual int get_n_parameters();
    virtual int get_parameter_type(int id);
    virtual std::string get_parameter_name(int id);
    virtual std::string get_parameter_text(int id);
    virtual void set_parameter_text(int id, std::string text, bool no_refresh = false);
    virtual bool steal_kbd_on_focus() { return true; }

    std::string text, oldtext, label;
    int style, cursor;
    bool down, hover, disabled;

    vg_bitmap buttonbmp[4];
};

struct vg_list_zone
{
    std::string name;
    int keylo, keyhi, keyroot, keyhifade, keylofade;
    int vello, velhi;
    int channel, id;
    bool visible, playing, mute;
    vg_rect r;
};

class vg_list : public vg_control
{
  public:
    vg_list(vg_window *o, int id, TiXmlElement *e);
    virtual ~vg_list();

    virtual void draw();
    virtual void processevent(vg_controlevent &e);
    virtual std::string serialize_tag() { return vgct_names[vgct_list]; }
    virtual void post_action(actiondata ad);
    virtual bool is_dragdrop_destination() { return true; }
    virtual void on_resize();

    vg_bitmap bmpdata[16];
    std::vector<vg_list_zone> zones;

    void refresh();
    void set_hover_id(int);
    void set_selected_id(int);
    int hover_id, selected_id, firstkey;
    std::set<int> multiselect;

    int hover_id_corners, selected_id_corners;
    vg_rect dragcorners[10];

    vg_menudata md; // context menu
    vg_menudata mdsub_layer, mdsub_part;
    int controlstate;

  private:
    void redraw_zone(int i); // used for midi visualization
    void redraw_key(int i);
    void draw_pad(int key, const char *labeloverride = 0);
    void draw_list_key(int key);
    void draw_list_zone(vg_list_zone *i, int j);
    int xy_to_key(int x, int y);
    std::vector<int> get_zones_at(vg_point p);

    void toggle_zoom(bool toggle);
    void update_dragcorners();
    vg_rect select_rect, rect_padmargin, r, rect_keybed, rect_zonebed;
    vg_point lastmouseloc;
    int pad_dx, pad_dy, pad_y_split;
    char keyvel[128];
    int key_zonecount[128];
    int lastkey; // used for keybed playing
    int mode;
    int zoomlevel, noteh;
    bool moved_since_mdown, mouse_inside;
};

// void scpe_get_limits(int cmode, float* fmin, float* fmax, float* fdefault, int* imin, int* imax,
// int* idefault);

class vg_waveeditor : public vg_control
{
  public:
    vg_waveeditor(vg_window *o, int id, TiXmlElement *e);
    virtual ~vg_waveeditor();

    virtual void draw();
    void draw_wave(bool be_quick, bool skip_wave_redraw);
    void queue_draw_wave(bool be_quick, bool skip_wave_redraw);
    void draw_plot();
    virtual void processevent(vg_controlevent &e);
    virtual std::string serialize_tag() { return vgct_names[vgct_waveedit]; }
    virtual void post_action(actiondata ad);
    virtual int get_min_w() { return 20; }
    virtual int get_min_h() { return 20; }
    virtual void set_rect(vg_rect r);

    int getpos(int sample);
    int getsample(int pos);

    vg_surface wavesurf;
    int dispmode;
    int zoom, zoom_max, start;
    int n_visible_pixels, n_hitpoints;
    int controlstate, dragid;
    float vzoom, vzoom_drag;
    void *sampleptr;
    vg_point lastmouseloc;
    unsigned int aatable[4][256];
    vg_bitmap bmpdata[16];
    int playmode, markerpos[256];
    vg_rect dragpoint[256];
    bool draw_be_quick, draw_skip_wave_redraw;
    vg_menudata md; // context menu
};

const int max_steps = 32;

class vg_steplfo : public vg_control
{
  public:
    vg_steplfo(vg_window *o, int id, TiXmlElement *e);
    virtual ~vg_steplfo();

    virtual void draw();
    virtual void processevent(vg_controlevent &e);
    virtual std::string serialize_tag() { return vgct_names[vgct_steplfo]; }
    virtual void post_action(actiondata ad);
    virtual int get_min_w() { return 20; }
    virtual int get_min_h() { return 20; }

    int n_steps;
    float shape;
    vg_rect steprect[max_steps];
    float step[max_steps];
    bool disabled;
};

struct pp_dstrect
{
    int id;
    vg_rect r;
    bool invert;
};

struct pp_catrect
{
    int id;
    vg_rect r;
    int category_entry_id;
    bool invert;
    int indent;
};

struct category_entry;
struct patch_entry;

class vg_browser : public vg_control
{
  public:
    vg_browser(vg_window *o, int id, TiXmlElement *e);
    virtual ~vg_browser();
    void set_style(int s);
    void draw_scrollbar(int id, vg_rect r, int n_visible, int n_entries, bool horizontal = false);

    void update_lists();
    void add_category_to_list(int id);

    virtual void draw();
    virtual void processevent(vg_controlevent &e);
    virtual std::string serialize_tag() { return vgct_names[vgct_browser]; }
    virtual void post_action(actiondata ad);
    virtual int get_min_w() { return 20; }
    virtual int get_min_h() { return 20; }

  protected:
    unsigned int split, style;
    std::string searchstring;
    vg_bitmap sb_tray, sb_head, divider, folder, file[2];
    vg_rect b_upper, b_lower, scroll_rect[2], scroll_rect_head[2];
    bool is_refreshing;

    std::vector<pp_dstrect> prect;
    std::vector<pp_catrect> crect;
    std::vector<category_entry> *categorylist;
    std::vector<patch_entry> *patchlist;

    void PreviewEntry(int Entry, bool AlwaysPreview);
    void SelectSingle(int Entry);

    int category, entry, controlstate, p_first, p_last, mNumVisiblePEntries;
    int scroll_pos[2];
    int database_type;
    std::set<int> multiselect;
    vg_menudata md; // context menu
    int moved_since_mdown;
};

class vg_textview : public vg_control
{
  public:
    vg_textview(vg_window *o, int id, TiXmlElement *e);

    virtual void draw();
    virtual void processevent(vg_controlevent &e);
    virtual std::string serialize_tag() { return vgct_names[vgct_textview]; }
    void set_style(int s);

    virtual void post_action(actiondata ad);

    virtual int get_n_parameters();
    virtual int get_parameter_type(int id);
    virtual std::string get_parameter_name(int id);
    virtual std::string get_parameter_text(int id);
    virtual void set_parameter_text(int id, std::string text, bool no_refresh = false);

    std::string text;
    int style;
    bool selectable, disabled;
    int selected_id;
};

class vg_VU : public vg_control
{
  public:
    vg_VU(vg_window *o, int id, TiXmlElement *e);

    virtual void draw();
    virtual void processevent(vg_controlevent &e);
    virtual std::string serialize_tag() { return vgct_names[vgct_VU]; }
    virtual void post_action(actiondata ad);

  protected:
    int NumChannels;
    int ChannelData[MAX_OUTPUTS];
};
