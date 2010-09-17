/* pk-channel.c
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

#include "pk-channel.h"
#include "pk-connection-lowlevel.h"
#include "pk-log.h"

/**
 * SECTION:pk-channel
 * @title: PkChannel
 * @short_description: 
 *
 * Section overview.
 */

G_DEFINE_TYPE(PkChannel, pk_channel, G_TYPE_OBJECT)

struct _PkChannelPrivate
{
	PkConnection *connection;
	gint id;
};

enum
{
	PROP_0,
	PROP_ID,
	PROP_CONNECTION,
};

/**
 * pk_channel_get_id:
 * @channel: A #PkChannel.
 *
 * Retrieves the identifier for the channel.
 *
 * Returns: A gint.
 * Side effects: None.
 */
gint
pk_channel_get_id (PkChannel *channel) /* IN */
{
	g_return_val_if_fail(PK_IS_CHANNEL(channel), -1);
	return channel->priv->id;
}

/**
 * pk_channel_get_connection:
 * @channel: A #PkChannel.
 *
 * Retrieves the connection for the channel.
 *
 * Returns: A PkConnection.
 * Side effects: None.
 */
PkConnection*
pk_channel_get_connection (PkChannel *channel) /* IN */
{
	g_return_val_if_fail(PK_IS_CHANNEL(channel), NULL);
	return channel->priv->connection;
}

/**
 * pk_channel_add_source:
 * @channel: A #PkChannel.
 * @source: A gint.
 * @error: A location for a #GError, or %NULL.
 *
 * Adds an existing source to the channel.  If the channel has already been
 * started, the source will be started immediately.  The source must not have
 * been previous added to another channel or this will fail.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_add_source (PkChannel  *channel, /* IN */
                       PkSource   *source,  /* IN */
                       GError    **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_add_source(
			priv->connection,
			priv->id,
			pk_source_get_id(source),
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_add_source_cb (GObject      *object,    /* IN */
                          GAsyncResult *result,    /* IN */
                          gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_add_source_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Adds an existing source to the channel.  If the channel has already been
 * started, the source will be started immediately.  The source must not have
 * been previous added to another channel or this will fail.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_add_source_async (PkChannel           *channel,     /* IN */
                             PkSource            *source,      /* IN */
                             GCancellable        *cancellable, /* IN */
                             GAsyncReadyCallback  callback,    /* IN */
                             gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_add_source_async);
	pk_connection_channel_add_source_async(
			priv->connection,
			priv->id,
			pk_source_get_id(source),
			cancellable,
			pk_channel_add_source_cb,
			result);
	EXIT;
}


/**
 * pk_channel_add_source_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Adds an existing source to the channel.  If the channel has already been
 * started, the source will be started immediately.  The source must not have
 * been previous added to another channel or this will fail.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_add_source_finish (PkChannel     *channel, /* IN */
                              GAsyncResult  *result,  /* IN */
                              GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_add_source_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_get_args:
 * @channel: A #PkChannel.
 * @args: A location for the args.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the arguments for target.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_args (PkChannel   *channel, /* IN */
                     gchar     ***args,    /* OUT */
                     GError     **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_get_args(
			priv->connection,
			priv->id,
			args,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_get_args_cb (GObject      *object,    /* IN */
                        GAsyncResult *result,    /* IN */
                        gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_get_args_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Retrieves the arguments for target.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_args_async (PkChannel           *channel,     /* IN */
                           GCancellable        *cancellable, /* IN */
                           GAsyncReadyCallback  callback,    /* IN */
                           gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_get_args_async);
	pk_connection_channel_get_args_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_channel_get_args_cb,
			result);
	EXIT;
}


/**
 * pk_channel_get_args_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the arguments for target.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_args_finish (PkChannel      *channel, /* IN */
                            GAsyncResult   *result,  /* IN */
                            gchar        ***args,    /* OUT */
                            GError        **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_get_args_finish(
			priv->connection,
			real_result,
			args,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_get_env:
 * @channel: A #PkChannel.
 * @env: A location for the env.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the environment for spawning the target process.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_env (PkChannel   *channel, /* IN */
                    gchar     ***env,     /* OUT */
                    GError     **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_get_env(
			priv->connection,
			priv->id,
			env,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_get_env_cb (GObject      *object,    /* IN */
                       GAsyncResult *result,    /* IN */
                       gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_get_env_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Retrieves the environment for spawning the target process.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_env_async (PkChannel           *channel,     /* IN */
                          GCancellable        *cancellable, /* IN */
                          GAsyncReadyCallback  callback,    /* IN */
                          gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_get_env_async);
	pk_connection_channel_get_env_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_channel_get_env_cb,
			result);
	EXIT;
}


/**
 * pk_channel_get_env_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the environment for spawning the target process.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_env_finish (PkChannel      *channel, /* IN */
                           GAsyncResult   *result,  /* IN */
                           gchar        ***env,     /* OUT */
                           GError        **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_get_env_finish(
			priv->connection,
			real_result,
			env,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_get_exit_status:
 * @channel: A #PkChannel.
 * @exit_status: A location for the exit_status.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the exit status of the process.  This is only set after the
 * process has exited.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_exit_status (PkChannel  *channel,     /* IN */
                            gint       *exit_status, /* OUT */
                            GError    **error)       /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_get_exit_status(
			priv->connection,
			priv->id,
			exit_status,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_get_exit_status_cb (GObject      *object,    /* IN */
                               GAsyncResult *result,    /* IN */
                               gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_get_exit_status_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Retrieves the exit status of the process.  This is only set after the
 * process has exited.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_exit_status_async (PkChannel           *channel,     /* IN */
                                  GCancellable        *cancellable, /* IN */
                                  GAsyncReadyCallback  callback,    /* IN */
                                  gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_get_exit_status_async);
	pk_connection_channel_get_exit_status_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_channel_get_exit_status_cb,
			result);
	EXIT;
}


/**
 * pk_channel_get_exit_status_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the exit status of the process.  This is only set after the
 * process has exited.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_exit_status_finish (PkChannel     *channel,     /* IN */
                                   GAsyncResult  *result,      /* IN */
                                   gint          *exit_status, /* OUT */
                                   GError       **error)       /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_get_exit_status_finish(
			priv->connection,
			real_result,
			exit_status,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_get_kill_pid:
 * @channel: A #PkChannel.
 * @kill_pid: A location for the kill_pid.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves if the process should be killed when the channel is stopped.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_kill_pid (PkChannel  *channel,  /* IN */
                         gboolean   *kill_pid, /* OUT */
                         GError    **error)    /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_get_kill_pid(
			priv->connection,
			priv->id,
			kill_pid,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_get_kill_pid_cb (GObject      *object,    /* IN */
                            GAsyncResult *result,    /* IN */
                            gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_get_kill_pid_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Retrieves if the process should be killed when the channel is stopped.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_kill_pid_async (PkChannel           *channel,     /* IN */
                               GCancellable        *cancellable, /* IN */
                               GAsyncReadyCallback  callback,    /* IN */
                               gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_get_kill_pid_async);
	pk_connection_channel_get_kill_pid_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_channel_get_kill_pid_cb,
			result);
	EXIT;
}


/**
 * pk_channel_get_kill_pid_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves if the process should be killed when the channel is stopped.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_kill_pid_finish (PkChannel     *channel,  /* IN */
                                GAsyncResult  *result,   /* IN */
                                gboolean      *kill_pid, /* OUT */
                                GError       **error)    /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_get_kill_pid_finish(
			priv->connection,
			real_result,
			kill_pid,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_get_pid:
 * @channel: A #PkChannel.
 * @pid: A location for the pid.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the process pid of the target process.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_pid (PkChannel  *channel, /* IN */
                    gint       *pid,     /* OUT */
                    GError    **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_get_pid(
			priv->connection,
			priv->id,
			pid,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_get_pid_cb (GObject      *object,    /* IN */
                       GAsyncResult *result,    /* IN */
                       gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_get_pid_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Retrieves the process pid of the target process.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_pid_async (PkChannel           *channel,     /* IN */
                          GCancellable        *cancellable, /* IN */
                          GAsyncReadyCallback  callback,    /* IN */
                          gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_get_pid_async);
	pk_connection_channel_get_pid_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_channel_get_pid_cb,
			result);
	EXIT;
}


/**
 * pk_channel_get_pid_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the process pid of the target process.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_pid_finish (PkChannel     *channel, /* IN */
                           GAsyncResult  *result,  /* IN */
                           gint          *pid,     /* OUT */
                           GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_get_pid_finish(
			priv->connection,
			real_result,
			pid,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_get_pid_set:
 * @channel: A #PkChannel.
 * @pid_set: A location for the pid_set.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves if the "pid" property was set manually.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_pid_set (PkChannel  *channel, /* IN */
                        gboolean   *pid_set, /* OUT */
                        GError    **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_get_pid_set(
			priv->connection,
			priv->id,
			pid_set,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_get_pid_set_cb (GObject      *object,    /* IN */
                           GAsyncResult *result,    /* IN */
                           gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_get_pid_set_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Retrieves if the "pid" property was set manually.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_pid_set_async (PkChannel           *channel,     /* IN */
                              GCancellable        *cancellable, /* IN */
                              GAsyncReadyCallback  callback,    /* IN */
                              gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_get_pid_set_async);
	pk_connection_channel_get_pid_set_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_channel_get_pid_set_cb,
			result);
	EXIT;
}


/**
 * pk_channel_get_pid_set_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves if the "pid" property was set manually.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_pid_set_finish (PkChannel     *channel, /* IN */
                               GAsyncResult  *result,  /* IN */
                               gboolean      *pid_set, /* OUT */
                               GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_get_pid_set_finish(
			priv->connection,
			real_result,
			pid_set,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_get_sources:
 * @channel: A #PkChannel.
 * @sources: A location for the sources.
 * @sources_len: A location for the sources_len.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the available sources.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_sources (PkChannel  *channel, /* IN */
                        GList     **sources, /* OUT */
                        GError    **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;
	gint *sources_ar = NULL;
	gsize sources_len = 0;
	gint i;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_get_sources(
			priv->connection,
			priv->id,
			&sources_ar,
			&sources_len,

			error))) {
		RETURN(FALSE);
	}
	*sources = NULL;
	for (i = 0; i < sources_len; i++) {
		*sources = g_list_prepend(
				*sources,
				g_object_new(PK_TYPE_SOURCE,
				             "connection", priv->connection,
				             "id", sources_ar[i],
				             NULL));
	}
	g_free(sources_ar);
	RETURN(ret);
}

static void
pk_channel_get_sources_cb (GObject      *object,    /* IN */
                           GAsyncResult *result,    /* IN */
                           gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_get_sources_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Retrieves the available sources.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_sources_async (PkChannel           *channel,     /* IN */
                              GCancellable        *cancellable, /* IN */
                              GAsyncReadyCallback  callback,    /* IN */
                              gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_get_sources_async);
	pk_connection_channel_get_sources_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_channel_get_sources_cb,
			result);
	EXIT;
}


/**
 * pk_channel_get_sources_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the available sources.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_sources_finish (PkChannel     *channel, /* IN */
                               GAsyncResult  *result,  /* IN */
                               GList        **sources, /* OUT */
                               GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;
	gint *sources_ar = NULL;
	gsize sources_len = 0;
	gint i;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_get_sources_finish(
			priv->connection,
			real_result,
			&sources_ar,
			&sources_len,

			error))) {
		RETURN(FALSE);
	}
	*sources = NULL;
	for (i = 0; i < sources_len; i++) {
		*sources = g_list_prepend(
				*sources,
				g_object_new(PK_TYPE_SOURCE,
				             "connection", priv->connection,
				             "id", sources_ar[i],
				             NULL));
	}
	g_free(sources_ar);
	RETURN(ret);
}


/**
 * pk_channel_get_state:
 * @channel: A #PkChannel.
 * @state: A location for the state.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the current state of the channel.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_state (PkChannel  *channel, /* IN */
                      gint       *state,   /* OUT */
                      GError    **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_get_state(
			priv->connection,
			priv->id,
			state,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_get_state_cb (GObject      *object,    /* IN */
                         GAsyncResult *result,    /* IN */
                         gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_get_state_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Retrieves the current state of the channel.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_state_async (PkChannel           *channel,     /* IN */
                            GCancellable        *cancellable, /* IN */
                            GAsyncReadyCallback  callback,    /* IN */
                            gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_get_state_async);
	pk_connection_channel_get_state_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_channel_get_state_cb,
			result);
	EXIT;
}


/**
 * pk_channel_get_state_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the current state of the channel.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_state_finish (PkChannel     *channel, /* IN */
                             GAsyncResult  *result,  /* IN */
                             gint          *state,   /* OUT */
                             GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_get_state_finish(
			priv->connection,
			real_result,
			state,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_get_target:
 * @channel: A #PkChannel.
 * @target: A location for the target.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the channels target.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_target (PkChannel  *channel, /* IN */
                       gchar     **target,  /* OUT */
                       GError    **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_get_target(
			priv->connection,
			priv->id,
			target,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_get_target_cb (GObject      *object,    /* IN */
                          GAsyncResult *result,    /* IN */
                          gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_get_target_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Retrieves the channels target.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_target_async (PkChannel           *channel,     /* IN */
                             GCancellable        *cancellable, /* IN */
                             GAsyncReadyCallback  callback,    /* IN */
                             gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_get_target_async);
	pk_connection_channel_get_target_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_channel_get_target_cb,
			result);
	EXIT;
}


/**
 * pk_channel_get_target_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the channels target.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_target_finish (PkChannel     *channel, /* IN */
                              GAsyncResult  *result,  /* IN */
                              gchar        **target,  /* OUT */
                              GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_get_target_finish(
			priv->connection,
			real_result,
			target,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_get_working_dir:
 * @channel: A #PkChannel.
 * @working_dir: A location for the working_dir.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the working directory of the target.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_working_dir (PkChannel  *channel,     /* IN */
                            gchar     **working_dir, /* OUT */
                            GError    **error)       /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_get_working_dir(
			priv->connection,
			priv->id,
			working_dir,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_get_working_dir_cb (GObject      *object,    /* IN */
                               GAsyncResult *result,    /* IN */
                               gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_get_working_dir_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Retrieves the working directory of the target.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_get_working_dir_async (PkChannel           *channel,     /* IN */
                                  GCancellable        *cancellable, /* IN */
                                  GAsyncReadyCallback  callback,    /* IN */
                                  gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_get_working_dir_async);
	pk_connection_channel_get_working_dir_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_channel_get_working_dir_cb,
			result);
	EXIT;
}


/**
 * pk_channel_get_working_dir_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the working directory of the target.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_get_working_dir_finish (PkChannel     *channel,     /* IN */
                                   GAsyncResult  *result,      /* IN */
                                   gchar        **working_dir, /* OUT */
                                   GError       **error)       /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_get_working_dir_finish(
			priv->connection,
			real_result,
			working_dir,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_mute:
 * @channel: A #PkChannel.
 * @error: A location for a #GError, or %NULL.
 *
 * Notifies @channel to silently drop manifest and sample updates until
 * unmute() is called.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_mute (PkChannel  *channel, /* IN */
                 GError    **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_mute(
			priv->connection,
			priv->id,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_mute_cb (GObject      *object,    /* IN */
                    GAsyncResult *result,    /* IN */
                    gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_mute_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Notifies @channel to silently drop manifest and sample updates until
 * unmute() is called.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_mute_async (PkChannel           *channel,     /* IN */
                       GCancellable        *cancellable, /* IN */
                       GAsyncReadyCallback  callback,    /* IN */
                       gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_mute_async);
	pk_connection_channel_mute_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_channel_mute_cb,
			result);
	EXIT;
}


/**
 * pk_channel_mute_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Notifies @channel to silently drop manifest and sample updates until
 * unmute() is called.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_mute_finish (PkChannel     *channel, /* IN */
                        GAsyncResult  *result,  /* IN */
                        GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_mute_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_set_args:
 * @channel: A #PkChannel.
 * @args: A gchar**.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the targets arguments.  This may only be set before the channel
 * has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_set_args (PkChannel  *channel, /* IN */
                     gchar     **args,    /* IN */
                     GError    **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_set_args(
			priv->connection,
			priv->id,
			(const gchar**)args,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_set_args_cb (GObject      *object,    /* IN */
                        GAsyncResult *result,    /* IN */
                        gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_set_args_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Sets the targets arguments.  This may only be set before the channel
 * has started.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_set_args_async (PkChannel            *channel,     /* IN */
                           gchar               **args,        /* IN */
                           GCancellable         *cancellable, /* IN */
                           GAsyncReadyCallback   callback,    /* IN */
                           gpointer              user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_set_args_async);
	pk_connection_channel_set_args_async(
			priv->connection,
			priv->id,
			(const gchar**)args,
			cancellable,
			pk_channel_set_args_cb,
			result);
	EXIT;
}


/**
 * pk_channel_set_args_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the targets arguments.  This may only be set before the channel
 * has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_set_args_finish (PkChannel     *channel, /* IN */
                            GAsyncResult  *result,  /* IN */
                            GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_set_args_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_set_env:
 * @channel: A #PkChannel.
 * @env: A gchar**.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the environment of the target process.  This may only be set before
 * the channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_set_env (PkChannel  *channel, /* IN */
                    gchar     **env,     /* IN */
                    GError    **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_set_env(
			priv->connection,
			priv->id,
			(const gchar**)env,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_set_env_cb (GObject      *object,    /* IN */
                       GAsyncResult *result,    /* IN */
                       gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_set_env_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Sets the environment of the target process.  This may only be set before
 * the channel has started.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_set_env_async (PkChannel            *channel,     /* IN */
                          gchar               **env,         /* IN */
                          GCancellable         *cancellable, /* IN */
                          GAsyncReadyCallback   callback,    /* IN */
                          gpointer              user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_set_env_async);
	pk_connection_channel_set_env_async(
			priv->connection,
			priv->id,
			(const gchar**)env,
			cancellable,
			pk_channel_set_env_cb,
			result);
	EXIT;
}


/**
 * pk_channel_set_env_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the environment of the target process.  This may only be set before
 * the channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_set_env_finish (PkChannel     *channel, /* IN */
                           GAsyncResult  *result,  /* IN */
                           GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_set_env_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_set_kill_pid:
 * @channel: A #PkChannel.
 * @kill_pid: A gboolean.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets if the process should be killed when the channel is stopped.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_set_kill_pid (PkChannel  *channel,  /* IN */
                         gboolean    kill_pid, /* IN */
                         GError    **error)    /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_set_kill_pid(
			priv->connection,
			priv->id,
			kill_pid,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_set_kill_pid_cb (GObject      *object,    /* IN */
                            GAsyncResult *result,    /* IN */
                            gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_set_kill_pid_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Sets if the process should be killed when the channel is stopped.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_set_kill_pid_async (PkChannel           *channel,     /* IN */
                               gboolean             kill_pid,    /* IN */
                               GCancellable        *cancellable, /* IN */
                               GAsyncReadyCallback  callback,    /* IN */
                               gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_set_kill_pid_async);
	pk_connection_channel_set_kill_pid_async(
			priv->connection,
			priv->id,
			kill_pid,
			cancellable,
			pk_channel_set_kill_pid_cb,
			result);
	EXIT;
}


/**
 * pk_channel_set_kill_pid_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets if the process should be killed when the channel is stopped.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_set_kill_pid_finish (PkChannel     *channel, /* IN */
                                GAsyncResult  *result,  /* IN */
                                GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_set_kill_pid_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_set_pid:
 * @channel: A #PkChannel.
 * @pid: A gint.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the target pid to attach to rather than spawning a process.  This can
 * only be set before the channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_set_pid (PkChannel  *channel, /* IN */
                    gint        pid,     /* IN */
                    GError    **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_set_pid(
			priv->connection,
			priv->id,
			pid,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_set_pid_cb (GObject      *object,    /* IN */
                       GAsyncResult *result,    /* IN */
                       gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_set_pid_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Sets the target pid to attach to rather than spawning a process.  This can
 * only be set before the channel has started.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_set_pid_async (PkChannel           *channel,     /* IN */
                          gint                 pid,         /* IN */
                          GCancellable        *cancellable, /* IN */
                          GAsyncReadyCallback  callback,    /* IN */
                          gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_set_pid_async);
	pk_connection_channel_set_pid_async(
			priv->connection,
			priv->id,
			pid,
			cancellable,
			pk_channel_set_pid_cb,
			result);
	EXIT;
}


/**
 * pk_channel_set_pid_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the target pid to attach to rather than spawning a process.  This can
 * only be set before the channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_set_pid_finish (PkChannel     *channel, /* IN */
                           GAsyncResult  *result,  /* IN */
                           GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_set_pid_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_set_target:
 * @channel: A #PkChannel.
 * @target: A const gchar*.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the channels target.  This may only be set before the channel has
 * started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_set_target (PkChannel    *channel, /* IN */
                       const gchar  *target,  /* IN */
                       GError      **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_set_target(
			priv->connection,
			priv->id,
			target,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_set_target_cb (GObject      *object,    /* IN */
                          GAsyncResult *result,    /* IN */
                          gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_set_target_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Sets the channels target.  This may only be set before the channel has
 * started.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_set_target_async (PkChannel           *channel,     /* IN */
                             const gchar         *target,      /* IN */
                             GCancellable        *cancellable, /* IN */
                             GAsyncReadyCallback  callback,    /* IN */
                             gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_set_target_async);
	pk_connection_channel_set_target_async(
			priv->connection,
			priv->id,
			target,
			cancellable,
			pk_channel_set_target_cb,
			result);
	EXIT;
}


/**
 * pk_channel_set_target_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the channels target.  This may only be set before the channel has
 * started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_set_target_finish (PkChannel     *channel, /* IN */
                              GAsyncResult  *result,  /* IN */
                              GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_set_target_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_set_working_dir:
 * @channel: A #PkChannel.
 * @working_dir: A const gchar*.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the targets working directory.  This may only be set before the
 * channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_set_working_dir (PkChannel    *channel,     /* IN */
                            const gchar  *working_dir, /* IN */
                            GError      **error)       /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_set_working_dir(
			priv->connection,
			priv->id,
			working_dir,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_set_working_dir_cb (GObject      *object,    /* IN */
                               GAsyncResult *result,    /* IN */
                               gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_set_working_dir_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Sets the targets working directory.  This may only be set before the
 * channel has started.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_set_working_dir_async (PkChannel           *channel,     /* IN */
                                  const gchar         *working_dir, /* IN */
                                  GCancellable        *cancellable, /* IN */
                                  GAsyncReadyCallback  callback,    /* IN */
                                  gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_set_working_dir_async);
	pk_connection_channel_set_working_dir_async(
			priv->connection,
			priv->id,
			working_dir,
			cancellable,
			pk_channel_set_working_dir_cb,
			result);
	EXIT;
}


/**
 * pk_channel_set_working_dir_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the targets working directory.  This may only be set before the
 * channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_set_working_dir_finish (PkChannel     *channel, /* IN */
                                   GAsyncResult  *result,  /* IN */
                                   GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_set_working_dir_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_start:
 * @channel: A #PkChannel.
 * @error: A location for a #GError, or %NULL.
 *
 * Start the channel. If required, the process will be spawned.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_start (PkChannel  *channel, /* IN */
                  GError    **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_start(
			priv->connection,
			priv->id,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_start_cb (GObject      *object,    /* IN */
                     GAsyncResult *result,    /* IN */
                     gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_start_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Start the channel. If required, the process will be spawned.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_start_async (PkChannel           *channel,     /* IN */
                        GCancellable        *cancellable, /* IN */
                        GAsyncReadyCallback  callback,    /* IN */
                        gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_start_async);
	pk_connection_channel_start_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_channel_start_cb,
			result);
	EXIT;
}


/**
 * pk_channel_start_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Start the channel. If required, the process will be spawned.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_start_finish (PkChannel     *channel, /* IN */
                         GAsyncResult  *result,  /* IN */
                         GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_start_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_stop:
 * @channel: A #PkChannel.
 * @error: A location for a #GError, or %NULL.
 *
 * Stop the channel. If @killpid, the inferior process is terminated.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_stop (PkChannel  *channel, /* IN */
                 GError    **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_stop(
			priv->connection,
			priv->id,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_stop_cb (GObject      *object,    /* IN */
                    GAsyncResult *result,    /* IN */
                    gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_stop_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Stop the channel. If @killpid, the inferior process is terminated.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_stop_async (PkChannel           *channel,     /* IN */
                       GCancellable        *cancellable, /* IN */
                       GAsyncReadyCallback  callback,    /* IN */
                       gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_stop_async);
	pk_connection_channel_stop_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_channel_stop_cb,
			result);
	EXIT;
}


/**
 * pk_channel_stop_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Stop the channel. If @killpid, the inferior process is terminated.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_stop_finish (PkChannel     *channel, /* IN */
                        GAsyncResult  *result,  /* IN */
                        GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_stop_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}


/**
 * pk_channel_unmute:
 * @channel: A #PkChannel.
 * @error: A location for a #GError, or %NULL.
 *
 * Resumes delivery of manifest and samples for sources within the channel.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_unmute (PkChannel  *channel, /* IN */
                   GError    **error)   /* OUT */
{
	PkChannelPrivate *priv;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	if (!(ret = pk_connection_channel_unmute(
			priv->connection,
			priv->id,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_unmute_cb (GObject      *object,    /* IN */
                      GAsyncResult *result,    /* IN */
                      gpointer      user_data) /* IN */
{
	GSimpleAsyncResult *real_result = user_data;

	g_return_if_fail(real_result != NULL);

	g_simple_async_result_set_op_res_gpointer(real_result,
	                                          g_object_ref(result),
	                                          g_object_unref);
	g_simple_async_result_complete(real_result);
}

/**
 * pk_channel_unmute_async:
 * @channel: A #PkChannel.
 * @cancellable: A #GCancellable, or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Resumes delivery of manifest and samples for sources within the channel.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_channel_unmute_async (PkChannel           *channel,     /* IN */
                         GCancellable        *cancellable, /* IN */
                         GAsyncReadyCallback  callback,    /* IN */
                         gpointer             user_data)   /* IN */
{
	PkChannelPrivate *priv;
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	result = g_simple_async_result_new(
			G_OBJECT(channel),
			callback,
			user_data,
			pk_channel_unmute_async);
	pk_connection_channel_unmute_async(
			priv->connection,
			priv->id,
			cancellable,
			pk_channel_unmute_cb,
			result);
	EXIT;
}


/**
 * pk_channel_unmute_finish:
 * @channel: A #PkChannel.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError, or %NULL.
 *
 * Resumes delivery of manifest and samples for sources within the channel.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_channel_unmute_finish (PkChannel     *channel, /* IN */
                          GAsyncResult  *result,  /* IN */
                          GError       **error)   /* OUT */
{
	PkChannelPrivate *priv;
	GAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PK_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	real_result = g_simple_async_result_get_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result));
	if (!(ret = pk_connection_channel_unmute_finish(
			priv->connection,
			real_result,
			error))) {
		RETURN(FALSE);
	}
	RETURN(ret);
}

static void
pk_channel_finalize (GObject *object) /* IN */
{
	G_OBJECT_CLASS(pk_channel_parent_class)->finalize(object);
}

static void
pk_channel_get_property (GObject    *object,  /* IN */
                        guint       prop_id, /* IN */
                        GValue     *value,   /* IN */
                        GParamSpec *pspec)   /* IN */
{
	switch (prop_id) {
	case PROP_ID:
		g_value_set_int(value,
		                pk_channel_get_id(PK_CHANNEL(object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_channel_set_property (GObject      *object,  /* IN */
                        guint         prop_id, /* IN */
                        const GValue *value,   /* IN */
                        GParamSpec   *pspec)   /* IN */
{
	switch (prop_id) {
	case PROP_ID:
		PK_CHANNEL(object)->priv->id = g_value_get_int(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_channel_class_init (PkChannelClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pk_channel_finalize;
	object_class->get_property = pk_channel_get_property;
	object_class->set_property = pk_channel_set_property;
	g_type_class_add_private(object_class, sizeof(PkChannelPrivate));

	/**
	 * PkChannel:id:
	 *
	 * The channel identifier.
	 */
	g_object_class_install_property(object_class,
	                                PROP_ID,
	                                g_param_spec_int("id",
	                                                 "Id",
	                                                 "Id",
	                                                 G_MININT,
	                                                 G_MAXINT,
	                                                 G_MININT,
	                                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * PkChannel:connection:
	 *
	 * The channel connection.
	 */
	g_object_class_install_property(object_class,
	                                PROP_CONNECTION,
	                                g_param_spec_object("connection",
	                                                    "Connection",
	                                                    "Connection",
	                                                    PK_TYPE_CONNECTION,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
pk_channel_init (PkChannel *channel) /* IN */
{
	channel->priv = G_TYPE_INSTANCE_GET_PRIVATE(channel, PK_TYPE_CHANNEL, PkChannelPrivate);
}
