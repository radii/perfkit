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

#include "pkd-source.h"
#include "pkd-source-info.h"

G_BEGIN_DECLS

#define PKD_TYPE_SOURCES            (pkd_sources_get_type ())
#define PKD_SOURCES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_SOURCES, PkdSources))
#define PKD_SOURCES_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_SOURCES, PkdSources const))
#define PKD_SOURCES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_TYPE_SOURCES, PkdSourcesClass))
#define PKD_IS_SOURCES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_TYPE_SOURCES))
#define PKD_IS_SOURCES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_TYPE_SOURCES))
#define PKD_SOURCES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_TYPE_SOURCES, PkdSourcesClass))
#define PKD_SOURCES_ERROR           (pkd_sources_error_quark ())

/**
 * PkdChannelsError:
 * @PKD_SOURCES_ERROR_INVALID_TYPE: The source type is invalid
 *
 * #PkdSourcesError error enumeration.
 */
typedef enum
{
	PKD_SOURCES_ERROR_INVALID_TYPE,
} PkdSourcesError;

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

GQuark     pkd_sources_error_quark (void) G_GNUC_CONST;
GType      pkd_sources_get_type    (void) G_GNUC_CONST;
GList*     pkd_sources_find_all    (void);
PkdSource* pkd_sources_add         (PkdSources            *sources,
                                    const gchar           *factory,
                                    GError               **error);
void       pkd_sources_register    (PkdSources            *sources,
                                    const gchar           *uid,
                                    const gchar           *name,
                                    const gchar           *version,
                                    const gchar           *description,
                                    PkdSourceFactoryFunc   factory_func,
                                    gpointer               user_data);

G_END_DECLS

#endif /* __PKD_SOURCES_H__ */
