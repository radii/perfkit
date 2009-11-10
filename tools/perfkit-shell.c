/* perfkit-shell.c
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
#include <config.h>
#endif

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>

#include <egg-line.h>

static EggLineEntry* channel_iter (EggLine *line, const gchar *text, gchar **end);
static void          missing_cmd  (EggLine *line, const gchar *text, gpointer user_data);
static void          ls_cb        (EggLine *line, gchar **args);
static void          cd_cb        (EggLine *line, gchar **args);

static GOptionEntry op_entries[] =
{
	{ NULL }
};

static EggLineEntry entries[] =
{
	{ "channel", channel_iter, NULL, "Manage perfkit data channels" },
	{ "source", NULL, NULL, "Manage perfkit data sources" },
	{ "help", NULL, NULL, "Get help on a command" },
	{ "ls", NULL, ls_cb, "List the contents of the current directory" },
	{ "cd", NULL, cd_cb, "Change the current directory" },
	{ NULL }
};

static EggLineEntry channel_entries[] =
{
	{ "show", NULL, NULL, "Show perfkit data channels" },
	{ "add", NULL, NULL, "Add a new perfkit data channel" },
	{ "remove", NULL, NULL, "Remove an existing perfkit data channel" },
	{ NULL }
};

gint
main (gint   argc,
      gchar *argv[])
{
	GOptionContext *context;
	GError         *error   = NULL;
	EggLine        *line;

	/* parse command line arguments */
	context = g_option_context_new ("- interactive perfkit shell");
	g_option_context_add_main_entries (context, op_entries, GETTEXT_PACKAGE);
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_printerr ("%s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	/* initialize gobject */
	g_type_init ();

	/* run the readline loop */
	line = egg_line_new ();
	egg_line_set_prompt (line, "perfkit> ");
	egg_line_set_entries (line, entries);
	g_signal_connect (line, "missing", G_CALLBACK (missing_cmd), NULL);
	egg_line_run (line);

	return EXIT_SUCCESS;
}

static void
missing_cmd (EggLine     *line,
             const gchar *text,
             gpointer     user_data)
{
	g_printerr ("Command not found: %s\n", text);
}

static EggLineEntry*
channel_iter (EggLine      *line,
              const gchar  *text,
              gchar       **end)
{
	return channel_entries;
}

static void
ls_cb (EggLine  *line,
       gchar   **args)
{
	gchar **cmd;
	gchar  *output = NULL;
	GError *error  = NULL;
	gint    len, i, j;

	len = g_strv_length (args);
	cmd = g_malloc0 ((len + 2) * sizeof (gchar*));

	cmd [0] = g_strdup ("ls");
	for (i = 0, j = 1; i < len; i++)
		if (args [i] && strlen (args [i]))
			cmd [j++] = g_strdup (args [i]);

	if (!g_spawn_sync (g_get_current_dir (),
	                   cmd,
	                   NULL,
	                   G_SPAWN_SEARCH_PATH,
	                   NULL,
	                   NULL,
	                   &output,
	                   NULL,
	                   NULL,
	                   &error))
	{
		g_printerr ("%s\n", error->message);
		g_error_free (error);
		error = NULL;
	}
	else {
		g_print ("%s", output);
		g_free (output);
	}

	g_strfreev (cmd);
}

static void
cd_cb (EggLine  *line,
       gchar   **args)
{
	g_return_if_fail (args != NULL);

	if (!args [0] || !strlen (args [0]))
		g_chdir (g_get_home_dir ());
	else
		g_chdir (args [0]);
}
