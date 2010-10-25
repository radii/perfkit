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

#include "ppg-instrument.h"
#include "ppg-session.h"

G_DEFINE_TYPE(PpgSession, ppg_session, G_TYPE_INITIALLY_UNOWNED)

struct _PpgSessionPrivate
{
	PkConnection  *conn;
	gint           channel;
	gchar         *target;
	gchar        **args;
	gchar        **env;
	GTimer        *timer;
	guint          position_handler;
};

enum
{
	PROP_0,
	PROP_ARGS,
	PROP_CHANNEL,
	PROP_CONNECTION,
	PROP_ENV,
	PROP_POSITION,
	PROP_TARGET,
	PROP_URI,
};

enum
{
	READY,
	DISCONNECTED,
	PAUSED,
	STARTED,
	STOPPED,
	UNPAUSED,
	INSTRUMENT_ADDED,
	INSTRUMENT_REMOVED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *position_pspec = NULL;

static gboolean
ppg_session_notify_position (gpointer user_data)
{
	g_object_notify_by_pspec(user_data, position_pspec);
	return TRUE;
}

static void
ppg_session_start_position_notifier (PpgSession *session)
{
	PpgSessionPrivate *priv = session->priv;

	if (priv->position_handler) {
		g_source_remove(priv->position_handler);
	}

	priv->position_handler = g_timeout_add((1000 / 13),
	                                       ppg_session_notify_position,
	                                       session);
}

static void
ppg_session_stop_position_notifier (PpgSession *session)
{
	PpgSessionPrivate *priv = session->priv;

	if (priv->position_handler) {
		g_source_remove(priv->position_handler);
		priv->position_handler = 0;
	}
}

static void
ppg_session_report_error (PpgSession  *session,
                          const gchar *func,
                          GError      *error)
{
	g_critical("%s(): %s", func, error->message);
}

static void
ppg_session_set_target (PpgSession  *session,
                        const gchar *target)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	g_free(priv->target);
	priv->target = g_strdup(target);
	g_object_notify(G_OBJECT(session), "target");
}

static void
ppg_session_set_args (PpgSession  *session,
                      gchar      **args)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	g_strfreev(priv->args);
	priv->args = g_strdupv(args);
	g_object_notify(G_OBJECT(session), "args");
}

static void
ppg_session_set_env (PpgSession  *session,
                      gchar      **env)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	g_strfreev(priv->env);
	priv->env = g_strdupv(env);
	g_object_notify(G_OBJECT(session), "env");
}

static const gchar *
ppg_session_get_target (PpgSession *session)
{
	g_return_val_if_fail(PPG_IS_SESSION(session), NULL);
	return session->priv->target;
}

static inline gdouble
ppg_session_get_position_fast (PpgSession *session)
{
	return g_timer_elapsed(session->priv->timer, NULL);
}

gdouble
ppg_session_get_position (PpgSession *session)
{
	PpgSessionPrivate *priv;

	g_return_val_if_fail(PPG_IS_SESSION(session), 0.0);

	priv = session->priv;

	if (priv->timer) {
		return ppg_session_get_position_fast(session);
	}

	return 0.0;
}

/**
 * ppg_session_channel_added:
 * @connection: (in): A #PkConnection.
 * @result: (in): A #GAsyncResult.
 * @user_data: (closure): User data for callback.
 *
 * Callback upon the channel being created.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_channel_added (GObject      *connection,
                           GAsyncResult *result,
                           gpointer      user_data)
{
	PpgSession *session = user_data;
	PpgSessionPrivate *priv;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	if (!pk_connection_manager_add_channel_finish(priv->conn, result,
	                                              &priv->channel, &error)) {
		ppg_session_report_error(session, G_STRFUNC, error);
		g_error_free(error);
		return;
	}

	g_signal_emit(session, signals[READY], 0);
}

/**
 * ppg_session_connection_connected:
 * @connection: (in): A #PkConnection.
 * @result: (in): A #GAsyncResult.
 * @user_data: (closure): User data for callback.
 *
 * Callback upon the connection connecting to the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_connection_connected (GObject      *connection,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
	PpgSession *session = user_data;
	PpgSessionPrivate *priv;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	if (!pk_connection_connect_finish(priv->conn, result, &error)) {
		ppg_session_report_error(session, G_STRFUNC, error);
		g_error_free(error);
		return;
	}

	pk_connection_manager_add_channel_async(priv->conn, NULL,
	                                        ppg_session_channel_added,
	                                        session);
}

/**
 * ppg_session_connection_connected:
 * @connection: (in): A #PkConnection.
 * @result: (in): A #GAsyncResult.
 * @user_data: (closure): User data for callback.
 *
 * Callback upon the connection connecting to the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_channel_stopped (GObject *object,
                             GAsyncResult *result,
                             gpointer user_data)
{
	PpgSession *session = user_data;
	PpgSessionPrivate *priv;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	if (!pk_connection_channel_stop_finish(priv->conn, result, &error)) {
		ppg_session_report_error(session, G_STRFUNC, error);
		g_error_free(error);
		return;

		/*
		 * FIXME: We need to make sure we handle this gracefully and stop
		 *        updating the UI. This goes for the rest of the states.
		 */
	}

	g_timer_stop(priv->timer);

	ppg_session_stop_position_notifier(session);

	g_signal_emit(session, signals[STOPPED], 0);
}


/**
 * ppg_session_channel_started:
 * @connection: (in): A #PkConnection.
 * @result: (in): A #GAsyncResult.
 * @user_data: (closure): User data for callback.
 *
 * Callback upon the channel starting.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_channel_started (GObject *object,
                             GAsyncResult *result,
                             gpointer user_data)
{
	PpgSession *session = user_data;
	PpgSessionPrivate *priv;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	if (!pk_connection_channel_start_finish(priv->conn, result, &error)) {
		ppg_session_report_error(session, G_STRFUNC, error);
		g_error_free(error);
		return;
	}

	priv->timer = g_timer_new();

	ppg_session_start_position_notifier(session);

	g_message("Channel %d started.", priv->channel);

	g_signal_emit(session, signals[STARTED], 0);
}

/**
 * ppg_session_channel_muted:
 * @connection: (in): A #PkConnection.
 * @result: (in): A #GAsyncResult.
 * @user_data: (closure): User data for callback.
 *
 * Callback upon the channel muting.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_channel_muted (GObject *object,
                           GAsyncResult *result,
                           gpointer user_data)
{
	PpgSession *session = user_data;
	PpgSessionPrivate *priv;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	if (!pk_connection_channel_mute_finish(priv->conn, result, &error)) {
		ppg_session_report_error(session, G_STRFUNC, error);
		g_error_free(error);
		return;
	}

	/*
	 * XXX: How do we deal with timer skew?
	 */
	g_timer_stop(priv->timer);

	ppg_session_stop_position_notifier(session);

	g_signal_emit(session, signals[PAUSED], 0);
}

/**
 * ppg_session_channel_unmuted:
 * @connection: (in): A #PkConnection.
 * @result: (in): A #GAsyncResult.
 * @user_data: (closure): User data for callback.
 *
 * Callback upon the channel unmuting.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_channel_unmuted (GObject *object,
                             GAsyncResult *result,
                             gpointer user_data)
{
	PpgSession *session = user_data;
	PpgSessionPrivate *priv;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	if (!pk_connection_channel_unmute_finish(priv->conn, result, &error)) {
		ppg_session_report_error(session, G_STRFUNC, error);
		g_error_free(error);
		return;
	}

	/*
	 * XXX: What do we do here for timer? Add adjust variable?
	 */
	g_timer_start(priv->timer);

	ppg_session_start_position_notifier(session);

	g_signal_emit(session, signals[UNPAUSED], 0);
}

/**
 * ppg_session_stop:
 * @session: (in): A #PpgSession.
 *
 * Stops the channel on the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_session_stop (PpgSession *session)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));
	g_return_if_fail(session->priv->conn != NULL);
	g_return_if_fail(session->priv->channel >= 0);

	priv = session->priv;

	pk_connection_channel_stop_async(priv->conn, priv->channel, NULL,
	                                 ppg_session_channel_stopped,
	                                 session);
}

/**
 * ppg_session_started:
 * @session: (in): A #PpgSession.
 *
 * Starts the session by starting the channel on the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_session_start (PpgSession *session)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));
	g_return_if_fail(session->priv->conn != NULL);
	g_return_if_fail(session->priv->channel >= 0);

	priv = session->priv;

	g_message("Starting channel %d", priv->channel);
	pk_connection_channel_start_async(priv->conn, priv->channel, NULL,
	                                  ppg_session_channel_started,
	                                  session);
}

/**
 * ppg_session_pause:
 * @session: (in): A #PpgSession.
 *
 * Pauses the session by muting the remote channel.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_session_pause (PpgSession *session)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));
	g_return_if_fail(session->priv->conn != NULL);
	g_return_if_fail(session->priv->channel >= 0);

	priv = session->priv;

	pk_connection_channel_mute_async(priv->conn, priv->channel, NULL,
	                                 ppg_session_channel_muted,
	                                 session);
}

/**
 * ppg_session_unpause:
 * @session: (in): A #PpgSession.
 *
 * Unpauses the session.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_session_unpause (PpgSession *session)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));
	g_return_if_fail(session->priv->conn != NULL);
	g_return_if_fail(session->priv->channel >= 0);

	priv = session->priv;

	pk_connection_channel_unmute_async(priv->conn, priv->channel, NULL,
	                                   ppg_session_channel_unmuted,
	                                   session);
}

void
ppg_session_add_instrument (PpgSession    *session,
                            PpgInstrument *instrument)
{
	g_signal_emit(session, signals[INSTRUMENT_ADDED], 0, instrument);
}

/**
 * ppg_session_set_uri:
 * @session: (in): A #PpgSession.
 *
 * Sets the URI for the session. The session will attempt to connect
 * to the target immediately.
 *
 * Returns: None.
 * Side effects: The connection is set and an attempt to connect is made.
 */
static void
ppg_session_set_uri (PpgSession  *session,
                     const gchar *uri)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));
	g_return_if_fail(session->priv->conn == NULL);
	g_return_if_fail(uri != NULL);

	priv = session->priv;

	if (!(priv->conn = pk_connection_new_from_uri(uri))) {
		g_critical("Invalid URI specified: %s", uri);
		return;
	}

	pk_connection_connect_async(priv->conn, NULL,
	                            ppg_session_connection_connected,
	                            session);
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
	PpgSessionPrivate *priv = PPG_SESSION(object)->priv;

	if (priv->timer) {
		g_timer_destroy(priv->timer);
		priv->timer = NULL;
	}

	G_OBJECT_CLASS(ppg_session_parent_class)->finalize(object);
}

/**
 * ppg_session_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_session_get_property (GObject    *object,  /* IN */
                          guint       prop_id, /* IN */
                          GValue     *value,   /* OUT */
                          GParamSpec *pspec)   /* IN */
{
	PpgSession *session = PPG_SESSION(object);

	switch (prop_id) {
	case PROP_ARGS:
		g_value_set_boxed(value, session->priv->args);
		break;
	case PROP_CONNECTION:
		g_value_set_object(value, session->priv->conn);
		break;
	case PROP_CHANNEL:
		g_value_set_int(value, session->priv->channel);
		break;
	case PROP_ENV:
		g_value_set_boxed(value, session->priv->env);
		break;
	case PROP_TARGET:
		g_value_set_string(value, ppg_session_get_target(session));
		break;
	case PROP_POSITION:
		g_value_set_double(value, ppg_session_get_position(session));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_session_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_session_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
	PpgSession *session = PPG_SESSION(object);

	switch (prop_id) {
	case PROP_URI:
		ppg_session_set_uri(session, g_value_get_string(value));
		break;
	case PROP_TARGET:
		ppg_session_set_target(session, g_value_get_string(value));
		break;
	case PROP_ARGS:
		ppg_session_set_args(session, g_value_get_boxed(value));
		break;
	case PROP_ENV:
		ppg_session_set_env(session, g_value_get_boxed(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
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
	object_class->get_property = ppg_session_get_property;
	object_class->set_property = ppg_session_set_property;
	g_type_class_add_private(object_class, sizeof(PpgSessionPrivate));

	g_object_class_install_property(object_class,
	                                PROP_URI,
	                                g_param_spec_string("uri",
	                                                    "uri",
	                                                    "uri",
	                                                    NULL,
	                                                    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_CHANNEL,
	                                g_param_spec_int("channel",
	                                                 "channel",
	                                                 "channel",
	                                                 -1,
	                                                 G_MAXINT,
	                                                 -1,
	                                                 G_PARAM_READABLE));

	g_object_class_install_property(object_class,
	                                PROP_CONNECTION,
	                                g_param_spec_object("connection",
	                                                    "connection",
	                                                    "connection",
	                                                    PK_TYPE_CONNECTION,
	                                                    G_PARAM_READABLE));

	g_object_class_install_property(object_class,
	                                PROP_TARGET,
	                                g_param_spec_string("target",
	                                                    "target",
	                                                    "target",
	                                                    NULL,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_ARGS,
	                                g_param_spec_boxed("args",
	                                                   "args",
	                                                   "args",
	                                                   G_TYPE_STRV,
	                                                   G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_ENV,
	                                g_param_spec_boxed("env",
	                                                   "env",
	                                                   "env",
	                                                   G_TYPE_STRV,
	                                                   G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_POSITION,
	                                position_pspec = g_param_spec_double("position",
	                                                                     "position",
	                                                                     "position",
	                                                                     0.0,
	                                                                     G_MAXDOUBLE,
	                                                                     0.0,
	                                                                     G_PARAM_READABLE));

	signals[READY] = g_signal_new("ready",
	                              PPG_TYPE_SESSION,
	                              G_SIGNAL_RUN_FIRST,
	                              0,
	                              NULL,
	                              NULL,
	                              g_cclosure_marshal_VOID__VOID,
	                              G_TYPE_NONE,
	                              0);

	signals[DISCONNECTED] = g_signal_new("disconnected",
	                                     PPG_TYPE_SESSION,
	                                     G_SIGNAL_RUN_FIRST,
	                                     0,
	                                     NULL,
	                                     NULL,
	                                     g_cclosure_marshal_VOID__VOID,
	                                     G_TYPE_NONE,
	                                     0);

	signals[PAUSED] = g_signal_new("paused",
	                               PPG_TYPE_SESSION,
	                               G_SIGNAL_RUN_FIRST,
	                               0,
	                               NULL,
	                               NULL,
	                               g_cclosure_marshal_VOID__VOID,
	                               G_TYPE_NONE,
	                               0);

	signals[STARTED] = g_signal_new("started",
	                                PPG_TYPE_SESSION,
	                                G_SIGNAL_RUN_FIRST,
	                                0,
	                                NULL,
	                                NULL,
	                                g_cclosure_marshal_VOID__VOID,
	                                G_TYPE_NONE,
	                                0);

	signals[STOPPED] = g_signal_new("stopped",
	                                PPG_TYPE_SESSION,
	                                G_SIGNAL_RUN_FIRST,
	                                0,
	                                NULL,
	                                NULL,
	                                g_cclosure_marshal_VOID__VOID,
	                                G_TYPE_NONE,
	                                0);

	signals[UNPAUSED] = g_signal_new("unpaused",
	                                 PPG_TYPE_SESSION,
	                                 G_SIGNAL_RUN_FIRST,
	                                 0,
	                                 NULL,
	                                 NULL,
	                                 g_cclosure_marshal_VOID__VOID,
	                                 G_TYPE_NONE,
	                                 0);

	signals[INSTRUMENT_ADDED] = g_signal_new("instrument-added",
	                                         PPG_TYPE_SESSION,
	                                         G_SIGNAL_RUN_FIRST,
	                                         0,
	                                         NULL,
	                                         NULL,
	                                         g_cclosure_marshal_VOID__OBJECT,
	                                         G_TYPE_NONE,
	                                         1,
	                                         PPG_TYPE_INSTRUMENT);

	signals[INSTRUMENT_REMOVED] = g_signal_new("instrument-removed",
	                                           PPG_TYPE_SESSION,
	                                           G_SIGNAL_RUN_FIRST,
	                                           0,
	                                           NULL,
	                                           NULL,
	                                           g_cclosure_marshal_VOID__OBJECT,
	                                           G_TYPE_NONE,
	                                           1,
	                                           PPG_TYPE_INSTRUMENT);
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
	session->priv = G_TYPE_INSTANCE_GET_PRIVATE(session,
	                                     PPG_TYPE_SESSION,
	                                     PpgSessionPrivate);

	
}
