/* main.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <clutter/clutter.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>

#include "ppg-log.h"
#include "ppg-welcome.h"

/*
 * Subsystem initialization.
 */
extern void ppg_config_init     (void);
extern void ppg_config_shutdown (void);
extern void ppg_log_init        (gboolean stdout_);
extern void ppg_log_shutdown    (void);
extern void ppg_panels_init     (void);

static GOptionEntry options[] = {
	{ NULL }
};

/**
 * sig_handler:
 * @signum: Signal number.
 *
 * Handles posix signals.
 *
 * Returns: None.
 * Side effects: Shuts down the system.
 */
static void
sig_handler (gint signum)
{
	g_print("\n");
	gtk_main_quit();
}

gint
main (gint   argc,
      gchar *argv[])
{
	GOptionContext *context;
	GError *error = NULL;

	/* initialize i18n */
	textdomain(GETTEXT_PACKAGE);
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	g_set_application_name(_("Perfkit Profiler"));

	/* initialize threading early */
	g_thread_init(NULL);

	/* parse command line arguments */
	context = g_option_context_new(_("- Perfkit Profiler"));
	g_option_context_add_main_entries(context, options, GETTEXT_PACKAGE);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_add_group(context, clutter_get_option_group());
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_printerr("%s\n", error->message);
		g_error_free(error);
		return EXIT_FAILURE;
	}

	/*
	 * Configure subsystems.
	 */
	ppg_log_init(TRUE);
	ppg_config_init();
	ppg_panels_init();

	/*
	 * Register signal handler.
	 */
	signal(SIGINT, sig_handler);

	/*
	 * Show the welcome dialog.
	 */
	gtk_window_set_default_icon_name("clock");
	ppg_welcome_show();
	gtk_main();

	/*
	 * Cleanup and shutdown.
	 */
	INFO("Shutting down gracefully.");
	ppg_config_shutdown();
	ppg_log_shutdown();

	return EXIT_SUCCESS;
}
