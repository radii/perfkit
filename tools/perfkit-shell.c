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

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>

#include <egg-line.h>

#include "pk-channel-dbus.h"
#include "pk-channels-dbus.h"

#define REPORT_ERROR(e) report_error (e, G_STRFUNC, __LINE__);

static EggLineEntry* channel_iter (EggLine *line, const gchar *text, gchar **end);
static void missing_cmd (EggLine *line, const gchar *text, gpointer user_data);
static EggLineStatus ls_cb (EggLine *line, gchar **args);
static EggLineStatus cd_cb (EggLine *line, gchar **args);
static EggLineStatus channel_show_cb (EggLine *line, gchar **args);
static EggLineStatus channel_add_cb (EggLine *line, gchar **args);
static EggLineStatus channel_remove_cb (EggLine *line, gchar **args);

static gboolean use_system = FALSE;

static GOptionEntry op_entries[] =
{
	{ "system", 's', 0, G_OPTION_ARG_NONE, &use_system, "", NULL },
	{ NULL }
};

static DBusGProxy      *channels  = NULL;
static DBusGConnection *dbus_conn = NULL;

static EggLineEntry entries[] =
{
	{ "channel", channel_iter, NULL,
	  "Manage perfkit data channels",
	  "channel [add|remove|show]" },
	{ "source", NULL, NULL,
	  "Manage perfkit data sources",
	  "source [add|remove|show]" },
	{ "help", NULL, NULL,
	  "Get help on a command",
	  "help <command>" },
	{ "ls", NULL, ls_cb,
	  "List the contents of the current directory",
	  "ls [OPTIONS] <directory>" },
	{ "cd", NULL, cd_cb,
	  "Change the current directory",
	  "cd <directory>" },
	{ NULL }
};

static EggLineEntry channel_entries[] =
{
	{ "show", NULL, channel_show_cb, "Show perfkit data channels" },
	{ "add", NULL, channel_add_cb, "Add a new perfkit data channel" },
	{ "remove", NULL, channel_remove_cb,
	  "Remove an existing perfkit data channel",
	  "channel remove <channel-id>" },
	{ NULL }
};

gint
main (gint   argc,
      gchar *argv[])
{
	GOptionContext *context;
	GError         *error   = NULL;
	EggLine        *line;
	gint            bus;

	/* TODO: This doesn't actually seem to work */
	rl_catch_signals = 1;

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

	/* connect to the DBUS */
	bus = use_system ? DBUS_BUS_SYSTEM : DBUS_BUS_SESSION;
	if (!(dbus_conn = dbus_g_bus_get (bus, &error))) {
		g_printerr ("%s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	/* retrieve the proxy to the Channels service */
	if (!(channels = dbus_g_proxy_new_for_name (
					dbus_conn,
					"com.dronelabs.Perfkit",
					"/com/dronelabs/Perfkit/Channels",
					"com.dronelabs.Perfkit.Channels")))
	{
		g_printerr ("Error connecting to perfkit channels service!\n");
		return EXIT_SUCCESS;
	}

	/* run the readline loop */
	line = egg_line_new ();
	egg_line_set_prompt (line, "perfkit> ");
	egg_line_set_entries (line, entries);
	g_signal_connect (line, "missing", G_CALLBACK (missing_cmd), NULL);
	egg_line_run (line);

	return EXIT_SUCCESS;
}

static void
report_error (GError       *error,
              const gchar  *func,
              gint          line)
{
	time_t    t;
	struct tm tt;

	g_return_if_fail (error != NULL);

	t = time (NULL);
	tt = *localtime (&t);

	g_printerr ("Error Report.....: \n" /* TODO: Add custom message */
	            "  Date and Time..: %s" /* XXX: asctime includes \n */
	            "  Domain.........: %d\n"
	            "  Code...........: %d\n"
	            "  Message........: %s\n"
	            "  File...........: %s\n"
	            "  Method.........: %s\n"
	            "  Line...........: %d\n",
	            asctime (&tt),
	            error->domain,
	            error->code,
	            error->message,
	            __FILE__,
	            func,
	            line);
}

static void
report_usage (const gchar *message)
{
	g_printerr ("usage:  %s\n\n", message);
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

static EggLineStatus
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
		REPORT_ERROR (error);
		g_error_free (error);
		error = NULL;
	}
	else {
		g_print ("%s", output);
		g_free (output);
	}

	g_strfreev (cmd);

	return EGG_LINE_OK;
}

static EggLineStatus
cd_cb (EggLine  *line,
       gchar   **args)
{
	g_return_val_if_fail (args != NULL, EGG_LINE_FAILURE);

	if (!args [0] || !strlen (args [0]))
		g_chdir (g_get_home_dir ());
	else
		g_chdir (args [0]);

	return EGG_LINE_OK;
}

static void
pk_channel_print (gchar    *path)
{
	DBusGProxy  *channel;
	gchar       *target = NULL;

	g_print ("%s\n", path);

	if (!(channel = dbus_g_proxy_new_for_name (dbus_conn,
	                                           "com.dronelabs.Perfkit",
	                                           path,
	                                           "com.dronelabs.Perfkit.Channel")))
	{
		/* XXX: Essentially this is just a race condition. */
		g_print ("  Deleted.\n");
		return;
	}

	com_dronelabs_Perfkit_Channel_get_target (channel, &target, NULL);

	g_print ("  Target: %s\n", target);

	g_object_unref (channel);
	g_free (target);
}

static EggLineStatus
channel_show_cb (EggLine  *line,
                 gchar   **args)
{
	GPtrArray *paths = NULL;
	GError    *error = NULL;

	g_return_val_if_fail (args != NULL, EGG_LINE_FAILURE);

	if (!com_dronelabs_Perfkit_Channels_find_all (channels, &paths, &error)) {
		REPORT_ERROR (error);
		g_error_free (error);
		return EGG_LINE_OK;
	}

	g_ptr_array_foreach (paths, (GFunc)pk_channel_print, NULL);
	g_ptr_array_unref (paths);

	return EGG_LINE_OK;
}

static EggLineStatus
channel_add_cb (EggLine  *line,
                gchar   **args)
{
	GError *error = NULL;
	gchar  *path  = NULL;

	g_return_val_if_fail (args != NULL, EGG_LINE_FAILURE);

	if (!com_dronelabs_Perfkit_Channels_add (channels, &path, &error)) {
		REPORT_ERROR (error);
		g_error_free (error);
		return EGG_LINE_OK;
	}

	pk_channel_print (path);
	g_free (path);

	return EGG_LINE_OK;
}

static EggLineStatus
channel_remove_cb (EggLine  *line,
                   gchar   **args)
{
	GError *error = NULL;
	gchar  *path  = NULL;
	gint    id    = 0;

	g_return_val_if_fail (args != NULL, EGG_LINE_FAILURE);

	if (g_strv_length (args) < 1)
		return EGG_LINE_BAD_ARGS;
	else if (!strlen (*args))
		return EGG_LINE_BAD_ARGS;

	errno = 0;
	id = atoi (*args);
	if (errno != 0)
		return EGG_LINE_BAD_ARGS;

	path = g_strdup_printf ("/com/dronelabs/Perfkit/Channels/%d", id);
	g_print ("Removing channel %d.\n", id);

	if (!com_dronelabs_Perfkit_Channels_remove (channels, path, &error)) {
		REPORT_ERROR (error);
		g_error_free (error);
		goto cleanup;
	}

cleanup:
	g_free (path);
	g_print ("\n");

	return EGG_LINE_OK;
}
