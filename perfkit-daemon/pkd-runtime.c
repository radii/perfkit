/* pkd-runtime.c
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
#include <ethos/ethos.h>

#include "pkd-channels.h"
#include "pkd-paths.h"
#include "pkd-runtime.h"
#include "pkd-service.h"
#include "pkd-source.h"
#include "pkd-source-simple.h"
#include "pkd-sources.h"

/**
 * SECTION:pkd-runtime
 * @title: PkdRuntime
 * @short_description: Runtime application services
 *
 * #PkdRuntime provides access to settings and runtime services within
 * the Perfkit Daemon process.
 */

static GMainLoop       *main_loop     = NULL;
static GList           *services      = NULL;
static GHashTable      *services_hash = NULL;
static gboolean         started       = FALSE;
static DBusGConnection *dbus_conn     = NULL;
static EthosManager    *manager       = NULL;

G_LOCK_DEFINE (services);

/**
 * pkd_runtime_initialize:
 *
 * Initializes and prepares the daemon for execution.
 *
 * Side effects:
 *       The process is setup and new services, caches, and other necessities
 *       are created.
 */
void
pkd_runtime_initialize (void)
{
	static gchar *plugin_dirs[2];
	GError       *error = NULL;

	g_return_if_fail (started == FALSE);

	g_message ("%s: Initializing runtime", G_STRFUNC);

	main_loop = g_main_loop_new (NULL, FALSE);
	services_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

	/* connect to dbus */
	if (!(dbus_conn = dbus_g_bus_get (DBUS_BUS_SESSION, &error))) {
		g_printerr ("%s", error->message);
		exit (EXIT_FAILURE);
	}

	/* if we do not get the bus name, another exists and we can exit */
	if (dbus_bus_request_name (dbus_g_connection_get_connection (dbus_conn),
	                           "com.dronelabs.Perfkit",
	                           DBUS_NAME_FLAG_DO_NOT_QUEUE,
	                           NULL) == DBUS_REQUEST_NAME_REPLY_EXISTS) {
		g_printerr ("Existing instance of perfkit-daemon was found. "
		            "Exiting gracefully.\n");
		exit (EXIT_SUCCESS);
	}

	g_message ("%s: Adding core services.", G_STRFUNC);

	started = TRUE;

	/* register core services */
	pkd_runtime_add_service ("Channels", g_object_new (PKD_TYPE_CHANNELS, NULL));
	pkd_runtime_add_service ("Sources", g_object_new (PKD_TYPE_SOURCES, NULL));

	/* load default listeners */

	g_message ("%s: Activating listeners.", G_STRFUNC);

	/* HACK:
	 *   This hack is here to ensure that pkd-source-simple.o is linked
	 *   into the resulting executable.  Due to it not being directly
	 *   used, the linker thinks it can completely drop the .o file.
	 */
	(void)g_type_children (PKD_TYPE_SOURCE_SIMPLE, NULL);

	g_message ("%s: Registering plugins.", G_STRFUNC);

	/* register plugins */
	if (g_getenv ("PERFKIT_PLUGIN_DIR"))
		plugin_dirs [0] = (gchar*)g_getenv ("PERFKIT_PLUGIN_DIR");
	else
		plugin_dirs [0] = (gchar*)PACKAGE_LIB_DIR  G_DIR_SEPARATOR_S
		                  "perfkit-daemon" G_DIR_SEPARATOR_S
		                  "plugins";
	plugin_dirs [1] = NULL;
	manager = ethos_manager_new ();
	ethos_manager_set_app_name (manager, "Pkd");
	ethos_manager_set_plugin_dirs (manager, plugin_dirs);
	ethos_manager_initialize (manager);

	g_message ("%s: Plugins registered.", G_STRFUNC);
}

/**
 * pkd_runtime_run:
 *
 * Starts the runtime system.  This method will block using a main loop for the
 * duration of applications life-time.
 *
 * Side effects: None
 */
void
pkd_runtime_run (void)
{
	g_return_if_fail (main_loop != NULL);
	g_main_loop_run (main_loop);
}

/**
 * pkd_runtime_quit:
 *
 * Stops the runtime system and gracefully shuts down the application.
 *
 * Side effects: None
 */
void
pkd_runtime_quit (void)
{
	g_return_if_fail (main_loop != NULL);
	g_main_loop_quit (main_loop);
}

/**
 * pkd_runtime_shutdown:
 *
 * Gracefully cleans up after the runtime.  This should be called in the
 * main thread after the runtim has quit.
 *
 * Side effects: None
 */
void
pkd_runtime_shutdown (void)
{
	G_LOCK (services);

	g_list_foreach (services, (GFunc)pkd_service_stop, NULL);
	g_list_foreach (services, (GFunc)g_object_unref, NULL);
	g_hash_table_destroy (services_hash);

	services_hash = NULL;
	services = NULL;

	G_UNLOCK (services);

	main_loop = NULL;
}

/**
 * pkd_runtime_add_service:
 * @name: the name of the service
 * @service: a #PkdService instance
 *
 * Adds a new #PkdService to the runtime.  If the runtime system has been
 * started, the service will in-turn be started.
 *
 * Side effects: A reference is taken to @service.
 */
void
pkd_runtime_add_service (const gchar *name,
                         PkdService  *service)
{
	gboolean  needs_start = FALSE;
	GError   *error       = NULL;
	gchar    *path;

	g_return_if_fail (PKD_IS_SERVICE (service));

	G_LOCK (services);

	if (NULL == g_hash_table_lookup (services_hash, name)) {
		g_hash_table_insert (services_hash, g_strdup (name), g_object_ref (service));
		services = g_list_append (services, g_object_ref (service));
		needs_start = started;

		path = g_strdup_printf ("/com/dronelabs/Perfkit/%s", name);
		dbus_g_connection_register_g_object (dbus_conn, path, G_OBJECT (service));
		g_free (path);
	}
	else {
		g_warning ("Service named \"%s\" already exists!", name);
	}

	G_UNLOCK (services);

	if (needs_start) {
		if (!pkd_service_start (service, &error)) {
			g_warning ("%s", error->message);
			g_error_free (error);
			pkd_runtime_remove_service (name);
		}
	}
}

/**
 * pkd_runtime_remove_service:
 * @name: the name of the service
 *
 * Removes an existing #PkdService from the runtime.  If the runtime system
 * has already by started, the service will be stopped before-hand.
 *
 * Side effects: None
 */
void
pkd_runtime_remove_service (const gchar *name)
{
	PkdService *service;
	gboolean needs_stop;

	G_LOCK (services);

	needs_stop = started;

	if (NULL != (service = g_hash_table_lookup (services_hash, name))) {
		services = g_list_remove (services, service);
		g_hash_table_remove (services_hash, name);
	}

	G_UNLOCK (services);

	if (needs_stop && service)
		pkd_service_stop (service);
}

/**
 * pkd_runtime_get_service:
 * @name: the name of the service
 *
 * Retrieves the #PkdService instance registered with the name @name.
 *
 * Return value: the #PkdService instance or %NULL.
 *
 * Side effects: None
 */
PkdService*
pkd_runtime_get_service (const gchar *name)
{
	PkdService *service;

	g_return_val_if_fail (services_hash != NULL, NULL);

	G_LOCK (services);
	service = g_hash_table_lookup (services_hash, name);
	G_UNLOCK (services);

	return service;
}

/**
 * pkd_runtime_get_connection:
 *
 * Retrieves the D-BUS connection for the application.
 *
 * Return value: the #DBusGConnection
 *
 * Side effects: None
 */
DBusGConnection*
pkd_runtime_get_connection (void)
{
	return dbus_conn;
}
