/*
 * Copyright (C) 2018-2019 Hertatijanto Hartono <dvertx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <lxpanel/plugin.h>
#include <stdio.h>
#include <string.h>
#include <wordexp.h>

/* location where deleted files ended up in Lubuntu 18.04 */
const char * trash_loc = "~/.local/share/Trash/files/*";

typedef struct
{
    GtkWidget * plugin;
    LXPanel * panel;
    config_setting_t * settings;

    GKeyFile * key_file;
    gchar * config_file;

    gchar * empty_icon_default;
    gchar * full_icon_default;
    gchar * empty_icon;
    gchar * full_icon;
    gboolean init_stage;
    gboolean empty_trash;
    gboolean is_empty;
    guint timer;
    gint icon_size;
    GtkWidget * canvas;
    GdkPixbuf * empty_pixbuf;
    GdkPixbuf * full_pixbuf;
} TrashCan;


static void trash_destructor(gpointer user_data);
static gboolean trash_apply_configuration(gpointer user_data);
static void trash_on_panel_reconfigured(LXPanel * panel, GtkWidget * widget);
static gboolean trash_set_icons(GtkWidget * widget);
static gboolean trash_update_display(GtkWidget * widget);

GtkWidget * trash_constructor(LXPanel *panel, config_setting_t *settings)
{
    GtkWidget * widget;
    TrashCan * trp = g_new0(TrashCan, 1);

    trp->panel = panel;
    trp->settings = settings;
    trp->empty_icon = NULL;
    trp->full_icon = NULL;
    trp->init_stage = TRUE;
    trp->empty_trash = FALSE;
    trp->is_empty = TRUE;
    trp->timer = 1000;
    trp->icon_size = 0;
    trp->canvas = NULL;
    trp->empty_pixbuf = NULL;
    trp->full_pixbuf = NULL;

    /* Load configuration file */
    trp->key_file = g_key_file_new();
    trp->config_file = g_build_filename(g_get_user_config_dir(), "trashcan", "config", NULL);
    if (g_key_file_load_from_file(trp->key_file, trp->config_file, G_KEY_FILE_NONE, NULL)) {
        trp->empty_icon = g_key_file_get_value(trp->key_file, "Config", "EmptyIcon", NULL);
        trp->full_icon = g_key_file_get_value(trp->key_file, "Config", "FullIcon", NULL);
        trp->empty_trash = g_key_file_get_integer(trp->key_file, "Config", "EmptyTrash", NULL);
    }

    trp->plugin = widget = gtk_event_box_new();
    gtk_widget_set_has_window(widget, FALSE);
    lxpanel_plugin_set_data(widget, trp, trash_destructor);

    trp->icon_size = panel_get_icon_size(panel);
    gtk_container_set_border_width(GTK_CONTAINER(widget), 1);
    gtk_widget_set_size_request(widget, trp->icon_size, trp->icon_size);

    /* set default icons */
    trp->empty_icon_default = g_build_filename(g_get_user_config_dir(), "trashcan", "user-trash.png", NULL);
    trp->full_icon_default = g_build_filename(g_get_user_config_dir(), "trashcan", "user-trash-full.png", NULL);

    trash_set_icons(widget);
    g_timeout_add(trp->timer, (GSourceFunc) trash_update_display, (gpointer) widget);
    trash_update_display(widget);
    gtk_container_add(GTK_CONTAINER(widget), trp->canvas);
    gtk_widget_show(trp->canvas);

    return widget;
}

static void trash_destructor(gpointer user_data)
{
    GtkWidget * widget = user_data;
    TrashCan * trp = lxpanel_plugin_get_data(widget);

    if (strlen(trp->empty_icon)) g_free(trp->empty_icon);
    if (strlen(trp->full_icon)) g_free(trp->full_icon);
    if (strlen(trp->empty_icon_default)) g_free(trp->empty_icon_default);
    if (strlen(trp->full_icon_default)) g_free(trp->full_icon_default);

    if (trp->empty_pixbuf) g_object_unref(trp->empty_pixbuf);
    if (trp->full_pixbuf) g_object_unref(trp->full_pixbuf);
    if (trp->canvas) g_object_unref(trp->canvas);

    g_free(trp->key_file);
    g_free(trp->config_file);
    g_free(trp);
}

static gboolean trash_update_display(GtkWidget * widget)
{
    TrashCan * trp = lxpanel_plugin_get_data(widget);
    wordexp_t p;

    wordexp(trash_loc, &p, 0);
    int len = strlen(p.we_wordv[0]);
    if ((!p.we_wordc) || ((p.we_wordc == 1) && (p.we_wordv[0][len-1] == '*'))) {
        if (trp->init_stage) {
            trp->canvas = gtk_image_new_from_pixbuf(trp->empty_pixbuf);
            trp->init_stage = FALSE;
        }
        else
            gtk_image_set_from_pixbuf(trp->canvas, trp->empty_pixbuf);
        trp->is_empty = TRUE;
    }
    else {
        if (trp->init_stage) {
            trp->canvas = gtk_image_new_from_pixbuf(trp->full_pixbuf);
            trp->init_stage = FALSE;
        }
        else
            gtk_image_set_from_pixbuf(trp->canvas, trp->full_pixbuf);
        trp->is_empty = FALSE;
    }
    wordfree(&p);

    return TRUE;
}

static gboolean trash_set_icons(GtkWidget * widget)
{
    TrashCan * trp = lxpanel_plugin_get_data(widget);
    wordexp_t p;
    char **w;

    if (trp->empty_pixbuf) g_object_unref(trp->empty_pixbuf);
    if (strlen(trp->empty_icon) == 0)
        trp->empty_pixbuf = gdk_pixbuf_new_from_file_at_size(trp->empty_icon_default, trp->icon_size, trp->icon_size, NULL);
    else {
        wordexp(trp->empty_icon, &p, 0);
        w = p.we_wordv;
        trp->empty_pixbuf = gdk_pixbuf_new_from_file_at_size(w[0], trp->icon_size, trp->icon_size, NULL);
        wordfree(&p);
        if (!trp->empty_pixbuf)
            trp->empty_pixbuf = gdk_pixbuf_new_from_file_at_size(trp->empty_icon_default, trp->icon_size, trp->icon_size, NULL);
    }

    if (trp->full_pixbuf) g_object_unref(trp->full_pixbuf);
    if (strlen(trp->full_icon) == 0)
        trp->full_pixbuf = gdk_pixbuf_new_from_file_at_size(trp->full_icon_default, trp->icon_size, trp->icon_size, NULL);
    else {
        wordexp(trp->full_icon, &p, 0);
        w = p.we_wordv;
        trp->full_pixbuf = gdk_pixbuf_new_from_file_at_size(w[0], trp->icon_size, trp->icon_size, NULL);
        wordfree(&p);
        if (!trp->full_pixbuf)
            trp->full_pixbuf = gdk_pixbuf_new_from_file_at_size(trp->full_icon_default, trp->icon_size, trp->icon_size, NULL);
    }

    return TRUE;
}

static gboolean trash_button_press_event(GtkWidget * widget, GdkEventButton * evt, LXPanel * panel)
{
    TrashCan * trp = lxpanel_plugin_get_data(widget);
    wordexp_t p;
    char **w;
    int i;

    if (evt->button != 1)
                return FALSE;

    if (trp->empty_trash == TRUE) {
        if (!trp->is_empty) {
            /* expands trash location path */
            wordexp(trash_loc, &p, 0);
            w = p.we_wordv;
            for (i = 0; i < p.we_wordc; i++) {
                g_remove(w[i]);
            }
            wordfree(&p);
            trp->is_empty = TRUE;
        }
    }
    else {
        /* assumes pcmanfm as the file manager */
        fm_launch_command_simple(NULL, NULL, 0, "pcmanfm trash:///", NULL);
    }

    return TRUE;
}

static gboolean trash_apply_configuration(gpointer user_data)
{
    GtkWidget * widget = user_data;
    TrashCan * trp = lxpanel_plugin_get_data(widget);

    config_group_set_string(trp->settings, "EmptyIcon", trp->empty_icon);
    config_group_set_string(trp->settings, "FullIcon", trp->full_icon);
    config_group_set_int(trp->settings, "EmptyTrash", trp->empty_trash);

    /* save settings into configuration file */
    if (!trp->empty_icon)
        g_key_file_set_string(trp->key_file, "Config", "EmptyIcon", "");
    else
        g_key_file_set_string(trp->key_file, "Config", "EmptyIcon", trp->empty_icon);
    if (!trp->full_icon)
        g_key_file_set_string(trp->key_file, "Config", "FullIcon", "");
    else
        g_key_file_set_string(trp->key_file, "Config", "FullIcon", trp->full_icon);
    g_key_file_set_integer(trp->key_file, "Config", "EmptyTrash", trp->empty_trash);
    g_key_file_save_to_file(trp->key_file, trp->config_file, NULL);

    trash_set_icons(widget);
    return FALSE;
}

static GtkWidget * trash_configure(LXPanel *panel, GtkWidget * widget)
{
    TrashCan * trp = lxpanel_plugin_get_data(widget);
    return lxpanel_generic_config_dlg("TrashCan", panel,
        trash_apply_configuration, widget,
        "Trash empty icon", &trp->empty_icon, CONF_TYPE_STR,
        "Trash full icon", &trp->full_icon, CONF_TYPE_STR,
        "Tick bellow if you really want to (unrecoverably) clean trash on click", NULL, CONF_TYPE_TRIM,
        "Empty trash can on click", &trp->empty_trash, CONF_TYPE_BOOL,
        NULL);
}

static void trash_on_panel_reconfigured(LXPanel * panel, GtkWidget * widget)
{
    trash_apply_configuration(widget);
}


FM_DEFINE_MODULE(lxpanel_gtk, trashcan)


LXPanelPluginInit fm_module_init_lxpanel_gtk = {
    .name = "TrashCan",
    .description = "Run a trash can plugin.",

    .new_instance = trash_constructor,
    .config = trash_configure,
    .reconfigure = trash_on_panel_reconfigured,
    .button_press_event = trash_button_press_event
};
