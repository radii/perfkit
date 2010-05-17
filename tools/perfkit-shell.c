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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <perfkit/perfkit.h>

#include "egg-fmt.h"
#include "egg-line.h"
#include "egg-time.h"

/*
 * Local error codes.
 */
#define PK_SHELL_ERROR (pk_shell_error_quark())
static GQuark pk_shell_error_quark (void) G_GNUC_CONST;
enum
{
	PK_SHELL_ERROR_URI,
};

/*
 * Global state.
 */
static EggFmtFunc    formatter  = NULL;
static PkConnection *connection = NULL;
static gboolean      opt_csv    = FALSE;
static GMainLoop    *mainloop   = NULL;
/*
 * Command line arguments.
 */
static GOptionEntry entries[] = {
	{ "csv", 'c', 0, G_OPTION_ARG_NONE, &opt_csv,
	  N_("Output comma-separated values"), NULL },
	{ NULL }
};


/*
 *----------------------------------------------------------------------------
 *
 * pk_util_parse_int --
 *
 *     Parses an integer from @str with proper condition checking.
 *
 * Returns:
 *     TRUE if successful; otherwise FALSE.
 *
 * Side Effects:
 *     None.
 *
 *----------------------------------------------------------------------------
 */

static gboolean
pk_util_parse_int (const gchar *str,    // IN
                   gint        *v_int)  // OUT
{
	gchar *ptr;
	gint   val;

	g_return_val_if_fail (str != NULL, FALSE);
	g_return_val_if_fail (v_int != NULL, FALSE);

	*v_int = 0;
	errno = 0;

	val = strtol (str, &ptr, 0);

	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0))
		return FALSE;

	if (str == ptr)
		return FALSE;

	*v_int = val;

	return TRUE;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_connect --
 *
 *     Connects the shell to the server found at @uri.
 *
 * Returns:
 *     TRUE if successful; otherwise FALSE.
 *
 * Side effects:
 *     The current connection is closed.
 *
 *----------------------------------------------------------------------------
 */

static gboolean
pk_shell_connect (const gchar  *uri,   // IN
                  GError      **error) // OUT
{
	g_return_val_if_fail (uri != NULL, FALSE);

	/*
	 * Close previous connection if needed.
	 */
	if (connection) {
		if (!pk_connection_disconnect(connection, error)) {
			g_print("Error disconnecting from agent.\n");
		}
		g_object_unref(connection);
		connection = NULL;
	}

	/*
	 * Create connection instance.
	 */
	connection = pk_connection_new_from_uri (uri);
	if (!connection) {
		g_set_error(error, PK_SHELL_ERROR, PK_SHELL_ERROR_URI,
		            "Invalid URI");
		return FALSE;
	}

	/*
	 * Connect to the agent.
	 */
	if (!pk_connection_connect(connection, error)) {
		g_object_unref(connection);
		connection = NULL;
		return FALSE;
	}

	return TRUE;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_missing_cb --
 *
 *    Callback when entered command does not exist.
 *
 * Returns:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static void
pk_shell_missing_cb (EggLine     *line,      // IN
                     const gchar *command,   // IN
                     gpointer     user_data) // IN
{
	g_printerr (_("Command not found: %s\n"), command);
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_util_channels_iter --
 *
 *    EggFmtIterNext implementation for iterating through a list of channels.
 *
 * Returns:
 *    TRUE if there are more items; otherwise FALSE.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static gboolean
pk_util_channels_iter (EggFmtIter *iter,
                       gpointer    user_data)
{
	gint i, channel_id, v_int;
	gchar *v_str, **v_strv;
	gint state;
	struct {
		gint n_channels;
		gint *channels;
		gint offset;
	} *s = user_data;

	if (!user_data || s->offset >= s->n_channels) {
		return FALSE;
	}

	channel_id = s->channels[s->offset++];

	/* load the data into the columns */
	for (i = 0; i < iter->n_columns; i++) {
		if (0 == g_ascii_strcasecmp(iter->column_names[i], "id")) {
			g_value_set_int(&iter->column_values[i], channel_id);
		}
		else if (0 == g_ascii_strcasecmp(iter->column_names[i], "pid")) {
			v_int = 0;
			pk_connection_channel_get_pid(connection, channel_id, &v_int, NULL);
			g_value_set_int(&iter->column_values [i], v_int);
		}
		else if (0 == g_ascii_strcasecmp (iter->column_names [i], "target")) {
			v_str = NULL;
			pk_connection_channel_get_target(connection, channel_id, &v_str, NULL);
			g_value_take_string (&iter->column_values [i], v_str);
		}
		else if (0 == g_ascii_strcasecmp (iter->column_names [i], "directory")) {
			v_str = NULL;
			pk_connection_channel_get_working_dir(connection, channel_id, &v_str, NULL);
			g_value_take_string (&iter->column_values [i], v_str);
		}
		else if (0 == g_ascii_strcasecmp (iter->column_names [i], "arguments")) {
			v_strv = NULL;
			pk_connection_channel_get_args(connection, channel_id, &v_strv, NULL);
			g_value_take_boxed (&iter->column_values [i], v_strv);
		}
		else if (0 == g_ascii_strcasecmp (iter->column_names [i], "state")) {
			state = 0;
			pk_connection_channel_get_state(connection, channel_id, &state, NULL);
			switch (state) {
			case PK_CHANNEL_READY:
				g_value_set_string (&iter->column_values [i], "READY");
				break;
			case PK_CHANNEL_RUNNING:
				g_value_set_string (&iter->column_values [i], "STARTED");
				break;
			case PK_CHANNEL_MUTED:
				g_value_set_string (&iter->column_values [i], "MUTED");
				break;
			case PK_CHANNEL_STOPPED:
				g_value_set_string (&iter->column_values [i], "STOPPED");
				break;
			case PK_CHANNEL_FAILED:
				g_value_set_string (&iter->column_values [i], "FAILED");
				break;
			default:
				g_value_set_string (&iter->column_values [i], "?");
				break;
			}
		}
		else {
			g_warn_if_reached ();
		}
	}

	return TRUE;
}

#if 0
static gboolean
pk_util_source_infos_iter (EggFmtIter *iter,
                           gpointer    user_data)
{
	PkSourceInfo *info;
	GList *list;
	gint i;

	if (!user_data)
		return FALSE;

	/* initialize our iter */
	if (!iter->user_data2) {
		iter->user_data2 = user_data;
		iter->user_data = user_data;
	}

	/* we are finished if the list is empty */
	if (!iter->user_data)
		return FALSE;

	list = iter->user_data;
	info = list->data;

	if (!info)
		return FALSE;

	/* load the data into the columns */
	for (i = 0; i < iter->n_columns; i++) {
		if (0 == g_ascii_strcasecmp (iter->column_names [i], "uid")) {
			g_value_set_string (&iter->column_values [i],
			                    pk_source_info_get_uid (info));
		}
		else if (0 == g_ascii_strcasecmp (iter->column_names [i], "name")) {
			g_value_set_string (&iter->column_values [i],
			                    pk_source_info_get_name (info));
		}
		else {
			g_warn_if_reached ();
		}
	}

	/* move to the next position */
	iter->user_data = g_list_next (iter->user_data);

	return TRUE;
}
#endif


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_quit --
 *
 *    Quit command callback.
 *
 * Returns:
 *    Command exit status.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_quit (EggLine  *line,
                   gint      argc,
                   gchar   **argv,
                   GError  **error)
{
	egg_line_quit (line);
	return EGG_LINE_STATUS_OK;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_ls --
 *
 *    ls command callback.
 *
 * Returns:
 *    Command exit status.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_ls (EggLine  *line,
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
	                   error)) {
		result = EGG_LINE_STATUS_FAILURE;
	}
	else {
		g_print ("%s", output);
		g_free (output);
	}

	g_strfreev (cmd);

	return result;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_cd --
 *
 *    Changes working directory of process.
 *
 * Returns:
 *    Command exit status.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_cd (EggLine  *line,
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


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_version --
 *
 *    Displays the version of the library protocol.
 *
 * Returns:
 *    Command exit status.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_version (EggLine  *line,
                      gint      argc,
                      gchar   **argv,
                      GError  **error)
{
	GError *lerror = NULL;
	gchar *ver = NULL;

	g_print("Protocol...:  %s\n", PK_VERSION_S);

	if (!pk_connection_manager_get_version(connection, &ver, &lerror)) {
		g_printerr("Error fetching server version: %s\n", lerror->message);
		g_error_free(lerror);
		lerror = NULL;
	}
	else {
		g_print("Agent......:  %s\n", ver);
		g_free(ver);
	}

	return EGG_LINE_STATUS_OK;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_help --
 *
 *    Displays help text for a given command.
 *
 * Returns:
 *    Command exit status.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_help (EggLine  *line,
                   gint      argc,
                   gchar   **argv,
                   GError  **error)
{
	EggLineCommand *command;
	gchar          *path;

	if (argc < 1)
		return EGG_LINE_STATUS_BAD_ARGS;

	path = g_strjoinv (" ", argv);
	command = egg_line_resolve (line, path, NULL, NULL);
	g_free (path);

	if (!command)
		return EGG_LINE_STATUS_BAD_ARGS;

	if (command->help)
		g_print ("%s\n", _(command->help));

	return EGG_LINE_STATUS_OK;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_load --
 *
 *    Loads a series of commands from an input file.
 *
 * Returns:
 *    Command exit status.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_load (EggLine  *line,
                   gint      argc,
                   gchar   **argv,
                   GError  **error)
{
	gint i;

	if (argc < 1) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	for (i = 0; i < argc; i++) {
		egg_line_execute_file (line, argv [i]);
	}

	return EGG_LINE_STATUS_OK;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_sleep --
 *
 *    Sleeps for a given period of time.
 *
 * Returns:
 *    Command exit status.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_sleep (EggLine  *line,
                    gint      argc,
                    gchar   **argv,
                    GError  **error)
{
	gint i = 0;

	if (argc != 1)
		return EGG_LINE_STATUS_BAD_ARGS;

	if (!pk_util_parse_int (argv [0], &i))
		return EGG_LINE_STATUS_BAD_ARGS;

	g_usleep (G_USEC_PER_SEC * i);

	return EGG_LINE_STATUS_OK;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_channel_list --
 *
 *    Lists the avaialble channels on the agent.
 *
 * Returns:
 *    Command exit status.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_channel_list (EggLine  *line,
                           gint      argc,
                           gchar   **argv,
                           GError  **error)
{
	EggFmtIter  iter;
	struct {
		gsize n_channels;
		gint *channels;
		gint offset;
	} s = {0,0};

	/*
	 * Get list of channels from agent.
	 */
	if (!pk_connection_manager_get_channels(connection,
	                                        &s.channels,
	                                        &s.n_channels,
	                                        error)) {
	    return EGG_LINE_STATUS_FAILURE;
	}

	if (!s.n_channels) {
	   g_print(_("No channels where found.\n"));
	   goto finish;
	}

	/*
	 * Write channel information to console.
	 */
	egg_fmt_iter_init(&iter,
	                  pk_util_channels_iter,
	                  "ID", G_TYPE_INT,
	                  "PID", G_TYPE_INT,
	                  "State", G_TYPE_STRING,
	                  "Target", G_TYPE_STRING,
	                  "Arguments", G_TYPE_STRV,
	                  "Directory", G_TYPE_STRING,
	                  NULL);
	formatter(&iter, &s, NULL);

finish:
	if (s.channels) {
		g_free(s.channels);
	}

	return EGG_LINE_STATUS_OK;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_channel_add --
 *
 *    Add a new channel on the agent.
 *
 * Returns:
 *    Command exit status.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_channel_add (EggLine  *line,
                          gint      argc,
                          gchar   **argv,
                          GError  **error)
{
	EggLineStatus ret = EGG_LINE_STATUS_FAILURE;
	PkSpawnInfo info;
	GPtrArray *env;
	gint channel = 0;
	gint i;

	memset(&info, 0, sizeof(info));
	env = g_ptr_array_new();

	/*
	 * Assign StartInfo properties.
	 */
	for (i = 0; (i + 1) < argc; i++) {
		if (g_strcmp0(argv[i], "target") == 0) {
			info.target = argv[++i];
		}
		else if (g_strcmp0(argv[i], "dir") == 0) {
			info.working_dir = argv[++i];
		}
		else if (g_strcmp0(argv[i], "--") == 0) {
			info.args = &argv[++i];
			i += g_strv_length(&argv[i]);
		}
		else if (g_strcmp0(argv[i], "pid") == 0) {
			pk_util_parse_int(argv[++i], (gint *)&info.pid);
		}
		else if (g_strcmp0(argv[i], "env") == 0) {
			g_ptr_array_add(env, argv[++i]);
		}
		else {
			g_printerr("Invalid property: %s\n", argv[i]);
			return EGG_LINE_STATUS_OK;
		}
	}

	g_ptr_array_add(env, NULL);
	info.env = (gchar **)env->pdata;

	/*
	 * Make sure we have required fields.
	 */
	if (!info.target && !info.pid) {
		g_printerr("Missing PID or Target.\n");
		goto finish;
	}

	/*
	 * Create channel.
	 */
	if (!pk_connection_manager_add_channel(connection, &channel, error)) {
		g_printerr("Could not create channel.\n");
		goto finish;
	}

	if (info.target) {
		if (!pk_connection_channel_set_target(connection, channel, info.target, error)) {
			g_printerr("Could not set target.\n");
			goto finish;
		}
	}

	if (info.args) {
		if (!pk_connection_channel_set_args(connection, channel, info.args, error)) {
			g_printerr("Could not set args.\n");
			goto finish;
		}
	}

	g_print("Added channel %d.\n", channel);
	ret = EGG_LINE_STATUS_OK;

  finish:
	g_ptr_array_unref(env);
	return ret;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_channel_start --
 *
 *    Start the given channels.
 *
 * Returns:
 *    Command exit status.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_channel_start (EggLine  *line,
                            gint      argc,
                            gchar   **argv,
                            GError  **error)
{
	GError *lerror = NULL;
	gint i, v_int;

	if (argc < 1) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	for (i = 0; i < argc; i++) {
		if (pk_util_parse_int(argv[i], &v_int)) {
			if (!pk_connection_channel_start(connection, v_int, &lerror)) {
				g_printerr("%s\n", lerror->message);
				g_error_free(lerror);
				lerror = NULL;
			} else {
				g_print("Channel %d started.\n", v_int);
			}
		}
	}

	return EGG_LINE_STATUS_OK;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_channel_stop --
 *
 *    Stop the given channels.
 *
 * Returns:
 *    Command exit status.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_channel_stop (EggLine  *line,
                           gint      argc,
                           gchar   **argv,
                           GError  **error)
{
	GError *lerror = NULL;
	gint i, v_int = 0;

	if (argc < 1) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	for (i = 0; i < argc; i++) {
		if (pk_util_parse_int(argv [i], &v_int)) {
			if (!pk_connection_channel_stop(connection, v_int, &lerror)) {
				g_printerr("%s.\n", lerror->message);
				g_error_free(lerror);
				lerror = NULL;
			} else {
				g_print("Channel %d stopped.\n", v_int);
			}
		}
	}

	return EGG_LINE_STATUS_OK;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_channel_mute --
 *
 *    Pause the given channels.
 *
 * Returns:
 *    Command exit status.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_channel_mute (EggLine  *line,
                            gint      argc,
                            gchar   **argv,
                            GError  **error)
{
	GError *lerror = NULL;
	gint i, v_int = 0;

	if (argc < 1) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	for (i = 0; i < argc; i++) {
		if (pk_util_parse_int (argv [i], &v_int)) {
			if (!pk_connection_channel_mute(connection, v_int, &lerror)) {
				g_printerr("Error pausing channel %d: %s.\n",
				           v_int, lerror->message);
				g_error_free(lerror);
				lerror = NULL;
			} else {
				g_print("Paused channel %d.\n", v_int);
			}
		}
	}

	return EGG_LINE_STATUS_OK;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_channel_unmute --
 *
 *    Unpause the given channels.
 *
 * Returns:
 *    Command exit status.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_channel_unmute (EggLine  *line,
                              gint      argc,
                              gchar   **argv,
                              GError  **error)
{
	GError *lerror = NULL;
	gint i, v_int = 0;

	if (argc < 1) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	for (i = 0; i < argc; i++) {
		if (pk_util_parse_int (argv [i], &v_int)) {
			if (!pk_connection_channel_unmute(connection, v_int, &lerror)) {
				g_printerr("Error pausing channel %d: %s.\n",
				           v_int, lerror->message);
				g_error_free(lerror);
				lerror = NULL;
			} else {
				g_print("Channel %d paused.\n", v_int);
			}
		}
	}

	return EGG_LINE_STATUS_OK;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_channel_get --
 *
 *    Get a properties of a channel.
 *
 * Returns:
 *    Command exit status.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_channel_get (EggLine  *line,
                          gint      argc,
                          gchar   **argv,
                          GError  **error)
{
	gint state = 0;
	GError *lerror = NULL;
	gint i, channel_id;
	gchar *v_str, **v_strv;
	GPid pid = 0;

	if (argc < 2) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	/*
	 * Get the channel identifier.
	 */
	if (!pk_util_parse_int(argv[0], &channel_id)) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	/*
	 * Iterate the rest of the property names and print their value.
	 */
	for (i = 1; i < argc; i++) {
		if (!g_str_equal (argv[i], "target") &&
	        !g_str_equal (argv[i], "args") &&
	        !g_str_equal (argv[i], "env") &&
	        !g_str_equal (argv[i], "pid") &&
	        !g_str_equal (argv[i], "dir") &&
	        !g_str_equal (argv[i], "state")) {
	        g_printerr("Invalid property: %s.\n", argv[i]);
	        continue;
		}

		if (g_str_equal(argv[i], "target")) {
			if (!pk_connection_channel_get_target(connection,
			                                      channel_id,
			                                      &v_str,
			                                      &lerror)) {
			    g_printerr("Error fetching target: %s.\n", lerror->message);
			    g_error_free(lerror);
			    lerror = NULL;
			} else {
				g_print("target: %s\n", v_str);
				g_free(v_str);
			}
		} else if (g_str_equal(argv[i], "dir")) {
			if (!pk_connection_channel_get_working_dir(connection,
			                                           channel_id,
			                                           &v_str,
			                                           &lerror)) {
			    g_printerr("Error fetching dir: %s.\n", lerror->message);
			    g_error_free(lerror);
			    lerror = NULL;
			} else {
				g_print("dir: %s\n", v_str);
				g_free(v_str);
			}
		}
		else if (g_str_equal(argv[i], "pid")) {
			if (!pk_connection_channel_get_pid(connection,
			                                   channel_id,
			                                   &pid,
			                                   &lerror)) {
			    g_printerr("Error fetching pid: %s.\n", lerror->message);
			    g_error_free(lerror);
			    lerror = NULL;
			} else {
				g_print("pid: %d\n", (gint)pid);
			}
		}
		else if (g_str_equal(argv[i], "state")) {
			if (!pk_connection_channel_get_state(connection,
			                                     channel_id,
			                                     &state,
			                                     &lerror)) {
			    g_printerr("Error fetching state: %s.\n", lerror->message);
			    g_error_free(lerror);
			    lerror = NULL;
			    continue;
			}

			g_print("state: ");

			switch (state) {
			case PK_CHANNEL_READY:
				g_print("READY\n");
				break;
			case PK_CHANNEL_STOPPED:
				g_print("STOPPED\n");
				break;
			case PK_CHANNEL_MUTED:
				g_print("MUTED\n");
				break;
			case PK_CHANNEL_RUNNING:
				g_print("STARTED\n");
				break;
			case PK_CHANNEL_FAILED:
				g_print("FAILED\n");
				break;
			default:
				g_print("UNKNOWN\n");
				break;
			}
		}
		else if (g_str_equal(argv[i], "args")) {
			if (!pk_connection_channel_get_args(connection,
			                                    channel_id,
			                                    &v_strv,
			                                    &lerror)) {
			    g_printerr("Error fetching args: %s.\n", lerror->message);
			    g_error_free(lerror);
			    lerror = NULL;
			    continue;
			}

			v_str = g_strjoinv(" ", v_strv);
			g_print("args: %s\n", v_str);
			g_strfreev(v_strv);
			g_free(v_str);
		}
		else if (g_str_equal(argv[i], "env")) {
			if (!pk_connection_channel_get_env(connection,
			                                   channel_id,
			                                   &v_strv,
			                                   &lerror)) {
			    g_printerr("Error fetching env: %s.\n", lerror->message);
			    g_error_free(lerror);
			    lerror = NULL;
			    continue;
			}

			v_str = g_strjoinv (" ", v_strv);
			g_print ("env: %s\n", v_str);
			g_strfreev (v_strv);
			g_free (v_str);
		}
	}

	return EGG_LINE_STATUS_OK;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_channel_add_source --
 *
 *    Add a source to a channel.
 *
 * Returns:
 *    Command exit code.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_channel_add_source(EggLine  *line,
                                gint      argc,
                                gchar   **argv,
                                GError  **error)
{
	GError *lerror = NULL;
	gint channel_id = 0, source_id = 0;

	if (argc < 2) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	if (!pk_util_parse_int(argv[0], &channel_id)) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	if (!pk_connection_plugin_create_source(connection, argv[1], &source_id, &lerror)) {
	    if (lerror) {
			g_printerr("ERROR: %s.\n", lerror->message);
			g_clear_error(&lerror);
		} else {
			g_printerr("Unknown error adding source.\n");
		}
	}
	else {
		g_print("Source %d created.\n", source_id);
	}

	return EGG_LINE_STATUS_OK;
}


static EggLineStatus
pk_shell_cmd_channel_remove (EggLine  *line,
                             gint      argc,
                             gchar   **argv,
                             GError  **error)
{
	gint channel_id = 0;
	gboolean removed = FALSE;
	GError *lerror = NULL;

	if (argc != 1) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	if (!pk_util_parse_int(argv[0], &channel_id)) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	if (channel_id < 0) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	if (!pk_connection_manager_remove_channel(connection,
	                                          channel_id,
	                                          &removed,
	                                          &lerror)) {
	    g_printerr("ERROR: %s\n", lerror->message);
	    g_error_free(lerror);
	} else {
		g_print("Removed channel %d.\n", channel_id);
	}

	return EGG_LINE_STATUS_OK;
}


static EggLineStatus
pk_shell_cmd_channel_remove_source (EggLine  *line,
                                    gint      argc,
                                    gchar   **argv,
                                    GError  **error)
{
	gint channel_id, source_id;
	//GError *lerror = NULL;

	if (argc != 2) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	if (!pk_util_parse_int(argv[0], &channel_id) ||
	    !pk_util_parse_int(argv[1], &source_id)) {
	    return EGG_LINE_STATUS_BAD_ARGS;
	}

	if (channel_id < 0 || source_id < 0) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	g_warning("Missing rpcs for removing sources.");

	/*
	if (!pk_connection_channel_remove_source(connection,
	                                         channel_id,
	                                         source_id,
	                                         &lerror)) {
	    g_printerr("ERROR: %s\n", lerror->message);
	    g_error_free(lerror);
	} else {
		g_print("Removed source %d from channel %d.\n", source_id, channel_id);
	}
	*/

	return EGG_LINE_STATUS_OK;
}

static PkManifest *current_manifest = NULL;

static void
print_manifest (PkManifest *manifest)
{
	const gchar *name;
	gint i, rows, len = 0;

	rows = pk_manifest_get_n_rows(manifest);

	for (i = 1; i <= rows; i++) {
		name = pk_manifest_get_row_name(manifest, i);
		g_print("%s,", name);
		len += 1 + (name ? strlen(name) : 6);
	}

	g_print("\n");

	for (i = 0; i < len; i++) {
		g_print("-");
	}

	g_print("\n");
}

#if 0
static void
monitor_on_manifest (PkManifest *manifest,
                     gpointer    user_data)
{
	print_manifest(manifest);
	if (current_manifest) {
		pk_manifest_unref(current_manifest);
	}
	current_manifest = pk_manifest_ref(manifest);
}

static void
monitor_on_sample (PkSample *sample,
                   gpointer  user_data)
{
	GValue value = {0};
	gint i, n_rows;

	g_return_if_fail(sample);
	g_return_if_fail(current_manifest);

	n_rows = pk_manifest_get_n_rows(current_manifest);

	for (i = 1; i <= n_rows; i++) {
		if (pk_sample_get_value(sample, i, &value)) {
			switch (G_VALUE_TYPE(&value)) {
			case G_TYPE_INT:
				g_print("%d,", g_value_get_int(&value));
				break;
			case G_TYPE_UINT:
				g_print("%u,", g_value_get_uint(&value));
				break;
			case G_TYPE_INT64:
				g_print("%" G_GINT64_FORMAT ",", g_value_get_uint64(&value));
				break;
			case G_TYPE_UINT64:
				g_print("%" G_GUINT64_FORMAT ",", g_value_get_uint64(&value));
				break;
			case G_TYPE_DOUBLE:
				g_print("%f,", g_value_get_double(&value));
				break;
			case G_TYPE_FLOAT:
				g_print("%f,", g_value_get_float(&value));
				break;
			default:
				g_debug("TYPE IS %s", g_type_name(G_VALUE_TYPE(&value)));
				g_print("NaN,");
			}
			g_value_unset(&value);
		}
	}

	g_print("\n");
}
#endif

static gboolean
show_manifest (gpointer data)
{
	print_manifest(current_manifest);
	return TRUE;
}

static void
monitor_sigint_handler(int sig)
{
	g_print("\nStopping monitor\n");
	g_main_loop_quit(mainloop);
}


static EggLineStatus
pk_shell_cmd_channel_monitor (EggLine  *line,
                              gint      argc,
                              gchar   **argv,
                              GError  **error)
{
	gint channel_id, sub_id = 0;
	struct sigaction sa;

	/*
	 * Catch Ctrl-C so we can exit the shell.  Auto reset the signal handler
	 * after the call.
	 */
	sa.sa_handler = monitor_sigint_handler;
	sa.sa_flags = SA_RESETHAND;

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		g_printerr("Failed to set signal handler.\n");
	}

	if (argc < 1) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	if (!pk_util_parse_int(argv[0], &channel_id)) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	/* Create main mainloop */
	mainloop = g_main_loop_new(NULL, FALSE);

	/* Create subscription to channel */
	if (!pk_connection_manager_add_subscription(connection,
	                                            //channel_id,
	                                            0,
	                                            0,
	                                            0, //NULL,
	                                            &sub_id,
	                                            error)) {
		return EGG_LINE_STATUS_FAILURE;
	}

	/* Set subscription callbacks */
	g_warn_if_reached();
	/*
	pk_connection_subscription_set_handlers(connection,
	                                        sub_id,
	                                        monitor_on_manifest,
	                                        monitor_on_sample,
	                                        NULL);
	*/

	if (!pk_connection_subscription_unmute(connection, sub_id, error)) {
		return EGG_LINE_STATUS_FAILURE;
	}

	/* show header every 5 seconds. */
	g_timeout_add_seconds(20, show_manifest, NULL);

	/* Start main loop */
	g_main_loop_run(mainloop);

	if (!pk_connection_subscription_mute(connection, sub_id, FALSE, error)) {
		g_printerr("Failed to disable subscription: %d\n", sub_id);
		return EGG_LINE_STATUS_FAILURE;
	}

	return EGG_LINE_STATUS_OK;
}


static EggLineCommand channel_commands[] = {
	{ "list", NULL, pk_shell_cmd_channel_list,
	  N_("List perfkit channels"),
	  "channel list" },
	{ "add", NULL, pk_shell_cmd_channel_add,
	  N_("Add a new perfkit channel"),
	  "channel add" },
	{ "start", NULL, pk_shell_cmd_channel_start,
	  N_("Start the perfkit channel"),
	  "channel start" },
	{ "stop", NULL, pk_shell_cmd_channel_stop,
	  N_("Stop the perfkit channel"),
	  "channel stop" },
	{ "mute", NULL, pk_shell_cmd_channel_mute,
	  N_("Pause the perfkit channel"),
	  "channel mute" },
	{ "unmute", NULL, pk_shell_cmd_channel_unmute,
	  N_("Unpause the perfkit channel"),
	  "channel unmute" },
	{ "get", NULL, pk_shell_cmd_channel_get,
	  N_("Retrieve channel properties"),
	  "channel get [CHANNEL] [pid|target|args|env|state]" },
	{ "add-source", NULL, pk_shell_cmd_channel_add_source,
	  N_("Add a source to the channel"),
	  "channel add-source [CHANNEL] [SOURCE-TYPE]" },
	{ "remove", NULL, pk_shell_cmd_channel_remove,
	  N_("Remove a channel"),
	  "channel remove [CHANNEL]" },
	{ "remove-source", NULL, pk_shell_cmd_channel_remove_source,
		N_("Remove a source from channel"),
		"channel remove-source [CHANNEL] [SOURCE]" },
	{ "monitor", NULL, pk_shell_cmd_channel_monitor,
	    N_("Monitor samples from a channel."),
	    "channel monitor [CHANNEL]" },
	{ NULL }
};

static EggLineCommand*
pk_shell_iter_channel (EggLine   *line,
                       gint      *argc,
                       gchar   ***argv)
{
	return channel_commands;
}


/*
 *----------------------------------------------------------------------------
 *
 * pk_shell_cmd_ping --
 *
 *    Pings the perfkit agent and displays the network latency.
 *
 * Returns:
 *    Command exit code.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------------
 */

static EggLineStatus
pk_shell_cmd_ping (EggLine  *line,
                   gint      argc,
                   gchar   **argv,
                   GError  **error)
{
	GTimeVal tv;
	GTimeVal ltv;
	GError *lerror = NULL;
	gchar *v_str;
	GTimeSpan rel;

	g_print("\n  Pinging \"%s\" ...\n\n",
	        pk_connection_get_uri(connection));

	g_get_current_time(&ltv);

	if (!pk_connection_manager_ping(connection, &tv, &lerror)) {
		g_printerr("    ERROR: %s.\n", lerror->message);
		g_error_free(lerror);
		lerror = NULL;
	}
	else {
		v_str = g_time_val_to_iso8601(&tv);
		g_print("    %s (%lu.%lu)\n\n", v_str, tv.tv_sec, tv.tv_usec);
		g_free(v_str);
		g_time_val_diff(&tv, &ltv, &rel);
		g_print("    Latency: %lu.%06lu\n\n",
		        (gulong)(rel / G_TIME_SPAN_SECOND),
		        (gulong)(rel % G_TIME_SPAN_SECOND));
	}

	return EGG_LINE_STATUS_OK;
}

static EggLineCommand commands[] = {
	{ "quit", NULL, pk_shell_cmd_quit,
	  N_("Quit perfkit-shell"), "quit" },
	{ "ls", NULL, pk_shell_cmd_ls,
	  N_("List directory contents"),
	  "ls [FILE]..." },
	{ "cd", NULL, pk_shell_cmd_cd,
	  N_("Change working directory"),
	  "cd [DIRECTORY]..." },
	{ "ping", NULL, pk_shell_cmd_ping,
	  N_("Ping the perfkit-agent"),
	  "ping" },
	{ "version", NULL, pk_shell_cmd_version,
	  N_("Display version information"),
	  "version" },
	{ "help", NULL, pk_shell_cmd_help,
	  N_("Display help information about perfkit"),
	  "help [COMMAND]..." },
	{ "load", NULL, pk_shell_cmd_load,
	  N_("Load commands from a file"),
	  "load [FILENAME]..." },
	{ "sleep", NULL, pk_shell_cmd_sleep,
	  N_("Delay for a specified amount of time"),
	  "sleep [SECONDS]" },
	{ "channel", pk_shell_iter_channel, NULL,
	  N_("Manage perfkit channels\n"
	     "\n"
	     "Commands:\n"
	     "\n"
	     "  add           - Add a new channel\n"
	     "  add-source    - Add a new source to the channel\n"
	     "  get           - Get channel properties\n"
	     "  list          - List available channels\n"
	     "  pause         - Pause a channel\n"
	     "  unpause       - Unpause a paused channel\n"
	     "  remove        - Remove a channel\n"
	     "  set           - Set channel properties\n"
	     "  start         - Start the channel recording\n"
	     "  stop          - Stop the channel recording\n"),
	  "channel [COMMAND]" },
	{ NULL }
};

gint
main (gint   argc,
      gchar *argv[])
{
	GOptionContext *context;
	GError         *error = NULL;
	EggLine        *line;
	gint            i;

	/* parse command line arguments */
	context = g_option_context_new ("- interactive perfkit shell");
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_printerr ("%s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	/* initialize libraries */
	g_thread_init(NULL);
	g_type_init ();

	/* connect to the agent */
	if (!pk_shell_connect ("dbus://", &error)) {
		g_printerr ("%s\n", error->message);
		return EXIT_FAILURE;
	}

	/* determine our console formatter */
	formatter = opt_csv ? egg_fmt_csv : egg_fmt_table;

	/* setup readline abstraction */
	line = egg_line_new ();
	egg_line_set_prompt (line, "perfkit> ");
	egg_line_set_commands (line, commands);
	g_signal_connect (line, "missing", G_CALLBACK (pk_shell_missing_cb), NULL);

	if (argc > 1) {
		/* run any filename arguments */
		for (i = 1; i < argc; i++)
			egg_line_execute_file (line, argv [i]);
	}
	else {
		/* run the shell main loop */
		egg_line_run (line);
	}

	return EXIT_SUCCESS;
}

static GQuark
pk_shell_error_quark (void)
{
	return g_quark_from_static_string("pk-shell-error-quark");
}
