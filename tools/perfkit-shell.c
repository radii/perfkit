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
#include <stdlib.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <perfkit/perfkit.h>
#include <perfkit/perfkit-lowlevel.h>

#include "egg-fmt.h"
#include "egg-line.h"

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
		pk_connection_disconnect(connection);
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
	 * Connect to the daemon.
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
	PkChannelState state;
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
			case PK_CHANNEL_PAUSED:
				g_value_set_string (&iter->column_values [i], "PAUSED");
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
	/* TODO: Add Admin Service to the Daemon
	 *       and retrieve the remote version.
	 */
	g_print ("Protocol:  %s\n", PK_VERSION_S);
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
 *    Lists the avaialble channels on the daemon.
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
		gint n_channels;
		gint *channels;
		gint offset;
	} s = {0,0};

	/*
	 * Get list of channels from daemon.
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
 *    Add a new channel on the daemon.
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
	PkSpawnInfo info;
	gint channel = 0;
	gint i;

	memset(&info, 0, sizeof(info));

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
		else {
			g_printerr("Invalid property: %s\n", argv[i]);
			return EGG_LINE_STATUS_OK;
		}
	}

	/*
	 * Make sure we have required fields.
	 */
	if (!info.target && !info.pid) {
		g_printerr("Missing PID or Target.\n");
		return EGG_LINE_STATUS_OK;
	}

	/*
	 * Create channel
	 */
	if (!pk_connection_manager_create_channel(connection,
	                                          &info,
	                                          &channel,
	                                          error)) {
	    return EGG_LINE_STATUS_FAILURE;
	}

	g_print("Added channel %d.\n", channel);

	return EGG_LINE_STATUS_OK;
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
			if (!pk_connection_channel_stop(connection, v_int, TRUE, &lerror)) {
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
 * pk_shell_cmd_channel_pause --
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
pk_shell_cmd_channel_pause (EggLine  *line,
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
			if (!pk_connection_channel_pause(connection, v_int, &lerror)) {
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
 * pk_shell_cmd_channel_unpause --
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
pk_shell_cmd_channel_unpause (EggLine  *line,
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
			if (!pk_connection_channel_unpause(connection, v_int, &lerror)) {
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
static EggLineStatus
pk_shell_cmd_channel_set (EggLine  *line,
                          gint      argc,
                          gchar   **argv,
                          GError  **error)
{
	PkChannel     *channel;
	gint           channel_id = 0,
	               v_int      = 0;
	EggLineStatus  result     = EGG_LINE_STATUS_OK;

	if (argc < 3)
		return EGG_LINE_STATUS_BAD_ARGS;
	else if (!pk_util_parse_int (argv [0], &channel_id))
		return EGG_LINE_STATUS_BAD_ARGS;
	else if (!g_str_equal ("target", argv [1]) &&
	         !g_str_equal ("args", argv [1]) &&
	         !g_str_equal ("env", argv [1]) &&
	         !g_str_equal ("dir", argv [1]) &&
	         !g_str_equal ("pid", argv [1]))
		return EGG_LINE_STATUS_BAD_ARGS;

	channel = pk_manager_get_channel(manager, channel_id);

	if (g_str_equal ("target", argv [1])) {
		pk_channel_set_target (channel, argv [2]);
	}
	else if (g_str_equal ("args", argv [1])) {
		pk_channel_set_args (channel, &argv [2]);
	}
	else if (g_str_equal ("env", argv [1])) {
		pk_channel_set_env (channel, &argv [2]);
	}
	else if (g_str_equal ("dir", argv [1])) {
		pk_channel_set_dir (channel, argv [2]);
	}
	else if (g_str_equal ("pid", argv [1])) {
		if (!pk_util_parse_int (argv [2], &v_int))
			result = EGG_LINE_STATUS_BAD_ARGS;
		else
			pk_channel_set_pid (channel, (GPid)v_int);
	}
	else {
		g_warn_if_reached ();
	}

	g_object_unref (channel);

	return result;
}
*/

static EggLineStatus
pk_shell_cmd_channel_get (EggLine  *line,
                          gint      argc,
                          gchar   **argv,
                          GError  **error)
{
	PkChannelState state = 0;
	GError *lerror = NULL;
	gint i, channel_id;
	gchar *v_str;
	GPid pid = 0;

	if (argc < 2) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	if (!pk_util_parse_int (argv[0], &channel_id)) {
		return EGG_LINE_STATUS_BAD_ARGS;
	}

	for (i = 1; i < argc; i++) {
		if (!g_str_equal (argv[i], "target") &&
	        !g_str_equal (argv[i], "args") &&
	        !g_str_equal (argv[i], "env") &&
	        !g_str_equal (argv[i], "pid") &&
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
				g_print("working-dir: %s\n", v_str);
				g_free(v_str);
			}
		}
		else if (g_str_equal (argv [1], "pid")) {
			pk_connection_channel_get_pid(connection, channel_id, &pid, NULL);
			g_print("pid: %d\n", (gint)pid);
		}
		else if (g_str_equal (argv [1], "state")) {
			pk_connection_channel_get_state(connection,
			                                channel_id,
			                                &state,
			                                NULL);
			switch (state) {
			case PK_CHANNEL_READY:
				g_print ("READY\n");
				break;
			case PK_CHANNEL_STOPPED:
				g_print ("STOPPED\n");
				break;
			case PK_CHANNEL_PAUSED:
				g_print ("PAUSED\n");
				break;
			case PK_CHANNEL_RUNNING:
				g_print ("STARTED\n");
				break;
			case PK_CHANNEL_FAILED:
				g_print ("FAILED\n");
				break;
			default:
				g_warn_if_reached ();
				break;
			}
		}
	}

	/*
	else if (g_str_equal (argv [1], "args")) {
		tmpv = pk_channel_get_args (channel);
		tmp = g_strjoinv (" ", tmpv);
		g_print ("%s\n", tmp);
		g_strfreev (tmpv);
		g_free (tmp);
	}
	else if (g_str_equal (argv [1], "env")) {
		tmpv = pk_channel_get_env (channel);
		tmp = g_strjoinv (" ", tmpv);
		g_print ("%s\n", tmp);
		g_strfreev (tmpv);
		g_free (tmp);
	}

	*/

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
	{ "pause", NULL, pk_shell_cmd_channel_pause,
	  N_("Pause the perfkit channel"),
	  "channel pause" },
	{ "unpause", NULL, pk_shell_cmd_channel_unpause,
	  N_("Unpause the perfkit channel"),
	  "channel unpause" },
	{ "get", NULL, pk_shell_cmd_channel_get,
	  N_("Retrieve channel properties"),
	  "channel get [CHANNEL] [pid|target|args|env|state]" },
	/*
	{ "set", NULL, pk_shell_cmd_channel_set,
	  N_("Set channel properties"),
	  "channel set [CHANNEL] [pid|target|args|env] [VALUE]" },
	*/
	{ NULL }
};

static EggLineCommand*
pk_shell_iter_channel (EggLine   *line,
                       gint      *argc,
                       gchar   ***argv)
{
	return channel_commands;
}

static EggLineStatus
pk_shell_cmd_source_types (EggLine  *line,
                           gint      argc,
                           gchar   **argv,
                           GError  **error)
{
	/*
	EggFmtIter  iter;
	GList      *list;

	list = pk_sources_get_source_types (sources);
	if (!g_list_length (list)) {
	   g_printerr ("No source types where found.\n"
	               "Try installing some plugins.\n");
	   return EGG_LINE_STATUS_OK;
   }

	egg_fmt_iter_init (&iter,
	                   pk_util_source_infos_iter,
	                   "UID", G_TYPE_STRING,
	                   "Name", G_TYPE_STRING,
	                   NULL);
	formatter (&iter, list, NULL);

	g_list_foreach (list, (GFunc)g_object_unref, NULL);
	g_list_free (list);
	*/

	return EGG_LINE_STATUS_OK;
}

static EggLineStatus
pk_shell_cmd_source_add (EggLine  *line,
                         gint      argc,
                         gchar   **argv,
                         GError  **error)
{
	/*
	PkSource *source = NULL;
	GList *list, *iter;
	gchar *name;

	if (argc != 1)
		return EGG_LINE_STATUS_BAD_ARGS;

	list = pk_sources_get_source_types (sources);
	for (iter = list; iter; iter = iter->next) {
		name = pk_source_info_get_uid (iter->data);
		if (g_ascii_strcasecmp (argv [0], name) == 0) {
			source = pk_source_info_create (iter->data);
			g_free (name);
			break;
		}
		g_free (name);
	}

	g_list_foreach (list, (GFunc)g_object_unref, NULL);
	g_list_free (list);

	if (source) {
		g_print (_("Created source %d of type \"%s\".\n"),
		         pk_source_get_id (source),
		         argv [0]);
		g_object_unref (source);
	}
	else {
		g_printerr (_("Could not create data source typed \"%s\".\n"),
		            argv [0]);
	}
	*/

	return EGG_LINE_STATUS_OK;
}

/*
static EggLineStatus
pk_shell_cmd_source_attach (EggLine  *line,
                            gint      argc,
                            gchar   **argv,
                            GError  **error)
{
	PkChannel  *channel    = NULL;
	PkSource   *source     = NULL;
	gint        source_id  = 0,
	            channel_id = 0;

	if (argc != 2)
		return EGG_LINE_STATUS_BAD_ARGS;
	else if (!pk_util_parse_int (argv [0], &source_id))
		return EGG_LINE_STATUS_BAD_ARGS;
	else if (!pk_util_parse_int (argv [1], &channel_id))
		return EGG_LINE_STATUS_BAD_ARGS;

	channel = pk_manager_get_channel(manager, channel_id);
	if (!channel)
		goto error;

	source = pk_manager_get_source(manager, source_id);
	if (!source)
		goto error;

	pk_source_set_channel (source, channel);

error:
	if (channel)
		g_object_unref (channel);

	if (source)
		g_object_unref (source);

	return EGG_LINE_STATUS_OK;
}
*/

static EggLineCommand source_commands[] = {
	{ "types", NULL, pk_shell_cmd_source_types,
	  N_("List available source types"),
	  "source types" },
	{ "add", NULL, pk_shell_cmd_source_add,
	  N_("Add a data source"),
	  "source add [TYPE]" },
	/*
	{ "attach", NULL, pk_shell_cmd_source_attach,
	  N_("Attach a source to a data channel"),
	  "source attach [SOURCE] [CHANNEL]" },
	*/
	{ NULL }
};

static EggLineCommand*
pk_shell_iter_source (EggLine   *line,
                      gint      *argc,
                      gchar   ***argv)
{
	return source_commands;
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
	{ "version", NULL, pk_shell_cmd_version,
	  N_("Display version information"),
	  "version" },
	{ "help", NULL, pk_shell_cmd_help,
	  N_("Display help information about perfkit"),
	  "help [COMMAND]..." },
	{ "load", NULL, pk_shell_cmd_load,
	  N_("Load commands from a file"),
	  "source [FILENAME]..." },
	{ "sleep", NULL, pk_shell_cmd_sleep,
	  N_("Delay for a specified amount of time"),
	  "sleep [SECONDS]" },
	{ "channel", pk_shell_iter_channel, NULL,
	  N_("Manage perfkit channels\n"
	     "\n"
	     "Commands:\n"
	     "\n"
	     "  add     - Add a new channel\n"
	     "  get     - Get channel properties\n"
	     "  list    - List available channels\n"
	     "  pause   - Pause a channel\n"
	     "  unpause - Unpause a paused channel\n"
	     "  remove  - Remove a channel\n"
	     "  set     - Set channel properties\n"
	     "  start   - Start the channel recording\n"
	     "  stop    - Stop the channel recording\n"),
	  "channel [COMMAND]" },
	{ "source", pk_shell_iter_source, NULL,
	  N_("Manage perfkit data sources"),
	  "source [COMMAND]" },
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
	g_type_init ();

	/* connect to the daemon */
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
