/* ppg-status-actor.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
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

#include <gtk/gtk.h>

#include "ppg-status-actor.h"

G_DEFINE_TYPE(PpgStatusActor, ppg_status_actor, CLUTTER_TYPE_CAIRO_TEXTURE)

struct _PpgStatusActorPrivate
{
	GtkStyle *style;
	PangoFontDescription *font_desc;
	gchar *label;

	guint resize_handler;
};

enum
{
	PROP_0,
	PROP_STYLE,
	PROP_LABEL,
};

static void
ppg_status_actor_paint (PpgStatusActor *actor)
{
	PpgStatusActorPrivate *priv;
	const guint radius = 10;
	PangoLayout *layout;
	cairo_t *cr;
	guint width;
	guint height;
	gint layout_width;
	gint layout_height;

	g_return_if_fail(PPG_IS_STATUS_ACTOR(actor));

	priv = actor->priv;

	if (!priv->style) {
		return;
	}

	g_object_get(actor,
	             "surface-height", &height,
	             "surface-width", &width,
	             NULL);

	clutter_cairo_texture_clear(CLUTTER_CAIRO_TEXTURE(actor));
	cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(actor));

	layout = pango_cairo_create_layout(cr);
	pango_layout_set_font_description(layout, priv->font_desc);
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_text(layout, priv->label, -1);
	pango_layout_set_width(layout, (width - radius - 3) * PANGO_SCALE);
	pango_layout_get_pixel_size(layout, &layout_width, &layout_height);

	if (layout_width + radius + 3 < width) {
		width = layout_width + radius + 3;
	}

	cairo_move_to(cr, 0, 0.5);
	cairo_line_to(cr, width - radius, 0.5);
	cairo_arc(cr,
	          width - radius - 0.5,
	          radius + 0.5,
	          radius,
	          -90.0 * (G_PI / 180.0),
	          0.0);
	cairo_line_to(cr, width - 0.5, height);
	cairo_line_to(cr, 0, height);
	cairo_line_to(cr, 0, 0);
	cairo_close_path(cr);
	gdk_cairo_set_source_color(cr, &priv->style->bg[GTK_STATE_NORMAL]);
	cairo_fill(cr);

	cairo_move_to(cr, 0, 0.5);
	cairo_line_to(cr, width - radius, 0.5);
	cairo_arc(cr,
	          width - radius - 0.5,
	          radius + 0.5,
	          radius,
	          -90.0 * (G_PI / 180.0),
	          0.0);
	cairo_line_to(cr, width - 0.5, height);
	cairo_set_line_width(cr, 1.0);
	gdk_cairo_set_source_color(cr, &priv->style->dark[GTK_STATE_NORMAL]);
	cairo_stroke(cr);

	gdk_cairo_set_source_color(cr, &priv->style->text[GTK_STATE_NORMAL]);
	cairo_move_to(cr, 3.0, (gint)((height - layout_height) / 2.0));
	pango_cairo_show_layout(cr, layout);
	g_object_unref(layout);

	cairo_destroy(cr);
}

static gboolean
ppg_status_actor_do_resize_surface (gpointer user_data)
{
	PpgStatusActor *actor = (PpgStatusActor *)user_data;
	PpgStatusActorPrivate *priv;
	ClutterGeometry geom;

	g_return_val_if_fail(PPG_IS_STATUS_ACTOR(actor), FALSE);

	priv = actor->priv;
	priv->resize_handler = 0;

	clutter_actor_get_geometry(CLUTTER_ACTOR(actor), &geom);
	g_object_set(actor,
	             "surface-height", geom.height,
	             "surface-width", geom.width,
	             NULL);

	ppg_status_actor_paint(actor);

	return FALSE;
}

static void
ppg_status_actor_notify_allocation (PpgStatusActor *actor,
                                    GParamSpec     *pspec,
                                    gpointer        user_data)
{
	PpgStatusActorPrivate *priv;

	g_return_if_fail(PPG_IS_STATUS_ACTOR(actor));

	priv = actor->priv;

	if (priv->resize_handler) {
		g_source_remove(priv->resize_handler);
	}

	priv->resize_handler =
		g_timeout_add(0, ppg_status_actor_do_resize_surface, actor);
}

static void
ppg_status_actor_set_style (PpgStatusActor *actor,
                            GtkStyle       *style)
{
	g_return_if_fail(PPG_IS_STATUS_ACTOR(actor));

	actor->priv->style = style;
	ppg_status_actor_notify_allocation(actor, NULL, NULL);
}

static void
ppg_status_actor_set_label (PpgStatusActor *actor,
                            const gchar *label)
{
	PpgStatusActorPrivate *priv;

	g_return_if_fail(PPG_IS_STATUS_ACTOR(actor));

	priv = actor->priv;

	g_free(priv->label);
	priv->label = g_strdup(label);

	ppg_status_actor_paint(actor);
	clutter_actor_set_opacity(CLUTTER_ACTOR(actor),
	                          (!label || !label[0]) ? 0x00 : 0xFF);
}

/**
 * ppg_status_actor_finalize:
 * @object: (in): A #PpgStatusActor.
 *
 * Finalizer for a #PpgStatusActor instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_status_actor_finalize (GObject *object)
{
	PpgStatusActorPrivate *priv = PPG_STATUS_ACTOR(object)->priv;

	pango_font_description_free(priv->font_desc);

	G_OBJECT_CLASS(ppg_status_actor_parent_class)->finalize(object);
}

/**
 * ppg_status_actor_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_status_actor_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
	PpgStatusActor *actor = PPG_STATUS_ACTOR(object);

	switch (prop_id) {
	case PROP_STYLE:
		ppg_status_actor_set_style(actor, g_value_get_object(value));
		break;
	case PROP_LABEL:
		ppg_status_actor_set_label(actor, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_status_actor_class_init:
 * @klass: (in): A #PpgStatusActorClass.
 *
 * Initializes the #PpgStatusActorClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_status_actor_class_init (PpgStatusActorClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_status_actor_finalize;
	object_class->set_property = ppg_status_actor_set_property;
	g_type_class_add_private(object_class, sizeof(PpgStatusActorPrivate));

	g_object_class_install_property(object_class,
	                                PROP_STYLE,
	                                g_param_spec_object("style",
	                                                    "style",
	                                                    "style",
	                                                    GTK_TYPE_STYLE,
	                                                    G_PARAM_WRITABLE));

	g_object_class_install_property(object_class,
	                                PROP_LABEL,
	                                g_param_spec_string("label",
	                                                    "label",
	                                                    "label",
	                                                    NULL,
	                                                    G_PARAM_WRITABLE));
}

/**
 * ppg_status_actor_init:
 * @actor: (in): A #PpgStatusActor.
 *
 * Initializes the newly created #PpgStatusActor instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_status_actor_init (PpgStatusActor *actor)
{
	PpgStatusActorPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(actor, PPG_TYPE_STATUS_ACTOR,
	                                   PpgStatusActorPrivate);
	actor->priv = priv;

	priv->label = g_strdup("");

	priv->font_desc = pango_font_description_new();
	pango_font_description_set_size(priv->font_desc, 8 * PANGO_SCALE);
	pango_font_description_set_family_static(priv->font_desc, "Sans");

	g_signal_connect_after(actor, "notify::allocation",
	                       G_CALLBACK(ppg_status_actor_notify_allocation),
	                       NULL);
}
