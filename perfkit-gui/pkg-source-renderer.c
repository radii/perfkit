/* pkg-source-renderer.c
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

#include "pkg-source-renderer.h"

/**
 * SECTION:pkg-source-renderer
 * @title: PkgSourceRenderer
 * @short_description: 
 *
 * Section overview.
 */

G_DEFINE_TYPE(PkgSourceRenderer, pkg_source_renderer, G_TYPE_INITIALLY_UNOWNED)

struct _PkgSourceRendererPrivate
{
	PkSource     *source;
	ClutterActor *texture;
	GTimeVal      begin;
	GTimeVal      end;
};

/**
 * pkg_source_renderer_new:
 *
 * Creates a new instance of #PkgSourceRenderer.
 *
 * Returns: the newly created instance of #PkgSourceRenderer.
 * Side effects: None.
 */
PkgSourceRenderer*
pkg_source_renderer_new (void)
{
	return g_object_new(PKG_TYPE_SOURCE_RENDERER, NULL);
}

/**
 * pkg_source_renderer_get_actor:
 * @source_renderer: A #PkgSourceRenderer.
 *
 * Retrieves the #ClutterActor to be used for renderering the source.
 *
 * Returns: a #ClutterActor
 * Side effects: None.
 */
ClutterActor*
pkg_source_renderer_get_actor (PkgSourceRenderer *source_renderer)
{
	g_return_val_if_fail(PKG_IS_SOURCE_RENDERER(source_renderer), NULL);
	return PKG_SOURCE_RENDERER_GET_CLASS(source_renderer)->
		get_actor(source_renderer);
}

/**
 * pkg_source_renderer_get_range:
 * @source_renderer: A #PkgSourceRenderer.
 *
 * Retrieves the time range to be used for rendering the source.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pkg_source_renderer_get_range (PkgSourceRenderer *source_renderer,
                               GTimeVal          *begin,
                               GTimeVal          *end)
{
	PkgSourceRendererPrivate *priv;

	g_return_if_fail(begin != NULL);
	g_return_if_fail(end != NULL);

	priv = source_renderer->priv;

	*begin = priv->begin;
	*end = priv->end;
}

/**
 * pkg_source_renderer_set_range:
 * @source_renderer: A #PkgSourceRenderer.
 *
 * Sets the time range for the renderer to render.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pkg_source_renderer_set_range (PkgSourceRenderer *source_renderer,
                               GTimeVal          *begin,
                               GTimeVal          *end)
{
	PkgSourceRendererPrivate *priv;

	g_return_if_fail(begin != NULL);
	g_return_if_fail(end != NULL);

	priv = source_renderer->priv;

	PKG_SOURCE_RENDERER_GET_CLASS(source_renderer)->
		set_range(source_renderer, begin, end);

	priv->begin = *begin;
	priv->end = *end;
}

void
pkg_source_renderer_set_source (PkgSourceRenderer *source_renderer,
                                PkSource          *source)
{
	PkgSourceRendererPrivate *priv;

	g_return_if_fail(PKG_IS_SOURCE_RENDERER(source_renderer));
	g_return_if_fail(PK_IS_SOURCE(source));

	if (priv->source == source) {
		return;
	} else if (priv->source) {
		g_object_remove_weak_pointer(G_OBJECT(source_renderer),
		                             (gpointer *)&priv->source);
	}

	priv->source = source;
	g_object_add_weak_pointer(G_OBJECT(source_renderer),
	                          (gpointer *)&priv->source);
}

PkSource*
pkg_source_renderer_get_source (PkgSourceRenderer *source_renderer)
{
	g_return_val_if_fail(PKG_IS_SOURCE_RENDERER(source_renderer), NULL);
	return source_renderer->priv->source;
}

void
pkg_source_renderer_render (PkgSourceRenderer *source_renderer,
                            ClutterActor      *actor,
                            gdouble            x,
                            gdouble            width)
{
	g_return_if_fail(PKG_IS_SOURCE_RENDERER(source_renderer));
	g_return_if_fail(CLUTTER_IS_ACTOR(actor));

	PKG_SOURCE_RENDERER_GET_CLASS(source_renderer)->
		render(source_renderer, actor, x, width);
}

static ClutterActor*
pkg_source_renderer_real_get_actor (PkgSourceRenderer *source_renderer)
{
	return source_renderer->priv->texture;
}

static void
pkg_source_renderer_real_set_range (PkgSourceRenderer *source_renderer,
                                    GTimeVal          *begin,
                                    GTimeVal          *end)
{
	PkgSourceRendererPrivate *priv;

	g_return_if_fail(PKG_IS_SOURCE_RENDERER(source_renderer));

	priv = source_renderer->priv;

	/* TODO: Shift COGL texture by range */
}

static void
pkg_source_renderer_real_render (PkgSourceRenderer *source_renderer,
                                 ClutterActor      *actor,
                                 gdouble            x,
                                 gdouble            width)
{
	PkgSourceRendererPrivate *priv;
	cairo_t *cr;
	cairo_pattern_t *p;
	gfloat w, h;
	gint i;

	g_return_if_fail(CLUTTER_IS_CAIRO_TEXTURE(actor));

	priv = source_renderer->priv;
	clutter_actor_get_size(actor, &w, &h);

	clutter_cairo_texture_clear(CLUTTER_CAIRO_TEXTURE(actor));
	cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(actor));
	p = cairo_pattern_create_linear(0, 0, 0, h);
	cairo_pattern_add_color_stop_rgb(p, .0, 1., 0, 0);
	cairo_pattern_add_color_stop_rgb(p, .4, .9411, .8862, .2039);
	cairo_pattern_add_color_stop_rgb(p, .8, .4509, .8235, .0862);
	cairo_set_source(cr, p);

	for (i = 0; i < width; i++) {
		cairo_line_to(cr, i, g_random_double_range(5, h - 10.));
		i += g_random_int_range(1, 5);
	}

	cairo_line_to(cr, x + width, h);
	cairo_line_to(cr, 0, h);
	cairo_fill_preserve(cr);
	cairo_set_source_rgb(cr, 0.3058, 0.6039, 0.0235);
	cairo_set_line_width(cr, 1.0);
	cairo_stroke(cr);
	cairo_destroy(cr);
}

static void
pkg_source_renderer_allocation_changed (ClutterActor           *actor,
                                        ClutterActorBox        *actor_box,
                                        ClutterAllocationFlags  flags,
                                        gpointer                user_data)
{
	gfloat width, height;

	width = clutter_actor_box_get_width(actor_box);
	height = clutter_actor_box_get_height(actor_box);
	clutter_cairo_texture_set_surface_size(CLUTTER_CAIRO_TEXTURE(actor),
	                                       width, height);
}

static void
pkg_source_renderer_finalize (GObject *object)
{
	G_OBJECT_CLASS(pkg_source_renderer_parent_class)->finalize(object);
}

static void
pkg_source_renderer_class_init (PkgSourceRendererClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pkg_source_renderer_finalize;
	g_type_class_add_private(object_class, sizeof(PkgSourceRendererPrivate));

	klass->get_actor = pkg_source_renderer_real_get_actor;
	klass->set_range = pkg_source_renderer_real_set_range;
	klass->render = pkg_source_renderer_real_render;
}

static void
pkg_source_renderer_init (PkgSourceRenderer *renderer)
{
	renderer->priv = G_TYPE_INSTANCE_GET_PRIVATE(renderer,
	                                             PKG_TYPE_SOURCE_RENDERER,
	                                             PkgSourceRendererPrivate);
	renderer->priv->texture = clutter_cairo_texture_new(1, 1);
	g_signal_connect(renderer->priv->texture,
	                 "allocation-changed",
	                 G_CALLBACK(pkg_source_renderer_allocation_changed),
	                 renderer);
	
}
