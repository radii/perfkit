/* ppg-runtime.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
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

#include <clutter/clutter.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "ppg-actions.h"
#include "ppg-instruments.h"
#include "ppg-log.h"
#include "ppg-paths.h"
#include "ppg-prefs.h"
#include "ppg-runtime.h"
#include "ppg-welcome-dialog.h"
#include "ppg-window.h"

static gint     exit_code   = 0;

/*
 * FIXME: Add command line options.
 */
static gboolean  use_stdount  = TRUE;
static gchar    *log_filename = NULL;

static GOptionEntry option_entries[] = {
	{ NULL }
};

/**
 * ppg_runtime_init:
 * @argc: (inout): A pointer to the programs argc.
 * @argv: (inout): A pointer to the programs argv.
 *
 * Initialize the runtime system for the profiler gui.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and the program should exit.
 * Side effects: Everything.
 */
gboolean
ppg_runtime_init (gint   *argc,
                  gchar **argv[])
{
	GOptionContext *context;
	GError *error = NULL;

	/*
	 * FIXME: Move this into ppg-prefs.
	 */

	context = g_option_context_new(_("- " PRODUCT_NAME));
	g_option_context_add_main_entries(context, option_entries, GETTEXT_PACKAGE);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_add_group(context, clutter_get_option_group());
	if (!g_option_context_parse(context, argc, argv, &error)) {
		g_printerr("%s\n", error->message);
		g_error_free(error);
		return FALSE;
	}

	ppg_log_init(use_stdount, log_filename);
	ppg_paths_init();
	ppg_prefs_init(NULL, NULL); /* FIXME: */
	gtk_window_set_default_icon_name("clock");
	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(),
	                                  ppg_paths_get_icon_dir());
	ppg_instruments_init();
	ppg_actions_init();

	return TRUE;
}

/**
 * ppg_runtime_run:
 *
 * Runs the perfkit profiler application. If an error code was set, it
 * will be returned. The caller should ignore shutdown and exit immediately
 * using the code.
 *
 * Returns: An error code if the caller should fail fast; otherwise Zero.
 * Side effects: Runs the GUI.
 */
gint
ppg_runtime_run (void)
{
	gtk_main();
	return exit_code;
}

/**
 * ppg_runtime_try_quit:
 *
 * Will quit the profiler application if all the windows have been closed
 * and no welcome dialog exists.
 *
 * Returns: FALSE always.
 * Side effects: Program shutdown if no windows exist.
 */
gboolean
ppg_runtime_try_quit (void)
{
	if (!ppg_window_count() && !ppg_welcome_dialog_count()) {
		ppg_runtime_quit();
	}
	return FALSE;
}

/**
 * ppg_runtime_quit:
 *
 * Quits the profiler application.
 *
 * Returns: None.
 * Side effects: The GUI is stopped.
 */
void
ppg_runtime_quit (void)
{
	gtk_main_quit();
}

/**
 * ppg_runtime_quit_fast:
 *
 * Quits the profiler application notifying the owner of the GUI thread
 * to forgo shutdown and fail immediately.
 *
 * Returns: None.
 * Side effects: The GUI is stopped.
 */
void
ppg_runtime_quit_fast (gint code)
{
	if (!code) {
		g_critical("The plugin calling %s() provided an error code of zero. "
		           "Use ppg_runtime_quit() instead.", G_STRFUNC);
		code = 1;
	}

	exit_code = code;
	gtk_main_quit();
}

/**
 * ppg_runtime_shutdown:
 *
 * Cleans up after a running profiler application. Allocated memory will
 * be freed.
 *
 * Returns: None.
 * Side effects: Everything.
 */
void
ppg_runtime_shutdown (void)
{
}
