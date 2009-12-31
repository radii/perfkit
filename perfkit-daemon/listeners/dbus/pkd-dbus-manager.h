/* pkd-dbus-manager.h
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

#ifndef __PKD_DBUS_MANAGER_H__
#define __PKD_DBUS_MANAGER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKD_DBUS_TYPE_MANAGER            (pkd_dbus_manager_get_type())
#define PKD_DBUS_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_DBUS_TYPE_MANAGER, PkdDBusManager))
#define PKD_DBUS_MANAGER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_DBUS_TYPE_MANAGER, PkdDBusManager const))
#define PKD_DBUS_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_DBUS_TYPE_MANAGER, PkdDBusManagerClass))
#define PKD_DBUS_IS_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_DBUS_TYPE_MANAGER))
#define PKD_DBUS_IS_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_DBUS_TYPE_MANAGER))
#define PKD_DBUS_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_DBUS_TYPE_MANAGER, PkdDBusManagerClass))

typedef struct _PkdDBusManager        PkdDBusManager;
typedef struct _PkdDBusManagerClass   PkdDBusManagerClass;
typedef struct _PkdDBusManagerPrivate PkdDBusManagerPrivate;

struct _PkdDBusManager
{
	GObject parent;

	/*< private >*/
	PkdDBusManagerPrivate *priv;
};

struct _PkdDBusManagerClass
{
	GObjectClass parent_class;
};

GType    pkd_dbus_manager_get_type            (void) G_GNUC_CONST;
gboolean pkd_dbus_manager_create_channel      (PkdDBusManager  *manager,
                                               GPid             pid,
                                               const gchar     *target,
                                               gchar          **args,
                                               gchar          **env,
                                               const gchar     *working_dir,
                                               gchar          **channel,
                                               GError         **error);
gboolean pkd_dbus_manager_create_subscription (PkdDBusManager  *manager,
                                               const gchar     *channel,
                                               guint            buffer_size,
                                               guint            buffer_timeout,
                                               const gchar     *encoder_info,
                                               gchar          **subscription,
                                               GError         **error);
gboolean pkd_dbus_manager_get_processes       (PkdDBusManager  *manager,
                                               GPtrArray      **processes,
                                               GError         **error);

G_END_DECLS

#endif /* __PKD_DBUS_MANAGER_H__ */
