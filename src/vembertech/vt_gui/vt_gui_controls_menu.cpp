#include "vt_gui_controls.h"

using std::string;

const int menurow_height = 12;

/*	vg_menu	*/

vg_menu::vg_menu(vg_window *o, int id, TiXmlElement *e) : vg_control(o, id, e)
{
    set_style(0);

    down = false;
    hover = false;
    disabled = false;
    color = 0xff000000;
    //	menu = 0;

    restore_data(e);
}

void vg_menu::set_style(int s) { style = s; }

void vg_menu::draw()
{
    draw_needed = false;
    unsigned int selectionblue = owner->get_syscolor(col_selected_bg);
    int colwidth = 120;

    // surf.clear(0x00ffffff);	// invalid, because only active columns are copied to the
    // "backbuffer"

    int nc = columns.size();
    for (int c = 0; c < nc; c++)
    {
        int n = columns[c].menu->entries.size();
        int maxrows = max(1, (surf.sizeY - 2) / menurow_height);
        int subcolumns = 1 + max(0, (n - 1) / maxrows);
        // n = min(n,maxrows);	// TODO proper multicolumn.. use min for now
        vg_rect r(0, 0, colwidth * subcolumns + 2, (menurow_height * min(n, maxrows)) + 2);
        r.move_constrained(columns[c].origin.x, columns[c].origin.y, location);
        columns[c].r = r;
        surf.draw_rect(r, owner->get_syscolor(col_frame), true);
        r.inset(1, 1);
        surf.fill_rect(r, owner->get_syscolor(col_standard_bg), true);
        int actualwidth = min(colwidth, r.get_w() / subcolumns);
        r.x2 = r.x + actualwidth;
        r.y2 = r.y + menurow_height;
        for (int i = 0; i < n; i++)
        {
            vg_rect tr = r;
            tr.offset(actualwidth * (i / maxrows), menurow_height * (i % maxrows));
            tr.bound(location);
            columns[c].menu->entries[i].r = tr;
            int textcol = 0;
            int bgcol = 0;

            if (columns[c].hover_id == i)
            {
                textcol = owner->get_syscolor(col_selected_text);
                bgcol = owner->get_syscolor(col_selected_bg);
            }
            else if (!columns[c].menu->entries[i].submenu &&
                     (columns[c].menu->entries[i].ad.actiontype == vga_nothing))
            {
                textcol = owner->get_syscolor(col_alt_text);
                bgcol = owner->get_syscolor(col_alt_bg);
            }
            else
            {
                textcol = owner->get_syscolor(col_standard_text);
                bgcol = owner->get_syscolor(col_standard_bg);
            }

            surf.fill_rect(tr, bgcol, true);
            tr.inset(5, 0);
            surf.draw_text(tr, columns[c].menu->entries[i].label, textcol, -1);
            if (columns[c].menu->entries[i].submenu)
                surf.draw_text(tr, "�", textcol, 1); // no ordinary comma
        }
    }
}

bool vg_menu::clicked_entry(vg_point p, int &c, int &e)
{
    for (int i = 0; i < columns.size(); i++)
    {
        if (columns[i].r.point_within(p))
        {
            c = i;
            for (int j = 0; j < columns[i].menu->entries.size(); j++)
            {
                if (columns[i].menu->entries[j].r.point_within(p))
                {
                    e = j;
                    if (!columns[i].menu->entries[j].submenu &&
                        (columns[i].menu->entries[j].ad.actiontype == vga_nothing))
                        return false; // non-action menues cannot be selected
                    return true;
                }
            }
            return false;
        }
    }
    return false;
}

void vg_menu::processevent(vg_controlevent &e)
{
    if (e.eventtype == vget_mousemove)
    {
        owner->set_cursor(cursor_arrow);
        bool do_update = false;
        moved = true;
        int column, entry;
        if (clicked_entry(vg_point(e.x, e.y), column, entry))
        {
            if (columns[column].hover_id != entry)
            {
                if (columns.size() > (column + 1))
                {
                    columns.resize(column + 1);
                    set_dirty(true); // must redraw all when submenus are removed
                }
                columns[column].hover_id = entry;
                do_update = true;

                if (columns[column].menu->entries[entry].submenu)
                {
                    vg_menucolumn mc;
                    mc.hover_id = -1;
                    mc.menu = columns[column].menu->entries[entry].submenu;
                    mc.origin.x = columns[column].r.x2;
                    mc.origin.y = columns[column].menu->entries[entry].r.y;
                    columns.push_back(mc);
                }
            }
        }
        else
        {
            if (!columns.empty() && (columns[columns.size() - 1].hover_id > -1))
            {
                columns[columns.size() - 1].hover_id = -1;
                do_update = true;
            }
        }
        if (do_update)
            set_dirty();
    }
    else if (!moved && (e.eventtype == vget_mouseup))
    {
        return; // do nothing
    }
    else if (((e.eventtype == vget_mouseup) || (e.eventtype == vget_mousedown)) &&
             (e.activebutton == 1))
    {
        int column, entry;
        if (clicked_entry(vg_point(e.x, e.y), column, entry))
        {
            if (columns[column].menu->entries[entry].submenu)
            {
                // don't do anything if entry with submenu is clicked
            }
            else
            {
                actiondata ad = columns[column].menu->entries[entry].ad;
                owner->post_action(ad, 0);
                owner->post_action_from_program(ad);
                owner->drop_focus(control_id);
                set_dirty(true);
            }
        }
        else
        {
            // click outside should close the menu
            owner->drop_focus(control_id);
            set_dirty(true);
        }
    }
    else if (e.eventtype == vget_mousedown) // other buttons should still be able to close the menu
    {
        int column, entry;
        if (!clicked_entry(vg_point(e.x, e.y), column, entry))
        {
            owner->drop_focus(control_id);
            set_dirty(true);
        }
    }
    else
        return; // skip update
}

void vg_menu::post_action(actiondata ad)
{
    if (ad.actiontype == vga_menu)
    {
        // menu = (vg_menudata*)ad.data.ptr[0];
        bool do_destroy =
            ad.data.i[2]; // if 'true' the menudata structure should be destroyed by the reciever
        vg_menucolumn mc;
        mc.origin.x = ad.data.i[3];
        mc.origin.y = ad.data.i[4];
        mc.hover_id = -1;
        mc.menu = (vg_menudata *)ad.data.ptr[0];
        moved = false;
        columns.clear();
        columns.push_back(mc);
        owner->take_focus(control_id);
        set_dirty();
    }
}

int vg_menu::get_n_parameters() { return 0; }
int vg_menu::get_parameter_type(int id) { return 0; }
string vg_menu::get_parameter_name(int id) { return "-"; }
string vg_menu::get_parameter_text(int id) { return "-"; }
void vg_menu::set_parameter_text(int id, string text, bool no_refresh)
{
    if (!no_refresh)
    {
        set_dirty();
    }
}

/*  */

/*	vg_optionmenu	*/

vg_optionmenu::vg_optionmenu(vg_window *o, int id, TiXmlElement *e) : vg_control(o, id, e)
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
    this->label = "-";
    value_int = 0;

    restore_data(e);
}

void vg_optionmenu::set_style(int s)
{
    style = s;
    for (int i = 0; i < 5; i++)
        buttonbmp[i] = owner->get_art(vgct_optionmenu, style, i);
}

void vg_optionmenu::draw()
{
    draw_needed = false;
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
    else if (down)
        idx = 2;
    else if (hover)
        idx = 1;

    surf.borderblit(vg_rect(0, 0, surf.sizeX, surf.sizeY), buttonbmp[idx]);

    int d = (surf.sizeY - buttonbmp[4].r.get_h()) >>
            1; // place arrow the same distance from the right edge as the top & bottom edges
    vg_rect tr(surf.sizeX - d - buttonbmp[4].r.get_w(), d, surf.sizeX - d, surf.sizeY - d);
    surf.blit_alphablend(tr, buttonbmp[4]);

    vg_rect r(0, 0, surf.sizeX - d, surf.sizeY);
    r.inset(5, 0);
    if (!disabled)
        surf.draw_text(r, label, owner->get_syscolor(col_standard_text));
}

void vg_optionmenu::processevent(vg_controlevent &e)
{
    if (disabled)
        return;
    if (stuck)
        return;

    if (e.eventtype == vget_mousewheel)
    {
        if (!md.entries.empty())
        {
            int pol = (e.eventdata[0] > 0) ? -1 : 1;

            value_int = max(0, min((int)md.entries.size() - 1, value_int + pol));
            owner->post_action(md.entries[value_int].ad, 0);
            label = md.entries[value_int].label;
        }
    }
    // else if((e.eventtype == vget_mousedown)&&(e.activebutton == 1)) down = true;
    // else if(((e.eventtype == vget_mouseup)&&(e.activebutton == 1)&&down)||(e.eventtype ==
    // vget_hotkey))
    else if (((e.eventtype == vget_mousedown) && (e.activebutton == 1)) ||
             (e.eventtype == vget_hotkey))
    {
        // down = false;
        actiondata ad;
        ad.id = control_id;
        ad.subid = 0;
        ad.data.ptr[0] = (void *)&md;
        ad.data.i[2] = false; // doesn't need destruction
        ad.data.i[3] = location.x;
        ad.data.i[4] = location.y2;
        ad.actiontype = vga_menu;
        owner->post_action(ad, 0);
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

void vg_optionmenu::post_action(actiondata ad)
{
    if (ad.actiontype == vga_disable_state)
    {
        disabled = ad.data.i[0];
        draw();
        set_dirty();
    }
    else if (ad.actiontype == vga_hide)
    {
        hidden = ad.data.i[0];
        set_dirty();
        draw_needed = !hidden; // no need to draw if hidden
    }
    else if (ad.actiontype == vga_stuck_state)
    {
        stuck = ad.data.i[0];
        set_dirty();
    }
    else if (ad.actiontype == vga_entry_add)
    {
        vg_menudata_entry mde;
        memcpy(&mde.ad, ad.data.ptr[0], sizeof(actiondata));
        mde.label = (char *)&ad.data.str[8];
        mde.submenu = 0;
        md.entries.push_back(mde);
    }
    else if (ad.actiontype == vga_entry_add_ival_from_self)
    {
        int id = md.entries.size();
        vg_menudata_entry mde;
        mde.ad.id = this->parameter_id;
        mde.ad.subid = this->parameter_subid;
        if (offset)
            mde.ad.subid += offset_amount * owner->filters[offset];
        mde.ad.actiontype = vga_intval;
        mde.ad.data.i[0] = id;
        mde.label = ad.data.str;
        mde.submenu = 0;
        md.entries.push_back(mde);
        if (id == value_int)
        {
            label = mde.label;
            set_dirty();
        }
    }
    else if (ad.actiontype == vga_entry_add_ival_from_self_with_id)
    {
        int id = md.entries.size();
        vg_menudata_entry mde;
        mde.ad.id = this->parameter_id;
        mde.ad.subid = this->parameter_subid;
        if (offset)
            mde.ad.subid += offset_amount * owner->filters[offset];
        mde.ad.actiontype = vga_intval;
        mde.ad.data.i[0] = ad.data.i[0];
        mde.label = (char *)&ad.data.str[4];
        mde.submenu = 0;
        md.entries.push_back(mde);
    }
    else if (ad.actiontype == vga_entry_replace_label_on_id)
    {
        unsigned int n = md.entries.size();
        for (unsigned int i = 0; i < n; i++)
        {
            if (md.entries[i].ad.data.i[0] == ad.data.i[0])
            {
                md.entries[i].label = &ad.data.str[4];
                if (i == value_int)
                {
                    label = md.entries[i].label;
                    set_dirty();
                }
                return;
            }
        }
    }
    else if (ad.actiontype == vga_entry_clearall)
    {
        md.entries.clear();
        set_dirty();
    }
    else if (ad.actiontype == vga_entry_setactive)
    {
        int i = ad.data.i[0];
        value_int = i;
        if ((i >= 0) && (i < md.entries.size()))
            label = md.entries[i].label;
        set_dirty();
    }
    else if (ad.actiontype == vga_intval)
    {
        for (int i = 0; i < md.entries.size(); i++)
        {
            if (md.entries[i].ad.data.i[0] == ad.data.i[0])
            {
                label = md.entries[i].label;
                value_int = i;
                set_dirty();
                return;
            }
        }
    }
}

int vg_optionmenu::get_n_parameters() { return 6; }
int vg_optionmenu::get_parameter_type(int id)
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
        return vgcp_string;
    }
    return 0;
}
string vg_optionmenu::get_parameter_name(int id)
{
    switch (id)
    {
    case 0:
        return "style";
    case 1:
        return "color";
    case 2:
        return "null";
    case 3:
        return "action";
    case 4:
        return "action_p1";
    case 5:
        return "action_p2";
    }
    return "-";
}
string vg_optionmenu::get_parameter_text(int id)
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
    {
        sprintf(t, "%x", color);
        return t;
    }
    case 2:
        return "";
    case 3:
        return action;
    case 4:
    case 5:
    {
        sprintf(t, "%i", action_p[id - 4]);
        return t;
    }
    }
    return "-";
}
void vg_optionmenu::set_parameter_text(int id, string text, bool no_refresh)
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
        break;
    case 3:
    {
        action = text;
        if (action.compare("filter") == 0)
            action_i = vga_filter;
        else
            action_i = vga_click;
    }
    break;
    case 4:
    case 5:
        action_p[id - 4] = atol(text.c_str());
        break;
    }
    if (!no_refresh)
    {
        set_dirty();
    }
}
