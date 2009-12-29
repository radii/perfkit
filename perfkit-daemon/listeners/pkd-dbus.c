/* pkd-dbus.c
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

#include <gmodule.h>
#include <dbus/dbus-glib.h>

#include "pkd-dbus.h"

G_DEFINE_TYPE (PkdDBus, pkd_dbus, PKD_TYPE_LISTENER)

struct _PkdDBusPrivate
{
	DBusGConnection *conn;
};

/**
 * pkd_dbus_new:
 *
 * Creates a new instance of #PkdDBus.
 *
 * Return value: the newly created #PkdDBus instance.
 */
PkdListener*
pkd_dbus_new(void)
{
	return g_object_new (PKD_TYPE_DBUS, NULL);
}

static gboolean
pkd_dbus_start(PkdListener  *listener,
              GError     **error)
{
	return TRUE;
}

static void
pkd_dbus_stop(PkdListener *listener)
{
}

static void
pkd_dbus_finalize(GObject *object)
{
	PkdDBusPrivate *priv;

	g_return_if_fail (PKD_IS_DBUS (object));

	priv = PKD_DBUS (object)->priv;

	G_OBJECT_CLASS (pkd_dbus_parent_class)->finalize (object);
}

static void
pkd_dbus_class_init(PkdDBusClass *klass)
{
	GObjectClass *object_class;
	PkdListenerClass *listener_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pkd_dbus_finalize;
	g_type_class_add_private (object_class, sizeof(PkdDBusPrivate));

	listener_class = PKD_LISTENER_CLASS(klass);
	listener_class->start = pkd_dbus_start;
	listener_class->stop = pkd_dbus_stop;
}

static void
pkd_dbus_init(PkdDBus *dbus)
{
	dbus->priv = G_TYPE_INSTANCE_GET_PRIVATE(dbus,
	                                         PKD_TYPE_DBUS,
	                                         PkdDBusPrivate);
}

G_MODULE_EXPORT void
pkd_listener_register(void)
{
	PkdListener *listener;

	if (pkd_config_get_boolean("listener.dbus", "disabled", FALSE)) {
		g_message("DBus listener disabled by config.");
		return;
	}

	listener = pkd_dbus_new();
	pkd_pipeline_add_listener(listener);
}
