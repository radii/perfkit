/* ppg-task.c
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

#include "ppg-task.h"

G_DEFINE_ABSTRACT_TYPE(PpgTask, ppg_task, G_TYPE_INITIALLY_UNOWNED)

struct _PpgTaskPrivate
{
	gboolean  is_sync;
	gboolean  finished;
	GError   *error;
};

enum
{
	STARTED,
	FINISHED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/**
 * ppg_task_schedule:
 * @task: (in): A #PpgTask.
 *
 * Schedules a task for execution.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_task_schedule (PpgTask *task)
{
	PpgTaskPrivate *priv;
	PpgTaskClass *task_class;

	g_return_if_fail(PPG_IS_TASK(task));

	priv = task->priv;

	/*
	 * FIXME: Support synchronous and asynchronous tasks.
	 */
	if (priv->is_sync) {
		g_critical("%s(): Only asynchronous tasks are supported.", G_STRFUNC);
		return;
	}

	task_class = PPG_TASK_GET_CLASS(task);
	if (task_class->run) {
		task_class->run(task);
	}
}

/**
 * ppg_task_finish_with_error:
 * @task: (in): A #PpgTask.
 *
 * Finishes the task with an error state and notifies listeners.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_task_finish_with_error (PpgTask       *task,
                            const GError  *error)
{
	PpgTaskPrivate *priv;

	g_return_if_fail(PPG_IS_TASK(task));

	priv = task->priv;
	priv->finished = TRUE;
	priv->error = g_error_copy(error);
}

/**
 * ppg_task_finish:
 * @task: (in): A #PpgTask.
 *
 * Finishes the task and signals listeners.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_task_finish (PpgTask *task)
{
	PpgTaskPrivate *priv;

	g_return_if_fail(PPG_IS_TASK(task));

	priv = task->priv;
	priv->finished = TRUE;
}

/**
 * ppg_task_is_error:
 * @task: (in): A #PpgTask.
 *
 * Checks if the task errored.
 *
 * Returns: None.
 * Side effects: None.
 */
gboolean
ppg_task_is_error (PpgTask *task)
{
	g_return_val_if_fail(PPG_IS_TASK(task), FALSE);
	return !!task->priv->error;
}

/**
 * ppg_task_get_error:
 * @task: (in): A #PpgTask.
 *
 * Retrieves the error for the task.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_task_get_error (PpgTask  *task,
                    GError  **error)
{
	PpgTaskPrivate *priv;

	g_return_if_fail(PPG_IS_TASK(task));

	priv = task->priv;
	if (error) {
		*error = g_error_copy(priv->error);
	}
}

/**
 * ppg_task_finalize:
 * @object: (in): A #PpgTask.
 *
 * Finalizer for a #PpgTask instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_task_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_task_parent_class)->finalize(object);
}

/**
 * ppg_task_class_init:
 * @klass: (in): A #PpgTaskClass.
 *
 * Initializes the #PpgTaskClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_task_class_init (PpgTaskClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_task_finalize;
	g_type_class_add_private(object_class, sizeof(PpgTaskPrivate));

	signals[STARTED] = g_signal_new("started",
	                                PPG_TYPE_TASK,
	                                G_SIGNAL_RUN_FIRST,
	                                0,
	                                NULL,
	                                NULL,
	                                g_cclosure_marshal_VOID__VOID,
	                                G_TYPE_NONE,
	                                0);

	signals[FINISHED] = g_signal_new("finished",
	                                 PPG_TYPE_TASK,
	                                 G_SIGNAL_RUN_FIRST,
	                                 0,
	                                 NULL,
	                                 NULL,
	                                 g_cclosure_marshal_VOID__VOID,
	                                 G_TYPE_NONE,
	                                 0);
}

/**
 * ppg_task_init:
 * @task: (in): A #PpgTask.
 *
 * Initializes the newly created #PpgTask instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_task_init (PpgTask *task)
{
	task->priv = G_TYPE_INSTANCE_GET_PRIVATE(task, PPG_TYPE_TASK,
	                                         PpgTaskPrivate);
}
