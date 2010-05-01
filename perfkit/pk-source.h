/* pk-source.h
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PK_SOURCE_H__
#define __PK_SOURCE_H__

#include <gio/gio.h>

#include "pk-connection.h"

G_BEGIN_DECLS

#define PK_TYPE_SOURCE            (pk_source_get_type())
#define PK_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_SOURCE, PkSource))
#define PK_SOURCE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_SOURCE, PkSource const))
#define PK_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_SOURCE, PkSourceClass))
#define PK_IS_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_SOURCE))
#define PK_IS_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_SOURCE))
#define PK_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_SOURCE, PkSourceClass))

typedef struct _PkSource        PkSource;
typedef struct _PkSourceClass   PkSourceClass;
typedef struct _PkSourcePrivate PkSourcePrivate;

struct _PkSource
{
	GInitiallyUnowned parent;

	/*< private >*/
	PkSourcePrivate *priv;
};

struct _PkSourceClass
{
	GInitiallyUnownedClass parent_class;
};

GType      pk_source_get_type            (void) G_GNUC_CONST;
PkSource*  pk_source_new_for_connection  (PkConnection        *connection);
void       pk_source_set_property_async  (PkSource            *source,
                                          const gchar         *name,
                                          const GValue        *value,
                                          GCancellable        *cancellable,
                                          GAsyncReadyCallback  callback,
                                          gpointer             user_data);
gboolean   pk_source_set_property_finish (PkSource             *source,
                                          GAsyncResult         *result,
                                          GError              **error);
void       pk_source_get_property_async  (PkSource            *source,
                                          const gchar         *name,
                                          GCancellable        *cancellable,
                                          GAsyncReadyCallback  callback,
                                          gpointer             user_data);
gboolean   pk_source_get_property_finish (PkSource             *source,
                                          GAsyncResult         *result,
                                          GValue               *value,
                                          GError              **error);
void       pk_source_get_plugin_async    (PkSource            *source,
                                          GCancellable        *cancellable,
                                          GAsyncReadyCallback  callback,
                                          gpointer             user_data);
gboolean   pk_source_get_plugin_finish   (PkSource             *source,
                                          GAsyncResult         *result,
                                          gchar               **plugin,
                                          GError              **error);

G_END_DECLS

#endif /* __PK_SOURCE_H__ */
