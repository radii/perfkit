/* pk-manager.h
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

#ifndef __PK_MANAGER_H__
#define __PK_MANAGER_H__

#include <gio/gio.h>

#include "pk-connection.h"

G_BEGIN_DECLS

#define PK_TYPE_MANAGER            (pk_manager_get_type())
#define PK_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_MANAGER, PkManager))
#define PK_MANAGER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_MANAGER, PkManager const))
#define PK_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_MANAGER, PkManagerClass))
#define PK_IS_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_MANAGER))
#define PK_IS_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_MANAGER))
#define PK_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_MANAGER, PkManagerClass))

typedef struct _PkManager        PkManager;
typedef struct _PkManagerClass   PkManagerClass;
typedef struct _PkManagerPrivate PkManagerPrivate;

struct _PkManager
{
	GInitiallyUnowned parent;

	/*< private >*/
	PkManagerPrivate *priv;
};

struct _PkManagerClass
{
	GInitiallyUnownedClass parent_class;
};

GType      pk_manager_get_type                   (void) G_GNUC_CONST;
PkManager* pk_manager_new_for_connection         (PkConnection        *connection);
void       pk_manager_get_channels_async         (PkManager           *manager,
                                                  GCancellable        *cancellable,
                                                  GAsyncReadyCallback  callback,
                                                  gpointer             user_data);
gboolean   pk_manager_get_channels_finish        (PkManager            *manager,
                                                  GAsyncResult         *result,
                                                  gint                **channels,
                                                  gsize                *channels_len,
                                                  GError              **error);
void       pk_manager_get_source_plugins_async   (PkManager           *manager,
                                                  GCancellable        *cancellable,
                                                  GAsyncReadyCallback  callback,
                                                  gpointer             user_data);
gboolean   pk_manager_get_source_plugins_finish  (PkManager             *manager,
                                                  GAsyncResult          *result,
                                                  gchar               ***plugins,
                                                  GError               **error);
void       pk_manager_get_version_async          (PkManager           *manager,
                                                  GCancellable        *cancellable,
                                                  GAsyncReadyCallback  callback,
                                                  gpointer             user_data);
gboolean   pk_manager_get_version_finish         (PkManager            *manager,
                                                  GAsyncResult         *result,
                                                  gchar               **version,
                                                  GError              **error);
void       pk_manager_ping_async                 (PkManager           *manager,
                                                  GCancellable        *cancellable,
                                                  GAsyncReadyCallback  callback,
                                                  gpointer             user_data);
gboolean   pk_manager_ping_finish                (PkManager            *manager,
                                                  GAsyncResult         *result,
                                                  GTimeVal             *tv,
                                                  GError              **error);
void       pk_manager_add_channel_async          (PkManager           *manager,
                                                  const PkSpawnInfo   *spawn_info,
                                                  GCancellable        *cancellable,
                                                  GAsyncReadyCallback  callback,
                                                  gpointer             user_data);
gboolean   pk_manager_add_channel_finish         (PkManager            *manager,
                                                  GAsyncResult         *result,
                                                  gint                 *channel,
                                                  GError              **error);
void       pk_manager_remove_channel_async       (PkManager           *manager,
                                                  gint                 channel,
                                                  GCancellable        *cancellable,
                                                  GAsyncReadyCallback  callback,
                                                  gpointer             user_data);
gboolean   pk_manager_remove_channel_finish      (PkManager            *manager,
                                                  GAsyncResult         *result,
                                                  GError              **error);
void       pk_manager_add_subscription_async     (PkManager           *manager,
                                                  gint                 channel,
                                                  gsize                buffer_size,
                                                  gulong               buffer_timeout,
                                                  const gchar         *encoder,
                                                  GCancellable        *cancellable,
                                                  GAsyncReadyCallback  callback,
                                                  gpointer             user_data);
gboolean   pk_manager_add_subscription_finish    (PkManager            *manager,
                                                  GAsyncResult         *result,
                                                  gint                 *subscription,
                                                  GError              **error);
void       pk_manager_remove_subscription_async  (PkManager           *manager,
                                                  gint                 subscription,
                                                  GCancellable        *cancellable,
                                                  GAsyncReadyCallback  callback,
                                                  gpointer             user_data);
gboolean   pk_manager_remove_subscription_finish (PkManager            *manager,
                                                  GAsyncResult         *result,
                                                  GError              **error);

G_END_DECLS

#endif /* __PK_MANAGER_H__ */
