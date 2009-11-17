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
#include <perfkit-daemon/pkd-version.h>

#define REPORT_ERROR(e)     report_error (e, G_STRFUNC, __LINE__);
#define PK_CHANNEL_FORMAT   "/com/dronelabs/Perfkit/Channels/%d"
#define PK_CHANNEL_FORMAT_S "/com/dronelabs/Perfkit/Channels/%s"
#define PK_CHANNEL_PREFIX   "/com/dronelabs/Perfkit/Channels/"

static void          missing_cmd       (EggLine *line, const gchar *text, gpointer user_data);
static EggLineEntry* channel_iter      (EggLine *line, gint *argc, gchar ***argv);
static EggLineStatus ls_cb             (EggLine *line, gint  argc, gchar  **argv, GError **error);
static EggLineStatus cd_cb             (EggLine *line, gint  argc, gchar  **argv, GError **error);
static EggLineStatus channel_show_cb   (EggLine *line, gint  argc, gchar  **argv, GError **error);
static EggLineStatus channel_list_cb   (EggLine *line, gint  argc, gchar  **argv, GError **error);
static EggLineStatus channel_add_cb    (EggLine *line, gint  argc, gchar  **argv, GError **error);
static EggLineStatus channel_remove_cb (EggLine *line, gint  argc, gchar  **argv, GError **error);
static EggLineStatus channel_set_cb    (EggLine *line, gint  argc, gchar  **argv, GError **error);
static EggLineStatus channel_get_cb    (EggLine *line, gint  argc, gchar  **argv, GError **error);
static EggLineStatus version_cb        (EggLine *line, gint  argc, gchar  **argv, GError **error);
static EggLineStatus help_cb           (EggLine *line, gint  argc, gchar  **argv, GError **error);

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
	{ "channel",
	  channel_iter,
	  NULL,
	  "Manage perfkit data channels\n"
	  "\n"
	  "Commands:\n"
	  "  add    - Add a new data channel\n"
	  "  remove - Remove an existing data channel\n"
	  "  show   - Display information on a data channel\n"
	  "  set    - Set a data channel setting\n"
	  "  get    - Get a data channel setting\n"
	  "\n",
	  "channel [add|remove|show|set|get]" },
	{ "source",
	  NULL,
	  NULL,
	  "Manage perfkit data sources\n"
	  "\n"
	  "Commands:\n"
	  "\n",
	  "source [add|remove|show]" },
	{ "help",
	  NULL,
	  help_cb,
	  "Get help on a command\n"
	  "\n"
	  "Usage:\n"
	  "  help <command..>\n"
	  "\n",
	  "help <command>" },
	{ "ls", NULL, ls_cb,
	  "List the contents of the current directory\n"
	  "\n"
	  "Usage:\n"
	  "  ls [OPTIONS] directory\n"
	  "\n"
	  "This command uses the host `ls' command.\n"
	  "\n",
	  "ls [OPTIONS] <directory>" },
	{ "cd",
	  NULL,
	  cd_cb,
	  "Change the current directory\n"
	  "\n"
	  "Usage:\n"
	  "  cd <directory>\n"
	  "\n",
	  "cd <directory>" },
	{ "version",
	  NULL,
	  version_cb,
	  "Show the perfkit-shell version",
	  "version" },
	{ NULL }
};

static EggLineEntry channel_entries[] =
{
	{ "list", NULL, channel_list_cb, "List perfkit data channels", NULL },
	{ "show", NULL, channel_show_cb, "Show perfkit data channels", NULL },
	{ "add", NULL, channel_add_cb, "Add a new perfkit data channel", NULL },
	{ "remove", NULL, channel_remove_cb,
	  "Remove an existing perfkit data channel",
	  "channel remove <channel-id>" },
	{ "set", NULL, channel_set_cb,
	  "Set a data channel setting",
	  "channel set <channel-id> [target|args|env|dir|pid] [setting]" },
	{ "get", NULL, channel_get_cb,
	  "Get a data channel setting",
	  "channel get <channel-id> [target|args|env|dir|pid]" },
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

	g_printerr ("ERROR REPORT\n"
	            "  Date and Time..: %s" /* XXX: asctime includes \n */
	            "  Domain.........: %s\n"
	            "  Code...........: %d\n"
	            "  Message........: %s\n"
	            "  File...........: %s\n"
	            "  Method.........: %s\n"
	            "  Line...........: %d\n",
	            asctime (&tt),
	            g_quark_to_string (error->domain),
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
channel_iter (EggLine   *line,
              gint      *argc,
              gchar   ***argv)
{
	return channel_entries;
}

static EggLineStatus
ls_cb (EggLine  *line,
       gint      argc,
       gchar   **argv,
       GError  **error)
{
	EggLineStatus   result = EGG_LINE_STATUS_OK;
	gchar         **cmd;
	gchar          *output = NULL;
	gint            i,
	                j;

	cmd = g_malloc0 ((argc + 2) * sizeof (gchar*));

	cmd [0] = g_strdup ("ls");
	for (i = 0, j = 1; i < argc; i++)
		if (argv [i] && strlen (argv [i]))
			cmd [j++] = g_strdup (argv [i]);

	if (!g_spawn_sync (g_get_current_dir (),
	                   cmd,
	                   NULL,
	                   G_SPAWN_SEARCH_PATH,
	                   NULL,
	                   NULL,
	                   &output,
	                   NULL,
	                   NULL,
	                   error))
	{
		REPORT_ERROR (*error);
		result = EGG_LINE_STATUS_FAILURE;
	}
	else {
		g_print ("%s", output);
		g_free (output);
	}

	g_strfreev (cmd);

	return result;
}

static EggLineStatus
cd_cb (EggLine  *line,
       gint      argc,
       gchar   **argv,
       GError  **error)
{
	if (argc == 0 || !argv [0] || !strlen (argv [0]))
		g_chdir (g_get_home_dir ());
	else
		g_chdir (argv [0]);

	return EGG_LINE_STATUS_OK;
}

static void
pk_channel_print (const gchar *lpath)
{
	DBusGProxy  *channel;
	gchar       *target = NULL,
	            *dir    = NULL,
	           **env    = NULL,
	           **args   = NULL,
	            *path,
	            *tmp;
	gint         pid    = 0,
	             i;

	if (!lpath || strlen (lpath) == 0)
		return;

	path = g_strdup (lpath);
	g_strstrip (path);

	if (!(channel = dbus_g_proxy_new_for_name (dbus_conn,
	                                           "com.dronelabs.Perfkit",
	                                           path,
	                                           "com.dronelabs.Perfkit.Channel")))
		goto cleanup;

	tmp = g_strrstr (path, "/");
	if (tmp)
		tmp++;

	if (!com_dronelabs_Perfkit_Channel_get_target (channel, &target, NULL) ||
	    !com_dronelabs_Perfkit_Channel_get_dir    (channel, &dir,    NULL) ||
	    !com_dronelabs_Perfkit_Channel_get_env    (channel, &env,    NULL) ||
	    !com_dronelabs_Perfkit_Channel_get_args   (channel, &args,   NULL) ||
	    !com_dronelabs_Perfkit_Channel_get_pid    (channel, &pid,    NULL))
	{
		g_printerr ("Channel \"%s\" not found.\n", tmp);
		goto cleanup;
	}

	g_print ("Channel #%s\n", tmp);
	g_print ("    Target: %s\n", (target && strlen (target)) ? target : "\"\"");
	g_print ("    Args..:");
	if (!args || g_strv_length (args) == 0)
		g_print (" \"\"");
	else
		for (i = 0; args && args [i]; i++)
			g_print (" %s", args [i]);
	g_print ("\n");
	g_print ("    Pid...: %d\n", pid);
	g_print ("    Dir...: %s\n", (dir && strlen (dir)) ? dir : "\"\"");
	g_print ("    Env...:");
	if (!env || g_strv_length (env) == 0)
		g_print (" \"\"");
	else
		for (i = 0; env && env [i]; i++)
			g_print (" %s", env [i]);
	g_print ("\n");

cleanup:
	g_object_unref (channel);
	g_free (target);
	g_free (path);
	g_free (dir);
	g_strfreev (args);
	g_strfreev (env);
}

static EggLineStatus
channel_show_cb (EggLine  *line,
                 gint      argc,
                 gchar   **argv,
                 GError  **error)
{
	gchar *path;
	gint   i;

	if (argc == 0 || strlen (argv [0]) == 0)
		return channel_list_cb (line, argc, argv, error);

	for (i = 0; i < argc; i++) {
		path = g_strdup_printf (PK_CHANNEL_FORMAT_S, argv [i]);
		pk_channel_print (path);
		g_free (path);
	}

	return EGG_LINE_STATUS_OK;
}

static EggLineStatus
channel_list_cb (EggLine  *line,
                 gint      argc,
                 gchar   **argv,
                 GError  **error)
{
	GPtrArray *paths = NULL;

	if (!com_dronelabs_Perfkit_Channels_find_all (channels, &paths, error)) {
		REPORT_ERROR (*error);
		return EGG_LINE_STATUS_FAILURE;
	}

	g_ptr_array_foreach (paths, (GFunc)pk_channel_print, NULL);
	g_ptr_array_unref (paths);

	return EGG_LINE_STATUS_OK;
}

static EggLineStatus
channel_add_cb (EggLine  *line,
                gint      argc,
                gchar   **argv,
                GError  **error)
{
	gchar *path = NULL;

	/* TODO: Support specifying properties */

	if (!com_dronelabs_Perfkit_Channels_add (channels, &path, error)) {
		REPORT_ERROR (*error);
		return EGG_LINE_STATUS_FAILURE;
	}

	pk_channel_print (path);
	g_free (path);

	return EGG_LINE_STATUS_OK;
}

static EggLineStatus
channel_remove_cb (EggLine  *line,
                   gint      argc,
                   gchar   **argv,
                   GError  **error)
{
	EggLineStatus  result = EGG_LINE_STATUS_OK;
	gchar         *path   = NULL;
	gint           id     = 0;

	if (argc < 1 || !strlen (argv [0]))
		return EGG_LINE_STATUS_BAD_ARGS;

	errno = 0;
	id = strtol (argv [0], NULL, 10);
	if (errno != 0)
		return EGG_LINE_STATUS_BAD_ARGS;

	path = g_strdup_printf (PK_CHANNEL_FORMAT, id);

	if (!com_dronelabs_Perfkit_Channels_remove (channels, path, error)) {
		REPORT_ERROR (*error);
		result = EGG_LINE_STATUS_FAILURE;
		goto cleanup;
	}

	g_print ("Channel %d removed.\n", id);

cleanup:
	g_free (path);

	return result;
}

static EggLineStatus
channel_get_cb (EggLine  *line,
                gint      argc,
                gchar   **argv,
                GError  **error)
{
	EggLineStatus   result = EGG_LINE_STATUS_OK;
	gint            id     = 0,
	                i,
		            v_int  = 0;
	gchar          *v_str  = NULL,
	              **v_strv = NULL,
	               *path;
	DBusGProxy     *channel;

	if (argc < 2)
		return EGG_LINE_STATUS_BAD_ARGS;

	errno = 0;
	id = strtol (argv [0], NULL, 10);
	if (errno != 0)
		return EGG_LINE_STATUS_BAD_ARGS;

	if (!argv [1] || strlen (argv [1]) == 0)
		return EGG_LINE_STATUS_BAD_ARGS;

	if (!g_str_equal ("target", argv [1]) &&
	    !g_str_equal ("args", argv [1]) &&
	    !g_str_equal ("env", argv [1]) &&
	    !g_str_equal ("dir", argv [1]) &&
	    !g_str_equal ("pid", argv [1]))
		return EGG_LINE_STATUS_BAD_ARGS;

	path = g_strdup_printf (PK_CHANNEL_FORMAT, id);
	channel = dbus_g_proxy_new_for_name (dbus_conn,
	                                     "com.dronelabs.Perfkit",
	                                     path,
	                                     "com.dronelabs.Perfkit.Channel");

#define PRINT_SHELL_STR(s) G_STMT_START { \
	gchar *tmp = g_strescape ((s), NULL); \
	g_print ("%s\n", tmp); \
	g_free (tmp); \
} G_STMT_END

	if (g_str_equal ("target", argv [1])) {
	    if (com_dronelabs_Perfkit_Channel_get_target (channel, &v_str, error)) {
	    	PRINT_SHELL_STR (v_str);
	    	g_free (v_str);
		}
		else {
			REPORT_ERROR (*error);
			result = EGG_LINE_STATUS_FAILURE;
		}
	}
	else if (g_str_equal ("pid", argv [1])) {
		if (com_dronelabs_Perfkit_Channel_get_pid (channel, &v_int, error)) {
			g_print ("%d\n", v_int);
		}
		else {
			REPORT_ERROR (*error);
			result = EGG_LINE_STATUS_FAILURE;
		}
	}
	else if (g_str_equal ("args", argv [1])) {
		if (com_dronelabs_Perfkit_Channel_get_args (channel, &v_strv, error)) {
			for (i = 0; v_strv [i]; i++)
				g_print ("%s ", v_strv [i]);
			g_print ("\n");
			g_strfreev (v_strv);
		}
		else {
			REPORT_ERROR (*error);
			result = EGG_LINE_STATUS_FAILURE;
		}
	}
	else if (g_str_equal ("env", argv [1])) {
		if (com_dronelabs_Perfkit_Channel_get_env (channel, &v_strv, error)) {
			for (i = 0; v_strv [i]; i++)
				g_print ("%s ", v_strv [i]);
			g_print ("\n");
			g_strfreev (v_strv);
		}
		else {
			REPORT_ERROR (*error);
			result = EGG_LINE_STATUS_FAILURE;
		}
	}
	else if (g_str_equal ("dir", argv [1])) {
		if (com_dronelabs_Perfkit_Channel_get_dir (channel, &v_str, error)) {
			PRINT_SHELL_STR (v_str);
	    	g_free (v_str);
		}
		else {
			REPORT_ERROR (*error);
			result = EGG_LINE_STATUS_FAILURE;
		}
	}

	g_object_unref (channel);
	g_free (path);

	return result;
}

static EggLineStatus
channel_set_cb (EggLine  *line,
                gint      argc,
                gchar   **argv,
                GError  **error)
{
	EggLineStatus  result = EGG_LINE_STATUS_OK;
	gint           id     = 0;
	DBusGProxy    *channel;
	gchar         *path;
	GPid           pid    = (GPid)0;

	if (argc < 3)
		return EGG_LINE_STATUS_BAD_ARGS;

	errno = 0;
	id = strtol (argv [0], NULL, 10);
	if (errno != 0)
		return EGG_LINE_STATUS_BAD_ARGS;

	if (!g_str_equal ("target", argv [1]) &&
	    !g_str_equal ("args", argv [1]) &&
	    !g_str_equal ("env", argv [1]) &&
	    !g_str_equal ("dir", argv [1]) &&
	    !g_str_equal ("pid", argv [1]))
		return EGG_LINE_STATUS_BAD_ARGS;

	path = g_strdup_printf (PK_CHANNEL_FORMAT, id);
	channel = dbus_g_proxy_new_for_name (dbus_conn,
	                                     "com.dronelabs.Perfkit",
	                                     path,
	                                     "com.dronelabs.Perfkit.Channel");
	g_free (path);

	if (g_str_equal ("target", argv [1])) {
		if (!com_dronelabs_Perfkit_Channel_set_target (channel, argv [2], error)) {
			REPORT_ERROR (*error);
			result = EGG_LINE_STATUS_FAILURE;
		}
	}
	else if (g_str_equal ("args", argv [1])) {
		if (!com_dronelabs_Perfkit_Channel_set_args (channel, (const gchar**)&argv [2], error)) {
			REPORT_ERROR (*error);
			result = EGG_LINE_STATUS_FAILURE;
		}
	}
	else if (g_str_equal ("env", argv [1])) {
		if (!com_dronelabs_Perfkit_Channel_set_env (channel, (const gchar**)&argv [2], error)) {
			REPORT_ERROR (*error);
			result = EGG_LINE_STATUS_FAILURE;
		}
	}
	else if (g_str_equal ("dir", argv [1])) {
		if (!com_dronelabs_Perfkit_Channel_set_dir (channel, argv [2], error)) {
			REPORT_ERROR (*error);
			result = EGG_LINE_STATUS_FAILURE;
		}
	}
	else if (g_str_equal ("pid", argv [1])) {
		errno = 0;
		pid = strtol (argv [2], NULL, 10);
		if (errno != 0) {
			result = EGG_LINE_STATUS_FAILURE;
			goto cleanup;
		}
		if (!com_dronelabs_Perfkit_Channel_set_pid (channel, pid, error)) {
			REPORT_ERROR (*error);
			result = EGG_LINE_STATUS_FAILURE;
		}
	}
	else g_warn_if_reached ();

cleanup:
	g_object_unref (channel);

	return result;
}

static EggLineStatus
version_cb (EggLine  *line,
            gint      argc,
            gchar   **argv,
            GError  **error)
{
	g_print ("%s\n", PKD_VERSION_S);
}

static EggLineStatus
help_cb (EggLine  *line,
         gint      argc,
         gchar   **argv,
         GError  **error)
{
	EggLineEntry  *entry;
	gchar         *path;

	if (argc < 1)
		return EGG_LINE_STATUS_BAD_ARGS;

	path = g_strjoinv (" ", argv);
	entry = egg_line_resolve (line, path, NULL, NULL);
	g_free (path);

	if (!entry)
		return EGG_LINE_STATUS_BAD_ARGS;

	if (entry->help)
		g_print ("%s\n", entry->help);

	return EGG_LINE_STATUS_OK;
}
