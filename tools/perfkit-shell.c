/* perfkit-shell.c
 *
 * Copyright 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "Shell"

#include <errno.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if __linux__
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/syscall.h>
#else
#error "Your platform is not yet supported"
#endif

#include <perfkit/perfkit.h>
#include "pk-util.h"
#include "egg-fmt.h"
#include "egg-line.h"

#define G_LOG_LEVEL_TRACE (1 << G_LOG_LEVEL_USER_SHIFT)

#define PK_SHELL_ERROR (pk_shell_error_quark())

typedef enum
{
	PK_SHELL_ERROR_ARGS,
	PK_SHELL_ERROR_VALUE,
} PkShellError;

typedef struct
{
	GMutex   *mutex;
	GCond    *cond;
	gboolean  result;
	GError   *error;
	gpointer  params[16];
} AsyncTask;

static gchar           hostname[64]  = { '\0' };
static EggFmtFunc      formatter     = NULL;
static GMainLoop      *main_loop     = NULL;
static PkConnection   *conn          = NULL;
static GLogLevelFlags  log_threshold = G_LOG_LEVEL_INFO;

static void
async_task_init (AsyncTask *task) /* IN */
{
	ENTRY;
	memset(task, 0, sizeof(*task));
	task->mutex = g_mutex_new();
	task->cond = g_cond_new();
	g_mutex_lock(task->mutex);
	EXIT;
}

static gboolean
async_task_wait (AsyncTask *task) /* IN */
{
	ENTRY;
	g_cond_wait(task->cond, task->mutex);
	RETURN(task->result);
}

static void
async_task_signal (AsyncTask *task) /* IN */
{
	ENTRY;
	g_mutex_lock(task->mutex);
	g_cond_signal(task->cond);
	g_mutex_unlock(task->mutex);
	EXIT;
}

static inline const gchar *
pk_shell_state_to_str (gint state)
{
	switch (state) {
	CASE_RETURN_STR(PK_CHANNEL_READY);
	CASE_RETURN_STR(PK_CHANNEL_RUNNING);
	CASE_RETURN_STR(PK_CHANNEL_MUTED);
	CASE_RETURN_STR(PK_CHANNEL_STOPPED);
	CASE_RETURN_STR(PK_CHANNEL_FAILED);
	default:
		return "UNKNOWN";
	}
}

static inline GQuark
pk_shell_error_quark (void)
{
	return g_quark_from_static_string("pk-shell-error-quark");
}

static inline gboolean
pk_shell_parse_int (const gchar *str,   /* IN */
                    gint        *v_int) /* OUT */
{
	gchar *ptr;
	gint val;

	*v_int = 0;
	errno = 0;
	val = strtol (str, &ptr, 0);
	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0)) {
		return FALSE;
	} else if (str == ptr) {
		return FALSE;
	}
	*v_int = val;
	return TRUE;
}

static inline gboolean
pk_shell_parse_boolean (const gchar *str,    /* IN */
                        gboolean    *v_bool) /* OUT */
{
	gchar *utf8;
	gboolean ret = TRUE;

	g_return_val_if_fail(str != NULL, FALSE);
	g_return_val_if_fail(v_bool != NULL, FALSE);

	ENTRY;
	utf8 = g_utf8_strdown(str, -1);
	if (g_strcmp0("yes", utf8) == 0) {
		*v_bool = TRUE;
	} else if (g_strcmp0("no", utf8) == 0) {
		*v_bool = FALSE;
	} else if (g_strcmp0("true", utf8) == 0) {
		*v_bool = TRUE;
	} else if (g_strcmp0("false", utf8) == 0) {
		*v_bool = FALSE;
	} else if (g_strcmp0("1", utf8) == 0) {
		*v_bool = TRUE;
	} else if (g_strcmp0("0", utf8) == 0) {
		*v_bool = FALSE;
	} else {
		*v_bool = FALSE;
		ret = FALSE;
	}
	g_free(utf8);
	RETURN(ret);
}


/**
 * pk_shell_channel_add_source_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_add_source_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_add_source_cb (GObject       *object,    /* IN */
                                GAsyncResult  *result,    /* IN */
                                gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_add_source_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_add_source:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_add_source (EggLine  *line,   /* IN */
                             gint      argc,   /* IN */
                             gchar    *argv[], /* IN */
                             GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gint source = 0;
	gint i = 0;

	ENTRY;
	if (argc != 2) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &source)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_channel_add_source_async(conn,
	                             channel,
	                             source,
	                             NULL,
	                             pk_shell_channel_add_source_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_get_args_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_get_args_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_get_args_cb (GObject       *object,    /* IN */
                              GAsyncResult  *result,    /* IN */
                              gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_get_args_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* args */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_get_args:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_get_args (EggLine  *line,   /* IN */
                           gint      argc,   /* IN */
                           gchar    *argv[], /* IN */
                           GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gchar** args;
	gchar *args_str;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &args;
	pk_connection_channel_get_args_async(conn,
	                             channel,
	                             NULL,
	                             pk_shell_channel_get_args_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	if (args && g_strv_length(args)) {
		args_str = g_strjoinv("', '", args);
		g_print("%16s: ['%s']\n", "args", args_str);
		g_free(args_str);
	} else {
		g_print("%16s: []\n", "args");
	}
	egg_line_set_variable(line, "1", "");
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_get_env_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_get_env_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_get_env_cb (GObject       *object,    /* IN */
                             GAsyncResult  *result,    /* IN */
                             gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_get_env_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* env */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_get_env:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_get_env (EggLine  *line,   /* IN */
                          gint      argc,   /* IN */
                          gchar    *argv[], /* IN */
                          GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gchar** env;
	gchar *env_str;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &env;
	pk_connection_channel_get_env_async(conn,
	                             channel,
	                             NULL,
	                             pk_shell_channel_get_env_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	if (env && g_strv_length(env)) {
		env_str = g_strjoinv("', '", env);
		g_print("%16s: ['%s']\n", "env", env_str);
		g_free(env_str);
	} else {
		g_print("%16s: []\n", "env");
	}
	egg_line_set_variable(line, "1", "");
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_get_exit_status_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_get_exit_status_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_get_exit_status_cb (GObject       *object,    /* IN */
                                     GAsyncResult  *result,    /* IN */
                                     gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_get_exit_status_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* exit_status */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_get_exit_status:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_get_exit_status (EggLine  *line,   /* IN */
                                  gint      argc,   /* IN */
                                  gchar    *argv[], /* IN */
                                  GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gint exit_status;
	gint i = 0;
	gchar *tmp;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &exit_status;
	pk_connection_channel_get_exit_status_async(conn,
	                             channel,
	                             NULL,
	                             pk_shell_channel_get_exit_status_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %d\n", "exit_status", (gint)exit_status);
	tmp = g_strdup_printf("%d", (gint)exit_status);
	egg_line_set_variable(line, "1", tmp);
	g_free(tmp);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_get_kill_pid_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_get_kill_pid_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_get_kill_pid_cb (GObject       *object,    /* IN */
                                  GAsyncResult  *result,    /* IN */
                                  gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_get_kill_pid_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* kill_pid */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_get_kill_pid:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_get_kill_pid (EggLine  *line,   /* IN */
                               gint      argc,   /* IN */
                               gchar    *argv[], /* IN */
                               GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gboolean kill_pid;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &kill_pid;
	pk_connection_channel_get_kill_pid_async(conn,
	                             channel,
	                             NULL,
	                             pk_shell_channel_get_kill_pid_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %s\n", "kill_pid", kill_pid ? "TRUE" : "FALSE");
	egg_line_set_variable(line, "1", kill_pid ? "1" : "0");
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_get_pid_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_get_pid_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_get_pid_cb (GObject       *object,    /* IN */
                             GAsyncResult  *result,    /* IN */
                             gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_get_pid_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* pid */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_get_pid:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_get_pid (EggLine  *line,   /* IN */
                          gint      argc,   /* IN */
                          gchar    *argv[], /* IN */
                          GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gint pid;
	gint i = 0;
	gchar *tmp;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &pid;
	pk_connection_channel_get_pid_async(conn,
	                             channel,
	                             NULL,
	                             pk_shell_channel_get_pid_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %d\n", "pid", (gint)pid);
	tmp = g_strdup_printf("%d", (gint)pid);
	egg_line_set_variable(line, "1", tmp);
	g_free(tmp);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_get_pid_set_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_get_pid_set_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_get_pid_set_cb (GObject       *object,    /* IN */
                                 GAsyncResult  *result,    /* IN */
                                 gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_get_pid_set_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* pid_set */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_get_pid_set:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_get_pid_set (EggLine  *line,   /* IN */
                              gint      argc,   /* IN */
                              gchar    *argv[], /* IN */
                              GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gboolean pid_set;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &pid_set;
	pk_connection_channel_get_pid_set_async(conn,
	                             channel,
	                             NULL,
	                             pk_shell_channel_get_pid_set_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %s\n", "pid_set", pid_set ? "TRUE" : "FALSE");
	egg_line_set_variable(line, "1", pid_set ? "1" : "0");
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_get_sources_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_get_sources_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_get_sources_cb (GObject       *object,    /* IN */
                                 GAsyncResult  *result,    /* IN */
                                 gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_get_sources_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* sources */
			task->params[1], /* sources_len */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_get_sources:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_get_sources (EggLine  *line,   /* IN */
                              gint      argc,   /* IN */
                              gchar    *argv[], /* IN */
                              GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gint* sources;
	gsize sources_len;
	gint i = 0;
	gchar *tmp;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &sources;
	task.params[1] = &sources_len;
	pk_connection_channel_get_sources_async(conn,
	                             channel,
	                             NULL,
	                             pk_shell_channel_get_sources_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	if (sources && sources_len) {
		g_print("%16s: [", "sources");
		for (i = 0; i < sources_len; i++) {
			g_print("%d%s", sources[i], ((i + 1) == sources_len) ? "" : ", ");
		}
		g_print("]\n");
	}
	egg_line_set_variable(line, "1", "");
	g_print("%16s: %d\n", "sources_len", (gint)sources_len);
	tmp = g_strdup_printf("%d", (gint)sources_len);
	egg_line_set_variable(line, "2", tmp);
	g_free(tmp);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_get_state_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_get_state_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_get_state_cb (GObject       *object,    /* IN */
                               GAsyncResult  *result,    /* IN */
                               gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_get_state_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* state */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_get_state:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_get_state (EggLine  *line,   /* IN */
                            gint      argc,   /* IN */
                            gchar    *argv[], /* IN */
                            GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gint state;
	gint i = 0;
	gchar *tmp;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &state;
	pk_connection_channel_get_state_async(conn,
	                             channel,
	                             NULL,
	                             pk_shell_channel_get_state_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %d (%s)\n", "state", (gint)state,
	        pk_shell_state_to_str(state));
	tmp = g_strdup_printf("%d", (gint)state);
	egg_line_set_variable(line, "1", tmp);
	g_free(tmp);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_get_target_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_get_target_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_get_target_cb (GObject       *object,    /* IN */
                                GAsyncResult  *result,    /* IN */
                                gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_get_target_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* target */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_get_target:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_get_target (EggLine  *line,   /* IN */
                             gint      argc,   /* IN */
                             gchar    *argv[], /* IN */
                             GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gchar* target;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &target;
	pk_connection_channel_get_target_async(conn,
	                             channel,
	                             NULL,
	                             pk_shell_channel_get_target_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %s\n", "target", target);
	egg_line_set_variable(line, "1", target);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_get_working_dir_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_get_working_dir_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_get_working_dir_cb (GObject       *object,    /* IN */
                                     GAsyncResult  *result,    /* IN */
                                     gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_get_working_dir_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* working_dir */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_get_working_dir:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_get_working_dir (EggLine  *line,   /* IN */
                                  gint      argc,   /* IN */
                                  gchar    *argv[], /* IN */
                                  GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gchar* working_dir;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &working_dir;
	pk_connection_channel_get_working_dir_async(conn,
	                             channel,
	                             NULL,
	                             pk_shell_channel_get_working_dir_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %s\n", "working_dir", working_dir);
	egg_line_set_variable(line, "1", working_dir);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_mute_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_mute_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_mute_cb (GObject       *object,    /* IN */
                          GAsyncResult  *result,    /* IN */
                          gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_mute_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_mute:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_mute (EggLine  *line,   /* IN */
                       gint      argc,   /* IN */
                       gchar    *argv[], /* IN */
                       GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_channel_mute_async(conn,
	                             channel,
	                             NULL,
	                             pk_shell_channel_mute_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_set_args_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_set_args_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_set_args_cb (GObject       *object,    /* IN */
                              GAsyncResult  *result,    /* IN */
                              gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_set_args_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_set_args:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_set_args (EggLine  *line,   /* IN */
                           gint      argc,   /* IN */
                           gchar    *argv[], /* IN */
                           GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gchar** args = NULL;
	gint i = 0;

	ENTRY;
	if (argc != 2) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!g_shell_parse_argv(argv[i++], NULL, &args, error)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_channel_set_args_async(conn,
	                             channel,
	                             args,
	                             NULL,
	                             pk_shell_channel_set_args_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_set_env_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_set_env_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_set_env_cb (GObject       *object,    /* IN */
                             GAsyncResult  *result,    /* IN */
                             gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_set_env_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_set_env:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_set_env (EggLine  *line,   /* IN */
                          gint      argc,   /* IN */
                          gchar    *argv[], /* IN */
                          GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gchar** env = NULL;
	gint i = 0;

	ENTRY;
	if (argc != 2) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!g_shell_parse_argv(argv[i++], NULL, &env, error)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_channel_set_env_async(conn,
	                             channel,
	                             env,
	                             NULL,
	                             pk_shell_channel_set_env_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_set_kill_pid_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_set_kill_pid_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_set_kill_pid_cb (GObject       *object,    /* IN */
                                  GAsyncResult  *result,    /* IN */
                                  gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_set_kill_pid_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_set_kill_pid:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_set_kill_pid (EggLine  *line,   /* IN */
                               gint      argc,   /* IN */
                               gchar    *argv[], /* IN */
                               GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gboolean kill_pid = 0;
	gint i = 0;

	ENTRY;
	if (argc != 2) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_boolean(argv[i++], &kill_pid)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_channel_set_kill_pid_async(conn,
	                             channel,
	                             kill_pid,
	                             NULL,
	                             pk_shell_channel_set_kill_pid_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_set_pid_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_set_pid_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_set_pid_cb (GObject       *object,    /* IN */
                             GAsyncResult  *result,    /* IN */
                             gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_set_pid_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_set_pid:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_set_pid (EggLine  *line,   /* IN */
                          gint      argc,   /* IN */
                          gchar    *argv[], /* IN */
                          GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gint pid = 0;
	gint i = 0;

	ENTRY;
	if (argc != 2) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &pid)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_channel_set_pid_async(conn,
	                             channel,
	                             pid,
	                             NULL,
	                             pk_shell_channel_set_pid_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_set_target_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_set_target_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_set_target_cb (GObject       *object,    /* IN */
                                GAsyncResult  *result,    /* IN */
                                gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_set_target_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_set_target:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_set_target (EggLine  *line,   /* IN */
                             gint      argc,   /* IN */
                             gchar    *argv[], /* IN */
                             GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	const gchar* target = NULL;
	gint i = 0;

	ENTRY;
	if (argc != 2) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	target = argv[i++];
	async_task_init(&task);
	pk_connection_channel_set_target_async(conn,
	                             channel,
	                             target,
	                             NULL,
	                             pk_shell_channel_set_target_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_set_working_dir_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_set_working_dir_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_set_working_dir_cb (GObject       *object,    /* IN */
                                     GAsyncResult  *result,    /* IN */
                                     gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_set_working_dir_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_set_working_dir:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_set_working_dir (EggLine  *line,   /* IN */
                                  gint      argc,   /* IN */
                                  gchar    *argv[], /* IN */
                                  GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	const gchar* working_dir = NULL;
	gint i = 0;

	ENTRY;
	if (argc != 2) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	working_dir = argv[i++];
	async_task_init(&task);
	pk_connection_channel_set_working_dir_async(conn,
	                             channel,
	                             working_dir,
	                             NULL,
	                             pk_shell_channel_set_working_dir_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_start_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_start_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_start_cb (GObject       *object,    /* IN */
                           GAsyncResult  *result,    /* IN */
                           gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_start_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_start:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_start (EggLine  *line,   /* IN */
                        gint      argc,   /* IN */
                        gchar    *argv[], /* IN */
                        GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_channel_start_async(conn,
	                             channel,
	                             NULL,
	                             pk_shell_channel_start_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_stop_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_stop_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_stop_cb (GObject       *object,    /* IN */
                          GAsyncResult  *result,    /* IN */
                          gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_stop_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_stop:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_stop (EggLine  *line,   /* IN */
                       gint      argc,   /* IN */
                       gchar    *argv[], /* IN */
                       GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_channel_stop_async(conn,
	                             channel,
	                             NULL,
	                             pk_shell_channel_stop_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_channel_unmute_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_channel_unmute_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_channel_unmute_cb (GObject       *object,    /* IN */
                            GAsyncResult  *result,    /* IN */
                            gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_channel_unmute_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_channel_unmute:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_channel_unmute (EggLine  *line,   /* IN */
                         gint      argc,   /* IN */
                         gchar    *argv[], /* IN */
                         GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_channel_unmute_async(conn,
	                             channel,
	                             NULL,
	                             pk_shell_channel_unmute_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_encoder_get_plugin_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_encoder_get_plugin_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_encoder_get_plugin_cb (GObject       *object,    /* IN */
                                GAsyncResult  *result,    /* IN */
                                gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_encoder_get_plugin_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* plugin */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_encoder_get_plugin:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_encoder_get_plugin (EggLine  *line,   /* IN */
                             gint      argc,   /* IN */
                             gchar    *argv[], /* IN */
                             GError  **error)  /* OUT */
{
	AsyncTask task;
	gint encoder = 0;
	gchar* plugin;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &encoder)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &plugin;
	pk_connection_encoder_get_plugin_async(conn,
	                             encoder,
	                             NULL,
	                             pk_shell_encoder_get_plugin_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %s\n", "plugin", plugin);
	egg_line_set_variable(line, "1", plugin);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_manager_add_channel_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_manager_add_channel_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_manager_add_channel_cb (GObject       *object,    /* IN */
                                 GAsyncResult  *result,    /* IN */
                                 gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_manager_add_channel_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* channel */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_manager_add_channel:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_manager_add_channel (EggLine  *line,   /* IN */
                              gint      argc,   /* IN */
                              gchar    *argv[], /* IN */
                              GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel;
	gchar *tmp;

	ENTRY;
	if (argc != 0) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &channel;
	pk_connection_manager_add_channel_async(conn,
	                             NULL,
	                             pk_shell_manager_add_channel_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %d\n", "channel", (gint)channel);
	tmp = g_strdup_printf("%d", (gint)channel);
	egg_line_set_variable(line, "1", tmp);
	g_free(tmp);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_manager_add_source_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_manager_add_source_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_manager_add_source_cb (GObject       *object,    /* IN */
                                GAsyncResult  *result,    /* IN */
                                gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_manager_add_source_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* source */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_manager_add_source:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_manager_add_source (EggLine  *line,   /* IN */
                             gint      argc,   /* IN */
                             gchar    *argv[], /* IN */
                             GError  **error)  /* OUT */
{
	AsyncTask task;
	const gchar* plugin = NULL;
	gint source;
	gint i = 0;
	gchar *tmp;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	plugin = argv[i++];
	async_task_init(&task);
	task.params[0] = &source;
	pk_connection_manager_add_source_async(conn,
	                             plugin,
	                             NULL,
	                             pk_shell_manager_add_source_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %d\n", "source", (gint)source);
	tmp = g_strdup_printf("%d", (gint)source);
	egg_line_set_variable(line, "1", tmp);
	g_free(tmp);
	RETURN(EGG_LINE_STATUS_OK);
}

static void
manifest_cb (PkManifest *manifest,  /* IN */
             gpointer    user_data) /* IN */
{
	ENTRY;
	g_message("Received manifest from %d.",
	          pk_manifest_get_source_id(manifest));
	EXIT;
}

static void
sample_cb (PkSample *sample,    /* IN */
           gpointer  user_data) /* IN */
{
	ENTRY;
	g_message("Received sample from %d",
	          pk_sample_get_source_id(sample));
	EXIT;
}

static void
set_handlers_cb (GObject      *source,    /* IN */
                 GAsyncResult *result,    /* IN */
                 gpointer      user_data) /* IN */
{
	ENTRY;
	pk_connection_subscription_set_handlers_finish(PK_CONNECTION(source),
	                                               result, NULL);
	EXIT;
}

/**
 * pk_shell_manager_add_subscription_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_manager_add_subscription_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_manager_add_subscription_cb (GObject       *object,    /* IN */
                                      GAsyncResult  *result,    /* IN */
                                      gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_manager_add_subscription_finish(PK_CONNECTION(object),
	                                                             result,
	                                                             task->params[0], /* subscription */
	                                                             &task->error);
	if (task->result) {
		pk_connection_subscription_set_handlers_async(PK_CONNECTION(object),
		                                              *((gint *)task->params[0]),
		                                              manifest_cb,
		                                              NULL,
		                                              NULL,
		                                              sample_cb,
		                                              NULL,
		                                              NULL,
		                                              NULL,
		                                              set_handlers_cb,
		                                              NULL);
	}
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_manager_add_subscription:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_manager_add_subscription (EggLine  *line,   /* IN */
                                   gint      argc,   /* IN */
                                   gchar    *argv[], /* IN */
                                   GError  **error)  /* OUT */
{
	AsyncTask task;
	gsize buffer_size = 0;
	gint buffer_size_int;
	gsize timeout = 0;
	gint timeout_int;
	gint subscription;
	gint i = 0;
	gchar *tmp;

	ENTRY;
	if (argc != 2) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &buffer_size_int)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	buffer_size = buffer_size_int;
	if (!pk_shell_parse_int(argv[i++], &timeout_int)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	timeout = timeout_int;
	async_task_init(&task);
	task.params[0] = &subscription;
	pk_connection_manager_add_subscription_async(conn,
	                                             buffer_size,
	                                             timeout,
	                                             NULL,
	                                             pk_shell_manager_add_subscription_cb,
	                                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %d\n", "subscription", (gint)subscription);
	tmp = g_strdup_printf("%d", (gint)subscription);
	egg_line_set_variable(line, "1", tmp);
	g_free(tmp);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_manager_get_channels_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_manager_get_channels_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_manager_get_channels_cb (GObject       *object,    /* IN */
            GAsyncResult  *result,    /* IN */
            gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_manager_get_channels_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* channels */
			task->params[1], /* channels_len */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_manager_get_channels:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_manager_get_channels (EggLine  *line,   /* IN */
                               gint      argc,   /* IN */
                               gchar    *argv[], /* IN */
                               GError  **error)  /* OUT */
{
	AsyncTask task;
	gint* channels;
	gsize channels_len;
	gint i = 0;
	gchar *tmp;

	ENTRY;
	if (argc != 0) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &channels;
	task.params[1] = &channels_len;
	pk_connection_manager_get_channels_async(conn,
	                             NULL,
	                             pk_shell_manager_get_channels_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	if (channels && channels_len) {
		g_print("%16s: [", "channels");
		for (i = 0; i < channels_len; i++) {
			g_print("%d%s", channels[i], ((i + 1) == channels_len) ? "" : ", ");
		}
		g_print("]\n");
	}
	egg_line_set_variable(line, "1", "");
	g_print("%16s: %d\n", "channels_len", (gint)channels_len);
	tmp = g_strdup_printf("%d", (gint)channels_len);
	egg_line_set_variable(line, "2", tmp);
	g_free(tmp);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_manager_get_plugins_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_manager_get_plugins_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_manager_get_plugins_cb (GObject       *object,    /* IN */
                                 GAsyncResult  *result,    /* IN */
                                 gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_manager_get_plugins_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* plugins */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_manager_get_plugins:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_manager_get_plugins (EggLine  *line,   /* IN */
                              gint      argc,   /* IN */
                              gchar    *argv[], /* IN */
                              GError  **error)  /* OUT */
{
	AsyncTask task;
	gchar** plugins;
	gchar *plugins_str;

	ENTRY;
	if (argc != 0) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &plugins;
	pk_connection_manager_get_plugins_async(conn,
	                             NULL,
	                             pk_shell_manager_get_plugins_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	if (plugins && g_strv_length(plugins)) {
		plugins_str = g_strjoinv("', '", plugins);
		g_print("%16s: ['%s']\n", "plugins", plugins_str);
		g_free(plugins_str);
	} else {
		g_print("%16s: []\n", "plugins");
	}
	egg_line_set_variable(line, "1", "");
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_manager_get_sources_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_manager_get_sources_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_manager_get_sources_cb (GObject       *object,    /* IN */
                                 GAsyncResult  *result,    /* IN */
                                 gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_manager_get_sources_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* sources */
			task->params[1], /* sources_len */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_manager_get_sources:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_manager_get_sources (EggLine  *line,   /* IN */
                              gint      argc,   /* IN */
                              gchar    *argv[], /* IN */
                              GError  **error)  /* OUT */
{
	AsyncTask task;
	gint* sources;
	gsize sources_len;
	gint i = 0;
	gchar *tmp;

	ENTRY;
	if (argc != 0) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &sources;
	task.params[1] = &sources_len;
	pk_connection_manager_get_sources_async(conn,
	                             NULL,
	                             pk_shell_manager_get_sources_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	if (sources && sources_len) {
		g_print("%16s: [", "sources");
		for (i = 0; i < sources_len; i++) {
			g_print("%d%s", sources[i], ((i + 1) == sources_len) ? "" : ", ");
		}
		g_print("]\n");
	}
	egg_line_set_variable(line, "1", "");
	g_print("%16s: %d\n", "sources_len", (gint)sources_len);
	tmp = g_strdup_printf("%d", (gint)sources_len);
	egg_line_set_variable(line, "2", tmp);
	g_free(tmp);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_manager_get_version_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_manager_get_version_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_manager_get_version_cb (GObject       *object,    /* IN */
                                 GAsyncResult  *result,    /* IN */
                                 gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_manager_get_version_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* version */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_manager_get_version:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_manager_get_version (EggLine  *line,   /* IN */
                              gint      argc,   /* IN */
                              gchar    *argv[], /* IN */
                              GError  **error)  /* OUT */
{
	AsyncTask task;
	gchar* version;

	ENTRY;
	if (argc != 0) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &version;
	pk_connection_manager_get_version_async(conn,
	                             NULL,
	                             pk_shell_manager_get_version_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %s\n", "version", version);
	egg_line_set_variable(line, "1", version);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_manager_ping_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_manager_ping_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_manager_ping_cb (GObject       *object,    /* IN */
                          GAsyncResult  *result,    /* IN */
                          gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_manager_ping_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* tv */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_manager_ping:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_manager_ping (EggLine  *line,   /* IN */
                       gint      argc,   /* IN */
                       gchar    *argv[], /* IN */
                       GError  **error)  /* OUT */
{
	AsyncTask task;
	GTimeVal tv;
	gchar *tv_str;

	ENTRY;
	if (argc != 0) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &tv;
	pk_connection_manager_ping_async(conn,
	                             NULL,
	                             pk_shell_manager_ping_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	tv_str = g_time_val_to_iso8601(&tv);
	g_print("%16s: %s\n", "tv", tv_str);
	egg_line_set_variable(line, "1", tv_str);
	g_free(tv_str);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_manager_remove_channel_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_manager_remove_channel_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_manager_remove_channel_cb (GObject       *object,    /* IN */
                                    GAsyncResult  *result,    /* IN */
                                    gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_manager_remove_channel_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* removed */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_manager_remove_channel:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_manager_remove_channel (EggLine  *line,   /* IN */
                                 gint      argc,   /* IN */
                                 gchar    *argv[], /* IN */
                                 GError  **error)  /* OUT */
{
	AsyncTask task;
	gint channel = 0;
	gboolean removed;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &removed;
	pk_connection_manager_remove_channel_async(conn,
	                             channel,
	                             NULL,
	                             pk_shell_manager_remove_channel_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %s\n", "removed", removed ? "TRUE" : "FALSE");
	egg_line_set_variable(line, "1", removed ? "1" : "0");
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_manager_remove_source_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_manager_remove_source_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_manager_remove_source_cb (GObject       *object,    /* IN */
                                   GAsyncResult  *result,    /* IN */
                                   gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_manager_remove_source_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_manager_remove_source:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_manager_remove_source (EggLine  *line,   /* IN */
                                gint      argc,   /* IN */
                                gchar    *argv[], /* IN */
                                GError  **error)  /* OUT */
{
	AsyncTask task;
	gint source = 0;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &source)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_manager_remove_source_async(conn,
	                             source,
	                             NULL,
	                             pk_shell_manager_remove_source_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_manager_remove_subscription_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_manager_remove_subscription_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_manager_remove_subscription_cb (GObject       *object,    /* IN */
                                         GAsyncResult  *result,    /* IN */
                                         gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_manager_remove_subscription_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* removed */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_manager_remove_subscription:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_manager_remove_subscription (EggLine  *line,   /* IN */
                                      gint      argc,   /* IN */
                                      gchar    *argv[], /* IN */
                                      GError  **error)  /* OUT */
{
	AsyncTask task;
	gint subscription = 0;
	gboolean removed;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &subscription)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &removed;
	pk_connection_manager_remove_subscription_async(conn,
	                             subscription,
	                             NULL,
	                             pk_shell_manager_remove_subscription_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %s\n", "removed", removed ? "TRUE" : "FALSE");
	egg_line_set_variable(line, "1", removed ? "1" : "0");
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_plugin_get_copyright_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_plugin_get_copyright_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_plugin_get_copyright_cb (GObject       *object,    /* IN */
                                  GAsyncResult  *result,    /* IN */
                                  gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_plugin_get_copyright_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* copyright */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_plugin_get_copyright:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_plugin_get_copyright (EggLine  *line,   /* IN */
                               gint      argc,   /* IN */
                               gchar    *argv[], /* IN */
                               GError  **error)  /* OUT */
{
	AsyncTask task;
	const gchar* plugin = NULL;
	gchar* copyright;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	plugin = argv[i++];
	async_task_init(&task);
	task.params[0] = &copyright;
	pk_connection_plugin_get_copyright_async(conn,
	                             plugin,
	                             NULL,
	                             pk_shell_plugin_get_copyright_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %s\n", "copyright", copyright);
	egg_line_set_variable(line, "1", copyright);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_plugin_get_description_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_plugin_get_description_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_plugin_get_description_cb (GObject       *object,    /* IN */
                                    GAsyncResult  *result,    /* IN */
                                    gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_plugin_get_description_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* description */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_plugin_get_description:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_plugin_get_description (EggLine  *line,   /* IN */
                                 gint      argc,   /* IN */
                                 gchar    *argv[], /* IN */
                                 GError  **error)  /* OUT */
{
	AsyncTask task;
	const gchar* plugin = NULL;
	gchar* description;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	plugin = argv[i++];
	async_task_init(&task);
	task.params[0] = &description;
	pk_connection_plugin_get_description_async(conn,
	                             plugin,
	                             NULL,
	                             pk_shell_plugin_get_description_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %s\n", "description", description);
	egg_line_set_variable(line, "1", description);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_plugin_get_name_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_plugin_get_name_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_plugin_get_name_cb (GObject       *object,    /* IN */
                             GAsyncResult  *result,    /* IN */
                             gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_plugin_get_name_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* name */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_plugin_get_name:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_plugin_get_name (EggLine  *line,   /* IN */
                          gint      argc,   /* IN */
                          gchar    *argv[], /* IN */
                          GError  **error)  /* OUT */
{
	AsyncTask task;
	const gchar* plugin = NULL;
	gchar* name;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	plugin = argv[i++];
	async_task_init(&task);
	task.params[0] = &name;
	pk_connection_plugin_get_name_async(conn,
	                             plugin,
	                             NULL,
	                             pk_shell_plugin_get_name_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %s\n", "name", name);
	egg_line_set_variable(line, "1", name);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_plugin_get_plugin_type_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_plugin_get_plugin_type_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_plugin_get_plugin_type_cb (GObject       *object,    /* IN */
                                    GAsyncResult  *result,    /* IN */
                                    gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_plugin_get_plugin_type_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* type */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_plugin_get_plugin_type:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_plugin_get_plugin_type (EggLine  *line,   /* IN */
                                 gint      argc,   /* IN */
                                 gchar    *argv[], /* IN */
                                 GError  **error)  /* OUT */
{
	AsyncTask task;
	const gchar* plugin = NULL;
	gint type;
	gint i = 0;
	gchar *tmp;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	plugin = argv[i++];
	async_task_init(&task);
	task.params[0] = &type;
	pk_connection_plugin_get_plugin_type_async(conn,
	                             plugin,
	                             NULL,
	                             pk_shell_plugin_get_plugin_type_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %d\n", "type", (gint)type);
	tmp = g_strdup_printf("%d", (gint)type);
	egg_line_set_variable(line, "1", tmp);
	g_free(tmp);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_plugin_get_version_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_plugin_get_version_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_plugin_get_version_cb (GObject       *object,    /* IN */
                                GAsyncResult  *result,    /* IN */
                                gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_plugin_get_version_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* version */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_plugin_get_version:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_plugin_get_version (EggLine  *line,   /* IN */
                             gint      argc,   /* IN */
                             gchar    *argv[], /* IN */
                             GError  **error)  /* OUT */
{
	AsyncTask task;
	const gchar* plugin = NULL;
	gchar* version;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	plugin = argv[i++];
	async_task_init(&task);
	task.params[0] = &version;
	pk_connection_plugin_get_version_async(conn,
	                             plugin,
	                             NULL,
	                             pk_shell_plugin_get_version_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %s\n", "version", version);
	egg_line_set_variable(line, "1", version);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_source_get_plugin_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_source_get_plugin_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_source_get_plugin_cb (GObject       *object,    /* IN */
                               GAsyncResult  *result,    /* IN */
                               gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_source_get_plugin_finish(
			PK_CONNECTION(object),
			result,
			task->params[0], /* plugin */
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_source_get_plugin:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_source_get_plugin (EggLine  *line,   /* IN */
                            gint      argc,   /* IN */
                            gchar    *argv[], /* IN */
                            GError  **error)  /* OUT */
{
	AsyncTask task;
	gint source = 0;
	gchar* plugin;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &source)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	task.params[0] = &plugin;
	pk_connection_source_get_plugin_async(conn,
	                             source,
	                             NULL,
	                             pk_shell_source_get_plugin_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	g_print("%16s: %s\n", "plugin", plugin);
	egg_line_set_variable(line, "1", plugin);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_subscription_add_channel_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_subscription_add_channel_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_subscription_add_channel_cb (GObject       *object,    /* IN */
                                      GAsyncResult  *result,    /* IN */
                                      gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_subscription_add_channel_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_subscription_add_channel:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_subscription_add_channel (EggLine  *line,   /* IN */
                                   gint      argc,   /* IN */
                                   gchar    *argv[], /* IN */
                                   GError  **error)  /* OUT */
{
	AsyncTask task;
	gint subscription = 0;
	gint channel = 0;
	gboolean monitor = 0;
	gint i = 0;

	ENTRY;
	if (argc != 3) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &subscription)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_boolean(argv[i++], &monitor)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_subscription_add_channel_async(conn,
	                             subscription,
	                             channel,
	                             monitor,
	                             NULL,
	                             pk_shell_subscription_add_channel_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_subscription_add_source_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_subscription_add_source_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_subscription_add_source_cb (GObject       *object,    /* IN */
                                     GAsyncResult  *result,    /* IN */
                                     gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_subscription_add_source_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_subscription_add_source:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_subscription_add_source (EggLine  *line,   /* IN */
                                  gint      argc,   /* IN */
                                  gchar    *argv[], /* IN */
                                  GError  **error)  /* OUT */
{
	AsyncTask task;
	gint subscription = 0;
	gint source = 0;
	gint i = 0;

	ENTRY;
	if (argc != 2) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &subscription)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &source)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_subscription_add_source_async(conn,
	                             subscription,
	                             source,
	                             NULL,
	                             pk_shell_subscription_add_source_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_subscription_mute_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_subscription_mute_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_subscription_mute_cb (GObject       *object,    /* IN */
                               GAsyncResult  *result,    /* IN */
                               gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_subscription_mute_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_subscription_mute:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_subscription_mute (EggLine  *line,   /* IN */
                            gint      argc,   /* IN */
                            gchar    *argv[], /* IN */
                            GError  **error)  /* OUT */
{
	AsyncTask task;
	gint subscription = 0;
	gboolean drain = 0;
	gint i = 0;

	ENTRY;
	if (argc != 2) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &subscription)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_boolean(argv[i++], &drain)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_subscription_mute_async(conn,
	                             subscription,
	                             drain,
	                             NULL,
	                             pk_shell_subscription_mute_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_subscription_remove_channel_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_subscription_remove_channel_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_subscription_remove_channel_cb (GObject       *object,    /* IN */
                                         GAsyncResult  *result,    /* IN */
                                         gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_subscription_remove_channel_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_subscription_remove_channel:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_subscription_remove_channel (EggLine  *line,   /* IN */
                                      gint      argc,   /* IN */
                                      gchar    *argv[], /* IN */
                                      GError  **error)  /* OUT */
{
	AsyncTask task;
	gint subscription = 0;
	gint channel = 0;
	gint i = 0;

	ENTRY;
	if (argc != 2) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &subscription)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &channel)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_subscription_remove_channel_async(conn,
	                             subscription,
	                             channel,
	                             NULL,
	                             pk_shell_subscription_remove_channel_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_subscription_remove_source_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_subscription_remove_source_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_subscription_remove_source_cb (GObject       *object,    /* IN */
                                        GAsyncResult  *result,    /* IN */
                                        gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_subscription_remove_source_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_subscription_remove_source:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_subscription_remove_source (EggLine  *line,   /* IN */
                                     gint      argc,   /* IN */
                                     gchar    *argv[], /* IN */
                                     GError  **error)  /* OUT */
{
	AsyncTask task;
	gint subscription = 0;
	gint source = 0;
	gint i = 0;

	ENTRY;
	if (argc != 2) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &subscription)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &source)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_subscription_remove_source_async(conn,
	                             subscription,
	                             source,
	                             NULL,
	                             pk_shell_subscription_remove_source_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_subscription_set_buffer_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_subscription_set_buffer_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_subscription_set_buffer_cb (GObject       *object,    /* IN */
                                     GAsyncResult  *result,    /* IN */
                                     gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_subscription_set_buffer_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_subscription_set_buffer:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_subscription_set_buffer (EggLine  *line,   /* IN */
                                  gint      argc,   /* IN */
                                  gchar    *argv[], /* IN */
                                  GError  **error)  /* OUT */
{
	AsyncTask task;
	gint subscription = 0;
	gint timeout = 0;
	gint size = 0;
	gint i = 0;

	ENTRY;
	if (argc != 3) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &subscription)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &timeout)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &size)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_subscription_set_buffer_async(conn,
	                             subscription,
	                             timeout,
	                             size,
	                             NULL,
	                             pk_shell_subscription_set_buffer_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_subscription_set_encoder_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_subscription_set_encoder_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_subscription_set_encoder_cb (GObject       *object,    /* IN */
                                      GAsyncResult  *result,    /* IN */
                                      gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_subscription_set_encoder_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_subscription_set_encoder:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_subscription_set_encoder (EggLine  *line,   /* IN */
                                   gint      argc,   /* IN */
                                   gchar    *argv[], /* IN */
                                   GError  **error)  /* OUT */
{
	AsyncTask task;
	gint subscription = 0;
	gint encoder = 0;
	gint i = 0;

	ENTRY;
	if (argc != 2) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &subscription)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &encoder)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_subscription_set_encoder_async(conn,
	                             subscription,
	                             encoder,
	                             NULL,
	                             pk_shell_subscription_set_encoder_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_subscription_unmute_cb:
 * @object: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #gpointer.
 *
 * Asynchronous completion of pk_connection_subscription_unmute_async().
 *
 * Returns: None.
 * Side effects: Blocking AsyncTask is signaled.
 */
static void
pk_shell_subscription_unmute_cb (GObject       *object,    /* IN */
                                 GAsyncResult  *result,    /* IN */
                                 gpointer       user_data) /* IN */
{
	AsyncTask *task = user_data;

	ENTRY;
	task->result = pk_connection_subscription_unmute_finish(
			PK_CONNECTION(object),
			result,
			&task->error);
	async_task_signal(task);
	EXIT;
}

/**
 * pk_shell_subscription_unmute:
 * @line: An #EggLine.
 * @argc: The number of arguments in @argv.
 * @argv: The arguments to the command.
 * @error: A location for #GError, or %NULL.
 *
 * 
 *
 * Returns: The commands status.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_subscription_unmute (EggLine  *line,   /* IN */
                              gint      argc,   /* IN */
                              gchar    *argv[], /* IN */
                              GError  **error)  /* OUT */
{
	AsyncTask task;
	gint subscription = 0;
	gint i = 0;

	ENTRY;
	if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (!pk_shell_parse_int(argv[i++], &subscription)) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	async_task_init(&task);
	pk_connection_subscription_unmute_async(conn,
	                             subscription,
	                             NULL,
	                             pk_shell_subscription_unmute_cb,
	                             &task);
	if (!async_task_wait(&task)) {
		g_propagate_error(error, task.error);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_shell_log:
 * @line: An #EggLine.
 * @argc: The argument count.
 * @argv: The arguments.
 * @error: A location for a #GError, or %NULL.
 *
 * Alters the level of log messages which are written to the console.
 *
 * Returns: An #EggLineStatus.
 * Side effects: The log threshold is set.
 */
static EggLineStatus
pk_shell_shell_log (EggLine  *line,   /* IN */
                    gint      argc,   /* IN */
                    gchar    *argv[], /* IN */
                    GError  **error)  /* OUT */
{
	if (argc < 1 || !argv[0]) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	if (g_strcmp0(argv[0], "trace") == 0) {
		log_threshold = G_LOG_LEVEL_TRACE;
	} else if (g_strcmp0(argv[0], "debug") == 0) {
		log_threshold = G_LOG_LEVEL_DEBUG;
	} else if (g_strcmp0(argv[0], "error") == 0) {
		log_threshold = G_LOG_LEVEL_ERROR;
	} else if (g_strcmp0(argv[0], "info") == 0) {
		log_threshold = G_LOG_LEVEL_INFO;
	} else if (g_strcmp0(argv[0], "message") == 0) {
		log_threshold = G_LOG_LEVEL_MESSAGE;
	} else if (g_strcmp0(argv[0], "warning") == 0) {
		log_threshold = G_LOG_LEVEL_WARNING;
	} else if (g_strcmp0(argv[0], "critical") == 0) {
		log_threshold = G_LOG_LEVEL_CRITICAL;
	} else {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_cd:
 * @line: An #EggLine.
 * @argc: Argument count.
 * @argv: Arguments.
 * @error: location for a #GError, or %NULL.
 *
 * Changes the working directory of the shell.
 *
 * Returns: An EggLineStatus.
 * Side effects: The processes working directory is changed.
 */
static EggLineStatus
pk_shell_cd (EggLine  *line,   /* IN */
             gint      argc,   /* IN */
             gchar    *argv[], /* IN */
             GError  **error)  /* OUT */
{
	ENTRY;
	if (argc < 1) {
		g_chdir(g_get_home_dir());
		RETURN(EGG_LINE_STATUS_OK);
	} else if (argc != 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	} else if (!g_file_test(argv[0], G_FILE_TEST_IS_DIR)) {
		g_set_error(error, PK_SHELL_ERROR, PK_SHELL_ERROR_VALUE,
		            "Directory not found: %s", argv[0]);
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	if (g_chdir(argv[0]) != 0) {
		g_set_error(error, PK_SHELL_ERROR, PK_SHELL_ERROR_VALUE,
		            "Could not change directories.");
		RETURN(EGG_LINE_STATUS_FAILURE);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_ls:
 * @line: An #EggLine.
 * @argc: Argument count.
 * @argv: Arguments.
 * @error: location for a #GError, or %NULL.
 *
 * Lists the contents of the current directory.
 *
 * Returns: An EggLineStatus.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_ls (EggLine  *line,   /* IN */
             gint      argc,   /* IN */
             gchar    *argv[], /* IN */
             GError  **error)  /* OUT */
{
	gchar **spawn_argv;
	gint i;

	ENTRY;
	spawn_argv = g_new0(gchar*, argc + 2);
	spawn_argv[0] = g_strdup("ls");
	for (i = 0; i < argc; i++) {
		spawn_argv[1 + i] = g_strdup(argv[i]);
	}
	g_spawn_sync(".", spawn_argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
	             NULL, NULL, NULL, NULL);
	g_strfreev(spawn_argv);
	RETURN(EGG_LINE_STATUS_OK);
}

/**
 * pk_shell_load:
 * @line: An #EggLine.
 * @argc: Argument count.
 * @argv: Arguments.
 * @error: A location for a #GError, or %NULL.
 *
 * Loads commands from the files provided.
 *
 * Returns: An EggLineStatus.
 * Side effects: None.
 */
static EggLineStatus
pk_shell_load (EggLine  *line,   /* IN */
             gint      argc,   /* IN */
             gchar    *argv[], /* IN */
             GError  **error)  /* OUT */
{
	gint i;

	ENTRY;
	if (argc < 1) {
		RETURN(EGG_LINE_STATUS_BAD_ARGS);
	}
	for (i = 0; argv[i]; i++) {
		egg_line_execute_file(line, argv[i]);
	}
	RETURN(EGG_LINE_STATUS_OK);
}

static EggLineStatus
pk_shell_echo (EggLine  *line,   /* IN */
               gint      argc,   /* IN */
               gchar    *argv[], /* IN */
               GError  **error)  /* OUT */
{
	gchar *text;

	ENTRY;
	text = g_strjoinv(" ", argv);
	g_print("%s\n", text);
	g_free(text);
	RETURN(EGG_LINE_STATUS_OK);
}

static EggLineStatus
pk_shell_help (EggLine  *line,   /* IN */
               gint      argc,   /* IN */
               gchar    *argv[], /* IN */
               GError  **error)  /* OUT */
{
	EggLineCommand *command;
	gchar *text;

	ENTRY;
	text = g_strjoinv(" ", argv);
	if ((command = egg_line_resolve(line, text, NULL, NULL))) {
		g_print("usage: %s\n\n%s\n", command->usage, command->help);
	}
	g_free(text);
	RETURN(EGG_LINE_STATUS_OK);
}

static EggLineCommand plugin_commands[] = {
	{
		.name      = "get-copyright",
		.help      = "The plugin copyright.\n"
		             "\n"
		             "options:\n"
		             "  PLUGIN:\t\tA string.\n"
		             "\n",
		.callback  = pk_shell_plugin_get_copyright,
		.usage     = "plugin get-copyright PLUGIN",
	},
	{
		.name      = "get-description",
		.help      = "The plugin description.\n"
		             "\n"
		             "options:\n"
		             "  PLUGIN:\t\tA string.\n"
		             "\n",
		.callback  = pk_shell_plugin_get_description,
		.usage     = "plugin get-description PLUGIN",
	},
	{
		.name      = "get-name",
		.help      = "The plugin name.\n"
		             "\n"
		             "options:\n"
		             "  PLUGIN:\t\tA string.\n"
		             "\n",
		.callback  = pk_shell_plugin_get_name,
		.usage     = "plugin get-name PLUGIN",
	},
	{
		.name      = "get-plugin-type",
		.help      = "The plugin type.\n"
		             "\n"
		             "options:\n"
		             "  PLUGIN:\t\tA string.\n"
		             "\n",
		.callback  = pk_shell_plugin_get_plugin_type,
		.usage     = "plugin get-plugin-type PLUGIN",
	},
	{
		.name      = "get-version",
		.help      = "The plugin version.\n"
		             "\n"
		             "options:\n"
		             "  PLUGIN:\t\tA string.\n"
		             "\n",
		.callback  = pk_shell_plugin_get_version,
		.usage     = "plugin get-version PLUGIN",
	},
	{ NULL }
};

static EggLineCommand encoder_commands[] = {
	{
		.name      = "get-plugin",
		.help      = "Retrieves the plugin which created the encoder instance.\n"
		             "\n"
		             "options:\n"
		             "  ENCODER:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_encoder_get_plugin,
		.usage     = "encoder get-plugin ENCODER",
	},
	{ NULL }
};

static EggLineCommand source_commands[] = {
	{
		.name      = "get-plugin",
		.help      = "Retrieves the plugin for which the source originates.\n"
		             "\n"
		             "options:\n"
		             "  SOURCE:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_source_get_plugin,
		.usage     = "source get-plugin SOURCE",
	},
	{ NULL }
};

static EggLineCommand manager_commands[] = {
	{
		.name      = "add-channel",
		.help      = "Adds a channel to the agent.",
		.callback  = pk_shell_manager_add_channel,
		.usage     = "manager add-channel ",
	},
	{
		.name      = "add-source",
		.help      = "Create a new source from a plugin in the Agent.",
		.callback  = pk_shell_manager_add_source,
		.usage     = "manager add-source PLUGIN",
	},
	{
		.name      = "add-subscription",
		.help      = "Adds a new subscription to the agent. @buffer_size is the size of the\ninternal buffer in bytes to queue before flushing data to the subscriber.\n@timeout is the maximum number of milliseconds that should pass before\nflushing data to the subscriber.\n\nIf @buffer_size and @timeout are 0, then no buffering will occur.\n\n@encoder is an optional encoder that can be used to encode the data\ninto a particular format the subscriber is expecting.",
		.callback  = pk_shell_manager_add_subscription,
		.usage     = "manager add-subscription BUFFER_SIZE TIMEOUT",
	},
	{
		.name      = "get-channels",
		.help      = "Retrieves the list of channels located within the agent.",
		.callback  = pk_shell_manager_get_channels,
		.usage     = "manager get-channels ",
	},
	{
		.name      = "get-plugins",
		.help      = "Retrieves the list of available plugins within the agent.",
		.callback  = pk_shell_manager_get_plugins,
		.usage     = "manager get-plugins ",
	},
	{
		.name      = "get-sources",
		.help      = "Retrieves the list of sources located within the agent.",
		.callback  = pk_shell_manager_get_sources,
		.usage     = "manager get-sources ",
	},
	{
		.name      = "get-version",
		.help      = "Retrieves the version of the agent.",
		.callback  = pk_shell_manager_get_version,
		.usage     = "manager get-version ",
	},
	{
		.name      = "ping",
		.help      = "Pings the agent over the RPC protocol to determine one-way latency.",
		.callback  = pk_shell_manager_ping,
		.usage     = "manager ping ",
	},
	{
		.name      = "remove-channel",
		.help      = "Removes a channel from the agent.",
		.callback  = pk_shell_manager_remove_channel,
		.usage     = "manager remove-channel CHANNEL",
	},
	{
		.name      = "remove-source",
		.help      = "Remove a source from the Agent.",
		.callback  = pk_shell_manager_remove_source,
		.usage     = "manager remove-source SOURCE",
	},
	{
		.name      = "remove-subscription",
		.help      = "Removes a subscription from the agent.",
		.callback  = pk_shell_manager_remove_subscription,
		.usage     = "manager remove-subscription SUBSCRIPTION",
	},
	{ NULL }
};

static EggLineCommand channel_commands[] = {
	{
		.name      = "add-source",
		.help      = "Adds an existing source to the channel.  If the channel has already been\nstarted, the source will be started immediately.  The source must not have\nbeen previous added to another channel or this will fail.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "  SOURCE:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_add_source,
		.usage     = "channel add-source CHANNEL SOURCE",
	},
	{
		.name      = "get-args",
		.help      = "Retrieves the arguments for target.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_get_args,
		.usage     = "channel get-args CHANNEL",
	},
	{
		.name      = "get-env",
		.help      = "Retrieves the environment for spawning the target process.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_get_env,
		.usage     = "channel get-env CHANNEL",
	},
	{
		.name      = "get-exit-status",
		.help      = "Retrieves the exit status of the process.  This is only set after the\nprocess has exited.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_get_exit_status,
		.usage     = "channel get-exit-status CHANNEL",
	},
	{
		.name      = "get-kill-pid",
		.help      = "Retrieves if the process should be killed when the channel is stopped.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_get_kill_pid,
		.usage     = "channel get-kill-pid CHANNEL",
	},
	{
		.name      = "get-pid",
		.help      = "Retrieves the process pid of the target process.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_get_pid,
		.usage     = "channel get-pid CHANNEL",
	},
	{
		.name      = "get-pid-set",
		.help      = "Retrieves if the \"pid\" property was set manually.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_get_pid_set,
		.usage     = "channel get-pid-set CHANNEL",
	},
	{
		.name      = "get-sources",
		.help      = "Retrieves the available sources.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_get_sources,
		.usage     = "channel get-sources CHANNEL",
	},
	{
		.name      = "get-state",
		.help      = "Retrieves the current state of the channel.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_get_state,
		.usage     = "channel get-state CHANNEL",
	},
	{
		.name      = "get-target",
		.help      = "Retrieves the channels target.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_get_target,
		.usage     = "channel get-target CHANNEL",
	},
	{
		.name      = "get-working-dir",
		.help      = "Retrieves the working directory of the target.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_get_working_dir,
		.usage     = "channel get-working-dir CHANNEL",
	},
	{
		.name      = "mute",
		.help      = "Notifies @channel to silently drop manifest and sample updates until\nunmute() is called.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_mute,
		.usage     = "channel mute CHANNEL",
	},
	{
		.name      = "set-args",
		.help      = "Sets the targets arguments.  This may only be set before the channel\nhas started.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "  ARGS:\t\tA string array.\n"
		             "\n",
		.callback  = pk_shell_channel_set_args,
		.usage     = "channel set-args CHANNEL ARGS",
	},
	{
		.name      = "set-env",
		.help      = "Sets the environment of the target process.  This may only be set before\nthe channel has started.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "  ENV:\t\tA string array.\n"
		             "\n",
		.callback  = pk_shell_channel_set_env,
		.usage     = "channel set-env CHANNEL ENV",
	},
	{
		.name      = "set-kill-pid",
		.help      = "Sets if the process should be killed when the channel is stopped.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "  KILL_PID:\t\tA boolean [yes, no, true, false, 0, 1].\n"
		             "\n",
		.callback  = pk_shell_channel_set_kill_pid,
		.usage     = "channel set-kill-pid CHANNEL KILL_PID",
	},
	{
		.name      = "set-pid",
		.help      = "Sets the target pid to attach to rather than spawning a process.  This can\nonly be set before the channel has started.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "  PID:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_set_pid,
		.usage     = "channel set-pid CHANNEL PID",
	},
	{
		.name      = "set-target",
		.help      = "Sets the channels target.  This may only be set before the channel has\nstarted.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "  TARGET:\t\tA string.\n"
		             "\n",
		.callback  = pk_shell_channel_set_target,
		.usage     = "channel set-target CHANNEL TARGET",
	},
	{
		.name      = "set-working-dir",
		.help      = "Sets the targets working directory.  This may only be set before the\nchannel has started.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "  WORKING_DIR:\t\tA string.\n"
		             "\n",
		.callback  = pk_shell_channel_set_working_dir,
		.usage     = "channel set-working-dir CHANNEL WORKING_DIR",
	},
	{
		.name      = "start",
		.help      = "Start the channel. If required, the process will be spawned.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_start,
		.usage     = "channel start CHANNEL",
	},
	{
		.name      = "stop",
		.help      = "Stop the channel. If @killpid, the inferior process is terminated.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_stop,
		.usage     = "channel stop CHANNEL",
	},
	{
		.name      = "unmute",
		.help      = "Resumes delivery of manifest and samples for sources within the channel.\n"
		             "\n"
		             "options:\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_channel_unmute,
		.usage     = "channel unmute CHANNEL",
	},
	{ NULL }
};

static EggLineCommand subscription_commands[] = {
	{
		.name      = "add-channel",
		.help      = "Adds all sources of @channel to the list of sources for which manifest\nand samples are delivered to the subscriber.\n\nIf @monitor is TRUE, then sources added to @channel will automatically\nbe added to the subscription.\n"
		             "\n"
		             "options:\n"
		             "  SUBSCRIPTION:\t\tAn integer.\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "  MONITOR:\t\tA boolean [yes, no, true, false, 0, 1].\n"
		             "\n",
		.callback  = pk_shell_subscription_add_channel,
		.usage     = "subscription add-channel SUBSCRIPTION CHANNEL MONITOR",
	},
	{
		.name      = "add-source",
		.help      = "Adds @source to the list of sources for which manifest and samples are\ndelivered to the subscriber.\n"
		             "\n"
		             "options:\n"
		             "  SUBSCRIPTION:\t\tAn integer.\n"
		             "  SOURCE:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_subscription_add_source,
		.usage     = "subscription add-source SUBSCRIPTION SOURCE",
	},
	{
		.name      = "mute",
		.help      = "Prevents the subscription from further manifest or sample delivery.  If\n@drain is set, the current buffer will be flushed.\n"
		             "\n"
		             "options:\n"
		             "  SUBSCRIPTION:\t\tAn integer.\n"
		             "  DRAIN:\t\tA boolean [yes, no, true, false, 0, 1].\n"
		             "\n",
		.callback  = pk_shell_subscription_mute,
		.usage     = "subscription mute SUBSCRIPTION DRAIN",
	},
	{
		.name      = "remove-channel",
		.help      = "Removes @channel and all of its sources from the subscription.  This\nprevents further manifest and sample delivery to the subscriber.\n"
		             "\n"
		             "options:\n"
		             "  SUBSCRIPTION:\t\tAn integer.\n"
		             "  CHANNEL:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_subscription_remove_channel,
		.usage     = "subscription remove-channel SUBSCRIPTION CHANNEL",
	},
	{
		.name      = "remove-source",
		.help      = "Removes @source from the subscription.  This prevents further manifest\nand sample delivery to the subscriber.\n"
		             "\n"
		             "options:\n"
		             "  SUBSCRIPTION:\t\tAn integer.\n"
		             "  SOURCE:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_subscription_remove_source,
		.usage     = "subscription remove-source SUBSCRIPTION SOURCE",
	},
	{
		.name      = "set-buffer",
		.help      = "Sets the buffering timeout and maximum buffer size for the subscription.\nIf @timeout milliseconds pass or @size bytes are consummed buffering,\nthe data will be delivered to the subscriber.\n\nSet @timeout and @size to 0 to disable buffering.\n"
		             "\n"
		             "options:\n"
		             "  SUBSCRIPTION:\t\tAn integer.\n"
		             "  TIMEOUT:\t\tAn integer.\n"
		             "  SIZE:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_subscription_set_buffer,
		.usage     = "subscription set-buffer SUBSCRIPTION TIMEOUT SIZE",
	},
	{
		.name      = "set-encoder",
		.help      = "Sets the encoder to use on the output buffers.  Data will be encoded\nusing this encoder before sending to the client.\n"
		             "\n"
		             "options:\n"
		             "  SUBSCRIPTION:\t\tAn integer.\n"
		             "  ENCODER:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_subscription_set_encoder,
		.usage     = "subscription set-encoder SUBSCRIPTION ENCODER",
	},
	{
		.name      = "unmute",
		.help      = "Enables the subscription for manifest and sample delivery.\n"
		             "\n"
		             "options:\n"
		             "  SUBSCRIPTION:\t\tAn integer.\n"
		             "\n",
		.callback  = pk_shell_subscription_unmute,
		.usage     = "subscription unmute SUBSCRIPTION",
	},
	{ NULL }
};


static EggLineCommand shell_commands[] = {
	{
		.name      = "log",
		.help      = "Set the shell's logging level.\n"
		             "\n"
		             "usage:\n"
		             "  shell log [LEVEL]\n"
		             "\n"
		             "levels:\n"
		             "  error    -- log error messages and higher\n"
		             "  critical -- log critical messages and higher\n"
		             "  warning  -- log warning messages and higher\n"
		             "  message  -- log generic messages and higher\n"
		             "  info     -- log informative messages and higher\n"
		             "  debug    -- log debug messages and higher\n"
		             "  trace    -- log function tracing and higher\n\n",
		.usage     = "shell log [error | critical | warning | message | info | debug | trace]",
		.callback  = pk_shell_shell_log,
		.generator = NULL,
	},
	{ NULL }
};

/**
 * pk_shell_plugin_generator:
 * @line: An #EggLine.
 * @argc: Argument count.
 * @argv: Arguments.
 *
 * Generator for #EggLine to retrieve the sub-commands of the current command
 * request.
 *
 * Returns: A %NULL terminated EggLineCommand array.
 * Side effects: None.
 */
static EggLineCommand*
pk_shell_plugin_generator (EggLine   *line, /* IN */
                           gint      *argc, /* IN/OUT */
                           gchar   ***argv) /* IN/OUT */
{
	ENTRY;
	RETURN(plugin_commands);
}

/**
 * pk_shell_encoder_generator:
 * @line: An #EggLine.
 * @argc: Argument count.
 * @argv: Arguments.
 *
 * Generator for #EggLine to retrieve the sub-commands of the current command
 * request.
 *
 * Returns: A %NULL terminated EggLineCommand array.
 * Side effects: None.
 */
static EggLineCommand*
pk_shell_encoder_generator (EggLine   *line, /* IN */
                            gint      *argc, /* IN/OUT */
                            gchar   ***argv) /* IN/OUT */
{
	ENTRY;
	RETURN(encoder_commands);
}

/**
 * pk_shell_source_generator:
 * @line: An #EggLine.
 * @argc: Argument count.
 * @argv: Arguments.
 *
 * Generator for #EggLine to retrieve the sub-commands of the current command
 * request.
 *
 * Returns: A %NULL terminated EggLineCommand array.
 * Side effects: None.
 */
static EggLineCommand*
pk_shell_source_generator (EggLine   *line, /* IN */
                           gint      *argc, /* IN/OUT */
                           gchar   ***argv) /* IN/OUT */
{
	ENTRY;
	RETURN(source_commands);
}

/**
 * pk_shell_manager_generator:
 * @line: An #EggLine.
 * @argc: Argument count.
 * @argv: Arguments.
 *
 * Generator for #EggLine to retrieve the sub-commands of the current command
 * request.
 *
 * Returns: A %NULL terminated EggLineCommand array.
 * Side effects: None.
 */
static EggLineCommand*
pk_shell_manager_generator (EggLine   *line, /* IN */
                            gint      *argc, /* IN/OUT */
                            gchar   ***argv) /* IN/OUT */
{
	ENTRY;
	RETURN(manager_commands);
}

/**
 * pk_shell_channel_generator:
 * @line: An #EggLine.
 * @argc: Argument count.
 * @argv: Arguments.
 *
 * Generator for #EggLine to retrieve the sub-commands of the current command
 * request.
 *
 * Returns: A %NULL terminated EggLineCommand array.
 * Side effects: None.
 */
static EggLineCommand*
pk_shell_channel_generator (EggLine   *line, /* IN */
                            gint      *argc, /* IN/OUT */
                            gchar   ***argv) /* IN/OUT */
{
	ENTRY;
	RETURN(channel_commands);
}

/**
 * pk_shell_subscription_generator:
 * @line: An #EggLine.
 * @argc: Argument count.
 * @argv: Arguments.
 *
 * Generator for #EggLine to retrieve the sub-commands of the current command
 * request.
 *
 * Returns: A %NULL terminated EggLineCommand array.
 * Side effects: None.
 */
static EggLineCommand*
pk_shell_subscription_generator (EggLine   *line, /* IN */
                                 gint      *argc, /* IN/OUT */
                                 gchar   ***argv) /* IN/OUT */
{
	ENTRY;
	RETURN(subscription_commands);
}


static EggLineCommand*
pk_shell_shell_generator (EggLine   *line, /* IN */
                          gint      *argc, /* IN/OUT */
                          gchar   ***argv) /* IN/OUT */
{
	ENTRY;
	RETURN(shell_commands);
}

static EggLineCommand root_commands[] = {
	{
		.name      = "plugin",
		.help      = "Plugin commands.",
		.callback  = NULL,
		.generator = pk_shell_plugin_generator,
		.usage     = "plugin [get-copyright | get-description | get-name | get-plugin-type | get-version]",
	},
	{
		.name      = "encoder",
		.help      = "Encoder commands.",
		.callback  = NULL,
		.generator = pk_shell_encoder_generator,
		.usage     = "encoder [get-plugin]",
	},
	{
		.name      = "source",
		.help      = "Source commands.",
		.callback  = NULL,
		.generator = pk_shell_source_generator,
		.usage     = "source [get-plugin]",
	},
	{
		.name      = "manager",
		.help      = "Manager commands.",
		.callback  = NULL,
		.generator = pk_shell_manager_generator,
		.usage     = "manager [add-channel | add-source | add-subscription | get-channels | get-plugins | get-sources | get-version | ping | remove-channel | remove-source | remove-subscription]",
	},
	{
		.name      = "channel",
		.help      = "Channel commands.",
		.callback  = NULL,
		.generator = pk_shell_channel_generator,
		.usage     = "channel [add-source | get-args | get-env | get-exit-status | get-kill-pid | get-pid | get-pid-set | get-sources | get-state | get-target | get-working-dir | mute | set-args | set-env | set-kill-pid | set-pid | set-target | set-working-dir | start | stop | unmute]",
	},
	{
		.name      = "subscription",
		.help      = "Subscription commands.",
		.callback  = NULL,
		.generator = pk_shell_subscription_generator,
		.usage     = "subscription [add-channel | add-source | mute | remove-channel | remove-source | set-buffer | set-encoder | unmute]",
	},
	{
		.name      = "shell",
		.help      = "Manage the interactive shell.",
		.callback  = NULL,
		.generator = pk_shell_shell_generator,
		.usage     = "shell [log]",
	},
	{
		.name      = "cd",
		.help      = "Change the working directory.",
		.usage     = "cd [DIRECTORY]",
		.generator = NULL,
		.callback  = pk_shell_cd,
	},
	{
		.name      = "ls",
		.help      = "List contents of the working directory.",
		.usage     = "ls [OPTIONS]",
		.generator = NULL,
		.callback  = pk_shell_ls,
	},
	{
		.name      = "load",
		.help      = "Load an external script.",
		.usage     = "load [FILENAME]",
		.generator = NULL,
		.callback  = pk_shell_load,
	},
	{
		.name      = "echo",
		.help      = "Echo text and variables to the console.",
		.usage     = "echo ...",
		.generator = NULL,
		.callback  = pk_shell_echo,
	},
	{
		.name      = "help",
		.help      = "Show help usage for a command.",
		.usage     = "help [COMMAND..]",
		.generator = NULL,
		.callback = pk_shell_help,
	},
	{ NULL }
};

/**
 * pk_shell_missing_cb:
 * @line: An #EggLine.
 * @command: The command that was called.
 * @user_data: user data supplied to g_signal_connect().
 *
 * Callback upon an unknown command being entered into the shell.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_shell_missing_cb (EggLine     *line,      /* IN */
                     const gchar *command,   /* IN */
                     gpointer     user_data) /* IN */
{
	ENTRY;
	g_printerr(_("Command not found: %s\n"), command);
	EXIT;
}

/**
 * pk_shell_log_level_to_str:
 * @log_level: A #GLogLevelFlags.
 *
 * Converts a log level into its string name.
 *
 * Returns: A const string.
 * Side effects: None.
 */
static const gchar *
pk_shell_log_level_to_str (GLogLevelFlags log_level) /* IN */
{
	#define CASE_LEVEL_STR(_n) case G_LOG_LEVEL_##_n: return #_n
	switch ((long)log_level) {
	CASE_LEVEL_STR(ERROR);
	CASE_LEVEL_STR(CRITICAL);
	CASE_LEVEL_STR(WARNING);
	CASE_LEVEL_STR(MESSAGE);
	CASE_LEVEL_STR(INFO);
	CASE_LEVEL_STR(DEBUG);
	CASE_LEVEL_STR(TRACE);
	default:
		return "UNKNOWN";
	}
	#undef CASE_LEVEL_STR
}

/**
 * pk_shell_get_thread:
 *
 * Retrieves the task id of the caller.  Every thread has a unique task id
 * on linux.  On other systems, this will return the process id.
 *
 * Returns: The task id of the thread.
 * Side effects: None.
 */
static inline gint
pk_shell_get_thread (void)
{
#if __linux__
	return (gint)syscall(SYS_gettid);
#else
	return getpid();
#endif /* __linux__ */
}

/**
 * pk_shell_log_handler:
 * @log_domain: The modules log domain.
 * @log_level: The message log level.
 * @message: A message to log.
 * @user_data: user data supplied to g_log_set_default_handler().
 *
 * Default handler for log messages from the GLib logging subsystem.  If
 * the log message meets or exceeds the current log level, it will be
 * logged to the console.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_shell_log_handler (const gchar    *log_domain, /* IN */
                      GLogLevelFlags  log_level,  /* IN */
                      const gchar    *message,    /* IN */
                      gpointer        user_data)  /* IN */
{
	struct timespec ts;
	struct tm tt;
	time_t t;
	const gchar *level;
	gchar ftime[32];
	gchar *buffer;

	if (log_threshold < log_level) {
		return;
	}
	level = pk_shell_log_level_to_str(log_level);
	clock_gettime(CLOCK_REALTIME, &ts);
	t = (time_t)ts.tv_sec;
	tt = *localtime(&t);
	strftime(ftime, sizeof(ftime), "%x %X", &tt);
	buffer = g_strdup_printf("%s.%04ld  %s: %10s[%d]: %8s: %s\n",
	                         ftime, ts.tv_nsec / 100000,
	                         hostname, log_domain ? log_domain : "Default",
	                         pk_shell_get_thread(), level, message);
	g_printerr("%s", buffer);
	g_free(buffer);
}

/**
 * pk_shell_egg_line_thread:
 * @line: A #EggLine.
 *
 * Worker thread to manage readline input.  Quits the main loop when it
 * finishes.
 *
 * Returns: None.
 * Side effects: Mainloop armageddon.
 */
static gpointer
pk_shell_egg_line_thread (EggLine *line) /* IN */
{
	ENTRY;
	egg_line_run(line);
	g_main_loop_quit(main_loop);
	RETURN(NULL);
}

/**
 * pk_shell_connect_cb:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: user data supplied to pk_connection_connect_async().
 *
 * Asynchronous callback when the connection has successfully connected.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_shell_connect_cb (PkConnection *connection, /* IN */
                     GAsyncResult *result,     /* IN */
                     gpointer      user_data)  /* IN */
{
	GError *error = NULL;

	/*
	 * Check if we connected.
	 */
	if (!pk_connection_connect_finish(connection, result, &error)) {
		g_error("Could not connect to Perfkit: %s",
		        error->message);
		EXIT;
	}

	/*
	 * Spawn the readline thread.
	 */
	if (!g_thread_create((GThreadFunc)pk_shell_egg_line_thread, user_data,
	                     FALSE, &error)) {
		g_error("Could not spawn readline thread: %s",
		        error->message);
		EXIT;
	}
}

gint
main (gint   argc,   /* IN */
      gchar *argv[]) /* IN */
{
	struct utsname u;
	EggLine *line;

	g_thread_init(NULL);
	g_type_init();

	/*
	 * TODO: Initialize i18n.
	 */

	/*
	 * Initialize logging.
	 */
	uname(&u);
	memcpy(hostname, u.nodename, sizeof(hostname));
	g_log_set_default_handler(pk_shell_log_handler, NULL);

	/*
	 * Initialize libraries.
	 */
	main_loop = g_main_loop_new(NULL, FALSE);
	formatter = egg_fmt_table;
	line = egg_line_new();
	egg_line_set_prompt(line, "perfkit> ");
	egg_line_set_commands(line, root_commands);
	g_signal_connect(line,
	                 "missing",
	                 G_CALLBACK(pk_shell_missing_cb),
	                 NULL);

	/*
	 * Create connection.
	 */
	if (!(conn = pk_connection_new_from_uri("dbus://"))) {
		g_printerr("Could not load DBus protocol. EXITING.\n");
		RETURN(EXIT_FAILURE);
	}

	/*
	 * Asynchronously connect to Perfkit.
	 */
	g_message("Connecting to Perfkit ...\n");
	pk_connection_connect_async(conn, NULL,
	                            (GAsyncReadyCallback)pk_shell_connect_cb,
	                            line);

	/*
	 * Run the main loop.
	 */
	g_main_loop_run(main_loop);

	RETURN(EXIT_SUCCESS);
}
