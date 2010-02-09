/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "../../deadbeef.h"
#include <gtk/gtk.h>
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include <string.h>
#include "gtkui.h"
#include "interface.h"
#include "support.h"
#include "drawing.h"

static int margin_size = 10;
static int tab_dragging = -1;
static int tab_movepos;
static int tab_clicked = -1;

void
tabbar_draw (GtkWidget *widget) {
    GdkDrawable *backbuf = widget->window;
    int hscrollpos = 0;
    int x = -hscrollpos;
    int w = 0;
    int h = widget->allocation.height;
    const char *detail = "button";
    int cnt = deadbeef->plt_get_count ();
    int tab_selected = deadbeef->plt_get_curr ();

    // fill background
    gdk_draw_rectangle (backbuf, widget->style->bg_gc[GTK_STATE_NORMAL], TRUE, 0, 0, widget->allocation.width, widget->allocation.height);
    draw_begin ((uintptr_t)backbuf);
    int need_draw_moving = 0;
    int idx;
    int widths[cnt];
    int fullwidth = 0;
    for (idx = 0; idx < cnt; idx++) {
        const char *title = deadbeef->plt_get_title (idx);
        widths[idx] = 0;
        int h = 0;
        draw_get_text_extents (title, strlen (title), &widths[idx], &h);
        widths[idx] += margin_size + 10;
        fullwidth += widths[idx];
    }
    fullwidth += margin_size;

    x = -hscrollpos + fullwidth;
    for (idx = cnt-1; idx >= 0; idx--) {
        w = widths[idx];
        x -= w + margin_size;
        GdkRectangle area;
        area.x = x;
        area.y = 0;
        area.width = w + margin_size;
        area.height = 24;
        if (idx != tab_selected) {
            gtk_paint_box (widget->style, widget->window, idx == tab_selected ? GTK_STATE_PRELIGHT : GTK_STATE_NORMAL, GTK_SHADOW_OUT, &area, widget, "button", x, idx == tab_selected ? 0 : 1, w+margin_size, 32);
            GdkColor *gdkfg = &widget->style->fg[0];
            float fg[3] = {(float)gdkfg->red/0xffff, (float)gdkfg->green/0xffff, (float)gdkfg->blue/0xffff};
            draw_set_fg_color (fg);
            const char *tab_title = deadbeef->plt_get_title (idx);
            draw_text (x + margin_size + 5, h/2-draw_get_font_size()/2, w - margin_size, 0, tab_title);
        }
        x += margin_size;
    }
    gdk_draw_line (backbuf, widget->style->dark_gc[GTK_STATE_NORMAL], 0, widget->allocation.height-1, widget->allocation.width, widget->allocation.height-1);
    // calc position for drawin selected tab
    x = -hscrollpos;
    for (idx = 0; idx < tab_selected; idx++) {
        x += widths[idx];
    }
    // draw selected
    {
        idx = tab_selected;
        w = widths[tab_selected] + margin_size;
        GdkRectangle area;
        area.x = x;
        area.y = 0;
        area.width = w;
        area.height = 24;
        gtk_paint_box (widget->style, widget->window, GTK_STATE_PRELIGHT, GTK_SHADOW_OUT, &area, widget, "button", x, idx == tab_selected ? 0 : 1, w, 32);
        GdkColor *gdkfg = &widget->style->fg[0];
        float fg[3] = {(float)gdkfg->red/0xffff, (float)gdkfg->green/0xffff, (float)gdkfg->blue/0xffff};
        draw_set_fg_color (fg);
        const char *tab_title = deadbeef->plt_get_title (idx);
        draw_text (x + margin_size + 5, h/2-draw_get_font_size()/2, w - margin_size, 0, tab_title);
    }
    if (need_draw_moving) {
        x = -hscrollpos;
        for (idx = 0; idx < 10; idx++) {
            w = widths[idx];
            if (idx == tab_dragging) {
                // draw empty slot
                if (x < widget->allocation.width) {
                    gtk_paint_box (widget->style, backbuf, GTK_STATE_ACTIVE, GTK_SHADOW_ETCHED_IN, NULL, widget, "button", x, 0, w, h);
                }
                x = tab_movepos;
                if (x >= widget->allocation.width) {
                    break;
                }
                if (w > 0) {
                    gtk_paint_box (widget->style, backbuf, GTK_STATE_SELECTED, GTK_SHADOW_OUT, NULL, widget, "button", x, 0, w, h);
                    GdkColor *gdkfg = &widget->style->fg[GTK_STATE_SELECTED];
                    float fg[3] = {(float)gdkfg->red/0xffff, (float)gdkfg->green/0xffff, (float)gdkfg->blue/0xffff};
                    draw_set_fg_color (fg);
                    //draw_text (x + 5, h/2-draw_get_font_size()/2, tab_width-10, 0, tab_title);
                }
                break;
            }
            x += w;
        }
    }
    draw_end ();
}

static int
get_tab_under_cursor (int x) {
    int idx;
    int cnt = deadbeef->plt_get_count ();
    int fw = 0;
    int tab_selected = deadbeef->plt_get_curr ();
    for (idx = 0; idx < cnt; idx++) {
        const char *title = deadbeef->plt_get_title (idx);
        int w = 0;
        int h = 0;
        draw_get_text_extents (title, strlen (title), &w, &h);
        w += margin_size + 10;
        if (tab_selected == idx) {
            w += margin_size;
        }
        fw += w;
        if (fw > x) {
            return idx;
        }
    }
    return -1;
}

gboolean
on_tabbar_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    tab_clicked = get_tab_under_cursor (event->x);
    if (event->button == 1)
    {
        if (tab_clicked != -1) {
            deadbeef->plt_set_curr (tab_clicked);
        }
    }
    else if (event->button == 3) {
        GtkWidget *menu = create_plmenu ();
        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, widget, 0, gtk_get_current_event_time());
    }
    return FALSE;
}


gboolean
on_tabbar_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{

  return FALSE;
}


gboolean
on_tabbar_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{

  return FALSE;
}


gboolean
on_tabbar_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
    tabbar_draw (widget);
    return FALSE;
}


gboolean
on_tabbar_motion_notify_event          (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
  return FALSE;
}

void
on_rename_playlist1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_remove_playlist1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (tab_clicked != -1) {
        deadbeef->plt_remove (tab_clicked);
    }
}


void
on_add_new_playlist1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int cnt = deadbeef->plt_get_count ();
    int i;
    int idx = 0;
    for (;;) {
        char name[100];
        if (!idx) {
            strcpy (name, "New Playlist");
        }
        else {
            snprintf (name, sizeof (name), "New Playlist (%d)", idx);
        }
        for (i = 0; i < cnt; i++) {
            const char *t = deadbeef->plt_get_title (i);
            if (!strcasecmp (t, name)) {
                break;
            }
        }
        if (i == cnt) {
            deadbeef->plt_add (cnt, name);
            break;
        }
        idx++;
    }
}


void
on_load_playlist1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_save_playlist1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_save_all_playlists1_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}

