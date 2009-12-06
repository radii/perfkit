/* pkg-runtime.c
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

#include <stdlib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "pkg-paths.h"
#include "pkg-runtime.h"
#include "pkg-service.h"
#include "services/pkg-gui-service.h"

static GMainLoop       *main_loop     = NULL;
static GList           *services      = NULL;
static GHashTable      *services_hash = NULL;
static gboolean         started       = FALSE;
static DBusGConnection *dbus_conn     = NULL;

G_LOCK_DEFINE (services);

/**
 * pkg_runtime_initialize:
 *
 * Initializes the runtime system.
 */
void
pkg_runtime_initialize (void)
{
	GError *error = NULL;

	g_return_if_fail (started == FALSE);

	main_loop = g_main_loop_new (NULL, FALSE);
	services_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

	/* TODO: support other busses */
	if (!(dbus_conn = dbus_g_bus_get (DBUS_BUS_SESSION, &error))) {
		g_printerr ("%s", error->message);
		exit (EXIT_FAILURE);
	}

	/* if we do not get the bus name, another exists and we can exit */
	if (dbus_bus_request_name (dbus_g_connection_get_connection (dbus_conn),
	                           "com.dronelabs.Perfkit",
	                           DBUS_NAME_FLAG_DO_NOT_QUEUE,
	                           NULL) == DBUS_REQUEST_NAME_REPLY_EXISTS) {
		g_printerr ("Existing instance of Pkg was found. Exiting gracefully.\n");
		exit (EXIT_SUCCESS);
	}

	/* add core services */
	pkg_runtime_add_service ("Gui", g_object_new (PKG_TYPE_GUI_SERVICE, NULL));

	started = TRUE;
}

/**
 * pkg_runtime_run:
 *
 * Starts the runtime system.  This method will block using a main loop for the
 * duration of applications life-time.
 */
void
pkg_runtime_run (void)
{
	g_return_if_fail (main_loop != NULL);
	g_main_loop_run (main_loop);
}

/**
 * pkg_runtime_quit:
 *
 * Stops the runtime system and gracefully shuts down the application.
 */
void
pkg_runtime_quit (void)
{
	g_return_if_fail (main_loop != NULL);
	g_main_loop_quit (main_loop);
}

/**
 * pkg_runtime_shutdown:
 *
 * Gracefully cleans up after the runtime.  This should be called in the
 * main thread after the runtim has quit.
 */
void
pkg_runtime_shutdown (void)
{
	G_LOCK (services);

	g_list_foreach (services, (GFunc)pkg_service_stop, NULL);
	g_list_foreach (services, (GFunc)g_object_unref, NULL);
	g_hash_table_destroy (services_hash);

	services_hash = NULL;
	services = NULL;

	G_UNLOCK (services);

	main_loop = NULL;
}

/**
 * pkg_runtime_add_service:
 * @name: the name of the service
 * @service: a #PkgService instance
 *
 * Adds a new #PkgService to the runtime.  If the runtime system has been
 * started, the service will in-turn be started.
 */
void
pkg_runtime_add_service (const gchar *name,
                         PkgService *service)
{
	gboolean  needs_start = FALSE;
	GError   *error       = NULL;

	G_LOCK (services);

	if (NULL == g_hash_table_lookup (services_hash, name)) {
		g_hash_table_insert (services_hash, g_strdup (name), g_object_ref (service));
		services = g_list_append (services, g_object_ref (service));
		needs_start = started;

		/* TODO: Enable DBUS service bridge.
		 *
		gchar *path = g_strdup_printf ("/com/dronelabs/PerfkitGui/%s", name);
		dbus_g_connection_register_g_object (dbus_conn, path, G_OBJECT (service));
		g_free (path);
		 *
		 */
	}
	else {
		g_warning ("Service named \"%s\" already exists!", name);
	}

	G_UNLOCK (services);

	if (needs_start) {
		if (!pkg_service_start (service, &error)) {
			g_warning ("%s", error->message);
			g_error_free (error);
			pkg_runtime_remove_service (name);
		}
	}
}

/**
 * pkg_runtime_remove_service:
 * @name: the name of the service
 *
 * Removes an existing #PkgService from the runtime.  If the runtime system
 * has already by started, the service will be stopped before-hand.
 */
void
pkg_runtime_remove_service (const gchar *name)
{
	PkgService *service;
	gboolean needs_stop;

	G_LOCK (services);

	needs_stop = started;

	if (NULL != (service = g_hash_table_lookup (services_hash, name))) {
		services = g_list_remove (services, service);
		g_hash_table_remove (services_hash, name);
	}

	G_UNLOCK (services);

	if (needs_stop && service)
		pkg_service_stop (service);
}

/**
 * pkg_runtime_get_service:
 * @name: the name of the service
 *
 * Retrieves the #PkgService instance registered with the name @name.
 *
 * Return value: the #PkgService instance or %NULL.
 */
PkgService*
pkg_runtime_get_service (const gchar *name)
{
	PkgService *service;

	g_return_val_if_fail (services_hash != NULL, NULL);

	G_LOCK (services);
	service = g_hash_table_lookup (services_hash, name);
	G_UNLOCK (services);

	return service;
}

/**
 * pkg_runtime_get_connection:
 *
 * Retrieves the D-BUS connection for the application.
 *
 * Return value: the #DBusGConnection
 */
DBusGConnection*
pkg_runtime_get_connection (void)
{
	return dbus_conn;
}
