/* pkd-dbus-manager.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "pkd-dbus.h"
#include "pkd-dbus-manager.h"
#include "pkd-dbus-manager-dbus.h"

G_DEFINE_TYPE (PkdDBusManager, pkd_dbus_manager, G_TYPE_OBJECT)

/**
 * SECTION:pkd_dbus-manager
 * @title: PkdDBusManager
 * @short_description: 
 *
 * 
 */

struct _PkdDBusManagerPrivate
{
	gpointer dummy;
};

gboolean
pkd_dbus_manager_create_channel (PkdDBusManager  *manager,
                                 gchar          **channel,
                                 GError         **error)
{
	return FALSE;
}

gboolean
pkd_dbus_manager_create_subscription (PkdDBusManager  *manager,
                                      const gchar     *channel,
                                      guint            buffer_size,
                                      guint            buffer_timeout,
                                      const gchar     *encoder_info,
                                      gchar          **subscription,
                                      GError         **error)
{
	return FALSE;
}

gboolean
pkd_dbus_manager_get_processes (PkdDBusManager  *manager,
                                GPtrArray      **processes,
                                GError         **error)
{
	return FALSE;
}

static void
pkd_dbus_manager_finalize (GObject *object)
{
	PkdDBusManagerPrivate *priv;

	g_return_if_fail (PKD_DBUS_IS_MANAGER (object));

	priv = PKD_DBUS_MANAGER (object)->priv;

	G_OBJECT_CLASS (pkd_dbus_manager_parent_class)->finalize (object);
}

static void
pkd_dbus_manager_class_init (PkdDBusManagerClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_dbus_manager_finalize;
	g_type_class_add_private (object_class, sizeof (PkdDBusManagerPrivate));

	dbus_g_object_type_install_info(PKD_DBUS_TYPE_MANAGER,
	                                &dbus_glib_pkd_dbus_manager_object_info);
}

static void
pkd_dbus_manager_init (PkdDBusManager *manager)
{
	manager->priv = G_TYPE_INSTANCE_GET_PRIVATE (manager,
	                                             PKD_DBUS_TYPE_MANAGER,
	                                             PkdDBusManagerPrivate);

	dbus_g_connection_register_g_object(pkd_dbus_get_connection(),
	                                    "/com/dronelabs/Perfkit/Manager",
	                                    G_OBJECT(manager));
}
