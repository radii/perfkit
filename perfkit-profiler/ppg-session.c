/* ppg-session.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <perfkit/perfkit.h>

#include "ppg-log.h"
#include "ppg-session.h"

G_DEFINE_TYPE(PpgSession, ppg_session, G_TYPE_OBJECT)

struct _PpgSessionPrivate
{
	PkConnection *conn;
	guint         state;
};

enum
{
	STATE_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

gboolean
ppg_session_start (PpgSession  *session,
                   GError     **error)
{
	INFO("Starting session %p", session);
	session->priv->state = PPG_SESSION_STARTED;
	g_signal_emit(session, signals[STATE_CHANGED], 0, session->priv->state);
	return TRUE;
}

gboolean
ppg_session_stop (PpgSession  *session,
                  GError     **error)
{
	INFO("Stopping session %p", session);
	session->priv->state = PPG_SESSION_STOPPED;
	g_signal_emit(session, signals[STATE_CHANGED], 0, session->priv->state);
	return TRUE;
}

gboolean
ppg_session_pause (PpgSession  *session,
                   GError     **error)
{
	INFO("Pause session %p", session);
	session->priv->state = PPG_SESSION_PAUSED;
	g_signal_emit(session, signals[STATE_CHANGED], 0, session->priv->state);
	return TRUE;
}

gboolean
ppg_session_unpause (PpgSession  *session,
                     GError     **error)
{
	INFO("Unpause session %p", session);
	session->priv->state = PPG_SESSION_STARTED;
	g_signal_emit(session, signals[STATE_CHANGED], 0, session->priv->state);
	return TRUE;
}

/**
 * ppg_session_finalize:
 * @object: (in): A #PpgSession.
 *
 * Finalizer for a #PpgSession instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_session_parent_class)->finalize(object);
}

/**
 * ppg_session_class_init:
 * @klass: (in): A #PpgSessionClass.
 *
 * Initializes the #PpgSessionClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_class_init (PpgSessionClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_session_finalize;
	g_type_class_add_private(object_class, sizeof(PpgSessionPrivate));

	signals[STATE_CHANGED] = g_signal_new("state-changed",
	                                      PPG_TYPE_SESSION,
	                                      G_SIGNAL_RUN_FIRST,
	                                      0,
	                                      NULL,
	                                      NULL,
	                                      g_cclosure_marshal_VOID__UINT,
	                                      G_TYPE_NONE,
	                                      1,
	                                      G_TYPE_UINT);
}

/**
 * ppg_session_init:
 * @session: (in): A #PpgSession.
 *
 * Initializes the newly created #PpgSession instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_init (PpgSession *session)
{
	PpgSessionPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(session, PPG_TYPE_SESSION,
	                                   PpgSessionPrivate);
	session->priv = priv;
}
