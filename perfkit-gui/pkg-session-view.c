/* pkg-session-view.c
 *
 * Copyright (C) 2010 Christian Hergert
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <clutter-gtk/clutter-gtk.h>

#include "pkg-session-view.h"

G_DEFINE_TYPE(PkgSessionView, pkg_session_view, GTK_TYPE_VBOX)

/**
 * SECTION:pkg-session_view
 * @title: PkgSessionView
 * @short_description: 
 *
 * 
 */

struct _PkgSessionViewPrivate
{
	PkgSession *session;
	GtkWidget  *label;
	GtkWidget  *vpaned;

	ClutterActor *bg_for_sources;
	ClutterActor *stage;
};

enum
{
    LAST_SIGNAL
};

//static guint signals [LAST_SIGNAL];

enum
{
    PROP_0,
};

/**
 * pkg_session_view_new:
 *
 * Creates a new instance of #PkgSessionView.
 *
 * Return value: the newly created #PkgSessionView instance.
 */
GtkWidget*
pkg_session_view_new (void)
{
	return g_object_new(PKG_TYPE_SESSION_VIEW, NULL);
}

void
pkg_session_view_set_session (PkgSessionView *session_view,
                              PkgSession     *session)
{
}

GtkWidget*
pkg_session_view_get_label (PkgSessionView *session_view)
{
	return session_view->priv->label;
}

static void
pkg_session_view_finalize (GObject *object)
{
	PkgSessionViewPrivate *priv;

	g_return_if_fail (PKG_IS_SESSION_VIEW (object));

	priv = PKG_SESSION_VIEW (object)->priv;

	G_OBJECT_CLASS (pkg_session_view_parent_class)->finalize (object);
}

static void
pkg_session_view_class_init (PkgSessionViewClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkg_session_view_finalize;
	g_type_class_add_private(object_class, sizeof(PkgSessionViewPrivate));
}

static ClutterActor*
create_source(PkgSessionView *session_view,
              const gchar    *title,
              ClutterActor   *stage,
              gint            offset,
              gboolean        selected)
{
	GtkWidget *w = GTK_WIDGET(session_view);
	ClutterColor color = {0,0,0,0xFF};
	GdkColor dark, mid, light, bg, fg;

	{
		ClutterActor *handle;
		cairo_t *cr;
		cairo_pattern_t *p;
		gint state = selected ? GTK_STATE_SELECTED : GTK_STATE_NORMAL;

		handle = clutter_cairo_texture_new(200, 60);
		clutter_container_add_actor(CLUTTER_CONTAINER(stage), handle);
		clutter_actor_set_position(handle, 0, offset);
		clutter_actor_set_size(handle, 200, 60);
		clutter_actor_show(handle);

		dark = w->style->dark[state];
		mid = w->style->mid[state];
		light = w->style->light[state];
		bg = w->style->bg[state];
		fg = w->style->fg[state];
		cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(handle));
		cairo_rectangle(cr, 0, 0, 200, 80);
		p = cairo_pattern_create_linear(0, 0, 0, 60);
		cairo_pattern_add_color_stop_rgb(p, 0.0f,
		                                 light.red / (double)0xFFFF,
		                                 light.green / (double)0xFFFF,
		                                 light.blue / (double)0xFFFF);
		cairo_pattern_add_color_stop_rgb(p, 0.5f,
		                                 mid.red / (double)0xFFFF,
		                                 mid.green / (double)0xFFFF,
		                                 mid.blue / (double)0xFFFF);
		cairo_pattern_add_color_stop_rgb(p, 0.51f,
		                                 dark.red / (double)0xFFFF,
		                                 dark.green / (double)0xFFFF,
		                                 dark.blue / (double)0xFFFF);
		cairo_pattern_add_color_stop_rgb(p, 1.0f,
		                                 bg.red / (double)0xFFFF,
		                                 bg.green / (double)0xFFFF,
		                                 bg.blue / (double)0xFFFF);
		cairo_set_source(cr, p);
		cairo_fill(cr);
		cairo_destroy(cr);
	}

	{
		ClutterActor *txt1, *txt2;

		color.red = 0xFF;
		color.green = 0xFF;
		color.blue = 0xFF;
		color.alpha = 0xFF;
		txt2 = clutter_text_new_full("Sans 16 Bold", title, &color);
		clutter_container_add_actor(CLUTTER_CONTAINER(stage), txt2);
		clutter_actor_set_position(txt2, 30, ((60 - clutter_actor_get_height(txt2)) / 2) + offset + 1);
		clutter_actor_show(txt2);

		color.red = fg.red / 255.0;
		color.green = fg.green / 255.0;
		color.blue = fg.blue / 255.0;
		color.alpha = 0xFF;
		txt1 = clutter_text_new_full("Sans 16 Bold", title, &color);
		clutter_container_add_actor(CLUTTER_CONTAINER(stage), txt1);
		clutter_actor_set_position(txt1, 30, ((60 - clutter_actor_get_height(txt2)) / 2) + offset);
		clutter_actor_show(txt1);
	}

	{
		ClutterActor *src_bg;

		src_bg = clutter_rectangle_new();
		color.red = bg.red / 255.0;
		color.green = bg.green / 255.0;
		color.blue = bg.blue / 255.0;
		color.alpha = 0xFF;
		g_object_set(src_bg, "color", &color, NULL);
		clutter_actor_set_size(src_bg, 1000, 60);
		clutter_container_add_actor(CLUTTER_CONTAINER(stage), src_bg);
		clutter_actor_set_position(src_bg, 201, offset);
		clutter_actor_show(src_bg);
	}

	return NULL;
}

static void
pkg_session_view_style_set (GtkWidget *embed,
                            GtkStyle  *old_style,
                            gpointer   user_data)
{
	PkgSessionViewPrivate *priv;
	ClutterColor cbg;
	GdkColor bg;

	priv = PKG_SESSION_VIEW(user_data)->priv;

	bg = gtk_widget_get_style(user_data)->bg[GTK_STATE_ACTIVE];
	cbg.red = bg.red / 0xFF;
	cbg.green = bg.green / 0xFF;
	cbg.blue = bg.blue / 0xFF;
	cbg.alpha = 0xFF;
	g_object_set(priv->bg_for_sources, "color", &cbg, NULL);

	// tmp
	{
		ClutterActor *src1, *src2, *src3, *src4;

		src1 = create_source(user_data, "Memory", priv->stage, 30, TRUE);
		src2 = create_source(user_data, "CPU", priv->stage, 90, FALSE);
		src3 = create_source(user_data, "Disk", priv->stage, 150, FALSE);
		src4 = create_source(user_data, "Network", priv->stage, 210, FALSE);

		g_debug("DONE");
	}
}

static void
pkg_session_view_init (PkgSessionView *session_view)
{
	PkgSessionViewPrivate *priv;
	GtkWidget *icon,
	          *text,
	          *table,
	          *vscroller,
	          *hscroller,
	          *zhbox,
	          *scale,
	          *embed,
	          *img1,
	          *img2;
	ClutterActor *stage;
	ClutterColor color;

	session_view->priv = G_TYPE_INSTANCE_GET_PRIVATE(session_view,
	                                                 PKG_TYPE_SESSION_VIEW,
	                                                 PkgSessionViewPrivate);
	priv = session_view->priv;

	/* setup label */
	priv->label = gtk_hbox_new(FALSE, 3);
	gtk_widget_show(priv->label);

	icon = gtk_image_new_from_icon_name("computer", GTK_ICON_SIZE_MENU);
	gtk_box_pack_start(GTK_BOX(priv->label), icon, FALSE, TRUE, 0);
	gtk_widget_show(icon);

	text = gtk_label_new("Session 0");
	gtk_box_pack_start(GTK_BOX(priv->label), text, TRUE, TRUE, 0);
	gtk_widget_show(text);

	priv->vpaned = gtk_vpaned_new();
	gtk_container_add(GTK_CONTAINER(session_view), priv->vpaned);
	gtk_widget_show(priv->vpaned);

	/* setup main layout */
	table = gtk_table_new(2, 3, FALSE);
	gtk_paned_add1(GTK_PANED(priv->vpaned), table);
	gtk_widget_show(table);

	vscroller = gtk_vscrollbar_new(NULL);
	gtk_table_attach(GTK_TABLE(table),
	                 vscroller,
	                 2, 3, 0, 1,
	                 GTK_FILL,
	                 GTK_FILL | GTK_EXPAND,
	                 0, 0);
	gtk_widget_show(vscroller);

	hscroller = gtk_hscrollbar_new(NULL);
	gtk_table_attach(GTK_TABLE(table),
	                 hscroller,
	                 1, 2, 1, 2,
	                 GTK_FILL | GTK_EXPAND,
	                 GTK_FILL,
	                 0, 0);
	gtk_widget_show(hscroller);

	/* add sources stage */
	embed = gtk_clutter_embed_new();
	gtk_table_attach(GTK_TABLE(table),
	                 embed,
	                 0, 2, 0, 1,
	                 GTK_FILL | GTK_EXPAND,
	                 GTK_FILL | GTK_EXPAND,
	                 0, 0);
	g_signal_connect(embed,
	                 "style-set",
	                 G_CALLBACK(pkg_session_view_style_set),
	                 session_view);
	stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(embed));
	gtk_widget_show(embed);
	priv->stage = stage;

	/* create zoom control */
	zhbox = gtk_hbox_new(FALSE, 3);
	gtk_table_attach(GTK_TABLE(table),
	                 zhbox,
	                 0, 1, 1, 2,
	                 GTK_FILL,
	                 GTK_FILL,
	                 6, 0);
	gtk_widget_set_size_request(zhbox, 188, -1);
	gtk_widget_show(zhbox);

	/* zoom out image */
	img1 = gtk_image_new_from_icon_name("zoom-out", GTK_ICON_SIZE_MENU);
	gtk_box_pack_start(GTK_BOX(zhbox), img1, FALSE, TRUE, 0);
	gtk_widget_show(img1);

	scale = gtk_hscale_new(NULL);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_box_pack_start(GTK_BOX(zhbox), scale, TRUE, TRUE, 0);
	gtk_widget_show(scale);

	/* zoom in image */
	img2 = gtk_image_new_from_icon_name("zoom-in", GTK_ICON_SIZE_MENU);
	gtk_box_pack_start(GTK_BOX(zhbox), img2, FALSE, TRUE, 0);
	gtk_widget_show(img2);

	/* create clutter actors for inside data view */
	priv->bg_for_sources = clutter_rectangle_new();
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->bg_for_sources);
	clutter_actor_set_size(priv->bg_for_sources, 200, 1000);
	clutter_actor_set_position(priv->bg_for_sources, 0, 0);
	color.red = 0x00;
	color.green = 0x00;
	color.blue = 0x00;
	color.alpha = 0xFF;
	g_object_set(priv->bg_for_sources, "color", &color, NULL);
	clutter_actor_show(priv->bg_for_sources);

	{
		ClutterActor *shadow;

		shadow = clutter_rectangle_new();
		clutter_container_add_actor(CLUTTER_CONTAINER(stage), shadow);
		clutter_actor_set_size(shadow, 1, 1000);
		clutter_actor_set_position(shadow, 200, 0);
		color.red = 0x88;
		color.green = 0x88;
		color.blue = 0x88;
		color.alpha = 0xff;
		g_object_set(shadow, "color", &color, NULL);
		clutter_actor_show(shadow);
	}
}
