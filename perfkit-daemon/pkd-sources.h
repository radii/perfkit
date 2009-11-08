/* pkd-sources.h
 * 
 * Copyright (C) 2009 Christian Hergert
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

#ifndef __PKD_SOURCES_H__
#define __PKD_SOURCES_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKD_TYPE_SOURCES            (pkd_sources_get_type ())
#define PKD_SOURCES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_SOURCES, PkdSources))
#define PKD_SOURCES_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_SOURCES, PkdSources const))
#define PKD_SOURCES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_TYPE_SOURCES, PkdSourcesClass))
#define PKD_IS_SOURCES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_TYPE_SOURCES))
#define PKD_IS_SOURCES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_TYPE_SOURCES))
#define PKD_SOURCES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_TYPE_SOURCES, PkdSourcesClass))

typedef struct _PkdSources        PkdSources;
typedef struct _PkdSourcesClass   PkdSourcesClass;
typedef struct _PkdSourcesPrivate PkdSourcesPrivate;

struct _PkdSources
{
	GObject parent;

	/*< private >*/
	PkdSourcesPrivate *priv;
};

struct _PkdSourcesClass
{
	GObjectClass parent_class;
};

GType pkd_sources_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PKD_SOURCES_H__ */
