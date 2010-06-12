/* pkg-source-renderer.h
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

#ifndef __PKG_SOURCE_RENDERER_H__
#define __PKG_SOURCE_RENDERER_H__

#include <clutter/clutter.h>
#include <perfkit/perfkit.h>

G_BEGIN_DECLS

#define PKG_TYPE_SOURCE_RENDERER            (pkg_source_renderer_get_type())
#define PKG_SOURCE_RENDERER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_SOURCE_RENDERER, PkgSourceRenderer))
#define PKG_SOURCE_RENDERER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_SOURCE_RENDERER, PkgSourceRenderer const))
#define PKG_SOURCE_RENDERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKG_TYPE_SOURCE_RENDERER, PkgSourceRendererClass))
#define PKG_IS_SOURCE_RENDERER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKG_TYPE_SOURCE_RENDERER))
#define PKG_IS_SOURCE_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKG_TYPE_SOURCE_RENDERER))
#define PKG_SOURCE_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKG_TYPE_SOURCE_RENDERER, PkgSourceRendererClass))

typedef struct _PkgSourceRenderer        PkgSourceRenderer;
typedef struct _PkgSourceRendererClass   PkgSourceRendererClass;
typedef struct _PkgSourceRendererPrivate PkgSourceRendererPrivate;

struct _PkgSourceRenderer
{
	GInitiallyUnowned parent;

	/*< private >*/
	PkgSourceRendererPrivate *priv;
};

struct _PkgSourceRendererClass
{
	GInitiallyUnownedClass parent_class;

	ClutterActor* (*get_actor) (PkgSourceRenderer *source_renderer);
	void          (*set_range) (PkgSourceRenderer *source_renderer,
	                            GTimeVal          *begin,
	                            GTimeVal          *end);
	void          (*render)    (PkgSourceRenderer *source_renderer,
	                            ClutterActor      *actor,
	                            gdouble            x,
	                            gdouble            width);
};

GType              pkg_source_renderer_get_type   (void) G_GNUC_CONST;
PkgSourceRenderer* pkg_source_renderer_new        (void);
ClutterActor*      pkg_source_renderer_get_actor  (PkgSourceRenderer *source_renderer);
PkSource*          pkg_source_renderer_get_source (PkgSourceRenderer *source_renderer);
void               pkg_source_renderer_set_source (PkgSourceRenderer *source_renderer,
                                                   PkSource          *source);
void               pkg_source_renderer_get_range  (PkgSourceRenderer *source_renderer,
                                                   GTimeVal          *begin,
                                                   GTimeVal          *end);
void               pkg_source_renderer_set_range  (PkgSourceRenderer *source_renderer,
                                                   GTimeVal          *begin,
                                                   GTimeVal          *end);
void               pkg_source_renderer_render     (PkgSourceRenderer *source_renderer,
                                                   ClutterActor      *actor,
                                                   gdouble            x,
                                                   gdouble            width);

G_END_DECLS

#endif /* __PKG_SOURCE_RENDERER_H__ */
