/* main.c
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
#include <glib/gi18n.h>
#include <glib/gthread.h>
#include <stdlib.h>

#include "pka-config.h"
#include "pka-manager.h"

extern void pka_config_init      (const gchar *filename);
extern void pka_config_shutdown  (void);
extern void pka_log_init         (gboolean     stdout_,
                                  const gchar *filename);
extern void pka_log_shutdown     (void);
extern void pka_manager_init     (void);
extern void pka_manager_quit     (void);
extern void pka_manager_run      (void);
extern void pka_manager_shutdown (void);

static gchar *opt_config = (gchar *)PACKAGE_SYSCONFDIR "/perfkit/agent.conf";
static gchar *opt_logfile = NULL;
static gboolean opt_stdout = FALSE;
static GOptionEntry options[] = {
	{ "conf", 'c', 0, G_OPTION_ARG_FILENAME, &opt_config, N_("Configuration filename."), N_("FILE") },
	{ "log", 'l', 0, G_OPTION_ARG_FILENAME, &opt_logfile, N_("Enable logging to FILE."), N_("FILE") },
	{ "stdout", '\0', 0, G_OPTION_ARG_NONE, &opt_stdout, N_("Enable logging to stdout."), NULL },
	{ NULL }
};

static void
sigint_handler(int signum) /* IN */
{
	g_print("\n");
	g_warning("SIGINT caught; shutting down gracefully.");
	pka_manager_quit();
}

gint
main (gint   argc,   /* IN */
      gchar *argv[]) /* IN */
{
	GOptionContext *context;
	GError *error = NULL;

	/*
	 * Setup I18N.
	 */
	textdomain(GETTEXT_PACKAGE);
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	g_set_application_name(_("Perfkit Agent"));

	/*
	 * Parse command line arguments.
	 */
	context = g_option_context_new(_("- Perfkit Agent"));
	g_option_context_add_main_entries(context, options, GETTEXT_PACKAGE);
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_printerr("%s\n", error->message);
		g_error_free(error);
		return EXIT_FAILURE;
	}

	/*
	 * Initialize required GLib subsystems.  It's not required, but better
	 * to initialize threading before the GObject Type System.
	 */
	g_thread_init(NULL);
	g_type_init();

	/*
	 * Initialize Perfkit Agent subsystems.  We initialize logging before
	 * the configuration system so logging is available in the case that
	 * the configuration fails to parse.
	 */
	pka_log_init(opt_stdout, opt_logfile);
	pka_config_init(opt_config);
	pka_manager_init();

	/*
	 * Setup signal handlers to properly shutdown in the case of an error.
	 */
	signal(SIGINT, sigint_handler);

	/*
	 * Block on the pipelines main loop.  This will block until a call to
	 * pka_pipeline_quit() is made.
	 */
	pka_manager_run();

	/*
	 * Shutdown subsystems.
	 */
	pka_manager_shutdown();
	pka_config_shutdown();
	pka_log_shutdown();

	/*
	 * Cleanup after option parsing.
	 */
	g_option_context_free(context);
	g_free(opt_config);
	g_free(opt_logfile);

	return EXIT_SUCCESS;
}
