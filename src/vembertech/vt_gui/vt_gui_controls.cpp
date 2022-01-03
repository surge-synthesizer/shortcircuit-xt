#include "vt_gui_controls.h"
#include "util/unitconversion.h"
#include <vt_util/vt_string.h>

const int menurow_height = 12;

vg_control *spawn_control(std::string type, TiXmlElement *data, vg_window *owner, int newid)
{
    if (type.compare(vgct_names[vgct_browser]) == 0)
    {
        return new vg_browser(owner, newid, data);
    }
    else if (type.compare(vgct_names[vgct_steplfo]) == 0)
    {
        return new vg_steplfo(owner, newid, data);
    }
    else if (type.compare(vgct_names[vgct_waveedit]) == 0)
    {
        return new vg_waveeditor(owner, newid, data);
    }
    else if (type.compare(vgct_names[vgct_paramedit]) == 0)
    {
        return new vg_paramedit(owner, newid, data);
    }
    else if (type.compare(vgct_names[vgct_multibutton]) == 0)
    {
        return new vg_multibutton(owner, newid, data);
    }
    else if (type.compare(vgct_names[vgct_list]) == 0)
    {
        return new vg_list(owner, newid, data);
    }
    else if (type.compare(vgct_names[vgct_optionmenu]) == 0)
    {
        return new vg_optionmenu(owner, newid, data);
    }
    else if (type.compare(vgct_names[vgct_textedit]) == 0)
    {
        return new vg_textedit(owner, newid, data);
    }
    else if (type.compare(vgct_names[vgct_textview]) == 0)
    {
        return new vg_textview(owner, newid, data);
    }
    else if (type.compare(vgct_names[vgct_slider]) == 0)
    {
        return new vg_slider(owner, newid, data);
    }
    else if (type.compare(vgct_names[vgct_button]) == 0)
    {
        return new vg_button(owner, newid, data);
    }
    else if (type.compare(vgct_names[vgct_VU]) == 0)
    {
        return new vg_VU(owner, newid, data);
    }

    assert(0);
    // no matching control found

    return new vg_control(owner, newid, data);
}

/*
enum temposyncmodes
{
        sm_8bar,
        sm_7bar,
        sm_6bar,
        sm_5bar,
        sm_4bar,
        sm_3bar,
        sm_2bar,
        sm_1bar,

        sm_128th,
        sm_64th_tri,
        sm_96th,
        sm_64th,
        sm_48th,
        sm_64th_dot,
        sm_32th,
        sm_24th,
        sm_32th_dot,
        sm_16th,
        sm_12th,
        sm_16th_dot,
        sm_8th,
        sm_6th,
        sm_8th_dot,
        sm_4th,
        sm_3rd,
        sm_4th_dot,
        sm_2th,
        sm_1bar_tri,
        sm_2th_dot,
        n_temposyncmodes,
};

const char temposynctitles[n_temposyncmodes][16] = {	"1/128","1/96","1/128 dot",
                                                                                        "1/64","1/48","1/64
dot", "1/32","1/24","1/32 dot", "1/16","1/12","1/16 dot", "1/8","1/6","1/8 dot", "1/4","1/3","1/4
dot", "1/2","2/3","1/2 dot", "1 bar", "2 bars", "3 bars", "4 bars", "5 bars", "6 bars", "7 bars", "8
bars"};

const float temposyncratio[n_temposyncmodes] = {
                                                        32, 24, 32.f/1.5f,
                                                        16, 12, 16.f/1.5f,
                                                        8, 6, 8.f/1.5f,
                                                        4, 3, 4.f/1.5f,
                                                        2, 1.5f, 2.f/1.5f,
                                                        1, 0.75f, 1.f/1.5f,
                                                        1.f/2.f, 0.375f, 0.5f/1.5f,
                                                        1.f/4.f,1.f/8.f,1.f/12.f,1.f/16.f,1.f/20.f,1.f/24.f,1.f/28.f,1.f/32.f};

                                                        */

float temposync_quantitize(float x)
{
    float a, b = modf(x, &a);
    if (b < 0)
    {
        b += 1.f;
        a -= 1.f;
    }
    b = powf(2.0f, b);
    b = min(floor(b * 2.f) / 2.f, floor(b * 3.f) / 3.f);
    b = log(b) / log(2.f);
    return a + b;
}

std::string temposynced_float_to_str(float x)
{
    x = temposync_quantitize(x);

    char txt[256];

    if (x > 4)
        sprintf(txt, "%.2f / 64th", 32.f * powf(2.0f, -x));
    else if (x > 3)
        sprintf(txt, "%.2f / 32th", 16.f * powf(2.0f, -x));
    else if (x > 2)
        sprintf(txt, "%.2f / 16th", 8.f * powf(2.0f, -x));
    else if (x > 1)
        sprintf(txt, "%.2f / 8th", 4.f * powf(2.0f, -x));
    else if (x > 0)
        sprintf(txt, "%.2f / 4th", 2.f * powf(2.0f, -x));
    else if (x > -1)
        sprintf(txt, "%.2f / 2th", 1.f * powf(2.0f, -x));
    else
        sprintf(txt, "%.2f bars", 0.5f * powf(2.0f, -x));

    return txt;
}

std::string dmode_int_to_str(int x, int dmode, std::string unit)
{
    std::string s;
    char ch[256];
    switch (dmode)
    {
    case 0:
    default:
        sprintf(ch, "%i", x);
        break;
    case 1: // x + 1 (for midi-channel)
        sprintf(ch, "%i", x + 1);
        break;
    case 2: // notes
    {
        int octave = (x / 12) - 2;
        char notenames[12][3] = {"C ", "C#", "D ", "D#", "E ", "F ",
                                 "F#", "G ", "G#", "A ", "A#", "B "};
        sprintf(ch, "%s%i", notenames[x % 12], octave);
    }
    break;
        /*case 3: // temposync
                {
                        if((x>=0)&&(x<n_temposyncmodes)) sprintf(ch,"%s",temposynctitles[x]);
                        else sprintf(ch,"-");
                }
                break;*/
    };

    s = ch;
    if (!unit.empty())
    {
        s += " ";
        s += unit;
    }

    return s;
}

std::string dmode_float_to_str(float x, int dmode, std::string unit)
{
    std::string s;
    char ch[256];
    bool noprefix = false;

    if (unit.empty())
        noprefix = true;
    if (!unit.compare("dB"))
        noprefix = true;
    if (!unit.compare("c"))
        noprefix = true;

    switch (dmode)
    {
    case 0:
    default:
        break;
    case 1: // percent
        x = 100.f * x;
        noprefix = true;
        break;
    case 4: // pow2
        x = powf(2.f, x);
        break;
    case 5: // Audible freq
        x = 440.f * powf(2.f, x);
        break;
    };

    float ax = fabs(x);

    if (dmode == 1)
    {
        if (fabs(x) >= 100.f)
            sprintf(ch, "%.0f", x);
        else
            sprintf(ch, "%.1f", x);
    }
    else if (!noprefix && (ax >= 1000))
        sprintf(ch, "%.2f", x * 0.001f);
    else if (!noprefix && (ax < 0.01f))
        sprintf(ch, "%.1f", x * 1000.f);
    else
        sprintf(ch, "%.2f", x);

    s = ch;
    if (!unit.empty())
    {
        s += " ";
        if (!noprefix)
        {
            if (ax >= 1000)
                s += "k";
            else if (ax < 0.01f)
                s += "m";
        }
        s += unit;
    }

    return s;
}

bool dmode_str_to_float(std::string str, float &xout, int dmode)
{
    if (str.empty())
        return false;
    float x = atof(str.c_str());
    switch (dmode)
    {
    case 0:
    default:
        break;
    case 1: // percent
        x = 0.01f * x;
        break;
    case 4: // pow2
        x = log2(x);
        break;
    case 5: // Audible freq
        x = log2(x * (1.f / 440.f));
        break;
    };
    xout = x;
    return true;
}

bool dmode_str_to_int(std::string str, int &xout, int dmode)
{
    if (str.empty())
        return false;

    if (dmode == 1)
    {
        xout = atoi(str.c_str()) - 1;
        return true;
    }

    char c = tolower(str[0]);
    if ((c >= 'a') && (c <= 'h') && (str.length() > 1)) // it's a note
    {
        int key = 0;
        int octavepol = 1;

        for (int i = 0; i < str.size(); i++)
        {
            char c = tolower(str[i]);
            switch (c)
            {
            case 'c':
                key = 0;
                break;
            case 'd':
                key = 2;
                break;
            case 'e':
                key = 4;
                break;
            case 'f':
                key = 5;
                break;
            case 'g':
                key = 7;
                break;
            case 'a':
                key = 9;
                break;
            case 'h':
                key = 11;
                break;
            case 'b':
                if (i)
                    key--;
                else
                    key = 11;
                break;
            case '-':
                octavepol = -1;
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
            {
                int oct = c - '0';
                key += (octavepol * oct + 2) * 12;
            }
            break;
            }
        }
        xout = key;
        return true;
    }
    else
    {
        int x = atoi(str.c_str());
        xout = x;
        return true;
    }
}

unsigned int hex2color(std::string colstring)
{
    int n = colstring.length();
    unsigned int col = 0;

    if ((n == 8) || (n == 6)) // with alpha
    {
        for (int i = 0; i < n; i++)
        {
            int v = colstring.at(i);
            v = tolower(v);
            switch (v)
            {
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
                v = v - '0';
                break;
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                v = 10 + v - 'a';
                break;
            default:
                v = 0;
                break;
            };
            col |= (v & 0xf) << (4 * (n - 1 - i));
        }
        if (n == 6)
            return col | 0xff000000;
        return col;
    }
    return 0xff000000;
}

/*	vg_button	*/

vg_button::vg_button(vg_window *o, int id, TiXmlElement *e) : vg_control(o, id, e)
{
    set_style(0);

    down = false;
    hover = false;
    disabled = false;
    stuck = false;
    action_i = vga_click;
    action_p[0] = 0;
    action_p[1] = 0;
    color = owner->get_syscolor(col_standard_text);

    restore_data(e);
}

void vg_button::set_style(int s)
{
    style = s;
    for (int i = 0; i < 4; i++)
        buttonbmp[i] = owner->get_art(vgct_button, style, i);
}

void vg_button::draw()
{
    draw_background();

    int idx = 0;
    if (disabled)
        idx = 3;
    else if (stuck)
        idx = 2;
    else if ((action_i == vga_filter) && (owner->filters[action_p[0]] == action_p[1]))
    {
        idx = 2;
    }
    else if ((action_i == vga_intval) && action_p[0])
        idx = 2;
    else if (down)
        idx = 2;
    else if (hover)
        idx = 1;

    surf.borderblit(vg_rect(0, 0, surf.sizeX, surf.sizeY), buttonbmp[idx]);
    surf.draw_text(vg_rect(0, 0, surf.sizeX, surf.sizeY), decode_vars(label),
                   disabled ? owner->get_syscolor(col_standard_text_disabled)
                            : (down ? owner->get_syscolor(col_standard_text)
                                    : owner->get_syscolor(col_standard_text)));
    draw_needed = false;
}

void vg_button::processevent(vg_controlevent &e)
{
    if (disabled)
        return;
    if (stuck)
        return;

    if (((e.eventtype == vget_mousedown) || (e.eventtype == vget_mousedblclick)) &&
        (e.activebutton == 1))
        down = true;
    else if (((e.eventtype == vget_mouseup) && (e.activebutton == 1) && down) ||
             (e.eventtype == vget_hotkey))
    {
        if (action_i == vga_intval)
            action_p[0] = action_p[0] ? 0 : 1;
        actiondata ad;
        ad.data.i[0] = action_p[0];
        ad.data.i[1] = action_p[1];
        ad.actiontype = action_i;
        if ((action_i == vga_exec_external) || (action_i == vga_url_external) ||
            (action_i == vga_doc_external))
        {
            vtCopyString((char *)ad.data.str, action_text.c_str(), actiondata_maxstring);
        }
        owner->post_action(ad, this);
        down = false;
    }
    else if (e.eventtype == vget_mouseenter)
        hover = true;
    else if (e.eventtype == vget_mouseleave)
    {
        down = false;
        hover = false;
    }
    else
        return; // skip update

    set_dirty();
}

void vg_button::post_action(actiondata ad)
{
    if (ad.actiontype == vga_disable_state)
    {
        disabled = ad.data.i[0];
        set_dirty();
    }
    else if (ad.actiontype == vga_stuck_state)
    {
        stuck = ad.data.i[0];
        set_dirty();
    }
    else if (ad.actiontype == vga_intval)
    {
        action_p[0] = ad.data.i[0];
        set_dirty();
    }
    else if (ad.actiontype == vga_label)
    {
        label = (char *)ad.data.str;
        set_dirty();
    }
    else if (ad.actiontype == vga_hide)
    {
        hidden = ad.data.i[0];
        set_dirty();
        draw_needed = !hidden; // no need to draw if hidden
    }
}

int vg_button::get_n_parameters() { return 7; }
int vg_button::get_parameter_type(int id)
{
    switch (id)
    {
    case 0:
    case 4:
    case 5:
        return vgcp_int;
    case 1:
    case 2:
    case 3:
    case 6:
        return vgcp_string;
    }
    return 0;
}
std::string vg_button::get_parameter_name(int id)
{
    switch (id)
    {
    case 0:
        return "style";
    case 1:
        return "color";
    case 2:
        return "label";
    case 3:
        return "action";
    case 4:
        return "action_p1";
    case 5:
        return "action_p2";
    case 6:
        return "action_text";
    }
    return "-";
}
std::string vg_button::get_parameter_text(int id)
{
    char t[256];
    switch (id)
    {
    case 0:
    {
        sprintf_s(t, sizeof(t), "%i", style);
        return t;
    }
    case 1:
    {
        sprintf_s(t, sizeof(t), "%x", color);
        return t;
    }
    case 2:
        return label;
    case 3:
        return action;
    case 4:
    case 5:
    {
        sprintf_s(t, sizeof(t), "%i", action_p[id - 4]);
        return t;
    }
    case 6:
        return action_text;
    }
    return "-";
}
void vg_button::set_parameter_text(int id, std::string text, bool no_refresh)
{
    switch (id)
    {
    case 0:
        set_style(atol(text.c_str()));
        break;
    case 1:
        color = hex2color(text);
        break;
    case 2:
        label = text;
        break;
    case 3:
    {
        action = text;
        if (action.compare("filter") == 0)
            action_i = vga_filter;
        else if (action.compare("int") == 0)
            action_i = vga_intval;
        else if (action.compare("exec") == 0)
            action_i = vga_exec_external;
        else if (action.compare("url") == 0)
            action_i = vga_url_external;
        else if (action.compare("doc") == 0)
            action_i = vga_doc_external;
        else if (action.compare("savepatch") == 0)
            action_i = vga_save_patch;
        else if (action.compare("savemulti") == 0)
            action_i = vga_save_multi;
        else
            action_i = vga_click;
    }
    break;
    case 4:
    case 5:
        action_p[id - 4] = atol(text.c_str());
        break;
    case 6:
        action_text = text;
        break;
    }
    if (!no_refresh)
    {
        set_dirty();
    }
}

/*	vg_multibutton	*/

vg_multibutton::vg_multibutton(vg_window *o, int id, TiXmlElement *e) : vg_button(o, id, e)
{
    sel_id = 0;
    hover_id = -1;
    sb_width = 1;
    sb_height = 1;

    sub_buttons = 0;
    sub_rows = 1;
    sub_columns = 1;
}

void vg_multibutton::draw()
{
    draw_needed = false;
    draw_background();

    // count buttons
    std::basic_string<char>::size_type a = 0, b;
    sub_buttons = count(label.begin(), label.end(), ',');
    sub_rows = count(label.begin(), label.end(), ';');
    sub_rows++;
    sub_buttons += sub_rows;
    sub_columns = max(1, sub_buttons / sub_rows);

    vg_rect r0(0, 0, surf.sizeX, surf.sizeY);
    sb_width = max(1, r0.get_w() / (sub_columns));
    sb_height = max(1, r0.get_h() / sub_rows);

    if (!label.empty())
    {
        a = 0;
        for (int i = 0; i < sub_buttons; i++)
        {
            vg_rect r = r0;

            r.x = (i % sub_columns) * sb_width;
            r.x2 = r.x + sb_width;
            r.y = (i / sub_columns) * sb_height;
            r.y2 = r.y + sb_height;

            std::string entry;
            b = label.find_first_of(",;", a);

            if (b == label.npos)
                b = label.size();

            entry = label.substr(a, b - a);

            int idx = 0;
            if (disabled)
                idx = 3;
            else if (stuck)
                idx = 2;
            else if ((action_i == vga_filter) && (owner->filters[action_p[0]] == i))
            {
                idx = 2;
            }
            if (sel_id == i)
                idx = 2;
            else if (hover_id == i)
                idx = 1;

            surf.borderblit(r, buttonbmp[idx]);
            surf.draw_text(r, entry,
                           disabled ? owner->get_syscolor(col_standard_text_disabled)
                                    : ((idx == 2) ? owner->get_syscolor(col_standard_text)
                                                  : owner->get_syscolor(col_standard_text)));
            if (b == label.size())
                break;
            a = b + 1;
        }
    }
}

void vg_multibutton::post_action(actiondata ad)
{
    if (ad.actiontype == vga_disable_state)
    {
        disabled = ad.data.i[0];
        set_dirty();
    }
    else if (ad.actiontype == vga_stuck_state)
    {
        stuck = ad.data.i[0];
        set_dirty();
    }
    else if (ad.actiontype == vga_intval)
    {
        sel_id = ad.data.i[0];
        set_dirty();
    }
    else if (ad.actiontype == vga_label)
    {
        label = (char *)ad.data.str;
        set_dirty();
    }
    else if (ad.actiontype == vga_hide)
    {
        hidden = ad.data.i[0];
        set_dirty();
        draw_needed = !hidden; // no need to draw if hidden
    }
}

void vg_multibutton::processevent(vg_controlevent &e)
{
    if (disabled)
        return;
    if (stuck)
        return;

    if ((e.eventtype == vget_mousedown) ||
        (e.eventtype == vget_mousedblclick) && (e.activebutton == 1))
    {
        int x = max(0, min(surf.sizeX, e.x));
        int y = max(0, min(surf.sizeY, e.y));
        int n_id = max(0, min(sub_columns - 1, (x / sb_width))) +
                   sub_columns * max(0, min(sub_rows - 1, (y / sb_height)));

        sel_id = max(0, min(n_id, sub_buttons - 1));
        down = true;
        owner->take_focus(control_id);
    }
    else if (((e.eventtype == vget_mouseup) && (e.activebutton == 1) && down))
    {
        actiondata ad;
        if (action_i == vga_click) // override vg_button's default click with intval
        {
            ad.actiontype = vga_intval;
            ad.data.i[0] = sel_id;
        }
        else
        {
            ad.data.i[0] = action_p[0];
            ad.data.i[1] = sel_id;
            ad.data.i[2] = action_p[1];
            ad.actiontype = action_i;
        }
        owner->post_action(ad, this);
        down = false;
        owner->drop_focus(control_id);
    }
    else if ((e.eventtype == vget_mouseenter) || (e.eventtype == vget_mousemove))
    {
        owner->set_cursor(cursor_arrow);
        int x = max(0, min(surf.sizeX, e.x));
        int y = max(0, min(surf.sizeY, e.y));
        int n_id = max(0, min(sub_columns - 1, (x / sb_width))) +
                   sub_columns * max(0, min(sub_rows - 1, (y / sb_height)));

        hover_id = max(0, min(n_id, sub_buttons - 1));
        if (e.buttonmask & vgm_LMB)
            sel_id = hover_id;
    }
    else if (e.eventtype == vget_mouseleave)
    {
        hover_id = -1;
    }
    else
        return; // skip update

    set_dirty();
}

/*	vg_slider	*/

enum
{
    dm_value = 0,
};

vg_slider::vg_slider(vg_window *o, int id, TiXmlElement *e) : vg_control(o, id, e)
{
    set_style(0);

    lastmouseloc.x = 0.f;
    lastmouseloc.y = 0.f;
    value_float = 0.0f;
    value_int = 0;
    f_default = 0.f;
    f_min = 0.f;
    f_max = 1.f;
    f_movespeed = 0.01f;
    i_default = 0;
    i_min = 0;
    i_max = 127;
    floatmode = true;
    temposync = false;
    dmode = 0;

    col[0] = owner->get_syscolor(col_slidertray1);
    col[1] = owner->get_syscolor(col_slidertray2);
    headid = 0;
    vertical = false;

    down = false;
    hover = false;
    disabled = false;

    restore_data(e);
}

void vg_slider::set_style(int s)
{
    style = s;
    for (int i = 0; i < 5; i++)
    {
        head[0][i] = owner->get_art(vgct_slider, style, i);
        head[1][i] = owner->get_art(vgct_slider, style, i + 5);
    }

    // vg_bitmap t = owner->get_art(vgct_slider,style,1);
    // labelrect = t.r;
}

void vg_slider::draw()
{
    draw_needed = false;
    draw_background();

    const int margin = 4;
    const int margin2 = 13;
    const int labelheight = 9;

    vg_rect sr_outer(0, 0, surf.sizeX, surf.sizeY);
    int c0 = owner->get_syscolor(col_slidertray1);
    int c0t = owner->get_syscolor(col_slidertray1);
    int c1 = owner->get_syscolor(col_slidertray2);
    if (disabled)
        c0 = (c0 & 0x00ffffff) | (((c0 & 0xff000000) >> 1) & 0xff000000);
    if (disabled)
        c0t = ((col[0] & (0x00fefefe)) + 0x00404040) | 0xff000000;
    if (disabled)
        c1 = (c1 & 0x00ffffff) | (((c1 & 0xff000000) >> 1) & 0xff000000);

    // label
    if (label.size())
    {
        vg_rect labelrect = sr_outer;
        sr_outer.y2 -= labelheight;
        labelrect.y = labelrect.y2 - surf.font->cell_height;
        // labelrect.x;

        if (vertical)
        {
            surf.draw_text(labelrect, label, c0t, 0, -1);
        }
        else
        {
            labelrect.inset(margin, 0);
            surf.draw_text(labelrect, label, c0t, -1, -1);
            char text[64];
            if (temposync)
                sprintf_s(text, sizeof(text), "%s", temposynced_float_to_str(value_float).c_str());
            else if (floatmode)
                sprintf_s(text, sizeof(text), "%s",
                          dmode_float_to_str(value_float, dmode, unit).c_str());
            else
                sprintf_s(text, sizeof(text), "%s",
                          dmode_int_to_str(value_int, dmode, unit).c_str());
            if (!disabled)
                surf.draw_text(labelrect, text, c0t, 1, -1);
        }
    }

    // draw tray
    {
        vg_rect rd = sr_outer;
        if (vertical)
            rd.inset((sr_outer.get_w() - 1) >> 1, margin);
        else
            rd.inset(margin, (sr_outer.get_h() - 1) >> 1);
        surf.fill_rect(rd, c0, false);

        bool bp_tray; // if true draw bipolar tray
        if (floatmode)
            bp_tray = fabs(f_max + f_min) < (0.1 * f_max);
        else
            bp_tray = (i_max + i_min) == 0;

        if (vertical)
        {
            rd = sr_outer;
            rd.inset(margin, margin2);
            rd.y2 = rd.y + 1;
            surf.fill_rect(rd, c0, false);
            rd = sr_outer;
            rd.inset(margin, margin2);
            rd.y = rd.y2 - 1;
            surf.fill_rect(rd, c0, false);

            // draw ramp
            if (bp_tray)
            {
                vg_rect rd = sr_outer;
                rd.inset(0, margin2 + 1);
                vg_rect rd1 = rd;
                vg_rect rd2 = rd;

                rd2.x += margin;
                rd2.x2 = (sr_outer.get_w() - 1) >> 1;
                rd1.x = rd2.x2 + 2;
                rd1.x2 -= margin;

                rd1.y2 -= (rd.get_h() >> 1) + 1;
                rd2.y = rd1.y2 + 2;
                surf.fill_ramp(rd1, c1, 0);
                surf.fill_ramp(rd2, c1, 3);
            }
            else
            {
                vg_rect rd = sr_outer;
                rd.inset(0, margin2 + 1);
                rd.x2 -= margin;
                rd.x = (sr_outer.get_w() + 3) >> 1;
                surf.fill_ramp(rd, c1, 0);
            }
        }
        else
        {
            rd = sr_outer;
            rd.inset(margin2, margin);
            rd.x2 = rd.x + 1;
            surf.fill_rect(rd, c0, false);
            rd = sr_outer;
            rd.inset(margin2, margin);
            rd.x = rd.x2 - 1;
            surf.fill_rect(rd, c0, false);

            // draw ramp
            if (bp_tray)
            {
                vg_rect rd = sr_outer;
                rd.inset(margin2 + 1, 0);
                vg_rect rd1 = rd;
                vg_rect rd2 = rd;

                rd2.y += margin;
                rd2.y2 = (sr_outer.get_h() - 1) >> 1;
                rd1.y = rd2.y2 + 2;
                rd1.y2 -= margin;

                rd1.x2 -= (rd.get_w() >> 1) + 1;
                rd2.x = rd1.x2 + 2;
                surf.fill_ramp(rd1, c1, 3);
                surf.fill_ramp(rd2, c1, 0);
            }
            else
            {
                vg_rect rd = sr_outer;
                rd.inset(margin2 + 1, 0);
                rd.y += margin;
                rd.y2 = (sr_outer.get_h() - 1) >> 1;
                surf.fill_ramp(rd, c1, 0);
            }
        }
    }

    // draw head
    if (!disabled)
    {
        headrect = head[vertical ? 1 : 0][headid].r;
        float v;
        if (floatmode)
        {
            float frange = (f_max - f_min);
            if (frange == 0.f)
                frange = 1.f;
            v = (value_float - f_min) / frange;
        }
        else
        {
            float frange = (i_max - i_min);
            if (frange == 0.f)
                frange = 1.f;
            v = (value_int - i_min) / (float)frange;
        }

        v = max(0.f, min(1.f, v));

        if (vertical)
        {
            v = 1.f - v;
            headrect.move(0, v * (float)(sr_outer.get_h() - headrect.get_h()));
            headrect.x2 = sr_outer.x2;
        }
        else
        {
            headrect.move(v * (float)(sr_outer.get_w() - headrect.get_w()), 0);
            headrect.y2 = sr_outer.y2;
        }
        surf.borderblit(headrect, head[vertical ? 1 : 0][headid]);
        if (hover || down)
            surf.borderblit(headrect, head[vertical ? 1 : 0][4]);
    }
}

void vg_slider::post_action(actiondata ad)
{
    if (ad.actiontype == vga_disable_state)
    {
        disabled = ad.data.i[0];
        set_dirty();
    }
    else if (ad.actiontype == vga_floatval)
    {
        value_float = ad.data.f[0];
        floatmode = true;
        set_dirty();
    }
    else if (ad.actiontype == vga_intval)
    {
        value_int = ad.data.i[0];
        floatmode = false;
        set_dirty();
    }
    else if (ad.actiontype == vga_label)
    {
        label = (char *)ad.data.str;
        set_dirty();
    }
    else if (ad.actiontype == vga_hide)
    {
        hidden = ad.data.i[0];
        set_dirty();
        draw_needed = !hidden; // no need to draw if hidden
    }
    else if (ad.actiontype == vga_datamode)
    {
        set_parameter_text(2, (char *)ad.data.str);
    }
    else if (ad.actiontype == vga_temposync)
    {
        temposync = ad.data.i[0];
        set_dirty();
    }
}

void vg_slider::processevent(vg_controlevent &e)
{
    if (disabled)
        return;
    if (e.eventtype == vget_mousewheel)
    {
        int pol = (e.eventdata[0] > 0) ? 1 : -1;
        if (floatmode)
        {
            value_float = value_float + pol * f_movespeed;
            value_float = max(f_min, min(f_max, value_float));
            actiondata ad;
            ad.actiontype = vga_floatval;
            ad.data.f[0] = value_float;
            if (temposync)
                ad.data.f[0] = temposync_quantitize(ad.data.f[0]);
            owner->post_action(ad, this);
        }
        else
        {
            value_int = max(i_min, min(i_max, value_int + pol));
            actiondata ad;
            ad.actiontype = vga_intval;
            ad.data.i[0] = value_int;
            owner->post_action(ad, this);
        }
    }
    else if (((e.eventtype == vget_mousedown) || (e.eventtype == vget_mousedblclick)) &&
             (e.activebutton == 1))
    {
        if ((e.buttonmask & vgm_control) || (e.eventtype == vget_mousedblclick)) // default value
        {
            actiondata ad;
            if (floatmode)
            {
                value_float = max(f_min, min(f_max, f_default));
                ad.actiontype = vga_floatval;
                ad.data.f[0] = value_float;
                if (temposync)
                    ad.data.f[0] = temposync_quantitize(ad.data.f[0]);
            }
            else
            {
                value_int = max(i_min, min(i_max, i_default));
                value_float = value_int;
                ad.actiontype = vga_intval;
                ad.data.i[0] = value_int;
            }
            owner->post_action(ad, this);
        }
        else
        {
            down = true;
            lastmouseloc.x = e.x;
            lastmouseloc.y = e.y;
            owner->take_focus(control_id);
            owner->show_mousepointer(false);
        }
    }
    else if (down && (e.eventtype == vget_mousemove) && (e.buttonmask & 1))
    {
        float delta;

        if (vertical)
        {
            delta = lastmouseloc.y - e.y;
        }
        else
        {
            delta = e.x - lastmouseloc.x;
        }

        if (e.buttonmask & vgm_RMB)
            delta *= 0.1f;
        if (e.buttonmask & vgm_shift)
            delta *= 0.1f;

        if (floatmode)
        {
            delta *= f_movespeed;
            value_float = value_float + delta;
            value_float = max(f_min, min(f_max, value_float));
            actiondata ad;
            ad.actiontype = vga_floatval;
            ad.data.f[0] = value_float;
            if (temposync)
                ad.data.f[0] = temposync_quantitize(ad.data.f[0]);
            owner->post_action(ad, this);
        }
        else
        {
            delta *= 0.25f;

            int oldval = value_int;
            value_float = value_float + delta;
            value_int = (int)value_float;
            value_int = max(i_min, min(i_max, value_int));
            if (oldval != value_int)
            {
                actiondata ad;
                ad.actiontype = vga_intval;
                ad.data.i[0] = value_int;
                owner->post_action(ad, this);
            }
        }

        owner->move_mousepointer(location.x + lastmouseloc.x, location.y + lastmouseloc.y);

        // lastmouseloc.x = e.x;
        // lastmouseloc.y = e.y;
    }
    else if (down && (e.eventtype == vget_mouseup) && (e.activebutton == 1))
    {
        down = false;
        owner->move_mousepointer(location.x + headrect.get_mid_x(),
                                 location.y + headrect.get_mid_y());
        owner->show_mousepointer(true);
        owner->drop_focus(control_id);
    }
    else if (e.eventtype == vget_mouseenter)
    {
        owner->set_cursor(cursor_arrow);
        hover = true;
    }
    else if (e.eventtype == vget_mouseleave)
    {
        down = false;
        hover = false;
    }
    else
        return; // skip update

    set_dirty();
}

int vg_slider::get_n_parameters() { return 6; }
int vg_slider::get_parameter_type(int id)
{
    switch (id)
    {
    case 0:
    case 3:
        return vgcp_int;
    case 1:
    case 4:
    case 5:
    case 2:
        return vgcp_string;
    }
    return 0;
}
std::string vg_slider::get_parameter_name(int id)
{
    switch (id)
    {
    case 0:
        return "style";
    case 1:
        return "label";
    case 2:
        return "datamode";
    case 3:
        return "vertical";
    case 4:
        return "color1";
    case 5:
        return "color2";
    }
    return "-";
}
std::string vg_slider::get_parameter_text(int id)
{
    char t[64];
    switch (id)
    {
    case 0:
    {
        sprintf(t, "%i", style);
        return t;
    }
    case 1:
        return label;
    case 2:
    {
        if (floatmode)
        {
            sprintf(t, "f,%.1f,%.3f,%.1f,%i,%s", f_min, f_movespeed, f_max, dmode, unit.c_str());
        }
        else
        {
            sprintf(t, "i,%i,%i,%i,%s", i_min, i_max, dmode, unit.c_str());
        }
        return t;
    }
    case 3:
    {
        sprintf(t, "%i", vertical ? 1 : 0);
        return t;
    }
    case 4:
    case 5:
    {
        sprintf(t, "%x", col[id - 4]);
        return t;
    }
    }
    return "-";
}
void vg_slider::set_parameter_text(int id, std::string text, bool no_refresh)
{
    switch (id)
    {
    case 0:
        set_style(atol(text.c_str()));
        break;
    case 1:
        label = text;
        break;
    case 2:
        if (text[0] == 'f')
        {
            floatmode = true;
            std::string entry;
            std::basic_string<char>::size_type a = 2, b;
            b = text.find_first_of(",", a);
            entry = text.substr(a, b - a);
            if (entry.empty())
                return;
            f_min = atof(entry.c_str());
            a = b + 1;
            b = text.find_first_of(",", a);
            entry = text.substr(a, b - a);
            if (entry.empty())
                return;
            f_movespeed = atof(entry.c_str());
            a = b + 1;
            b = text.find_first_of(",", a);
            entry = text.substr(a, b - a);
            if (entry.empty())
                return;
            f_max = atof(entry.c_str());
            a = b + 1;
            b = text.find_first_of(",", a);
            entry = text.substr(a, b - a);
            if (entry.empty())
                return;
            dmode = atol(entry.c_str());
            a = b + 1;
            b = text.find_first_of(",", a);
            unit = text.substr(a, b - a);
        }
        else if (text[0] == 'i')
        {
            floatmode = false;
            std::string entry;
            std::basic_string<char>::size_type a = 2, b;
            b = text.find_first_of(",", a);
            entry = text.substr(a, b - a);
            if (entry.empty())
                return;
            i_min = atol(entry.c_str());
            a = b + 1;
            b = text.find_first_of(",", a);
            entry = text.substr(a, b - a);
            if (entry.empty())
                return;
            i_max = atol(entry.c_str());
            a = b + 1;
            b = text.find_first_of(",", a);
            entry = text.substr(a, b - a);
            if (entry.empty())
                return;
            dmode = atol(entry.c_str());
            a = b + 1;
            b = text.find_first_of(",", a);
            unit = text.substr(a, b - a);
        }
        break;
    case 3:
        vertical = (atol(text.c_str()) != 0);
        break;
    case 4:
    case 5:
        col[id - 4] = hex2color(text);
        break;
    }
    if (!no_refresh)
    {
        set_dirty();
    }
}

/*	vg_textedit	*/

vg_textedit::vg_textedit(vg_window *o, int id, TiXmlElement *e) : vg_control(o, id, e)
{
    set_style(0);

    down = false;
    hover = false;
    disabled = false;
    cursor = 0;

    restore_data(e);
}

void vg_textedit::set_style(int s)
{
    style = s;
    for (int i = 0; i < 4; i++)
        buttonbmp[i] = owner->get_art(vgct_textedit, style, i);
}

void vg_textedit::draw()
{
    draw_needed = false;
    draw_background();

    int idx = 0;
    if (disabled)
        idx = 3;
    else if (down)
        idx = 2;
    else if (hover)
        idx = 1;

    int padding = 4;
    if (label.size() > 0)
    {
        int xsplit = (3 * surf.sizeX) >> 3;
        surf.borderblit(vg_rect(xsplit, 0, surf.sizeX, surf.sizeY), buttonbmp[idx]);
        surf.draw_text(vg_rect(xsplit + padding, 0, surf.sizeX - padding, surf.sizeY), text,
                       disabled ? owner->get_syscolor(col_standard_text_disabled)
                                : owner->get_syscolor(col_standard_text),
                       0, 0, down ? cursor : -1, down);
        surf.draw_text(vg_rect(0, 0, xsplit, surf.sizeY), label,
                       disabled ? owner->get_syscolor(col_standard_text_disabled)
                                : owner->get_syscolor(col_standard_text));
    }
    else
    {
        surf.borderblit(vg_rect(0, 0, surf.sizeX, surf.sizeY), buttonbmp[idx]);
        surf.draw_text(vg_rect(padding, 0, surf.sizeX - padding, surf.sizeY), text,
                       disabled ? owner->get_syscolor(col_standard_text_disabled)
                                : owner->get_syscolor(col_standard_text),
                       0, 0, down ? cursor : -1, down);
    }
}

void vg_textedit::processevent(vg_controlevent &e)
{
    if (disabled)
        return;

    if (((e.eventtype == vget_mousedown) || (e.eventtype == vget_mousedblclick)) &&
        (e.activebutton == 1))
    {
        if ((e.x < 0.f) || (e.x > location.get_w()) || (e.y < 0.f) || (e.y > location.get_h()))
        {
            // outside
            down = false;
            owner->drop_focus(control_id);
            actiondata ad;
            ad.actiontype = vga_text;
            char *c = (char *)ad.data.str;
            vtCopyString(c, text.c_str(), actiondata_maxstring);
            c[actiondata_maxstring] = 0;
            owner->post_action(ad, this);
        }
        else
        {
            if (!down && (e.x))
            {
                down = true;
                oldtext = text;
                cursor = text.size();
                owner->take_focus(control_id);
            }
        }
    }
    else if (((e.eventtype == vget_mousedown) || (e.eventtype == vget_mousedblclick)) &&
             (e.activebutton == 2))
    {
        text.clear();
        cursor = 0;
        actiondata ad;
        ad.actiontype = vga_text;
        ad.data.i[0] = 0;
        owner->post_action(ad, this);
    }
    else if (down && (e.eventtype == vget_character))
    {
        if (!(e.eventdata[0] & 0xffffff00))
        {
            char c = e.eventdata[0] & 0xff;
            switch (c)
            {
            case 0:
                break;
            case 0x8: // backspace
                if (cursor && (text.size() > 0))
                    text.erase(text.begin() + cursor - 1);
                cursor--;
                break;
            default:
                // text += c;
                text.insert(text.begin() + cursor, c);
                cursor++;
                break;
            }
        }
    }
    else if (down && (e.eventtype == vget_keypress))
    {
        switch (e.eventdata[0])
        {
        case VK_ESCAPE:
            text = oldtext;
            owner->drop_focus(control_id);
            down = false;
            hover = false;
            break;
        case VK_RETURN:
        {
            owner->drop_focus(control_id);
            down = false;
            hover = false;
            actiondata ad;
            ad.actiontype = vga_text;
            char *c = (char *)ad.data.str;
            vtCopyString(c, text.c_str(), actiondata_maxstring);
            c[actiondata_maxstring] = 0;
            owner->post_action(ad, this);
        }
        break;
        case VK_DELETE:
            if ((cursor < text.size()) && (text.size() > 0))
                text.erase(text.begin() + cursor);
            break;
        case VK_LEFT:
            cursor = max(0, cursor - 1);
            break;
        case VK_RIGHT:
            cursor = min(text.size(), cursor + 1);
            break;
        case VK_HOME:
            cursor = 0;
            break;
        case VK_END:
            cursor = text.size();
            break;
        default:
            break;
        };
    }
    else if (e.eventtype == vget_mouseenter)
    {
        hover = true;
        owner->set_cursor(cursor_text);
    }
    else if (e.eventtype == vget_mousemove)
    {
        if (!((e.x < 0.f) || (e.x > location.get_w()) || (e.y < 0.f) || (e.y > location.get_h())))
            owner->set_cursor(cursor_text);
    }
    else if (e.eventtype == vget_mouseleave)
    {
        hover = false;
        owner->set_cursor(cursor_arrow);
    }
    else
        return; // skip update

    cursor = max(0, min(cursor, text.length()));
    set_dirty();
}

void vg_textedit::post_action(actiondata ad)
{
    if (ad.actiontype == vga_disable_state)
    {
        disabled = ad.data.i[0];
        set_dirty();
    }
    else if (ad.actiontype == vga_text)
    {
        text = (char *)ad.data.str;
        set_dirty();
    }
    else if (ad.actiontype == vga_label)
    {
        label = (char *)ad.data.str;
        set_dirty();
    }
}

int vg_textedit::get_n_parameters() { return 2; }
int vg_textedit::get_parameter_type(int id)
{
    switch (id)
    {
    case 0:
        return vgcp_int;
    case 1:
        return vgcp_string;
    }
    return 0;
}
std::string vg_textedit::get_parameter_name(int id)
{
    switch (id)
    {
    case 0:
        return "style";
    case 1:
        return "label";
    }
    return "-";
}
std::string vg_textedit::get_parameter_text(int id)
{
    char t[64];
    switch (id)
    {
    case 0:
    {
        sprintf(t, "%i", style);
        return t;
    }
    case 1:
        return label;
    }
    return "-";
}
void vg_textedit::set_parameter_text(int id, std::string text, bool no_refresh)
{
    switch (id)
    {
    case 0:
        set_style(atol(text.c_str()));
        break;
    case 1:
        label = text;
        break;
    }
    if (!no_refresh)
    {
        set_dirty();
    }
}

/*	vg_paramedit	*/

vg_paramedit::vg_paramedit(vg_window *o, int id, TiXmlElement *e) : vg_control(o, id, e)
{
    set_style(0);

    down = false;
    hover = false;
    disabled = false;
    textedit = false;
    cursor = 0;
    text = "";

    floatmode = true;
    value_int = 0;
    value_float = 0;
    labelcolor = owner->get_syscolor(col_standard_text);
    labelsplit = 8;
    f_min = 0.f;
    f_max = 1.f;
    f_default = 0.f;
    i_default = 0;
    i_min = 0;
    i_max = 127;
    f_movespeed = 0.01f;
    dmode = 0;
    unit = "";

    restore_data(e);
}

void vg_paramedit::set_style(int s)
{
    style = s;
    for (int i = 0; i < 4; i++)
        buttonbmp[i] = owner->get_art(vgct_textedit, style, i);
}

void vg_paramedit::draw()
{
    draw_needed = false;
    draw_background();

    int idx = 0;
    if (disabled)
        idx = 3;
    else if (down || textedit)
        idx = 2;
    else if (hover)
        idx = 1;

    int padding = 4;

    char txt[64];
    if (textedit)
        sprintf(txt, "%s", text.c_str());
    else if (floatmode)
        sprintf(txt, "%s", dmode_float_to_str(value_float, dmode, unit).c_str());
    else
        sprintf(txt, "%s", dmode_int_to_str(value_int, dmode, unit).c_str());

    textrect.set(0, 0, surf.sizeX, surf.sizeY);

    if (label.size() > 0)
    {
        vg_rect labelrect = textrect;
        if (labelsplit > 0)
        {
            int xsplit = (surf.sizeX * abs(labelsplit)) >> 4;
            labelrect.x2 = xsplit;
            textrect.x = xsplit;
        }
        else
        {
            int xsplit = (surf.sizeX * abs(16 + labelsplit)) >> 4;
            labelrect.x = xsplit;
            textrect.x2 = xsplit;
        }
        surf.draw_text(labelrect, label,
                       disabled ? owner->get_syscolor(col_standard_text_disabled)
                                : owner->get_syscolor(col_standard_text));
    }
    surf.borderblit(textrect, buttonbmp[idx]);
    textrect.inset(padding, 0);
    if (!disabled)
        surf.draw_text(textrect, txt, owner->get_syscolor(col_standard_text), 0, 0,
                       textedit ? cursor : -1);
}

void vg_paramedit::processevent(vg_controlevent &e)
{
    if (disabled)
        return;

    if (e.eventtype == vget_mousewheel)
    {
        int pol = (e.eventdata[0] > 0) ? 1 : -1;
        if (floatmode)
        {
            value_float = max(f_min, min(f_max, value_float + pol * f_movespeed));
            actiondata ad;
            ad.actiontype = vga_floatval;
            ad.data.f[0] = value_float;
            owner->post_action(ad, this);
        }
        else
        {
            value_int = max(i_min, min(i_max, value_int + pol));
            actiondata ad;
            ad.actiontype = vga_intval;
            ad.data.i[0] = value_int;
            owner->post_action(ad, this);
        }
        actiondata ad;
        ad.actiontype = vga_endedit;
        owner->post_action(ad, this);
    }
    else if ((e.eventtype == vget_mousedown) && (e.activebutton == 1))
    {
        if (textedit)
        {
            if ((e.x < 0.f) || (e.x > location.get_w()) || (e.y < 0.f) || (e.y > location.get_h()))
            {
                // outside
                textedit = false;
                hover = false;
                owner->drop_focus(control_id);
                interpret_textedit();
            }
        }
        else if (!down)
        {
            if (e.buttonmask & vgm_control) // check for default value
            {
                actiondata ad;
                if (floatmode)
                {
                    value_float = max(f_min, min(f_max, f_default));
                    ad.actiontype = vga_floatval;
                    ad.data.f[0] = value_float;
                    if (temposync)
                        ad.data.f[0] = temposync_quantitize(ad.data.f[0]);
                }
                else
                {
                    value_int = max(i_min, min(i_max, i_default));
                    value_float = value_int;
                    ad.actiontype = vga_intval;
                    ad.data.i[0] = value_int;
                }
                owner->post_action(ad, this);
            }
            else
            {
                down = true;
                owner->take_focus(control_id);
                owner->show_mousepointer(false);
                lastmouseloc.x = e.x;
                lastmouseloc.y = e.y;
                if (!floatmode)
                {
                    value_float = value_int;
                }
            }
        }
    }
    else if ((e.eventtype == vget_mousedblclick) && (e.activebutton == 1))
    {
        // enter text-edit mode
        down = false;
        textedit = true;
        if (floatmode)
            text = dmode_float_to_str(value_float, dmode, "");
        else
            text = dmode_int_to_str(value_float, dmode, "");
        cursor = text.size();
        owner->take_focus(control_id);
    }
    else if (down && (e.eventtype == vget_mousemove) && (e.buttonmask & 1))
    {
        float delta;

        bool vertical = false; // TODO get from global setting
        /*if(vertical)
        {
                delta = lastmouseloc.y - e.y;
        }
        else
        {
                delta = e.x - lastmouseloc.x;
        }		*/
        delta = (lastmouseloc.y - e.y) + (e.x - lastmouseloc.x);

        if (e.buttonmask & vgm_RMB)
            delta *= 0.05f;
        if (e.buttonmask & vgm_shift)
            delta *= 0.05f;

        if (floatmode)
        {
            delta *= f_movespeed;

            value_float = max(f_min, min(f_max, value_float + delta));
            actiondata ad;
            ad.actiontype = vga_floatval;
            ad.data.f[0] = value_float;
            owner->post_action(ad, this);
        }
        else
        {
            delta *= 0.25f;

            int oldval = value_int;
            value_float = value_float + delta;
            value_int = (int)value_float;
            value_int = max(i_min, min(i_max, value_int));
            if (oldval != value_int)
            {
                actiondata ad;
                ad.actiontype = vga_intval;
                ad.data.i[0] = value_int;
                owner->post_action(ad, this);
            }
        }
        owner->move_mousepointer(location.x + lastmouseloc.x, location.y + lastmouseloc.y);
        /*lastmouseloc.x = e.x;
        lastmouseloc.y = e.y;*/
    }
    else if (down && (e.eventtype == vget_mouseup))
    {
        down = false;
        // owner->move_mousepointer(location.x + textrect.get_mid_x(), location.y +
        // textrect.get_mid_y());
        owner->move_mousepointer(location.x + lastmouseloc.x, location.y + lastmouseloc.y);
        owner->show_mousepointer(true);
        owner->drop_focus(control_id);

        actiondata ad;
        ad.actiontype = vga_endedit;
        owner->post_action(ad, this);
    }
    else if (e.eventtype == vget_mouseenter)
    {
        owner->set_cursor(cursor_arrow);
        hover = true;
    }
    else if (e.eventtype == vget_mouseleave)
    {
        hover = false;
    }
    else if (textedit && (e.eventtype == vget_character))
    {
        if (!(e.eventdata[0] & 0xffffff00))
        {
            char c = e.eventdata[0] & 0xff;
            switch (c)
            {
            case 0:
                break;
            case 0x8: // backspace
                if (cursor && (text.size() > 0))
                    text.erase(text.begin() + cursor - 1);
                cursor--;
                cursor = max(0, cursor);
                break;
            default:
                // text += c;
                text.insert(text.begin() + cursor, c);
                cursor++;
                break;
            }
        }
    }
    else if (textedit && (e.eventtype == vget_keypress))
    {
        switch (e.eventdata[0])
        {
        case VK_ESCAPE:
            text = oldtext;
            owner->drop_focus(control_id);
            textedit = false;
            hover = false;
            break;
        case VK_RETURN:
        {
            owner->drop_focus(control_id);
            textedit = false;
            hover = false;
            interpret_textedit();
        }
        break;
        case VK_DELETE:
            if ((cursor < text.size()) && (text.size() > 0))
                text.erase(text.begin() + cursor);
            break;
        case VK_LEFT:
            cursor = max(0, cursor - 1);
            break;
        case VK_RIGHT:
            cursor = min(text.size(), cursor + 1);
            break;
        case VK_HOME:
            cursor = 0;
            break;
        case VK_END:
            cursor = text.size();
            break;
        default:
            break;
        };
    }
    else
        return; // skip update

    set_dirty();
}

void vg_paramedit::post_action(actiondata ad)
{
    if (ad.actiontype == vga_disable_state)
    {
        disabled = ad.data.i[0];
        set_dirty();
    }
    else if (ad.actiontype == vga_hide)
    {
        hidden = ad.data.i[0];
        set_dirty(); // no need to draw if hidden
        draw_needed = !hidden;
    }
    else if (ad.actiontype == vga_label)
    {
        label = (char *)ad.data.str;
        set_dirty();
    }
    else if (ad.actiontype == vga_intval)
    {
        floatmode = false;
        value_int = ad.data.i[0];
        set_dirty();
    }
    else if (ad.actiontype == vga_floatval)
    {
        floatmode = true;
        value_float = ad.data.f[0];
        set_dirty();
    }
    else if (ad.actiontype == vga_datamode)
    {
        set_parameter_text(4, (char *)ad.data.str);
    }
}

void vg_paramedit::interpret_textedit()
{
    float f;
    int i;
    bool result;
    if (floatmode)
    {
        if (dmode_str_to_float(text, f, dmode))
        {
            value_float = max(f_min, min(f_max, f));
            actiondata ad;
            ad.actiontype = vga_floatval;
            ad.data.f[0] = value_float;
            owner->post_action(ad, this);
        }
    }
    else
    {
        if (dmode_str_to_int(text, i, dmode))
        {
            value_int = max(i_min, min(i_max, i));
            actiondata ad;
            ad.actiontype = vga_intval;
            ad.data.i[0] = value_int;
            owner->post_action(ad, this);
        }
    }
    actiondata ad;
    ad.actiontype = vga_endedit;
    owner->post_action(ad, this);
}

int vg_paramedit::get_n_parameters() { return 5; }
int vg_paramedit::get_parameter_type(int id)
{
    switch (id)
    {
    case 0:
    case 3:
        return vgcp_int;
    case 1:
    case 2:
    case 4:
        return vgcp_string;
    }
    return 0;
}
std::string vg_paramedit::get_parameter_name(int id)
{
    switch (id)
    {
    case 0:
        return "style";
    case 1:
        return "label";
    case 2:
        return "labelcol";
    case 3:
        return "labelsplit";
    case 4:
        return "datamode";
    }
    return "-";
}
std::string vg_paramedit::get_parameter_text(int id)
{
    char t[64];
    switch (id)
    {
    case 0:
    {
        sprintf(t, "%i", style);
        return t;
    }
    case 1:
        return label;
    case 2:
    {
        sprintf(t, "%x", labelcolor);
        return t;
    }
    case 3:
    {
        sprintf(t, "%i", labelsplit);
        return t;
    }
    case 4:
    {
        if (floatmode)
        {
            sprintf(t, "f,%.1f,%.3f,%.1f,%i,%s", f_min, f_movespeed, f_max, dmode, unit.c_str());
        }
        else
        {
            sprintf(t, "i,%i,%i,%i,%s", i_min, i_max, dmode, unit.c_str());
        }
        return t;
    }
    }
    return "-";
}
void vg_paramedit::set_parameter_text(int id, std::string text, bool no_refresh)
{
    switch (id)
    {
    case 0:
        set_style(atol(text.c_str()));
        break;
    case 1:
        label = text;
        break;
    case 2:
        labelcolor = hex2color(text);
        break;
    case 3:
        labelsplit = atol(text.c_str());
        break;
    case 4:
        if (text[0] == 'f')
        {
            floatmode = true;
            std::string entry;
            std::basic_string<char>::size_type a = 2, b;
            b = text.find_first_of(",", a);
            entry = text.substr(a, b - a);
            if (entry.empty())
                return;
            f_min = atof(entry.c_str());
            a = b + 1;
            b = text.find_first_of(",", a);
            entry = text.substr(a, b - a);
            if (entry.empty())
                return;
            f_movespeed = atof(entry.c_str());
            a = b + 1;
            b = text.find_first_of(",", a);
            entry = text.substr(a, b - a);
            if (entry.empty())
                return;
            f_max = atof(entry.c_str());
            a = b + 1;
            b = text.find_first_of(",", a);
            entry = text.substr(a, b - a);
            if (entry.empty())
                return;
            dmode = atol(entry.c_str());
            a = b + 1;
            b = text.find_first_of(",", a);
            unit = text.substr(a, b - a);
        }
        else if (text[0] == 'i')
        {
            floatmode = false;
            std::string entry;
            std::basic_string<char>::size_type a = 2, b;
            b = text.find_first_of(",", a);
            entry = text.substr(a, b - a);
            if (entry.empty())
                return;
            i_min = atol(entry.c_str());
            a = b + 1;
            b = text.find_first_of(",", a);
            entry = text.substr(a, b - a);
            if (entry.empty())
                return;
            i_max = atol(entry.c_str());
            a = b + 1;
            b = text.find_first_of(",", a);
            entry = text.substr(a, b - a);
            if (entry.empty())
                return;
            dmode = atol(entry.c_str());
            a = b + 1;
            b = text.find_first_of(",", a);
            unit = text.substr(a, b - a);
        }
        break;
    }
    if (!no_refresh)
    {
        set_dirty();
    }
}

/*	vg_textview	*/

vg_textview::vg_textview(vg_window *o, int id, TiXmlElement *e) : vg_control(o, id, e)
{
    set_style(0);

    disabled = false;
    selectable = false;
    selected_id = -1;

    restore_data(e);
}

void vg_textview::set_style(int s) { style = s; }

void vg_textview::draw()
{
    draw_needed = false;
    draw_background();

    int margin = 2;
    std::string textdata = text.empty() ? label : text;

    char *cs = (char *)textdata.c_str();
    for (int i = 0; i < textdata.size(); i++)
    {
        if (!cs[i])
            break;

        if (cs[i] == ';')
            cs[i] = '\n';
    }

    surf.draw_text_multiline(vg_rect(margin, 0, surf.sizeX - margin, surf.sizeY), textdata,
                             owner->get_syscolor(col_standard_text), 0, -1);
}

void vg_textview::processevent(vg_controlevent &e) {}

void vg_textview::post_action(actiondata ad)
{
    if (ad.actiontype == vga_disable_state)
    {
        disabled = ad.data.i[0];
        set_dirty();
    }
    else if (ad.actiontype == vga_text)
    {
        text = (char *)ad.data.str;
        set_dirty();
    }
}

int vg_textview::get_n_parameters() { return 2; }
int vg_textview::get_parameter_type(int id)
{
    switch (id)
    {
    case 0:
        return vgcp_int;
    case 1:
        return vgcp_string;
    }
    return 0;
}
std::string vg_textview::get_parameter_name(int id)
{
    switch (id)
    {
    case 0:
        return "style";
    case 1:
        return "label";
    }
    return "-";
}
std::string vg_textview::get_parameter_text(int id)
{
    char t[64];
    switch (id)
    {
    case 0:
    {
        sprintf(t, "%i", style);
        return t;
    }
    case 1:
        return label;
    }
    return "-";
}
void vg_textview::set_parameter_text(int id, std::string text, bool no_refresh)
{
    switch (id)
    {
    case 0:
        set_style(atol(text.c_str()));
        break;
    case 1:
        label = text;
        break;
    }
    if (!no_refresh)
    {
        set_dirty();
    }
}

/* VU */

vg_VU::vg_VU(vg_window *o, int id, TiXmlElement *e) : vg_control(o, id, e)
{
    NumChannels = 0;
    assert(sizeof(VUdata) == 4);
}

void vg_VU::draw()
{
    if (!NumChannels)
        return;

    draw_background();

    int w = surf.sizeX / (NumChannels * 2);

    vg_rect r(0, 0, 2 * w, surf.sizeY);
    for (int i = 0; i < NumChannels; i++)
    {
        VUdata vu = *((VUdata *)&ChannelData[i]);
        if (vu.stereo)
        {
            vg_rect r0 = r;
            r0.inset(2, 0);
            vg_rect r1 = r0, r2 = r0;
            r1.x2 = r0.get_mid_x();
            r2.x = r0.get_mid_x();
            r1.inset(1, 0);
            r2.inset(1, 0);
            if (vu.clip1)
                surf.fill_rect(r1, owner->get_syscolor(col_vu_overload));
            else
            {
                surf.fill_rect(r1, owner->get_syscolor(col_vu_back));
                int h = (int)r1.get_h() * vu.ch1;
                r1.y = r1.y2 - (h >> 8);
                surf.fill_rect(r1, owner->get_syscolor(col_vu_front));
            }
            if (vu.clip2)
                surf.fill_rect(r2, owner->get_syscolor(col_vu_overload));
            else
            {
                surf.fill_rect(r2, owner->get_syscolor(col_vu_back));
                int h = (int)r2.get_h() * vu.ch2;
                r2.y = r2.y2 - (h >> 8);
                surf.fill_rect(r2, owner->get_syscolor(col_vu_front));
            }
        }
        else
        {
            // not yet available
            assert(0);
        }
        r.offset(2 * w, 0);
    }
}
void vg_VU::processevent(vg_controlevent &e) {}
void vg_VU::post_action(actiondata ad)
{
    if (ad.actiontype == vga_vudata)
    {
        NumChannels = ad.data.i[0];
        for (int i = 0; i < NumChannels; i++)
        {
            assert((i + 1) < 13);
            ChannelData[i] = ad.data.i[i + 1];
        }
        set_dirty();
    }
}