/* pk-dbus.c
 *
 * Copyright (C) 2010 Christian Hergert
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gmodule.h>
#include <dbus/dbus-glib.h>

#include "pk-dbus.h"
#include "pk-manager-dbus.h"

G_DEFINE_TYPE(PkDbus, pk_dbus, PK_TYPE_CONNECTION)

struct _PkDbusPrivate
{
	DBusGConnection *dbus;
	DBusGProxy      *manager;
};

static gboolean
pk_dbus_manager_ping (PkConnection  *connection,
                      GTimeVal      *tv,
                      GError       **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	gchar *str = NULL;
	gboolean res;

	res = com_dronelabs_Perfkit_Manager_ping(priv->manager, &str, error);
	if (res) {
		g_time_val_from_iso8601(str, tv);
	}
	return res;
}

static void
pk_dbus_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_dbus_parent_class)->finalize(object);
}

static void
pk_dbus_class_init (PkDbusClass *klass)
{
	GObjectClass *object_class;
	PkConnectionClass *conn_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_dbus_finalize;
	g_type_class_add_private(object_class, sizeof(PkDbusPrivate));

	conn_class = PK_CONNECTION_CLASS(klass);
	conn_class->manager_ping = pk_dbus_manager_ping;
}

static void
pk_dbus_init (PkDbus *dbus)
{
	dbus->priv = G_TYPE_INSTANCE_GET_PRIVATE(dbus,
	                                         PK_TYPE_DBUS,
	                                         PkDbusPrivate);
}

G_MODULE_EXPORT GType
pk_connection_plugin (void)
{
	return PK_TYPE_DBUS;
}
