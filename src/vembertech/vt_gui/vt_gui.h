#pragma once
#include <tinyxml/tinyxml.h>
#include "sampler_wrapper_actiondata.h"
#include <vector>
#include <list>
#include <set>
#include <map>
#include <algorithm>

#if WINDOWS
#include <windows.h>
#include <windowsx.h>
#endif

#include <threadsafety.h>

class vg_rect;
class vg_window;
class vg_surface;
class vg_control;
class vg_point;
class vg_fpoint;
class vg_font;
class vt_LockFree;

#define vg_use_gdi_for_surfaces 1

class vg_rect
{
  public:
    vg_rect();
    vg_rect(int x, int y, int x2, int y2);
    bool bound(vg_rect r);
    bool point_within(vg_point p);
    bool point_within(vg_fpoint p);
    void offset(int x, int y);
    void inset(int x, int y);
    void flip_if_needed();
    bool does_intersect(vg_rect r);
    bool does_encase(vg_rect r);
    void move(int x, int y);
    void move_centerpivot(int x, int y);
    void move_constrained(int x, int y, vg_rect bound);
    void set(int x, int y, int x2, int y2);
    int get_w() { return x2 - x; }
    int get_h() { return y2 - y; }
    int get_mid_x() { return x + ((x2 - x) >> 1); }
    int get_mid_y() { return y + ((y2 - y) >> 1); }
    int x, y, x2, y2;
};

// coordinate system: origin is top-left corner

class vg_point
{
  public:
    vg_point()
    {
        x = 0;
        y = 0;
    }
    vg_point(int nx, int ny)
    {
        x = nx;
        y = ny;
    }
    int x, y;
};

class vg_fpoint
{
  public:
    float x, y;
};

struct vg_controlevent
{
    int eventtype;
    int activebutton; // relevant button for the event. button-agnostic events such as mousemove has
                      // activebutton = 0
    int buttonmask; // mask of currently pressed buttons.. same flags as windows (MK_LBUTTON etc..)
    int eventdata[3];
    float x, y;
};

struct vg_bitmap // a "bitmap" is just a surface with a special rectangle assigned to it
{
    vg_surface *surf;
    vg_rect r;
};

class vg_surface
{
  public:
    vg_surface(vg_window *o);
    vg_surface();
    ~vg_surface();
    bool create(int sizeX, int sizeY, bool DDB = false);
    bool create(std::string filename);
    int read_png(std::string filename);
    void destroy();

    // drawing operations
    void clear(unsigned int color);
    void set_pixel(int x, int y, unsigned int color);
    void fill_rect(vg_rect r, unsigned int color, bool passthrualpha = false);
    void draw_rect(vg_rect r, unsigned int color, bool passthrualpha = false);
    void fill_ramp(vg_rect r, unsigned int color, int quadrant);
    void set_HDC(HDC nhdc);
    bool is_initiated() { return (imgdata != 0); }
    void blit(vg_rect r, vg_surface *source);
    void blit(vg_rect r, vg_bitmap source);
    void blitGDI(vg_rect r, vg_surface *source, HDC hdc);
    void blitGDI(vg_rect r, vg_bitmap source, HDC hdc);
    void blit_alphablend(vg_rect r, vg_surface *source);
    void blit_alphablend(vg_rect r, vg_bitmap source);
    void blit_alphablendGDI(vg_rect r, vg_surface *source, HDC hdc);
    void blit_alphablendGDI(vg_rect r, vg_bitmap source, HDC hdc);
    void borderblit(
        vg_rect r,
        vg_bitmap source); // will stretch the bitmap over the rectangle by keeping the corners
                           // intanct but repeating the middle pixels (use odd-sized bitmaps)
    void draw_text(vg_rect r, std::string text, unsigned int color, int halign = 0, int valign = 0,
                   int ibeampos = -1, bool IgnoreParsing = false);
    void draw_text_multiline(vg_rect r, std::string text, unsigned int color, int halign = 0,
                             int valign = 0);

    unsigned int *imgdata;
    int sizeX, sizeY;   // user requested-size
    int sizeXA, sizeYA; // actual size of the surface
    vg_font *font;
    bool DIB;
    HBITMAP hbmp;
    BITMAPINFO bmpinfo;
    HDC hdc;
    vg_window *owner;
};

class vg_font
{
  public:
    vg_font();
    ~vg_font();
    bool load(std::string filename);

    unsigned char *data;
    int cell_width, cell_height, char_spacing, cell_bytes;
    unsigned char char_width[256];
};

inline void alphablend_pixel(unsigned int src, unsigned int &dst)
{
    // assume ARGB
    // will be reversed on PPC

    int alpha = ((src >> 24) & 0xff);
    alpha += (alpha >> 7); // 256 steps instead of 255
    int alpham1 = 256 - alpha;
    int r = ((src >> 16) & 0xff) * alpha + ((dst >> 16) & 0xff) * alpham1;
    int g = ((src >> 8) & 0xff) * alpha + ((dst >> 8) & 0xff) * alpham1;
    int b = ((src)&0xff) * alpha + ((dst)&0xff) * alpham1;
    dst = ((r << 8) & 0xff0000) | (g & 0xff00) | ((b >> 8) & 0xff);
}

enum
{
    vgcp_string = 0,
    vgcp_int,
    vgcp_float,
};

class vg_control
{
  public:
    vg_control(vg_window *o, int id, TiXmlElement *e);
    virtual ~vg_control();
    virtual void draw();
    bool visible();
    virtual void processevent(vg_controlevent &e);
    virtual std::string serialize_tag()
    {
        return "none";
    } // defining the tags in two different places is somewhat incoherent
    virtual void serialize_data(TiXmlElement *e);
    virtual void restore_data(TiXmlElement *e);
    vg_rect get_rect() { return location; }
    virtual void set_rect(vg_rect r);
    void set_dirty(bool redraw_all = false);
    virtual void
    post_action(actiondata ad){}; // messages sent from editor to the control regarding its state
    virtual int get_min_w() { return 10; }
    virtual int get_min_h() { return 10; }
    virtual void on_resize(){};
    void setlabel(std::string lbl) { label = lbl; /*draw(); set_dirty();*/ }
    // used by editor to query parameters (for layout editing purposes)
    virtual int get_n_parameters() { return 0; }
    virtual int get_parameter_type(int id) { return 0; }
    virtual std::string get_parameter_name(int id) { return ""; }
    virtual std::string get_parameter_text(int id) { return ""; }
    virtual void set_parameter_text(int id, std::string text, bool no_refresh = false) { return; }
    virtual bool steal_kbd_on_focus() { return false; }
    virtual bool is_dragdrop_destination() { return false; }
    // virtual int get_parameter_int(int id);
    // virtual float get_parameter_float(int id);
    std::string decode_vars(std::string s);
    void draw_background(); // used by child classes to render the background of the window

    vg_surface surf;
    int parameter_id, parameter_subid;
    int parameter_subid_tempoverride; // host window put overridden subid (from subpages) here
    int filter, filter_entry;         // filter_entry is stored as a bitmask
    int offset, offset_amount;        // offset for parameter_subid
    int hotkey;
    bool hidden;
    bool draw_needed; // the surface is old/invalid. A draw() call is needed before showing it
    int debug_drawcount;

  protected:
    std::string label;
    vg_rect location;
    vg_window *owner;
    int control_id;
};

vg_control *spawn_control(std::string type, TiXmlElement *data, vg_window *owner, int newid);

unsigned int bitmask_from_hexlist(const char *str);
void hexlist_from_bitmask(char *str, unsigned int bitmask);

const int max_vgwindow_filters = 16;
const int layout_menu = 65536;

struct dropfile
{
    std::string path;
    std::string label;
    int database_id, db_type;
    int key_root;
    int key_lo, key_hi;
    int vel_lo, vel_hi;
    bool has_key, has_startend;
};

class vg_window
{
  public:
    vg_window();
    ~vg_window();
    bool processevent(vg_controlevent &e);
    virtual bool processevent_user(vg_controlevent &e);
    bool create(std::string filename, bool is_toolbox = false, vg_window *owner = 0,
                void *syswindow = 0);
    void destroy();
    void draw(void *, int ctrlid = -1); // draw all the children and put them in the
    void draw_child(int id, bool redraw, HDC hdc);
    void draw_sizer(vg_rect r);
    bool take_focus(int id); // with focus the control will continue to recieve event even when they
                             // are outside it's area. other controls will be blocked
    void drop_focus(int id);
    void set_child_dirty(int id, bool redraw_all = false);
    void move_mousepointer(int x, int y);
    void show_mousepointer(bool s);
    void build_children_by_pid();
    int get_syscolor(int id);

    void
    post_action(actiondata ad,
                vg_control *c); // controls should call this when something relevant has happened
    virtual void post_action_to_program(actiondata ad){};
    virtual void post_action_from_program(actiondata ad);
    virtual void process_action_to_editor(actiondata ad){};
    virtual void process_actions_from_program();
    std::vector<int> p2l_map; // map external "interaction parameters" to a control id
    // register callback or create an eventqueue that store actions
    void post_action_to_control(
        actiondata ad); // used to send actions back to controls (to change their state/values)
    void save_layout();
    bool load_layout();

    virtual std::string param_get_label_from_id(int id) { return "-"; }
    virtual int param_get_id_from_label(std::string s) { return -1; }
    virtual int param_get_n() { return 0; }
    virtual int param_get_n_subparams(int id) { return 0; }

    bool delete_child(int id);
    void align_selected(int mode);
    void nudge_selected_control(int x, int y);

    void toolbox_selectedstate(bool selected, bool multiselect, int tool);
    void events_from_toolbox(actiondata ad);
    void open_toolbox();
    void close_toolbox();

    bool dialog_confirm(std::wstring label, std::wstring text, int buttonmode);
    std::wstring dialog_save(std::wstring label, std::wstring startdir, int type);
    void dialog_save_part();
    void dialog_save_multi();

    vg_rect get_rect();
    vg_surface surf, background, altbg[8];
    std::string background_filename, altbg_filename[8];
    std::vector<vg_control *> children;
    std::multimap<int, vg_control *> children_by_pid;
    std::set<int> dirtykids; // add to list when dirty, remove when redrawn
    c_sec dirtykids_csec;
    int bgcolor;
    unsigned int colorpalette[256];

    // art assets
    void load_art();
    vg_bitmap get_art(unsigned int bank, unsigned int style, unsigned int entry);
    std::vector<vg_surface *> artdata_loaded;
    vg_font *get_font(unsigned int id);
    void set_cursor(int);
    TiXmlDocument artdoc;
    std::vector<vg_surface *> artsurfs;
    std::vector<vg_font *> artfonts;
    int filters[max_vgwindow_filters];
    HWND syswindow;
    bool ignore_next_mousemove;
    std::vector<dropfile> dropfiles;
    std::wstring savepart_fname;
    void dropfiles_process(int mode, int key);
    int keyboardmode;

  protected:
    std::string layoutfile;
    HWND hWnd;
    HBITMAP hbmp;
    HHOOK kbdhook;
    BITMAPINFO bmpinfo;
    std::string rootdir_skin, rootdir_default;
    bool editmode;
    bool is_editor, initialized;
    int editmode_drag;
    int editmode_tool;
    vg_control *menu;
    vg_point editmode_drag_origin, editmode_cursor_origin;
    bool redraw_all, redraw_all_controls;
    void set_selected_id(int s);
    int selected_id; // -1 = unselected, used in layout mode
    std::set<int> multiselector;
    vg_rect select_rect;
    bool select_rect_visible;
    int focus_id, hover_id; // used in normal mode
    vg_window *owner;
    vg_window *editwin;
    vg_rect sizer_rects[10]; // arranged as a numpad (5 = move)

    vt_LockFree *ActionBuffer;

#ifdef WINDOWS
    HCURSOR hcursor_arrowcopy, hcursor_vmove, hcursor_hmove, hcursor_zoomin, hcursor_zoomout,
        hcursor_panhand;
#endif

    // fpu flags
    int sse_state_flag, old_sse_state, old_fpu_state;
    void fpuflags_set();
    void fpuflags_restore();
};
