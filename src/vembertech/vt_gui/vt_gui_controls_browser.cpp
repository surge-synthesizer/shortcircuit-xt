#include "vt_gui_controls.h"
#include "browserdata.h"
#include "interaction_parameters.h"

#include <vt_util/vt_string.h>
using std::set;
using std::vector;

vg_browser::vg_browser(vg_window *o, int id, TiXmlElement *e) : vg_control(o, id, e)
{
    restore_data(e);
    set_style(0);
    split = 220;

    categorylist = 0;
    patchlist = 0;

    is_refreshing = false;

    category = 0;
    entry = -1;
    controlstate = 0;
    database_type = 0;

    mNumVisiblePEntries = 0;
}

void vg_browser::set_style(int s)
{
    style = s;

    sb_tray = owner->get_art(vgct_browser, style, 0);
    sb_head = owner->get_art(vgct_browser, style, 1);
    divider = owner->get_art(vgct_browser, style, 2);
    folder = owner->get_art(vgct_browser, style, 3);
    file[0] = owner->get_art(vgct_browser, style, 4);
    file[1] = owner->get_art(vgct_browser, style, 5);
}

vg_browser::~vg_browser() {}

// promote to parent class? (available to all controls, not as control)
void vg_browser::draw_scrollbar(int id, vg_rect r, int n_visible, int n_entries, bool horizontal)
{
    surf.borderblit(r, sb_tray);
    scroll_rect[id] = r;
    scroll_rect_head[id] = r;

    if (!n_entries)
        return;

    vg_rect r2 = r;

    int a = max(0, min(scroll_pos[id], n_entries - n_visible));
    int b = min(n_visible, n_entries);

    scroll_pos[id] = a;

    if (horizontal)
    {
        r2.x = r.x + ((((a * r.get_w()) << 8) / n_entries) >> 8);
        r2.x2 = r2.x + ((((b * r.get_w()) << 8) / n_entries) >> 8);
    }
    else
    {
        r2.y = r.y + ((((a * r.get_h()) << 8) / n_entries) >> 8);
        r2.y2 = r2.y + ((((b * r.get_h()) << 8) / n_entries) >> 8);
    }
    scroll_rect_head[id] = r2;
    // surf.borderblit(r2,sb_head);
    r2.inset(2, 2);
    r2.y2 = max(r2.y + 3, r2.y2);
    surf.fill_rect(r2, owner->get_syscolor(col_frame));
}

void vg_browser::add_category_to_list(int id)
{
    bool selected = (id == category);

    pp_catrect e;
    e.category_entry_id = id;
    e.id = 0;
    // e.r = textrow;
    // e.r =
    e.invert = selected;
    e.indent = categorylist->at(id).depth;
    crect.push_back(e);
}

void vg_browser::update_lists()
{
    if (!categorylist)
        return;
    if (!patchlist)
        return;

    crect.clear();
    prect.clear();

    category = max(0, min(category, categorylist->size() - 1));
    entry = max(0, min(entry, patchlist->size() - 1));

    int c = category;
    int i = 0, row = 0;

    add_category_to_list(0); // draw root

    if (categorylist->at(category).parent >= 0)
    {
        c = categorylist->at(c).parent;
        if ((categorylist->at(category).child < 0) && (categorylist->at(c).parent >= 0))
        {
            c = categorylist->at(c).parent;
        }
    }
    if (c)
        add_category_to_list(c);

    int d = categorylist->at(c).child;
    while (d >= 0)
    {
        add_category_to_list(d);
        if (d == category)
        {
            int e = categorylist->at(d).child;
            while (e >= 0)
            {
                add_category_to_list(e);
                e = categorylist->at(e).sibling;
            }
        }
        else if (d == categorylist->at(category).parent)
        {
            int e = categorylist->at(d).child;
            while (e >= 0)
            {
                add_category_to_list(e);
                e = categorylist->at(e).sibling;
            }
        }
        d = categorylist->at(d).sibling;
    }

    int startp = categorylist->at(category).patch_id_start;
    int endp = categorylist->at(category).patch_id_end;

    scroll_pos[1] = 0;
    entry = -1;

    for (int p = startp; p < endp; p++)
    {
        if (p > patchlist->size())
            break;

        bool exclude = false;
        if (searchstring.size())
        {
            char temp[256], temp2[256];
            vtCopyString(temp, patchlist->at(p).name, 256);
            strlwr(temp);
            vtCopyString(temp2, searchstring.c_str(), 256);
            strlwr(temp2);
            if (!strstr(temp, temp2))
                exclude = true;
        }
        if (!exclude)
        {
            pp_dstrect e;
            e.id = p;
            e.invert = (p == entry);
            prect.push_back(e);
        }
    }
    set_dirty();
}

void vg_browser::draw()
{
    draw_needed = false;

    int sb_width = 12;
    int row_height = 12;
    int col_text = owner->get_syscolor(col_standard_text),
        col_bg = owner->get_syscolor(col_standard_bg),
        col_text_sel = owner->get_syscolor(col_selected_text),
        col_bg_sel = owner->get_syscolor(col_selected_bg);

    if (is_refreshing)
    {
        vg_rect r(0, 0, surf.sizeX, surf.sizeY);
        vg_rect tr = r;
        surf.draw_rect(r, owner->get_syscolor(col_frame), true);
        tr.inset(1, 1);
        surf.fill_rect(tr, col_bg, true);
        surf.draw_text(tr, "Refreshing database...", col_text);
        return;
    }

    vg_rect r(0, 0, surf.sizeX, surf.sizeY);
    vg_rect tr = r;
    tr.x2 -= sb_width - 1;
    surf.draw_rect(r, owner->get_syscolor(col_frame), true);
    tr.inset(1, 1);
    surf.fill_rect(tr, col_bg, true);

    tr = r;
    tr.y = split;
    tr.y2 = split + divider.r.get_h();
    surf.borderblit(tr, divider);

    b_upper = r;
    b_upper.y2 = tr.y + 1;

    b_lower = r;
    b_lower.y = tr.y2 - 1;

    int margin1 = 4, margin2 = 4;
    ;

    // upper
    tr = b_upper;
    b_upper.x2 -= sb_width;
    b_upper.inset(margin1, margin1);
    int n_visible = b_upper.get_h() / row_height;

    tr.x = tr.x2 - sb_width;
    draw_scrollbar(0, tr, n_visible, crect.size());

    int nr = 0;
    for (int i = scroll_pos[0]; i < crect.size(); i++)
    {
        vg_rect row = b_upper;
        row.y += row_height * nr;
        row.y2 = row.y + row_height;
        row.bound(b_upper);
        if (crect[i].invert)
            surf.fill_rect(row, col_bg_sel);
        row.inset(4, 0);
        row.x += crect[i].indent * 8;
        row.bound(b_upper);
        crect[i].r = row;
        vg_rect iconrect = row;
        iconrect.x2 = iconrect.x + folder.r.get_w();
        surf.blit_alphablend(iconrect, folder);
        row.x = iconrect.x2 + margin2;
        surf.draw_text(row, categorylist->at(crect[i].category_entry_id).name,
                       crect[i].invert ? col_text_sel : col_text, -1, 0);
        if (nr++ > n_visible)
            break;
    }

    // lower
    tr = b_lower;
    b_lower.x2 -= sb_width;
    b_lower.inset(margin1, margin1);
    mNumVisiblePEntries = b_lower.get_h() / row_height;

    tr.x = tr.x2 - sb_width;
    draw_scrollbar(1, tr, mNumVisiblePEntries, prect.size());

    nr = 0;
    int i;
    for (i = scroll_pos[1]; i < prect.size(); i++)
    {
        vg_rect row = b_lower;
        row.y += row_height * nr;
        row.y2 = row.y + row_height;
        bool sel = multiselect.find(i) != multiselect.end();

        row.bound(b_lower);
        if (sel)
            surf.fill_rect(row, col_bg_sel);
        row.inset(margin2, 0);
        row.bound(b_lower);
        prect[i].r = row;
        vg_rect iconrect = row;
        iconrect.x2 = iconrect.x + file[0].r.get_w();
        surf.blit_alphablend(iconrect, file[patchlist->at(prect[i].id).filetype & 1]);
        row.x = iconrect.x2 + margin2;
        vg_rect col2 = row;
        row.x2 -= 22;
        col2.x = row.x2;
        surf.draw_text(row, patchlist->at(prect[i].id).name, sel ? col_text_sel : col_text, -1, 0);
        surf.draw_text(col2, patchlist->at(prect[i].id).column2, sel ? col_text_sel : col_text, 0,
                       0);
        if (nr++ > mNumVisiblePEntries)
            break;
    }
    p_first = scroll_pos[1];
    p_last = i;
}

enum
{
    cs_default = 0,
    cs_dragsource,
    cs_scrollbar1,
    cs_scrollbar2,
};

void vg_browser::PreviewEntry(int Entry, bool AlwaysPreview)
{
    // Sample preview

    if ((entry >= 0) && (entry < prect.size()))
    {
        owner->dropfiles.clear();
        dropfile td;
        td.path = patchlist->at(prect[entry].id).path;
        owner->dropfiles.push_back(td);

        actiondata ad;
        ad.id = control_id;
        ad.data.i[0] = AlwaysPreview ? 1 : 0; // if 1, play regardless of AutoPreview setting
        ad.subid = 0;
        ad.actiontype = vga_browser_preview_start;
        owner->post_action(ad, 0);
    }
}

void vg_browser::SelectSingle(int NewEntry)
{
    entry = NewEntry;

    multiselect.clear();
    multiselect.insert(entry);

    // make sure selection is inside scroll window
    scroll_pos[1] = min(entry, scroll_pos[1]);
    scroll_pos[1] = max(entry - mNumVisiblePEntries, scroll_pos[1]);

    PreviewEntry(entry, false);

    set_dirty();
}

void vg_browser::processevent(vg_controlevent &e)
{
    vg_point p((int)e.x, (int)e.y);

    switch (controlstate)
    {
    case cs_dragsource:
    {
        if (e.eventtype == vget_mouseup)
        {
            owner->drop_focus(control_id);
            owner->set_cursor(cursor_arrow);
            controlstate = cs_default;
            // drop focus first so the control doesnt' catch this event

            if (moved_since_mdown > 5)
            {
                vg_controlevent e2;
                e2.eventtype = vget_dropfiles;
                e2.x = e.x + location.x;
                e2.y = e.y + location.y;
                e2.buttonmask = e.buttonmask;
                owner->processevent_user(e2);
            }
            else
            {
                SelectSingle(entry);
            }
        }
        else if (e.eventtype == vget_mousemove)
        {
            owner->set_cursor(cursor_copy);

            // this will not work with focus (bypass in eventloop)
            vg_controlevent e2;
            e2.eventtype = vget_dragfiles;
            e2.x = e.x + location.x;
            e2.y = e.y + location.y;
            e2.buttonmask = e.buttonmask;
            owner->processevent_user(e2);
            moved_since_mdown++;
        }
    }
    break;
    case cs_scrollbar1:
    case cs_scrollbar2:
        if (e.eventtype == vget_mouseup)
        {
            owner->drop_focus(control_id);
            controlstate = cs_default;
        }
        else if (e.eventtype == vget_mousemove)
        {
            int i = controlstate - cs_scrollbar1;
            float a = (e.y - scroll_rect[i].y);
            a /= max(1, scroll_rect[i].get_h());
            if (i == 1)
                a *= prect.size();
            else
                a *= crect.size();
            scroll_pos[i] = (int)a;
            set_dirty();
        }
        break;
    default:
    {
        if ((e.eventtype == vget_mousedown) && (e.activebutton == 1))
        {
            if (scroll_rect[0].point_within(p) || scroll_rect[1].point_within(p))
            {
                int i = scroll_rect[1].point_within(p) ? 1 : 0;
                controlstate = cs_scrollbar1 + i;
                owner->take_focus(control_id);
            }
            else if (b_upper.point_within(p))
            {
                for (int i = scroll_pos[0]; i < crect.size(); i++)
                {
                    if (crect[i].r.point_within(p))
                    {
                        category = crect[i].category_entry_id;
                        update_lists();
                    }
                }
            }
            else if (b_lower.point_within(p))
            {
                for (int i = p_first; i < min(p_last, prect.size()); i++)
                {
                    if (prect[i].r.point_within(p))
                    {
                        if (e.activebutton == 1)
                        {
                            if (e.buttonmask & vgm_shift)
                            {
                                if (entry >= 0)
                                {
                                    for (int j = min(entry, i); j <= max(entry, i); j++)
                                        multiselect.insert(j);
                                }
                            }
                            else if (e.buttonmask & vgm_control)
                            {
                                if (multiselect.find(i) != multiselect.end())
                                    multiselect.erase(i);
                                else
                                    multiselect.insert(i);
                            }
                            else
                            {
                                if ((multiselect.find(i) == multiselect.end()) ||
                                    (multiselect.size() < 2))
                                {
                                    multiselect.clear();
                                }
                                multiselect.insert(i);
                            }

                            entry = i;

                            actiondata ad;
                            ad.actiontype = vga_intval;
                            ad.data.i[0] = prect[i].id;
                            owner->post_action(ad, this);

                            set_dirty();

                            if (database_type == 1)
                            {
                                actiondata ad;
                                ad.actiontype = vga_load_patch;
                                owner->dropfiles.clear();
                                dropfile td;
                                td.path = patchlist->at(prect[i].id).path;
                                td.label = patchlist->at(prect[i].id).name;
                                td.database_id = prect[i].id;
                                td.db_type = database_type;
                                owner->dropfiles.push_back(td);
                                owner->dropfiles_process(0, 0);
                                owner->post_action(ad, this);
                            }
                            else if (owner->take_focus(control_id))
                            {
                                controlstate = cs_dragsource;
                                moved_since_mdown = 0;
                                if (e.buttonmask & (vgm_shift | vgm_control))
                                    moved_since_mdown = 0xffff; // Se till att multiselect inte töms
                                                                // vid shift & ctrl-klick
                                owner->dropfiles.clear();

                                set<int>::iterator it = multiselect.begin();
                                while (it != multiselect.end())
                                {
                                    int ee = *it;
                                    if ((ee >= 0) && (ee < prect.size()) && (prect[ee].id >= 0) &&
                                        (prect[ee].id < patchlist->size()))
                                    {
                                        dropfile td;
                                        td.path = patchlist->at(prect[ee].id).path;
                                        td.label = patchlist->at(prect[ee].id).name;
                                        td.database_id = prect[ee].id;
                                        td.db_type = database_type;
                                        owner->dropfiles.push_back(td);
                                    }
                                    ++it;
                                }

                                if (owner->dropfiles.empty())
                                {
                                    // abort if no files are dropped
                                    controlstate = cs_default;
                                    owner->drop_focus(control_id);
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (e.eventtype == vget_mousewheel)
        {
            const int slines = 15;
            if (b_lower.point_within(p))
            {
                if (e.eventdata[0] < 0)
                    scroll_pos[1] += slines;
                else
                    scroll_pos[1] -= slines;
                set_dirty();
            }
            else if (b_upper.point_within(p))
            {
                if (e.eventdata[0] < 0)
                    scroll_pos[0] += slines;
                else
                    scroll_pos[0] -= slines;
                set_dirty();
            }
        }
        else if ((e.eventtype == vget_mousedown) && (e.activebutton == 3))
        {
            PreviewEntry(entry, true);
        }
        else if ((e.eventtype == vget_mousedown) && (e.activebutton == 2))
        {
            // context menu

            md.entries.clear();
            vg_menudata_entry mde;
            mde.submenu = 0;
            mde.ad.actiontype = vga_click;
            mde.ad.id = ip_config_refresh_db;
            mde.ad.subid = 0;
            mde.label = "Refresh database";
            md.entries.push_back(mde);
            /*mde.ad.actiontype = vga_nothing;
            mde.label = "Zone";	 md.entries.push_back(mde);*/

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
    break;
    }
}
void vg_browser::post_action(actiondata ad)
{
    switch (ad.actiontype)
    {
    case vga_browser_listptr:
        categorylist = (vector<category_entry> *)ad.data.ptr[0];
        patchlist = (vector<patch_entry> *)ad.data.ptr[1];
        database_type = ad.data.i[4];
        category = 0;
        entry = -1;
        controlstate = 0;
        multiselect.clear();
        update_lists();
        break;
    case vga_text:
        searchstring = (char *)ad.data.str;
        update_lists();
        break;
    case vga_browser_category_child:
        if (categorylist->at(category).child >= 0)
            category = categorylist->at(category).child;
        update_lists();
        break;
    case vga_browser_category_parent:
        if (categorylist->at(category).parent >= 0)
            category = categorylist->at(category).parent;
        update_lists();
        break;
    case vga_browser_category_next:
        if (categorylist->at(category).sibling >= 0)
            category = categorylist->at(category).sibling;
        else
            category++;
        update_lists();
        break;
    case vga_browser_category_prev:
        // if(categorylist->at(category).sibling >= 0) category =
        // categorylist->at(category).sibling;
        category = max(0, category - 1);
        update_lists();
        break;
    case vga_browser_entry_next:
        SelectSingle(entry + 1);
        break;
    case vga_browser_entry_prev:
        SelectSingle(entry - 1);
        break;
    case vga_browser_entry_load:
    {
        actiondata ad;
        ad.actiontype = vga_load_patch;
        ad.id = control_id;
        owner->post_action(ad, this);
    }
    break;
    case vga_browser_is_refreshing:
        is_refreshing = ad.data.i[0];
        if (!is_refreshing)
            update_lists();
        set_dirty();
        break;
    };
}