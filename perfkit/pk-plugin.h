/* pk-plugin.h
 *
 * Copyright 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PK_PLUGIN_H__
#define __PK_PLUGIN_H__

#include "pk-connection.h"

G_BEGIN_DECLS

#define PK_TYPE_PLUGIN            (pk_plugin_get_type())
#define PK_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_PLUGIN, PkPlugin))
#define PK_PLUGIN_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_PLUGIN, PkPlugin const))
#define PK_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_PLUGIN, PkPluginClass))
#define PK_IS_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_PLUGIN))
#define PK_IS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_PLUGIN))
#define PK_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_PLUGIN, PkPluginClass))

typedef struct _PkPlugin        PkPlugin;
typedef struct _PkPluginClass   PkPluginClass;
typedef struct _PkPluginPrivate PkPluginPrivate;

struct _PkPlugin
{
	GObject parent;

	/*< private >*/
	PkPluginPrivate *priv;
};

struct _PkPluginClass
{
	GObjectClass parent_class;
};

GType         pk_plugin_get_type                 (void) G_GNUC_CONST;
const gchar*  pk_plugin_get_id                   (PkPlugin              *plugin) G_GNUC_PURE;
PkConnection* pk_plugin_get_connection           (PkPlugin              *plugin) G_GNUC_PURE;
gboolean      pk_plugin_get_copyright            (PkPlugin              *plugin,
                                                  gchar                **copyright,
                                                  GError               **error);
void          pk_plugin_get_copyright_async      (PkPlugin              *plugin,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_plugin_get_copyright_finish     (PkPlugin              *plugin,
                                                  GAsyncResult          *result,
                                                  gchar                **copyright,
                                                  GError               **error);
gboolean      pk_plugin_get_description          (PkPlugin              *plugin,
                                                  gchar                **description,
                                                  GError               **error);
void          pk_plugin_get_description_async    (PkPlugin              *plugin,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_plugin_get_description_finish   (PkPlugin              *plugin,
                                                  GAsyncResult          *result,
                                                  gchar                **description,
                                                  GError               **error);
gboolean      pk_plugin_get_name                 (PkPlugin              *plugin,
                                                  gchar                **name,
                                                  GError               **error);
void          pk_plugin_get_name_async           (PkPlugin              *plugin,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_plugin_get_name_finish          (PkPlugin              *plugin,
                                                  GAsyncResult          *result,
                                                  gchar                **name,
                                                  GError               **error);
gboolean      pk_plugin_get_plugin_type          (PkPlugin              *plugin,
                                                  gint                  *type,
                                                  GError               **error);
void          pk_plugin_get_plugin_type_async    (PkPlugin              *plugin,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_plugin_get_plugin_type_finish   (PkPlugin              *plugin,
                                                  GAsyncResult          *result,
                                                  gint                  *type,
                                                  GError               **error);
gboolean      pk_plugin_get_version              (PkPlugin              *plugin,
                                                  gchar                **version,
                                                  GError               **error);
void          pk_plugin_get_version_async        (PkPlugin              *plugin,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_plugin_get_version_finish       (PkPlugin              *plugin,
                                                  GAsyncResult          *result,
                                                  gchar                **version,
                                                  GError               **error);

G_END_DECLS

#endif /* __PK_PLUGIN_H__ */
