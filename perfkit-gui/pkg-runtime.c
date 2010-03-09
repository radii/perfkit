/* pkg-runtime.c
 * 
 * Copyright (C) 2010 Christian Hergert
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
#include <gtk/gtk.h>
#include <ethos/ethos.h>
#include <ethos/ethos-ui.h>

#include "pkg-panels.h"
#include "pkg-prefs.h"
#include "pkg-runtime.h"
#include "pkg-window.h"

/**
 * SECTION:pkg-runtime
 * @title: Pkg
 * @short_description: runtime helpers for perfkit-gui.
 *
 * 
 */

static GList        *windows = NULL;
static gboolean      started = FALSE;
static EthosManager *manager = NULL;

/**
 * pkg_runtime_initialize:
 *
 * Initializes the runtime system.
 */
gboolean
pkg_runtime_initialize (gint     *argc,
                        gchar  ***argv,
                        GError  **error)
{
	gchar **plugin_dirs;

	if (!pkg_prefs_init(argc, argv, error)) {
		return FALSE;
	}

	/*
	 * Get the plugin directory paths.
	 */
	plugin_dirs = g_malloc0(sizeof(gchar*) * 3);
	plugin_dirs[0] = g_build_filename(g_get_user_config_dir(),
	                                  "perfkit-gui",
	                                  "plugins",
	                                  NULL);
	plugin_dirs[1] = g_build_filename(PACKAGE_LIB_DIR,
	                                  "perfkit-gui",
	                                  "plugins",
	                                  NULL);

	/*
	 * Set default windows.
	 */
	gtk_window_set_default_icon_name("clock");
	pkg_panels_init();

	/*
	 * Indicate we are started.
	 */
	started = TRUE;

	/*
	 * Prepare the ethos plugin engine.
	 */
	manager = ethos_manager_new();
	ethos_manager_set_app_name(manager, "Perfkit");
	ethos_manager_set_plugin_dirs(manager, plugin_dirs);
	ethos_manager_initialize(manager);

	return TRUE;
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
	gtk_main();
}

/**
 * pkg_runtime_quit:
 *
 * Stops the runtime system and gracefully shuts down the application.
 */
void
pkg_runtime_quit (void)
{
	if (!started) {
		exit(EXIT_FAILURE);
	}
	gtk_main_quit();
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
}

/**
 * pkg_runtime_get_manager:
 *
 * Retrieves the #EthosManager plugin manager for the process.
 *
 * Returns: An #EthosManager instance.
 */
EthosManager*
pkg_runtime_get_manager (void)
{
	return manager;
}
