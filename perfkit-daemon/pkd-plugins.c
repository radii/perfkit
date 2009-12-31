/* pkd-plugins.c
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

#include "pkd-encoder-info.h"
#include "pkd-pipeline.h"
#include "pkd-plugins.h"
#include "pkd-source-info.h"

static void
pkd_plugins_init_encoders (void)
{
	PkdEncoderInfo *info;
	gchar *plugin_dir;
	GDir *dir;
	GError *error;
	gchar *path;
	const gchar *name;

	/*
	 * Retrieve the plugin directory.
	 */
	if (g_getenv("PERFKIT_ENCODERS_DIR"))
		plugin_dir = g_strdup(g_getenv("PERFKIT_ENCODERS_DIR"));
	else
		plugin_dir = g_build_filename(PACKAGE_LIB_DIR, "perfkit-daemon", "encoders", NULL);

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
		info = pkd_encoder_info_new();
		path = g_build_filename(plugin_dir, name, NULL);
		error = NULL;

		if (!pkd_encoder_info_load_from_file(info, path, &error)) {
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
		pkd_pipeline_add_encoder_info(info);
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

static void
pkd_plugins_init_sources (void)
{
	PkdSourceInfo *info;
	gchar *plugin_dir;
	GDir *dir;
	GError *error;
	gchar *path;
	const gchar *name;

	/*
	 * Retrieve the plugin directory.
	 */
	if (g_getenv("PERFKIT_SOURCES_DIR"))
		plugin_dir = g_strdup(g_getenv("PERFKIT_SOURCES_DIR"));
	else
		plugin_dir = g_build_filename(PACKAGE_LIB_DIR, "perfkit-daemon", "sources", NULL);

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
		info = pkd_source_info_new();
		path = g_build_filename(plugin_dir, name, NULL);
		error = NULL;

		if (!pkd_source_info_load_from_file(info, path, &error)) {
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
		pkd_pipeline_add_source_info(info);
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

static void
pkd_plugins_init_listeners (void)
{
	void (*Pkd_listener_register) (void);
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
		listener_dir = g_build_filename(PACKAGE_LIB_DIR, "perfkit-daemon", "listeners", NULL);

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
		 * Lookup the pkd_listener_register symbol.
		 */
		g_module_symbol(module, "pkd_listener_register",
		                (gpointer *)&Pkd_listener_register);

		/*
		 * Execute the symbol.
		 * If it didn't exist, just close the module.
		 */
		if (Pkd_listener_register) {
			Pkd_listener_register();
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
 * pkd_plugins_init:
 *
 * Initializes the plugin subsystem.  Plugins will be introspected and loaded
 * into the process space.  This method should only ever be called once;
 * during startup.
 *
 * Side effects: Plugin shared libraries are loaded into the process space.
 */
void
pkd_plugins_init (void)
{
	pkd_plugins_init_sources();
	pkd_plugins_init_encoders();
	pkd_plugins_init_listeners();
}
