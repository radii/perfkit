/* ppg-restart-task.c
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

#include <perfkit/perfkit.h>

#include "ppg-restart-task.h"
#include "ppg-session.h"

G_DEFINE_TYPE(PpgRestartTask, ppg_restart_task, PPG_TYPE_TASK)

struct _PpgRestartTaskPrivate
{
	PpgSession *session;
};

enum
{
	PROP_0,
	PROP_SESSION,
};

static void
ppg_restart_task_channel_started (GObject      *object,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
	PkConnection *conn = (PkConnection *)object;
	PpgTask *task = (PpgTask *)user_data;
	GError *error = NULL;

	g_return_if_fail(PK_IS_CONNECTION(conn));
	g_return_if_fail(PPG_IS_TASK(task));

	if (!pk_connection_channel_start_finish(conn, result, &error)) {
		ppg_task_finish_with_error(task, error);
		g_error_free(error);
		return;
	}

	ppg_task_finish(task);
}

static void
ppg_restart_task_channel_stopped (GObject      *object,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
	PpgRestartTaskPrivate *priv;
	PkConnection *conn = (PkConnection *)object;
	PpgRestartTask *restart = (PpgRestartTask *)user_data;
	PpgTask *task = (PpgTask *)user_data;
	GError *error = NULL;
	gint channel;

	g_return_if_fail(PK_IS_CONNECTION(conn));
	g_return_if_fail(PPG_IS_RESTART_TASK(restart));
	g_return_if_fail(PPG_IS_TASK(task));

	priv = restart->priv;

	if (!pk_connection_channel_stop_finish(conn, result, &error)) {
		ppg_task_finish_with_error(task, error);
		g_error_free(error);
		return;
	}

	g_object_get(priv->session,
	             "channel", &channel,
	             NULL);

	pk_connection_channel_start_async(conn, channel, NULL,
	                                  ppg_restart_task_channel_started,
	                                  task);
}

static void
ppg_restart_task_run (PpgTask *task)
{
	PpgRestartTaskPrivate *priv;
	PpgRestartTask *restart = (PpgRestartTask *)task;
	PkConnection *conn;
	gint channel;

	g_return_if_fail(PPG_IS_RESTART_TASK(restart));

	priv = restart->priv;

	if (!priv->session) {
		g_critical("%s(): Task scheduled before session was set.", G_STRFUNC);
		return;
	}

	g_object_get(priv->session,
	             "connection", &conn,
	             "channel", &channel,
	             NULL);

	pk_connection_channel_stop_async(conn, channel, NULL,
	                                 ppg_restart_task_channel_stopped,
	                                 task);

	g_object_unref(conn);
}

/**
 * ppg_restart_task_finalize:
 * @object: (in): A #PpgRestartTask.
 *
 * Finalizer for a #PpgRestartTask instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_restart_task_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_restart_task_parent_class)->finalize(object);
}

/**
 * ppg_restart_task_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_restart_task_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
	PpgRestartTask *task = PPG_RESTART_TASK(object);

	switch (prop_id) {
	case PROP_SESSION:
		task->priv->session = g_value_get_object(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}
/**
 * ppg_restart_task_class_init:
 * @klass: (in): A #PpgRestartTaskClass.
 *
 * Initializes the #PpgRestartTaskClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_restart_task_class_init (PpgRestartTaskClass *klass)
{
	GObjectClass *object_class;
	PpgTaskClass *task_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_restart_task_finalize;
	object_class->set_property = ppg_restart_task_set_property;
	g_type_class_add_private(object_class, sizeof(PpgRestartTaskPrivate));

	task_class = PPG_TASK_CLASS(klass);
	task_class->run = ppg_restart_task_run;

	g_object_class_install_property(object_class,
	                                PROP_SESSION,
	                                g_param_spec_object("session",
	                                                    "session",
	                                                    "session",
	                                                    PPG_TYPE_SESSION,
	                                                    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

/**
 * ppg_restart_task_init:
 * @task: (in): A #PpgRestartTask.
 *
 * Initializes the newly created #PpgRestartTask instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_restart_task_init (PpgRestartTask *task)
{
	task->priv = G_TYPE_INSTANCE_GET_PRIVATE(task, PPG_TYPE_RESTART_TASK,
	                                         PpgRestartTaskPrivate);
}
