#include "vt_gui_controls.h"

#include <sampler_state.h>
#include <interaction_parameters.h>

using std::set;
using std::string;
using std::vector;

unsigned int framecol = 0xff000000;

/* thoughts

should support multiselect
vel-dragging (hi/lo)
note-dragging (lo/hi/root at the same time)

does not need to support ms:
key hi/lo

doubleclick to create new empty zone

ctrl-drag to clone
alt-click to delete

rmb up/dn drag on keybed to scroll up/dn

*/

enum
{
    cs_default = 0,
    cs_dragcorner,
    cs_selectionbox,
    cs_drag_selection,
    cs_keybed,
    cs_vdrag,
    cs_drag_dropfiles,
    cs_playzone,

    mode_list = 0,
    mode_pad,
};

vg_list::vg_list(vg_window *o, int id, TiXmlElement *e) : vg_control(o, id, e)
{
    restore_data(e);

    for (int i = 0; i < 16; i++)
        bmpdata[i] = owner->get_art(vgct_list, 0, i);

    refresh();

    mode = mode_list;

    zoomlevel = 0; // 0 = 13 px/row, 1 = 7 px/row
    hover_id = -1;
    mouse_inside = false;
    selected_id = -1;
    hover_id_corners = 0;
    selected_id_corners = 0;
    firstkey = 36;
    memset(keyvel, 0, 128 * sizeof(char));
    noteh = 13;

    controlstate = 0;
}
vg_list::~vg_list() {}

void vg_list::on_resize()
{
    r = vg_rect(0, 0, surf.sizeX, surf.sizeY);
    rect_padmargin = r;
    rect_padmargin.inset(3, 1);
    rect_keybed = r;
    rect_keybed.x2 = bmpdata[0].r.get_w();
    rect_zonebed = r;
    rect_zonebed.x = bmpdata[0].r.get_w();

    pad_y_split = 4;
    pad_dx = rect_padmargin.get_w() >> 2;
    pad_dy = (rect_padmargin.get_h() - 3 * pad_y_split) >> 4;
}

void vg_list::refresh()
{
    actiondata ad;
    ad.actiontype = vga_request_refresh;
    owner->post_action(ad, this);
}

int vg_list::xy_to_key(int x, int y)
{
    if (mode == mode_pad)
    {
        int dy = (rect_padmargin.y2 - y);
        dy -= (dy / (pad_dy * 4 + pad_y_split)) * pad_y_split;
        return (((dy / pad_dy) * 4 + min(3, x / pad_dx) + firstkey) & 0xff);
    }
    else
        return ((((surf.sizeY - y) / noteh) + firstkey) & 0xff);
}

void vg_list::toggle_zoom(bool toggle)
{
    if (toggle)
        zoomlevel = zoomlevel ? 0 : 1;
    noteh = zoomlevel ? 7 : 13;
}

void vg_list::draw_pad(int key, const char *labeloverride)
{
    if ((key < firstkey) || (key > (firstkey + 63)))
        return;

    int col = (key - firstkey) & 0x3;
    int row = 15 - ((key - firstkey) >> 2);

    vg_rect r3;
    r3.x = rect_padmargin.x + col * pad_dx;
    r3.x2 = r3.x + pad_dx;
    r3.y = rect_padmargin.y + row * pad_dy + pad_y_split * (row >> 2);
    r3.y2 = r3.y + pad_dy;
    r3.inset(1, 1);
    r3.bound(r);

    int subimage = 12;
    vector<vg_list_zone>::iterator z;
    for (z = zones.begin(); z != zones.end(); z++)
    {
        if ((key == z->keyroot))
        {
            int j = z - zones.begin();
            subimage = 4;
            if (z->playing)
                subimage = 9;
            else if (selected_id == j)
                subimage = 6;
            else if (multiselect.find(j) != multiselect.end())
                subimage = 7;
            else if (hover_id == j)
                subimage = 5;
            z->visible = true;
            z->r = r3;
            break;
        }
    }

    if (z == zones.end())
    {
        if (keyvel[key])
            subimage = 15;
    }
    if (labeloverride)
        subimage = 13;
    surf.fill_rect(r3, owner->get_syscolor(col_list_bg));
    surf.borderblit(r3, bmpdata[subimage]);
    vg_rect r4 = r3;
    r4.inset(3, 3);
    if (labeloverride)
        surf.draw_text(r4, labeloverride, owner->get_syscolor(col_list_text_unassigned));
    else if (z != zones.end())
        surf.draw_text(r4, z->name, owner->get_syscolor(col_list_text), -1, 1);
    else
        surf.draw_text(r4, dmode_int_to_str(key, 2, ""),
                       owner->get_syscolor(col_list_text_unassigned));
    r3.inset(3, 0);
}

void vg_list::draw_list_key(int key)
{
    int y = (key - firstkey) * noteh;

    vg_bitmap keybedk = bmpdata[2 * zoomlevel];
    keybedk.r.y2 -= noteh * (key % 12);
    keybedk.r.y = keybedk.r.y2 - noteh;

    vg_rect r2 = rect_keybed;
    r2.y2 = r.get_h() - y;
    r2.y = r2.y2 - noteh;

    if (keyvel[key])
    {
        keybedk.r.offset(keybedk.r.get_w(), 0);
    }

    surf.blit(r2, keybedk);
    // surf.fill_rect(r2,(keyvel[key]<<25)&0xff000000 | 0x00ff0000);

    if ((key % 12) == 0)
    {
        r2.x = r2.x2 - noteh * 2;
        r2.x2 -= 5;
        // r2.offset(0,noteh);
        r2.y = min(r2.y, r2.y2 - 10);
        string temp;
        int oct = (key / 12) - 2;
        if (oct < 0)
            temp += '-';
        temp += '0' + abs(oct);
        surf.draw_text(r2, temp, owner->get_syscolor(col_list_text_keybed), 1, 0);
    }
}

// void draw_list_zone(lowkey, highkey, lowvel, highvel etc..

void vg_list::draw_list_zone(vg_list_zone *i, int j)
{
    int endkey = firstkey + (r.get_h() / max(1, noteh));

    if ((endkey > i->keylo) && (firstkey <= i->keyhi))
    {
        vg_rect r2 = rect_zonebed;
        r2.y2 = rect_zonebed.y2 - (i->keylo - firstkey) * noteh;
        r2.y = rect_zonebed.y2 - (i->keyhi + 1 - firstkey) * noteh - 1;
        int w = r2.get_w();
        r2.x2 = r2.x + (((1 + i->velhi) * w) >> 7) + 1;
        r2.x += ((i->vello * w) >> 7);

        i->r = r2;

        int subimage = 4;
        if (j < 0)
            subimage = 13; // drop from browser
        else if (i->playing)
            subimage = 9;
        else if (selected_id == j)
            subimage = 6;
        else if (multiselect.find(j) != multiselect.end())
            subimage = 7;
        else if (hover_id == j)
            subimage = 5;

        r2.bound(rect_zonebed);
        surf.borderblit(r2, bmpdata[subimage]);

        if ((selected_id == j))
        {
            update_dragcorners();
            if (selected_id_corners == 8)
                surf.borderblit(r2, bmpdata[10]);
            else if (selected_id_corners == 2)
                surf.borderblit(r2, bmpdata[11]);
        }

        r2.inset(3, 0);
        if ((r2.get_h() > 8) && (r2.get_w() > 8))
            surf.draw_text(r2, i->name,
                           i->mute ? owner->get_syscolor(col_standard_text_disabled)
                                   : owner->get_syscolor(col_list_text));

        i->visible = true;
    }
    else
        i->visible = false;
}

void vg_list::draw()
{
    draw_needed = false;
    toggle_zoom(false); // update note variable
    // draw_background();

    if (mode == mode_pad)
    {
        surf.fill_rect(r, owner->get_syscolor(col_list_bg));
        for (int i = 0; i < 128; i++)
        {
            draw_pad(i);
        }
    }
    else
    {
        // draw background for note-area
        surf.fill_rect(rect_zonebed, owner->get_syscolor(col_list_bg));

        int endkey = firstkey + (r.get_h() / noteh);

        for (int i = firstkey; i <= endkey; i++)
        {
            draw_list_key(i);
        }

        for (int i = 0; i < zones.size(); i++)
        {
            draw_list_zone(&zones.at(i), i);
        }
    }
    if (controlstate == cs_selectionbox)
    {
        vg_rect t = select_rect;
        t.flip_if_needed();
        surf.draw_rect(t, 0x600000ff);
        t.inset(1, 1);
        surf.fill_rect(t, 0x200000ff);
    }
    else if (controlstate == cs_drag_dropfiles)
    {
        if (mouse_inside)
        {
            vector<dropfile>::iterator i;
            for (i = owner->dropfiles.begin(); i != owner->dropfiles.end(); i++)
            {
                if (mode == mode_pad)
                {
                    draw_pad(i->key_root, i->label.c_str());
                }
                else
                {
                    vg_list_zone z;
                    z.keylo = i->key_lo;
                    z.keyhi = i->key_hi;
                    z.keyroot = i->key_root;
                    z.vello = i->vel_lo;
                    z.velhi = i->vel_hi;
                    z.name = i->label;
                    z.playing = false;
                    z.mute = false;
                    draw_list_zone(&z, -1);
                }
            }
        }
    }
}

void vg_list::update_dragcorners()
{
    if ((selected_id >= 0) && (selected_id < zones.size()))
    {
        int sz = 3;

        dragcorners[8] = zones[selected_id].r;
        dragcorners[8].y2 = dragcorners[8].y + sz;
        dragcorners[2] = zones[selected_id].r;
        dragcorners[2].y = dragcorners[2].y2 - sz;
    }
}

std::vector<int> vg_list::get_zones_at(vg_point p)
{
    std::vector<int> get;

    int n = zones.size();
    int j = 0;
    for (int i = 0; i < n; i++)
    {
        if (zones[i].visible && zones[i].r.point_within(p))
        {
            get.push_back(i);
        }
    }
    return get;
}

void vg_list::processevent(vg_controlevent &e)
{
    vg_point p(e.x, e.y);

    int n = zones.size();
    if (controlstate == cs_keybed) // note playing
    {
        if (e.eventtype == vget_mousemove)
        {
            int key = xy_to_key(e.x, e.y);

            if (lastkey != key)
            {
                // send note-off
                actiondata ad;
                ad.id = control_id;
                ad.subid = 0;
                ad.actiontype = vga_note;
                ad.data.i[0] = lastkey;
                ad.data.i[1] = 0;
                owner->post_action(ad, 0);

                // send note-on
                ad.data.i[0] = key;
                ad.data.i[1] = 100;
                owner->post_action(ad, 0);

                lastkey = key;
            }
        }
        else if (e.eventtype == vget_mouseup)
        {
            controlstate = cs_default;
            owner->drop_focus(control_id);

            // send note-off
            actiondata ad;
            ad.id = control_id;
            ad.subid = 0;
            ad.actiontype = vga_note;
            ad.data.i[0] = lastkey;
            ad.data.i[1] = 0;
            owner->post_action(ad, 0);

            set_dirty();
        }
    }
    else if (controlstate == cs_drag_dropfiles)
    {
        if (e.eventtype == vget_mouseenter)
        {
            mouse_inside = true;
            set_dirty();
        }
        else if (e.eventtype == vget_mouseleave)
        {
            mouse_inside = false;
            set_dirty();
        }
        else if (e.eventtype == vget_dragfiles)
        {
            mouse_inside = true;
            int mde =
                ((mode == mode_list) && rect_keybed.point_within(vg_point((int)e.x, (int)e.y)))
                    ? 2
                    : ((e.buttonmask & vgm_control) ? 1 : 0);
            owner->dropfiles_process(mde, xy_to_key(e.x, e.y));
            set_dirty();
        }
        else if (e.eventtype == vget_dropfiles)
        {
            actiondata ad;
            ad.id = parameter_id;
            ad.subid = parameter_subid;
            ad.actiontype = vga_load_dropfiles;
            owner->post_action(ad, 0);
            controlstate = cs_default;
            set_dirty();
        }
        else
            controlstate = cs_default;
    }
    else if (controlstate == cs_selectionbox) // selection
    {
        if (e.eventtype == vget_mousemove)
        {
            select_rect.x2 = e.x;
            select_rect.y2 = e.y;
            vg_rect t = select_rect;
            t.flip_if_needed();
            multiselect.clear();
            for (int i = 0; i < n; i++)
            {
                if (zones[i].visible && t.does_intersect(zones[i].r))
                {
                    multiselect.insert(i);
                    if (selected_id < 0)
                        selected_id = i;
                }
            }
            if ((e.buttonmask & vgm_control) &&
                (multiselect.find(selected_id) == multiselect.end()))
                selected_id = -1;
            set_dirty();
        }
        else if (e.eventtype == vget_mouseup)
        {
            controlstate = cs_default;
            owner->drop_focus(control_id);

            actiondata ad;

            if ((selected_id >= 0) && (selected_id < n))
            {
                ad.actiontype = vga_select_zone_primary;
                ad.id = ip_kgv_or_list;
                ad.data.i[0] = zones[selected_id].id; // zone
                owner->post_action(ad, 0);

                for (int i = 0; i < n; i++)
                {
                    if (multiselect.find(i) != multiselect.end())
                    {
                        ad.actiontype = vga_select_zone_secondary;
                        ad.id = ip_kgv_or_list;
                        ad.data.i[0] = zones[i].id; // zone
                        owner->post_action(ad, 0);
                    }
                }
            }
            else
            {
                // deselect?
                ad.actiontype = vga_select_zone_clear;
                ad.id = ip_kgv_or_list;
                owner->post_action(ad, 0);
            }

            refresh();
        }
    }
    else if (controlstate == cs_drag_selection) // selection dragging
    {
        if (e.eventtype == vget_mousemove)
        {
            moved_since_mdown = true;
            if (e.buttonmask & vgm_control)
                owner->set_cursor(cursor_copy);
            else
                owner->set_cursor(cursor_move);

            int diffX = 0, diffY = 0;
            if (mode == mode_pad)
            {
                diffX = (e.x - lastmouseloc.x) / pad_dx;
                diffY = (e.y - lastmouseloc.y) / pad_dy;
            }
            else
                diffY = (e.y - lastmouseloc.y) / noteh;
            if (diffX || diffY)
            {
                int diff;
                if (mode == mode_pad)
                {
                    diff = -diffY * 4 + diffX;
                    lastmouseloc.x += pad_dx * diffX;
                    lastmouseloc.y += pad_dy * diffY;
                }
                else
                {
                    diff = -diffY;
                    lastmouseloc.y += noteh * diffY;
                }
                for (int i = 0; i < n; i++)
                {
                    if (multiselect.find(i) != multiselect.end())
                    {
                        zones[i].keylo += diff;
                        zones[i].keyhi += diff;
                        zones[i].keyroot += diff;
                        zones[i].keylo = max(0, min(127, zones[i].keylo));
                        zones[i].keyhi = max(0, min(127, zones[i].keyhi));
                        zones[i].keyroot = max(0, min(127, zones[i].keyroot));
                    }
                }
                set_dirty();
            }
        }
        else if (e.eventtype == vget_mouseup)
        {
            controlstate = cs_default;
            owner->drop_focus(control_id);

            if (moved_since_mdown)
            {
                for (int i = 0; i < n; i++)
                {
                    if (multiselect.find(i) != multiselect.end())
                    {
                        actiondata ad;
                        if (e.buttonmask & vgm_control)
                            ad.actiontype = vga_set_zone_keyspan_clone;
                        else
                            ad.actiontype = vga_set_zone_keyspan;
                        ad.id = ip_kgv_or_list;
                        ad.data.i[0] = zones[i].id; // zone
                        ad.data.i[1] = zones[i].keylo;
                        ad.data.i[2] = zones[i].keyroot;
                        ad.data.i[3] = zones[i].keyhi;
                        owner->post_action(ad, 0);
                    }
                }
                refresh();
            }
            else
            {
                // no drag, select (only) the clicked zone
                for (int i = 0; i < n; i++)
                {
                    if (zones[i].visible && zones[i].r.point_within(vg_point(e.x, e.y)))
                    {
                        set_selected_id(i);
                        break;
                    }
                }
            }
        }
    }
    else if (controlstate == cs_dragcorner) // corner dragging
    {
        if (selected_id_corners)
        {
            if (e.eventtype == vget_mousemove)
            {
                owner->set_cursor(cursor_sizeNS);
                if ((selected_id >= 0) && (selected_id < n))
                {
                    int vdir = (selected_id_corners == 2) ? -1 : 1;
                    int newkey = ((surf.sizeY - e.y - vdir * (noteh >> 1)) / noteh) + firstkey;
                    newkey = max(firstkey, min(newkey, 127));
                    // zones[selected_id].keylo
                    if (vdir < 0)
                        zones[selected_id].keylo = newkey;
                    else if (vdir > 0)
                        zones[selected_id].keyhi = newkey;

                    if (zones[selected_id].keylo > zones[selected_id].keyhi)
                    {
                        if (vdir < 0)
                            zones[selected_id].keyhi = zones[selected_id].keylo;
                        else
                            zones[selected_id].keylo = zones[selected_id].keyhi;
                    }

                    int sz = 4;
                    dragcorners[8] = zones[selected_id].r;
                    dragcorners[8].y2 = dragcorners[8].y + sz;
                    dragcorners[2] = zones[selected_id].r;
                    dragcorners[2].y = dragcorners[2].y2 - sz;

                    set_dirty();
                }
            }
            else if ((e.eventtype == vget_mouseup) || (e.eventtype == vget_mouseleave))
            {
                selected_id_corners = 0;
                controlstate = cs_default;
                owner->drop_focus(control_id);

                actiondata ad;
                ad.actiontype = vga_set_zone_keyspan;
                ad.id = ip_kgv_or_list;
                ad.data.i[0] = zones[selected_id].id; // zone
                ad.data.i[1] = zones[selected_id].keylo;
                ad.data.i[2] = zones[selected_id].keyroot;
                ad.data.i[3] = zones[selected_id].keyhi;
                owner->post_action(ad, 0);
                refresh();
            }
        }
        else
            controlstate = cs_default;
    }
    else if (controlstate == cs_vdrag)
    {
        if (e.eventtype == vget_mousemove)
        {
            owner->set_cursor(cursor_move);

            int diff = (e.y - lastmouseloc.y) / noteh;
            if (diff)
            {
                lastmouseloc.y += noteh * diff;

                firstkey += diff;
                firstkey = max(0, min(127 - (surf.sizeY / noteh), firstkey));

                set_dirty();
            }
        }
        else if (e.eventtype == vget_mouseup)
        {
            controlstate = cs_default;
            owner->drop_focus(control_id);
            set_dirty();
        }
    }
    else
    { // default
        if (e.eventtype == vget_mousewheel)
        {
            int dir = (e.eventdata[0] > 1) ? 4 : -4;
            firstkey = max(0, min(127 - (surf.sizeY / noteh), (firstkey + dir))) & 0xfc;
            set_dirty();
        }
        else if (e.eventtype == vget_dragfiles)
        {
            mouse_inside = true;
            controlstate = cs_drag_dropfiles;
            owner->dropfiles_process(0, xy_to_key(e.x, e.y));
            set_dirty();
        }
        else if (e.eventtype == vget_dropfiles) // drop without previous drag
        {
            owner->dropfiles_process(0, xy_to_key(e.x, e.y));

            actiondata ad;
            ad.actiontype = vga_load_dropfiles;
            owner->post_action(ad, this);
            controlstate = cs_default;
            set_dirty();
        }
        else if ((e.eventtype == vget_mousedown) || (e.eventtype == vget_mousedblclick))
        {
            if (e.activebutton == 1)
            {
                bool found = false;
                if ((mode == mode_list) && rect_keybed.point_within(p))
                {
                    controlstate = cs_keybed;
                    int key = xy_to_key(e.x, e.y);
                    lastkey = key;

                    // send note
                    actiondata ad;
                    ad.id = control_id;
                    ad.subid = 0;
                    ad.actiontype = vga_note;
                    ad.data.i[0] = key;
                    ad.data.i[1] = 100;
                    owner->post_action(ad, 0);

                    owner->take_focus(control_id);
                    set_dirty();
                }
                else
                {
                    for (int i = 0; i < n; i++)
                    {
                        if (zones[i].visible && zones[i].r.point_within(p))
                        {
                            found = true;

                            if ((selected_id == i) && (multiselect.size() < 2) &&
                                (dragcorners[2].point_within(p)))
                            {
                                selected_id_corners = 2;
                                owner->set_cursor(cursor_sizeNS);
                                controlstate = cs_dragcorner;
                                owner->take_focus(control_id);
                                set_dirty();
                            }
                            else if ((selected_id == i) && (multiselect.size() < 2) &&
                                     (dragcorners[8].point_within(p)))
                            {
                                selected_id_corners = 8;
                                owner->set_cursor(cursor_sizeNS);
                                controlstate = cs_dragcorner;
                                owner->take_focus(control_id);
                                set_dirty();
                            }
                            else
                            {
                                selected_id_corners = 0;
                                if (multiselect.find(i) != multiselect.end())
                                {
                                    // initiate drag
                                    controlstate = cs_drag_selection;
                                    moved_since_mdown = false;
                                    lastmouseloc.x = e.x;
                                    lastmouseloc.y = e.y;
                                    owner->take_focus(control_id);
                                    set_dirty();
                                }
                                else
                                {
                                    if (e.buttonmask & vgm_control)
                                    {
                                        multiselect.insert(i);
                                        actiondata ad;
                                        ad.actiontype = vga_select_zone_secondary;
                                        ad.data.i[0] = zones[i].id;
                                        owner->post_action(ad, this);
                                    }
                                    else
                                    {
                                        set_selected_id(i);
                                    }
                                }
                            }
                            break;
                        }
                    }
                    if (!found)
                    {
                        if (!(e.buttonmask & vgm_control))
                        {
                            set_selected_id(-1);
                        }
                        owner->take_focus(control_id);
                        select_rect.x = e.x;
                        select_rect.y = e.y;
                        select_rect.x2 = e.x;
                        select_rect.y2 = e.y;
                        controlstate = cs_selectionbox;
                    }
                }
            }
            else if ((e.activebutton == 2) && rect_keybed.point_within(p) && (mode == mode_list))
            {
                controlstate = cs_vdrag;
                owner->take_focus(control_id);
                lastmouseloc.x = e.x;
                lastmouseloc.y = e.y;
            }
            else if ((e.activebutton == 3) && (mode == mode_list))
            {
                int oldkey = xy_to_key(e.x, e.y);
                toggle_zoom(true);
                int newkey = xy_to_key(e.x, e.y);
                firstkey -= (newkey - oldkey);
                firstkey = max(0, min(127 - (surf.sizeY / noteh), firstkey));
                set_dirty();
            }
        }
        /*else if(e.eventtype == vget_mousedblclick)
        {
                int key = e.y;
                key = y_to_key(key);
                lastkey = key;

                actiondata ad;
                ad.id = control_id;
                ad.subid = 0;
                ad.actiontype = vga_createemptyzone;
                ad.data.i[0] = key;
                owner->post_action(ad);
        }*/
        else if (e.eventtype == vget_mouseup)
        {
            if (e.activebutton == 2)
            {
                // TODO if the zone is not in the selection, add the selection to the zone

                // fill menu
                md.entries.clear();
                vg_menudata_entry mde;
                mde.submenu = 0;
                mde.label = "Mode";
                mde.ad.actiontype = vga_nothing;
                md.entries.push_back(mde);
                if (mode == mode_list)
                    mde.label = "Switch to pad-view";
                else
                    mde.label = "Switch to key-view";
                mde.ad.actiontype = vga_zonelist_mode;
                mde.ad.data.i[0] = 1 - mode;
                md.entries.push_back(mde);

                std::vector<int> sel = get_zones_at(p);
                if (!sel.empty())
                {
                    mde.label = "Select";
                    mde.ad.actiontype = vga_nothing;
                    md.entries.push_back(mde);
                    mde.ad.actiontype = vga_select_zone_primary;
                    for (int i = 0; i < sel.size(); i++)
                    {
                        mde.label = zones[sel[i]].name;
                        mde.ad.data.i[0] = zones[sel[i]].id;
                        md.entries.push_back(mde);
                    }
                }

                if (selected_id >= 0)
                {
                    mde.submenu = 0;
                    if (multiselect.size() > 1)
                        mde.label = "Zones";
                    else
                        mde.label = "Zone";
                    mde.ad.actiontype = vga_nothing;
                    md.entries.push_back(mde);

                    mde.ad.actiontype = vga_deletezone;
                    if (multiselect.size() > 1)
                        mde.label = "Delete selected Zones";
                    else
                        mde.label = "Delete selected Zone";
                    md.entries.push_back(mde);
                    /*mde.ad.actiontype = vga_clonezone;
                    mde.label = "clone zone";	 md.entries.push_back(mde);
                    mde.ad.actiontype = vga_clonezone_next;
                    mde.label = "clone zone (next key)";	 md.entries.push_back(mde);*/

                    vg_menudata_entry mdesub;

                    mde.ad.actiontype = 0;
                    mde.submenu = &mdsub_part;
                    mde.label = "Move to Part";
                    md.entries.push_back(mde);
                    mde.submenu = &mdsub_layer;
                    mde.label = "Move to Layer";
                    md.entries.push_back(mde);
                    mde.submenu = 0;

                    mde.ad.actiontype = vga_movezonetopart;
                    mdsub_part.entries.clear();
                    for (int i = 0; i < 16; i++)
                    {
                        mde.ad.data.i[0] = i;
                        char ch[16];
                        sprintf(ch, "%i", i + 1);
                        mde.label = ch;
                        mdsub_part.entries.push_back(mde);
                    }
                    mde.ad.actiontype = vga_movezonetolayer;
                    mdsub_layer.entries.clear();
                    for (int i = 0; i < num_layers; i++)
                    {
                        mde.ad.data.i[0] = i;
                        char ch[2];
                        ch[0] = 'A' + i;
                        ch[1] = 0;
                        mde.label = ch;
                        mdsub_layer.entries.push_back(mde);
                    }
                }

                actiondata ad;
                ad.id = control_id;
                ad.subid = 0;
                ad.data.ptr[0] = ((void *)&md);
                ad.data.i[2] = false; // doesn't need destruction
                ad.data.i[3] = location.x + e.x;
                ad.data.i[4] = location.y + e.y;
                ad.actiontype = vga_menu;
                owner->post_action(ad, 0);
            }
        }
        else if (e.eventtype == vget_mouseleave)
        {
            set_hover_id(-1);
        }
        else if (e.eventtype == vget_mousemove)
        {
            owner->set_cursor(cursor_arrow);

            for (int i = 0; i < zones.size(); i++)
            {
                bool found = false;
                vg_point p(e.x, e.y);

                if (zones[i].visible && zones[i].r.point_within(p))
                {
                    set_hover_id(i);

                    if (multiselect.find(i) != multiselect.end())
                        owner->set_cursor(cursor_move);

                    if ((hover_id == selected_id) && (multiselect.size() < 2))
                    {
                        if (dragcorners[2].point_within(p))
                        {
                            owner->set_cursor(cursor_sizeNS);
                        }
                        else if (dragcorners[8].point_within(p))
                        {
                            owner->set_cursor(cursor_sizeNS);
                        }
                    }

                    found = true;
                    break;
                }
                if (!found)
                {
                    set_hover_id(-1);
                    owner->set_cursor(cursor_arrow);
                }
            }
        }
    }
}

void vg_list::set_hover_id(int i)
{
    if (i != hover_id)
    {
        hover_id = i;
        set_dirty();
    }
}
void vg_list::set_selected_id(int i)
{
    if (i != selected_id)
    {
        selected_id = i;
        multiselect.clear();
        multiselect.insert(i);
        hover_id = i;
        set_dirty();

        actiondata ad;
        ad.id = control_id;
        if ((i >= 0) && (i < zones.size()))
        {
            ad.actiontype = vga_select_zone_primary;
            ad.data.i[0] = zones[i].id;
        }
        else
        {
            ad.actiontype = vga_select_zone_clear;
        }
        owner->post_action(ad, 0);
    }
}

void vg_list::post_action(actiondata ad)
{
    switch (ad.actiontype)
    {
    case vga_toggle_zoom:
        toggle_zoom(true);
        set_dirty();
        break;
    case vga_zonelist_clear:
        zones.clear();
        multiselect.clear();
        selected_id = -1;
        break;
    case vga_zonelist_populate:
    {
        ad_zonedata *z = (ad_zonedata *)&ad;

        vg_list_zone nz;
        nz.channel = z->part;
        nz.name = z->name;
        nz.id = z->zid;
        nz.keylo = z->keylo;
        nz.keyhi = z->keyhi;
        nz.keyroot = z->keyroot;
        nz.vello = z->vello;
        nz.velhi = z->velhi;
        nz.keylofade = z->keylofade;
        nz.keyhifade = z->keyhifade;
        nz.mute = z->mute;
        nz.playing = false;
        int i = zones.size();
        if (z->is_active)
            selected_id = i;
        if (z->is_selected)
            multiselect.insert(i);
        zones.push_back(nz);
    }
    break;
    case vga_zonelist_done:
        set_dirty();
        break;
    case vga_zonelist_mode:
        mode = ad.data.i[0];
        set_dirty();
        break;
    case vga_zone_playtrigger:
        for (int i = 0; i < zones.size(); i++)
        {
            if (zones[i].id == ad.data.i[0])
            {
                zones[i].playing = ad.data.i[1];
                redraw_zone(i);
            }
        }
        break;
    case vga_note:
    {
        keyvel[ad.data.i[0] & 0x7f] = ad.data.i[1];
        redraw_key(ad.data.i[0]);
    }
    break;
    }
}
void vg_list::redraw_zone(int i)
{
    if (mode == mode_list)
        draw_list_zone(&zones.at(i), i);
    else
    {
        draw_pad(zones[i].keyroot);
    }

    // Set control dirty but don't redraw the rest of it
    owner->set_child_dirty(control_id);
}
void vg_list::redraw_key(int i)
{
    if (mode == mode_list)
        draw_list_key(i);
    else
        draw_pad(i);

    owner->set_child_dirty(control_id);
}
