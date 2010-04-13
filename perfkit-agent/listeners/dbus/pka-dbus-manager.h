/* pka-dbus-manager.h
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

#ifndef __PKA_DBUS_MANAGER_H__
#define __PKA_DBUS_MANAGER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKA_DBUS_TYPE_MANAGER            (pka_dbus_manager_get_type())
#define PKA_DBUS_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_DBUS_TYPE_MANAGER, PkaDBusManager))
#define PKA_DBUS_MANAGER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_DBUS_TYPE_MANAGER, PkaDBusManager const))
#define PKA_DBUS_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKA_DBUS_TYPE_MANAGER, PkaDBusManagerClass))
#define PKA_DBUS_IS_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKA_DBUS_TYPE_MANAGER))
#define PKA_DBUS_IS_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKA_DBUS_TYPE_MANAGER))
#define PKA_DBUS_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKA_DBUS_TYPE_MANAGER, PkaDBusManagerClass))
#define PKA_DBUS_MANAGER_ERROR           (pka_dbus_manager_error_quark())

typedef enum
{
	PKA_DBUS_MANAGER_ERROR_SUBSCRIPTION,
} PkaDBusManagerError;

typedef struct _PkaDBusManager        PkaDBusManager;
typedef struct _PkaDBusManagerClass   PkaDBusManagerClass;
typedef struct _PkaDBusManagerPrivate PkaDBusManagerPrivate;

struct _PkaDBusManager
{
	GObject parent;

	/*< private >*/
	PkaDBusManagerPrivate *priv;
};

struct _PkaDBusManagerClass
{
	GObjectClass parent_class;
};

GType    pka_dbus_manager_get_type            (void) G_GNUC_CONST;
GQuark   pka_dbus_manager_error_quark         (void) G_GNUC_CONST;
gboolean pka_dbus_manager_create_channel      (PkaDBusManager  *manager,
                                               GPid             pid,
                                               const gchar     *target,
                                               gchar          **args,
                                               gchar          **env,
                                               const gchar     *working_dir,
                                               gchar          **channel,
                                               GError         **error);
gboolean pka_dbus_manager_create_subscription (PkaDBusManager  *manager,
                                               const gchar     *delivery_address,
                                               const gchar     *delivery_path,
                                               const gchar     *channel,
                                               guint            buffer_size,
                                               guint            buffer_timeout,
                                               const gchar     *encoder_info,
                                               gchar          **subscription,
                                               GError         **error);
gboolean pka_dbus_manager_get_processes       (PkaDBusManager  *manager,
                                               GPtrArray      **processes,
                                               GError         **error);
gboolean pka_dbus_manager_get_version         (PkaDBusManager  *manager,
                                               gchar          **version,
                                               GError         **error);
gboolean pka_dbus_manager_remove_channel      (PkaDBusManager  *manager,
                                               const gchar     *path,
                                               GError         **error);
gboolean pka_dbus_manager_get_source_plugins  (PkaDBusManager   *manager,
                                               gchar          ***paths,
                                               GError          **error);

G_END_DECLS

#endif /* __PKA_DBUS_MANAGER_H__ */
