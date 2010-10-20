/* ppg-session-task.c
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

#include "ppg-session.h"
#include "ppg-session-task.h"

G_DEFINE_TYPE(PpgSessionTask, ppg_session_task, PPG_TYPE_TASK)

struct _PpgSessionTaskPrivate
{
	PpgSession *session;
};

enum
{
	PROP_0,
	PROP_SESSION,
};

PpgSession*
ppg_session_task_get_session (PpgSessionTask *task)
{
	g_return_val_if_fail(PPG_IS_SESSION_TASK(task), NULL);
	return task->priv->session;
}

void
ppg_session_task_set_session (PpgSessionTask *task,
                              PpgSession     *session)
{
	PpgSessionTaskPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION_TASK(task));
	g_return_if_fail(!session || PPG_IS_SESSION(session));

	priv = task->priv;

	if (priv->session) {
		g_object_unref(priv->session);
		priv->session = NULL;
	}

	if (session) {
		priv->session = g_object_ref(session);
	}
}

/**
 * ppg_session_task_dispose:
 * @object: (in): A #GObject.
 *
 * Dispose callback for @object.  This method releases references held
 * by the #GObject instance.
 *
 * Returns: None.
 * Side effects: Plenty.
 */
static void
ppg_session_task_dispose (GObject *object)
{
	PpgSessionTask *task = PPG_SESSION_TASK(object);

	ppg_session_task_set_session(task, NULL);

	G_OBJECT_CLASS(ppg_session_task_parent_class)->dispose(object);
}

/**
 * ppg_session_task_finalize:
 * @object: (in): A #PpgSessionTask.
 *
 * Finalizer for a #PpgSessionTask instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_task_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_session_task_parent_class)->finalize(object);
}

/**
 * ppg_session_task_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_session_task_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
	PpgSessionTask *task = PPG_SESSION_TASK(object);

	switch (prop_id) {
	case PROP_SESSION:
		g_value_set_object(value, ppg_session_task_get_session(task));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_session_task_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_session_task_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
	PpgSessionTask *task = PPG_SESSION_TASK(object);

	switch (prop_id) {
	case PROP_SESSION:
		ppg_session_task_set_session(task, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_session_task_class_init:
 * @klass: (in): A #PpgSessionTaskClass.
 *
 * Initializes the #PpgSessionTaskClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_task_class_init (PpgSessionTaskClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->dispose = ppg_session_task_dispose;
	object_class->finalize = ppg_session_task_finalize;
	object_class->get_property = ppg_session_task_get_property;
	object_class->set_property = ppg_session_task_set_property;
	g_type_class_add_private(object_class, sizeof(PpgSessionTaskPrivate));

	g_object_class_install_property(object_class,
	                                PROP_SESSION,
	                                g_param_spec_object("session",
	                                                    "session",
	                                                    "session",
	                                                    PPG_TYPE_SESSION,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

/**
 * ppg_session_task_init:
 * @task: (in): A #PpgSessionTask.
 *
 * Initializes the newly created #PpgSessionTask instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_task_init (PpgSessionTask *task)
{
	task->priv = G_TYPE_INSTANCE_GET_PRIVATE(task, PPG_TYPE_SESSION_TASK,
	                                         PpgSessionTaskPrivate);
}
