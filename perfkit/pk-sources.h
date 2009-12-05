/* pk-sources.h
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PK_SOURCES_H__
#define __PK_SOURCES_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PK_TYPE_SOURCES            (pk_sources_get_type ())
#define PK_SOURCES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_SOURCES, PkSources))
#define PK_SOURCES_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_SOURCES, PkSources const))
#define PK_SOURCES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_SOURCES, PkSourcesClass))
#define PK_IS_SOURCES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_SOURCES))
#define PK_IS_SOURCES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_SOURCES))
#define PK_SOURCES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_SOURCES, PkSourcesClass))

typedef struct _PkSources        PkSources;
typedef struct _PkSourcesClass   PkSourcesClass;
typedef struct _PkSourcesPrivate PkSourcesPrivate;

struct _PkSources
{
	GObject parent;

	/*< private >*/
	PkSourcesPrivate *priv;
};

struct _PkSourcesClass
{
	GObjectClass parent_class;
};

GType      pk_sources_get_type  (void) G_GNUC_CONST;
gchar**    pk_sources_get_types (PkSources *sources);
PkSource*  pk_sources_get       (PkSources *sources,
                                 gint       source_id);

G_END_DECLS

#endif /* __PK_SOURCES_H__ */
