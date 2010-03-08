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

	priv->begin = *begin;
	priv->end = *end;
	clutter_actor_queue_redraw(pkg_source_renderer_get_actor(source_renderer));
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
}

static void
pkg_source_renderer_init (PkgSourceRenderer *renderer)
{
	renderer->priv = G_TYPE_INSTANCE_GET_PRIVATE(renderer,
	                                             PKG_TYPE_SOURCE_RENDERER,
	                                             PkgSourceRendererPrivate);
}
