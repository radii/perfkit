/* pkd-source.h
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

#ifndef __PKD_SOURCE_H__
#define __PKD_SOURCE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKD_TYPE_SOURCE            (pkd_source_get_type ())
#define PKD_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_SOURCE, PkdSource))
#define PKD_SOURCE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_SOURCE, PkdSource const))
#define PKD_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_TYPE_SOURCE, PkdSourceClass))
#define PKD_IS_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_TYPE_SOURCE))
#define PKD_IS_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_TYPE_SOURCE))
#define PKD_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_TYPE_SOURCE, PkdSourceClass))

typedef struct _PkdSource        PkdSource;
typedef struct _PkdSourceClass   PkdSourceClass;
typedef struct _PkdSourcePrivate PkdSourcePrivate;

struct _PkdSource
{
	GObject parent;

	/*< private >*/
	PkdSourcePrivate *priv;
};

struct _PkdSourceClass
{
	GObjectClass parent_class;

	gboolean (*needs_spawn) (PkdSource  *source);
	gboolean (*spawn)       (PkdSource  *source,
	                         GError    **error);
	gboolean (*start)       (PkdSource  *source,
	                         GError    **error);
	void     (*stop)        (PkdSource  *source);
};

GType       pkd_source_get_type    (void) G_GNUC_CONST;
PkdSource * pkd_source_new         (void);
gboolean    pkd_source_needs_spawn (PkdSource  *source);
gboolean    pkd_source_spawn       (PkdSource  *source,
                                    GError    **error);
gboolean    pkd_source_start       (PkdSource  *source,
                                    GError    **error);
void        pkd_source_stop        (PkdSource  *source);

G_END_DECLS

#endif /* __PKD_SOURCE_H__ */
