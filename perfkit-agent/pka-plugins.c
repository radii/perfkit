/* pka-plugins.c
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
#include <gmodule.h>

#include "pka-encoder-info.h"
#include "pka-pipeline.h"
#include "pka-plugins.h"
#include "pka-source-info.h"

/**
 * SECTION:pka-plugins
 * @title: Plugins
 * @short_description: Runtime plugin management
 *
 * This module provides control over the plugin subsystem.  When initialized,
 * it will crack open the available plugins on the system and extract the
 * necessary meta-data for use during runtime.
 *
 * Listeners have a callback method which is invoked to setup the listener
 * during startup.  The listeners will often check the configuration file
 * to see if they are enabled.  If not, they do nothing.
 *
 * Both #PkaSource and #PkaEncoder instances are created dynamically through
 * a factory method within the plugin modules.  Information on these plugins
 * is extracted and loaded into #PkaSourceInfo and #PkaEncoderInfo objects
 * accordingly.
 */

static GList *listeners = NULL;

/**
 * pka_plugins_init_encoders:
 *
 * Initialize the #PkaEncoder plugins.  This is performed by opening the
 * various shared libraries and extracting the pk_encoder_plugin symbol to
 * find more information.
 *
 * Side effects:
 *   The encoder shared modules are opened and introspected.
 */
static void
pka_plugins_init_encoders (void)
{
	PkaEncoderInfo *info;
	gchar *plugin_dir;
	GDir *dir;
	GError *error;
	gchar *path;
	const gchar *name;

	/*
	 * Retrieve the plugin directory.
	 */
	if (g_getenv("PERFKIT_ENCODERS_DIR")) {
		plugin_dir = g_strdup(g_getenv("PERFKIT_ENCODERS_DIR"));
	} else {
		plugin_dir = g_build_filename(PACKAGE_LIB_DIR,
		                              "perfkit-agent",
		                              "encoders",
		                              NULL);
	}

	/*
	 * Open the plugin directory.
	 */
	dir = g_dir_open(plugin_dir, 0, NULL);
	if (!dir) {
		g_warning("Could not open encoders directory: %s", plugin_dir);
		goto error;
	}

	/*
	 * Attempt to load the modules found in the directory.
	 */
	name = g_dir_read_name(dir);
	while (name) {
		if (!g_str_has_suffix(name, G_MODULE_SUFFIX))
			goto next;

		g_message("Loading %s encoder module.", name);
		info = pka_encoder_info_new();
		path = g_build_filename(plugin_dir, name, NULL);
		error = NULL;

		if (!pka_encoder_info_load_from_file(info, path, &error)) {
			g_warning("%s", error->message);
			g_free(path);
			g_error_free(error);
			g_object_unref(info);
			goto next;
		}

		g_free(path);

		/*
		 * Store the source info.
		 */
		pka_pipeline_add_encoder_info(info);
		g_object_unref(info);

	next:
		/*
		 * Read the next filename.
		 */
		name = g_dir_read_name(dir);
	}

error:
	if (dir)
		g_dir_close(dir);
	g_free(plugin_dir);
}

/**
 * pka_plugins_init_sources:
 *
 * Initialize the #PkaSource plugins.  This is performed by opening the
 * various shared libraries and extracting the pk_source_plugin symbol to
 * find more information.
 *
 * Side effects:
 *   The source plugin shared modules are opened and introspected.
 */
static void
pka_plugins_init_sources (void)
{
	PkaSourceInfo *info;
	gchar *plugin_dir;
	GDir *dir;
	GError *error;
	gchar *path;
	const gchar *name;

	/*
	 * Retrieve the plugin directory.
	 */
	if (g_getenv("PERFKIT_SOURCES_DIR")) {
		plugin_dir = g_strdup(g_getenv("PERFKIT_SOURCES_DIR"));
	} else {
		plugin_dir = g_build_filename(PACKAGE_LIB_DIR,
		                              "perfkit-agent",
		                              "sources",
		                              NULL);
	}

	/*
	 * Open the plugin directory.
	 */
	dir = g_dir_open(plugin_dir, 0, NULL);
	if (!dir) {
		g_warning("Could not open sources directory: %s", plugin_dir);
		goto error;
	}

	/*
	 * Attempt to load the modules found in the directory.
	 */
	name = g_dir_read_name(dir);
	while (name) {
		if (!g_str_has_suffix(name, G_MODULE_SUFFIX))
			goto next;

		g_message("Loading %s source module.", name);
		info = pka_source_info_new();
		path = g_build_filename(plugin_dir, name, NULL);
		error = NULL;

		if (!pka_source_info_load_from_file(info, path, &error)) {
			g_warning("%s", error->message);
			g_free(path);
			g_error_free(error);
			g_object_unref(info);
			goto next;
		}

		g_free(path);

		/*
		 * Store the source info.
		 */
		pka_pipeline_add_source_info(info);
		g_object_unref(info);

	next:
		/*
		 * Read the next filename.
		 */
		name = g_dir_read_name(dir);
	}

error:
	if (dir)
		g_dir_close(dir);
	g_free(plugin_dir);
}

/**
 * pka_plugins_init_listeners:
 *
 * Initializes the available #PkaListener plugins.  This is performed by
 * opening the listener plugins shared-modules and locating the
 * pka_listener_register symbol.  That symbol is executed at which point
 * the module is reponsible for creating a #PkaListener instance if
 * necessary and registering it with the pipeline via
 * pka_pipeline_add_listener().
 */
static void
pka_plugins_init_listeners (void)
{
	void (*Pka_listener_register) (void);
	GModule *module;
	const gchar *name;
	gchar *listener_dir;
	gchar *path;
	GDir *dir;

	/*
	 * Retrieve the listeners directory.
	 */
	if (g_getenv("PERFKIT_LISTENERS_DIR"))
		listener_dir = g_strdup(g_getenv("PERFKIT_LISTENERS_DIR"));
	else
		listener_dir = g_build_filename(PACKAGE_LIB_DIR, "perfkit-agent", "listeners", NULL);

	/*
	 * Open the listener directory.
	 */
	dir = g_dir_open(listener_dir, 0, NULL);
	if (!dir) {
		g_warning("Could not open listener directory: %s", listener_dir);
		goto error;
	}

	/*
	 * Attempt to load the modules found in the directory.
	 */
	name = g_dir_read_name(dir);
	while (name) {
		if (!g_str_has_suffix(name, G_MODULE_SUFFIX))
			goto next;

		g_message("Loading %s listener module.", name);
		path = g_build_filename(listener_dir, name, NULL);

		/*
		 * Crack open the module.
		 */
		module = g_module_open(path, G_MODULE_BIND_MASK);
		g_free(path);
		if (!module)
			goto next;

		/*
		 * Lookup the pka_listener_register symbol.
		 */
		g_module_symbol(module, "pka_listener_register",
		                (gpointer *)&Pka_listener_register);

		/*
		 * Execute the symbol.
		 * If it didn't exist, just close the module.
		 */
		if (Pka_listener_register) {
			Pka_listener_register();
			listeners = g_list_append(listeners, module);
		} else {
			g_module_close(module);
		}

	next:
		/*
		 * Read the next filename.
		 */
		name = g_dir_read_name(dir);
	}

error:
	if (dir)
		g_dir_close(dir);
	g_free(listener_dir);
}

/**
 * pka_plugins_init:
 *
 * Initializes the plugin subsystem.  Plugins will be introspected and loaded
 * into the process space.  This method should only ever be called once;
 * during startup.
 *
 * Side effects:
 *   Plugin shared libraries are loaded into the process space.
 */
void
pka_plugins_init (void)
{
	pka_plugins_init_sources();
	pka_plugins_init_encoders();
	pka_plugins_init_listeners();
}

/**
 * pka_plugins_shutdown:
 *
 * Shuts down the plugin subsystem.
 *
 * Returns: None.
 * Side effects: Listener modules are unloaded.
 */
void
pka_plugins_shutdown (void)
{
	/*
	 * Don't unload modules if we are running under valgrind.  This is required
	 * because it will cause issues with valgrind resolving symbols.
	 */
	if (!g_getenv("PERFKIT_VALGRIND")) {
		g_list_foreach(listeners, (GFunc)g_module_close, NULL);
	}

	g_list_free(listeners);
}
