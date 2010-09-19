/* pk-connection.c
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

#include <gmodule.h>
#include <stdio.h>
#include <string.h>

#include "pk-connection.h"
#include "pk-connection-lowlevel.h"
#include "pk-log.h"
#include "pk-manager.h"

/**
 * SECTION:pk-connection:
 * @title: Perfkit client connection
 * @short_description: 
 *
 * #PkConnection provides the client communication for Perfkit.
 * Connections are established through the use of URIs using
 * pk_connection_new_from_uri().
 *
 * For example, to create a connection using the DBus implementation:
 *
 * [[
 * PkConnection *conn = pk_connection_new_from_uri("dbus://");
 * ]]
 */

#define RPC_ASYNC(_n)                                               \
    if (!PK_CONNECTION_GET_CLASS(connection)->_n##_async) {         \
        g_warning("%s does not support the #_n RPC.",               \
                  g_type_name(G_TYPE_FROM_INSTANCE(connection)));   \
        EXIT;                                                       \
    }                                                               \
    PK_CONNECTION_GET_CLASS(connection)->_n##_async

#define RPC_FINISH(_r, _n)                                          \
    if (!PK_CONNECTION_GET_CLASS(connection)->_n##_finish) {        \
        g_critical("%s does not support the #_n RPC.",              \
                   g_type_name(G_TYPE_FROM_INSTANCE(connection)));  \
        g_set_error(error, PK_CONNECTION_ERROR,                     \
                    PK_CONNECTION_ERROR_NOT_IMPLEMENTED,            \
                    "The #_n RPC is not supported over your "       \
                    "connection.");                                 \
        RETURN(FALSE);                                              \
    }                                                               \
    _r = PK_CONNECTION_GET_CLASS(connection)->_n##_finish

#define CHECK_FOR_RPC(_n)                                           \
    G_STMT_START {                                                  \
        if ((!PK_CONNECTION_GET_CLASS(connection)->_n##_async) ||   \
            (!PK_CONNECTION_GET_CLASS(connection)->_n##_finish)) {  \
            g_set_error(error, PK_CONNECTION_ERROR,                 \
                        PK_CONNECTION_ERROR_NOT_IMPLEMENTED,        \
                        "The #_n RPC is not supported over "        \
                        "your connection.");                        \
            RETURN(FALSE);                                          \
        }                                                           \
    } G_STMT_END

G_DEFINE_TYPE(PkConnection, pk_connection, G_TYPE_OBJECT)

struct _PkConnectionPrivate
{
	GStaticRWLock      rw_lock; /* Synchronization */
	gchar             *uri;     /* Connection path/uri */
	PkConnectionState  state;   /* Current connection state */
	PkManager         *manager; /* Toplevel of Object Model API */
};

typedef struct
{
	GMutex     *mutex;
	GCond      *cond;
	gboolean    completed;
	gboolean    result;
	GError    **error;
	gpointer    params[16];
} PkConnectionSync;

enum
{
	PROP_0,
	PROP_URI,
};

enum
{
	STATE_CHANGED,
	CHANNEL_ADDED,
	CHANNEL_REMOVED,
	ENCODER_ADDED,
	ENCODER_REMOVED,
	PLUGIN_ADDED,
	PLUGIN_REMOVED,
	SOURCE_ADDED,
	SOURCE_REMOVED,
	SUBSCRIPTION_ADDED,
	SUBSCRIPTION_REMOVED,
	LAST_SIGNAL
};

static guint         signals[LAST_SIGNAL] = { 0 };
static GStaticMutex  protocol_mutex       = G_STATIC_MUTEX_INIT;
static GHashTable   *protocol_types       = NULL;
static gsize         protocol_init        = FALSE;

/**
 * pk_connection_sync_init:
 * @sync: A #PkConnectionSync.
 *
 * Initializes a #PkConnectionSync allocated on the stack.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
pk_connection_sync_init (PkConnectionSync *sync) /* IN */
{
	memset(sync, 0, sizeof(*sync));

	sync->completed = FALSE;
	sync->result = FALSE;
	sync->error = NULL;
	sync->mutex = g_mutex_new();
	sync->cond = g_cond_new();
	g_mutex_lock(sync->mutex);
}

/**
 * pk_connection_sync_destroy:
 * @sync: A #PkConnectionSync.
 *
 * Cleans up dynamically allocated data within the structure.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
pk_connection_sync_destroy (PkConnectionSync *sync) /* IN */
{
	g_mutex_unlock(sync->mutex);
	g_mutex_free(sync->mutex);
	g_cond_free(sync->cond);
}

/**
 * pk_connection_sync_wait:
 * @sync: A #PkConnectionSync.
 *
 * Waits for an sync operation in a secondary thread to complete.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_sync_wait (PkConnectionSync *sync) /* IN */
{
	GMainContext *context;

	/*
	 * Get the application main context.
	 */
	context = g_main_context_default();

	/*
	 * If we acquire the context, we can use it for blocking. Otherwise,
	 * another thread owns it and it is safe to block on the condition.
	 */
	if (g_main_context_acquire(context)) {
		g_mutex_unlock(sync->mutex);
		do {
			g_main_context_iteration(context, TRUE);
		} while (!g_atomic_int_get(&sync->completed));
		g_mutex_lock(sync->mutex);
		g_main_context_release(context);
	} else {
		/*
		 * Block on the condition which will be signaled by the main thread.
		 */
		g_cond_wait(sync->cond, sync->mutex);
	}
}

/**
 * pk_connection_sync_signal:
 * @sync: A #PkConnectionSync.
 *
 * Signals a thread blocked in pk_connection_sync_wait() that an sync
 * operation has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
pk_connection_sync_signal (PkConnectionSync *sync) /* IN */
{
	g_mutex_lock(sync->mutex);
	g_atomic_int_set(&sync->completed, TRUE);
	g_cond_signal(sync->cond);
	g_mutex_unlock(sync->mutex);
}

/**
 * pk_connection_channel_add_source_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_add_source" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_add_source_cb (GObject      *source,    /* IN */
                                     GAsyncResult *result,    /* IN */
                                     gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_add_source_finish(PK_CONNECTION(source),
	                                                       result,
	                                                       sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_add_source:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_add_source" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Adds an existing source to the channel.  If the channel has already been
 * started, the source will be started immediately.  The source must not have
 * been previous added to another channel or this will fail.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_add_source (PkConnection  *connection, /* IN */
                                  gint           channel,    /* IN */
                                  gint           source,     /* IN */
                                  GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_add_source);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_channel_add_source_async(connection,
	                                       channel,
	                                       source,
	                                       NULL,
	                                       pk_connection_channel_add_source_cb,
	                                       &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_add_source_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_add_source_async" RPC.
 *
 * Adds an existing source to the channel.  If the channel has already been
 * started, the source will be started immediately.  The source must not have
 * been previous added to another channel or this will fail.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_add_source_async (PkConnection        *connection,  /* IN */
                                        gint                 channel,     /* IN */
                                        gint                 source,      /* IN */
                                        GCancellable        *cancellable, /* IN */
                                        GAsyncReadyCallback  callback,    /* IN */
                                        gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_add_source)(connection,
	                              channel,
	                              source,
	                              cancellable,
	                              callback,
	                              user_data);
	EXIT;
}

/**
 * pk_connection_channel_add_source_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_add_source_finish" RPC.
 *
 * Adds an existing source to the channel.  If the channel has already been
 * started, the source will be started immediately.  The source must not have
 * been previous added to another channel or this will fail.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_add_source_finish (PkConnection  *connection, /* IN */
                                         GAsyncResult  *result,     /* IN */
                                         GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_add_source)(connection,
	                                    result,
	                                    error);
	RETURN(ret);
}

/**
 * pk_connection_channel_get_args_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_get_args" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_args_cb (GObject      *source,    /* IN */
                                   GAsyncResult *result,    /* IN */
                                   gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_get_args_finish(PK_CONNECTION(source),
	                                                     result,
	                                                     sync->params[0],
	                                                     sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_get_args:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_get_args" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the arguments for target.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_args (PkConnection   *connection, /* IN */
                                gint            channel,    /* IN */
                                gchar        ***args,       /* OUT */
                                GError        **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_args);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = args;
	pk_connection_channel_get_args_async(connection,
	                                     channel,
	                                     NULL,
	                                     pk_connection_channel_get_args_cb,
	                                     &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_get_args_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_get_args_async" RPC.
 *
 * Retrieves the arguments for target.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_get_args_async (PkConnection        *connection,  /* IN */
                                      gint                 channel,     /* IN */
                                      GCancellable        *cancellable, /* IN */
                                      GAsyncReadyCallback  callback,    /* IN */
                                      gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_get_args)(connection,
	                            channel,
	                            cancellable,
	                            callback,
	                            user_data);
	EXIT;
}

/**
 * pk_connection_channel_get_args_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_get_args_finish" RPC.
 *
 * Retrieves the arguments for target.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_args_finish (PkConnection   *connection, /* IN */
                                       GAsyncResult   *result,     /* IN */
                                       gchar        ***args,       /* OUT */
                                       GError        **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_get_args)(connection,
	                                  result,
	                                  args,
	                                  error);
	RETURN(ret);
}

/**
 * pk_connection_channel_get_created_at_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_get_created_at" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_created_at_cb (GObject      *source,    /* IN */
                                         GAsyncResult *result,    /* IN */
                                         gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_get_created_at_finish(PK_CONNECTION(source),
	                                                           result,
	                                                           sync->params[0],
	                                                           sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_get_created_at:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_get_created_at" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the time at which the channel was created.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_created_at (PkConnection  *connection, /* IN */
                                      gint           channel,    /* IN */
                                      GTimeVal      *tv,         /* OUT */
                                      GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_created_at);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = tv;
	pk_connection_channel_get_created_at_async(connection,
	                                           channel,
	                                           NULL,
	                                           pk_connection_channel_get_created_at_cb,
	                                           &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_get_created_at_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_get_created_at_async" RPC.
 *
 * Retrieves the time at which the channel was created.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_get_created_at_async (PkConnection        *connection,  /* IN */
                                            gint                 channel,     /* IN */
                                            GCancellable        *cancellable, /* IN */
                                            GAsyncReadyCallback  callback,    /* IN */
                                            gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_get_created_at)(connection,
	                                  channel,
	                                  cancellable,
	                                  callback,
	                                  user_data);
	EXIT;
}

/**
 * pk_connection_channel_get_created_at_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_get_created_at_finish" RPC.
 *
 * Retrieves the time at which the channel was created.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_created_at_finish (PkConnection  *connection, /* IN */
                                             GAsyncResult  *result,     /* IN */
                                             GTimeVal      *tv,         /* OUT */
                                             GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_get_created_at)(connection,
	                                        result,
	                                        tv,
	                                        error);
	RETURN(ret);
}

/**
 * pk_connection_channel_get_env_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_get_env" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_env_cb (GObject      *source,    /* IN */
                                  GAsyncResult *result,    /* IN */
                                  gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_get_env_finish(PK_CONNECTION(source),
	                                                    result,
	                                                    sync->params[0],
	                                                    sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_get_env:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_get_env" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the environment for spawning the target process.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_env (PkConnection   *connection, /* IN */
                               gint            channel,    /* IN */
                               gchar        ***env,        /* OUT */
                               GError        **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_env);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = env;
	pk_connection_channel_get_env_async(connection,
	                                    channel,
	                                    NULL,
	                                    pk_connection_channel_get_env_cb,
	                                    &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_get_env_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_get_env_async" RPC.
 *
 * Retrieves the environment for spawning the target process.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_get_env_async (PkConnection        *connection,  /* IN */
                                     gint                 channel,     /* IN */
                                     GCancellable        *cancellable, /* IN */
                                     GAsyncReadyCallback  callback,    /* IN */
                                     gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_get_env)(connection,
	                           channel,
	                           cancellable,
	                           callback,
	                           user_data);
	EXIT;
}

/**
 * pk_connection_channel_get_env_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_get_env_finish" RPC.
 *
 * Retrieves the environment for spawning the target process.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_env_finish (PkConnection   *connection, /* IN */
                                      GAsyncResult   *result,     /* IN */
                                      gchar        ***env,        /* OUT */
                                      GError        **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_get_env)(connection,
	                                 result,
	                                 env,
	                                 error);
	RETURN(ret);
}

/**
 * pk_connection_channel_get_exit_status_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_get_exit_status" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_exit_status_cb (GObject      *source,    /* IN */
                                          GAsyncResult *result,    /* IN */
                                          gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_get_exit_status_finish(PK_CONNECTION(source),
	                                                            result,
	                                                            sync->params[0],
	                                                            sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_get_exit_status:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_get_exit_status" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the exit status of the process.  This is only set after the
 * process has exited.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_exit_status (PkConnection  *connection,  /* IN */
                                       gint           channel,     /* IN */
                                       gint          *exit_status, /* OUT */
                                       GError       **error)       /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_exit_status);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = exit_status;
	pk_connection_channel_get_exit_status_async(connection,
	                                            channel,
	                                            NULL,
	                                            pk_connection_channel_get_exit_status_cb,
	                                            &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_get_exit_status_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_get_exit_status_async" RPC.
 *
 * Retrieves the exit status of the process.  This is only set after the
 * process has exited.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_get_exit_status_async (PkConnection        *connection,  /* IN */
                                             gint                 channel,     /* IN */
                                             GCancellable        *cancellable, /* IN */
                                             GAsyncReadyCallback  callback,    /* IN */
                                             gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_get_exit_status)(connection,
	                                   channel,
	                                   cancellable,
	                                   callback,
	                                   user_data);
	EXIT;
}

/**
 * pk_connection_channel_get_exit_status_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_get_exit_status_finish" RPC.
 *
 * Retrieves the exit status of the process.  This is only set after the
 * process has exited.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_exit_status_finish (PkConnection  *connection,  /* IN */
                                              GAsyncResult  *result,      /* IN */
                                              gint          *exit_status, /* OUT */
                                              GError       **error)       /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_get_exit_status)(connection,
	                                         result,
	                                         exit_status,
	                                         error);
	RETURN(ret);
}

/**
 * pk_connection_channel_get_kill_pid_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_get_kill_pid" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_kill_pid_cb (GObject      *source,    /* IN */
                                       GAsyncResult *result,    /* IN */
                                       gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_get_kill_pid_finish(PK_CONNECTION(source),
	                                                         result,
	                                                         sync->params[0],
	                                                         sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_get_kill_pid:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_get_kill_pid" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves if the process should be killed when the channel is stopped.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_kill_pid (PkConnection  *connection, /* IN */
                                    gint           channel,    /* IN */
                                    gboolean      *kill_pid,   /* OUT */
                                    GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_kill_pid);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = kill_pid;
	pk_connection_channel_get_kill_pid_async(connection,
	                                         channel,
	                                         NULL,
	                                         pk_connection_channel_get_kill_pid_cb,
	                                         &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_get_kill_pid_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_get_kill_pid_async" RPC.
 *
 * Retrieves if the process should be killed when the channel is stopped.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_get_kill_pid_async (PkConnection        *connection,  /* IN */
                                          gint                 channel,     /* IN */
                                          GCancellable        *cancellable, /* IN */
                                          GAsyncReadyCallback  callback,    /* IN */
                                          gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_get_kill_pid)(connection,
	                                channel,
	                                cancellable,
	                                callback,
	                                user_data);
	EXIT;
}

/**
 * pk_connection_channel_get_kill_pid_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_get_kill_pid_finish" RPC.
 *
 * Retrieves if the process should be killed when the channel is stopped.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_kill_pid_finish (PkConnection  *connection, /* IN */
                                           GAsyncResult  *result,     /* IN */
                                           gboolean      *kill_pid,   /* OUT */
                                           GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_get_kill_pid)(connection,
	                                      result,
	                                      kill_pid,
	                                      error);
	RETURN(ret);
}

/**
 * pk_connection_channel_get_pid_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_get_pid" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_pid_cb (GObject      *source,    /* IN */
                                  GAsyncResult *result,    /* IN */
                                  gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_get_pid_finish(PK_CONNECTION(source),
	                                                    result,
	                                                    sync->params[0],
	                                                    sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_get_pid:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_get_pid" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the process pid of the target process.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_pid (PkConnection  *connection, /* IN */
                               gint           channel,    /* IN */
                               gint          *pid,        /* OUT */
                               GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_pid);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = pid;
	pk_connection_channel_get_pid_async(connection,
	                                    channel,
	                                    NULL,
	                                    pk_connection_channel_get_pid_cb,
	                                    &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_get_pid_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_get_pid_async" RPC.
 *
 * Retrieves the process pid of the target process.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_get_pid_async (PkConnection        *connection,  /* IN */
                                     gint                 channel,     /* IN */
                                     GCancellable        *cancellable, /* IN */
                                     GAsyncReadyCallback  callback,    /* IN */
                                     gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_get_pid)(connection,
	                           channel,
	                           cancellable,
	                           callback,
	                           user_data);
	EXIT;
}

/**
 * pk_connection_channel_get_pid_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_get_pid_finish" RPC.
 *
 * Retrieves the process pid of the target process.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_pid_finish (PkConnection  *connection, /* IN */
                                      GAsyncResult  *result,     /* IN */
                                      gint          *pid,        /* OUT */
                                      GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_get_pid)(connection,
	                                 result,
	                                 pid,
	                                 error);
	RETURN(ret);
}

/**
 * pk_connection_channel_get_pid_set_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_get_pid_set" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_pid_set_cb (GObject      *source,    /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_get_pid_set_finish(PK_CONNECTION(source),
	                                                        result,
	                                                        sync->params[0],
	                                                        sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_get_pid_set:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_get_pid_set" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves if the "pid" property was set manually.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_pid_set (PkConnection  *connection, /* IN */
                                   gint           channel,    /* IN */
                                   gboolean      *pid_set,    /* OUT */
                                   GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_pid_set);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = pid_set;
	pk_connection_channel_get_pid_set_async(connection,
	                                        channel,
	                                        NULL,
	                                        pk_connection_channel_get_pid_set_cb,
	                                        &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_get_pid_set_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_get_pid_set_async" RPC.
 *
 * Retrieves if the "pid" property was set manually.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_get_pid_set_async (PkConnection        *connection,  /* IN */
                                         gint                 channel,     /* IN */
                                         GCancellable        *cancellable, /* IN */
                                         GAsyncReadyCallback  callback,    /* IN */
                                         gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_get_pid_set)(connection,
	                               channel,
	                               cancellable,
	                               callback,
	                               user_data);
	EXIT;
}

/**
 * pk_connection_channel_get_pid_set_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_get_pid_set_finish" RPC.
 *
 * Retrieves if the "pid" property was set manually.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_pid_set_finish (PkConnection  *connection, /* IN */
                                          GAsyncResult  *result,     /* IN */
                                          gboolean      *pid_set,    /* OUT */
                                          GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_get_pid_set)(connection,
	                                     result,
	                                     pid_set,
	                                     error);
	RETURN(ret);
}

/**
 * pk_connection_channel_get_sources_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_get_sources" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_sources_cb (GObject      *source,    /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_get_sources_finish(PK_CONNECTION(source),
	                                                        result,
	                                                        sync->params[0],
	                                                        sync->params[1],
	                                                        sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_get_sources:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_get_sources" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the available sources.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_sources (PkConnection  *connection,  /* IN */
                                   gint           channel,     /* IN */
                                   gint         **sources,     /* OUT */
                                   gsize         *sources_len, /* OUT */
                                   GError       **error)       /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_sources);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = sources;
	sync.params[1] = sources_len;
	pk_connection_channel_get_sources_async(connection,
	                                        channel,
	                                        NULL,
	                                        pk_connection_channel_get_sources_cb,
	                                        &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_get_sources_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_get_sources_async" RPC.
 *
 * Retrieves the available sources.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_get_sources_async (PkConnection        *connection,  /* IN */
                                         gint                 channel,     /* IN */
                                         GCancellable        *cancellable, /* IN */
                                         GAsyncReadyCallback  callback,    /* IN */
                                         gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_get_sources)(connection,
	                               channel,
	                               cancellable,
	                               callback,
	                               user_data);
	EXIT;
}

/**
 * pk_connection_channel_get_sources_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_get_sources_finish" RPC.
 *
 * Retrieves the available sources.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_sources_finish (PkConnection  *connection,  /* IN */
                                          GAsyncResult  *result,      /* IN */
                                          gint         **sources,     /* OUT */
                                          gsize         *sources_len, /* OUT */
                                          GError       **error)       /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_get_sources)(connection,
	                                     result,
	                                     sources,
	                                     sources_len,
	                                     error);
	RETURN(ret);
}

/**
 * pk_connection_channel_get_state_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_get_state" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_state_cb (GObject      *source,    /* IN */
                                    GAsyncResult *result,    /* IN */
                                    gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_get_state_finish(PK_CONNECTION(source),
	                                                      result,
	                                                      sync->params[0],
	                                                      sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_get_state:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_get_state" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the current state of the channel.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_state (PkConnection  *connection, /* IN */
                                 gint           channel,    /* IN */
                                 gint          *state,      /* OUT */
                                 GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_state);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = state;
	pk_connection_channel_get_state_async(connection,
	                                      channel,
	                                      NULL,
	                                      pk_connection_channel_get_state_cb,
	                                      &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_get_state_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_get_state_async" RPC.
 *
 * Retrieves the current state of the channel.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_get_state_async (PkConnection        *connection,  /* IN */
                                       gint                 channel,     /* IN */
                                       GCancellable        *cancellable, /* IN */
                                       GAsyncReadyCallback  callback,    /* IN */
                                       gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_get_state)(connection,
	                             channel,
	                             cancellable,
	                             callback,
	                             user_data);
	EXIT;
}

/**
 * pk_connection_channel_get_state_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_get_state_finish" RPC.
 *
 * Retrieves the current state of the channel.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_state_finish (PkConnection  *connection, /* IN */
                                        GAsyncResult  *result,     /* IN */
                                        gint          *state,      /* OUT */
                                        GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_get_state)(connection,
	                                   result,
	                                   state,
	                                   error);
	RETURN(ret);
}

/**
 * pk_connection_channel_get_target_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_get_target" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_target_cb (GObject      *source,    /* IN */
                                     GAsyncResult *result,    /* IN */
                                     gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_get_target_finish(PK_CONNECTION(source),
	                                                       result,
	                                                       sync->params[0],
	                                                       sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_get_target:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_get_target" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the channels target.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_target (PkConnection  *connection, /* IN */
                                  gint           channel,    /* IN */
                                  gchar        **target,     /* OUT */
                                  GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_target);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = target;
	pk_connection_channel_get_target_async(connection,
	                                       channel,
	                                       NULL,
	                                       pk_connection_channel_get_target_cb,
	                                       &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_get_target_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_get_target_async" RPC.
 *
 * Retrieves the channels target.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_get_target_async (PkConnection        *connection,  /* IN */
                                        gint                 channel,     /* IN */
                                        GCancellable        *cancellable, /* IN */
                                        GAsyncReadyCallback  callback,    /* IN */
                                        gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_get_target)(connection,
	                              channel,
	                              cancellable,
	                              callback,
	                              user_data);
	EXIT;
}

/**
 * pk_connection_channel_get_target_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_get_target_finish" RPC.
 *
 * Retrieves the channels target.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_target_finish (PkConnection  *connection, /* IN */
                                         GAsyncResult  *result,     /* IN */
                                         gchar        **target,     /* OUT */
                                         GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_get_target)(connection,
	                                    result,
	                                    target,
	                                    error);
	RETURN(ret);
}

/**
 * pk_connection_channel_get_working_dir_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_get_working_dir" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_working_dir_cb (GObject      *source,    /* IN */
                                          GAsyncResult *result,    /* IN */
                                          gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_get_working_dir_finish(PK_CONNECTION(source),
	                                                            result,
	                                                            sync->params[0],
	                                                            sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_get_working_dir:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_get_working_dir" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the working directory of the target.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_working_dir (PkConnection  *connection,  /* IN */
                                       gint           channel,     /* IN */
                                       gchar        **working_dir, /* OUT */
                                       GError       **error)       /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_working_dir);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = working_dir;
	pk_connection_channel_get_working_dir_async(connection,
	                                            channel,
	                                            NULL,
	                                            pk_connection_channel_get_working_dir_cb,
	                                            &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_get_working_dir_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_get_working_dir_async" RPC.
 *
 * Retrieves the working directory of the target.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_get_working_dir_async (PkConnection        *connection,  /* IN */
                                             gint                 channel,     /* IN */
                                             GCancellable        *cancellable, /* IN */
                                             GAsyncReadyCallback  callback,    /* IN */
                                             gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_get_working_dir)(connection,
	                                   channel,
	                                   cancellable,
	                                   callback,
	                                   user_data);
	EXIT;
}

/**
 * pk_connection_channel_get_working_dir_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_get_working_dir_finish" RPC.
 *
 * Retrieves the working directory of the target.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_working_dir_finish (PkConnection  *connection,  /* IN */
                                              GAsyncResult  *result,      /* IN */
                                              gchar        **working_dir, /* OUT */
                                              GError       **error)       /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_get_working_dir)(connection,
	                                         result,
	                                         working_dir,
	                                         error);
	RETURN(ret);
}

/**
 * pk_connection_channel_mute_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_mute" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_mute_cb (GObject      *source,    /* IN */
                               GAsyncResult *result,    /* IN */
                               gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_mute_finish(PK_CONNECTION(source),
	                                                 result,
	                                                 sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_mute:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_mute" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Notifies @channel to silently drop manifest and sample updates until
 * unmute() is called.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_mute (PkConnection  *connection, /* IN */
                            gint           channel,    /* IN */
                            GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_mute);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_channel_mute_async(connection,
	                                 channel,
	                                 NULL,
	                                 pk_connection_channel_mute_cb,
	                                 &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_mute_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_mute_async" RPC.
 *
 * Notifies @channel to silently drop manifest and sample updates until
 * unmute() is called.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_mute_async (PkConnection        *connection,  /* IN */
                                  gint                 channel,     /* IN */
                                  GCancellable        *cancellable, /* IN */
                                  GAsyncReadyCallback  callback,    /* IN */
                                  gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_mute)(connection,
	                        channel,
	                        cancellable,
	                        callback,
	                        user_data);
	EXIT;
}

/**
 * pk_connection_channel_mute_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_mute_finish" RPC.
 *
 * Notifies @channel to silently drop manifest and sample updates until
 * unmute() is called.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_mute_finish (PkConnection  *connection, /* IN */
                                   GAsyncResult  *result,     /* IN */
                                   GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_mute)(connection,
	                              result,
	                              error);
	RETURN(ret);
}

/**
 * pk_connection_channel_set_args_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_set_args" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_set_args_cb (GObject      *source,    /* IN */
                                   GAsyncResult *result,    /* IN */
                                   gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_set_args_finish(PK_CONNECTION(source),
	                                                     result,
	                                                     sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_set_args:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_set_args" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Sets the targets arguments.  This may only be set before the channel
 * has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_set_args (PkConnection  *connection, /* IN */
                                gint           channel,    /* IN */
                                const gchar  **args,       /* IN */
                                GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_set_args);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_channel_set_args_async(connection,
	                                     channel,
	                                     args,
	                                     NULL,
	                                     pk_connection_channel_set_args_cb,
	                                     &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_set_args_async:
 * @connection: (in): A #PkConnection.
 * @channel: (in): The channel.
 * @args: (in): A %NULL terminated list of args.
 *
 * Asynchronous implementation of the "channel_set_args_async" RPC.
 *
 * Sets the targets arguments.  This may only be set before the channel
 * has started.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_set_args_async (PkConnection         *connection,  /* IN */
                                      gint                  channel,     /* IN */
                                      const gchar         **args,        /* IN */
                                      GCancellable         *cancellable, /* IN */
                                      GAsyncReadyCallback   callback,    /* IN */
                                      gpointer              user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_set_args)(connection,
	                            channel,
	                            args,
	                            cancellable,
	                            callback,
	                            user_data);
	EXIT;
}

/**
 * pk_connection_channel_set_args_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_set_args_finish" RPC.
 *
 * Sets the targets arguments.  This may only be set before the channel
 * has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_set_args_finish (PkConnection  *connection, /* IN */
                                       GAsyncResult  *result,     /* IN */
                                       GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_set_args)(connection,
	                                  result,
	                                  error);
	RETURN(ret);
}

/**
 * pk_connection_channel_set_env_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_set_env" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_set_env_cb (GObject      *source,    /* IN */
                                  GAsyncResult *result,    /* IN */
                                  gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_set_env_finish(PK_CONNECTION(source),
	                                                    result,
	                                                    sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_set_env:
 * @connection: A #PkConnection.
 * @channel: (in): The channel
 * @env: (in): A %NULL terminated string array of environtment variables.
 *
 * Synchronous implemenation of the "channel_set_env" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Sets the environment of the target process.  This may only be set before
 * the channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_set_env (PkConnection  *connection, /* IN */
                               gint           channel,    /* IN */
                               const gchar  **env,        /* IN */
                               GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_set_env);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_channel_set_env_async(connection,
	                                    channel,
	                                    env,
	                                    NULL,
	                                    pk_connection_channel_set_env_cb,
	                                    &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_set_env_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_set_env_async" RPC.
 *
 * Sets the environment of the target process.  This may only be set before
 * the channel has started.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_set_env_async (PkConnection         *connection,  /* IN */
                                     gint                  channel,     /* IN */
                                     const gchar         **env,         /* IN */
                                     GCancellable         *cancellable, /* IN */
                                     GAsyncReadyCallback   callback,    /* IN */
                                     gpointer              user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_set_env)(connection,
	                           channel,
	                           env,
	                           cancellable,
	                           callback,
	                           user_data);
	EXIT;
}

/**
 * pk_connection_channel_set_env_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_set_env_finish" RPC.
 *
 * Sets the environment of the target process.  This may only be set before
 * the channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_set_env_finish (PkConnection  *connection, /* IN */
                                      GAsyncResult  *result,     /* IN */
                                      GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_set_env)(connection,
	                                 result,
	                                 error);
	RETURN(ret);
}

/**
 * pk_connection_channel_set_kill_pid_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_set_kill_pid" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_set_kill_pid_cb (GObject      *source,    /* IN */
                                       GAsyncResult *result,    /* IN */
                                       gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_set_kill_pid_finish(PK_CONNECTION(source),
	                                                         result,
	                                                         sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_set_kill_pid:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_set_kill_pid" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Sets if the process should be killed when the channel is stopped.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_set_kill_pid (PkConnection  *connection, /* IN */
                                    gint           channel,    /* IN */
                                    gboolean       kill_pid,   /* IN */
                                    GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_set_kill_pid);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_channel_set_kill_pid_async(connection,
	                                         channel,
	                                         kill_pid,
	                                         NULL,
	                                         pk_connection_channel_set_kill_pid_cb,
	                                         &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_set_kill_pid_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_set_kill_pid_async" RPC.
 *
 * Sets if the process should be killed when the channel is stopped.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_set_kill_pid_async (PkConnection        *connection,  /* IN */
                                          gint                 channel,     /* IN */
                                          gboolean             kill_pid,    /* IN */
                                          GCancellable        *cancellable, /* IN */
                                          GAsyncReadyCallback  callback,    /* IN */
                                          gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_set_kill_pid)(connection,
	                                channel,
	                                kill_pid,
	                                cancellable,
	                                callback,
	                                user_data);
	EXIT;
}

/**
 * pk_connection_channel_set_kill_pid_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_set_kill_pid_finish" RPC.
 *
 * Sets if the process should be killed when the channel is stopped.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_set_kill_pid_finish (PkConnection  *connection, /* IN */
                                           GAsyncResult  *result,     /* IN */
                                           GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_set_kill_pid)(connection,
	                                      result,
	                                      error);
	RETURN(ret);
}

/**
 * pk_connection_channel_set_pid_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_set_pid" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_set_pid_cb (GObject      *source,    /* IN */
                                  GAsyncResult *result,    /* IN */
                                  gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_set_pid_finish(PK_CONNECTION(source),
	                                                    result,
	                                                    sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_set_pid:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_set_pid" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Sets the target pid to attach to rather than spawning a process.  This can
 * only be set before the channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_set_pid (PkConnection  *connection, /* IN */
                               gint           channel,    /* IN */
                               gint           pid,        /* IN */
                               GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_set_pid);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_channel_set_pid_async(connection,
	                                    channel,
	                                    pid,
	                                    NULL,
	                                    pk_connection_channel_set_pid_cb,
	                                    &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_set_pid_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_set_pid_async" RPC.
 *
 * Sets the target pid to attach to rather than spawning a process.  This can
 * only be set before the channel has started.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_set_pid_async (PkConnection        *connection,  /* IN */
                                     gint                 channel,     /* IN */
                                     gint                 pid,         /* IN */
                                     GCancellable        *cancellable, /* IN */
                                     GAsyncReadyCallback  callback,    /* IN */
                                     gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_set_pid)(connection,
	                           channel,
	                           pid,
	                           cancellable,
	                           callback,
	                           user_data);
	EXIT;
}

/**
 * pk_connection_channel_set_pid_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_set_pid_finish" RPC.
 *
 * Sets the target pid to attach to rather than spawning a process.  This can
 * only be set before the channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_set_pid_finish (PkConnection  *connection, /* IN */
                                      GAsyncResult  *result,     /* IN */
                                      GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_set_pid)(connection,
	                                 result,
	                                 error);
	RETURN(ret);
}

/**
 * pk_connection_channel_set_target_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_set_target" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_set_target_cb (GObject      *source,    /* IN */
                                     GAsyncResult *result,    /* IN */
                                     gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_set_target_finish(PK_CONNECTION(source),
	                                                       result,
	                                                       sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_set_target:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_set_target" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Sets the channels target.  This may only be set before the channel has
 * started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_set_target (PkConnection  *connection, /* IN */
                                  gint           channel,    /* IN */
                                  const gchar   *target,     /* IN */
                                  GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_set_target);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_channel_set_target_async(connection,
	                                       channel,
	                                       target,
	                                       NULL,
	                                       pk_connection_channel_set_target_cb,
	                                       &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_set_target_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_set_target_async" RPC.
 *
 * Sets the channels target.  This may only be set before the channel has
 * started.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_set_target_async (PkConnection        *connection,  /* IN */
                                        gint                 channel,     /* IN */
                                        const gchar         *target,      /* IN */
                                        GCancellable        *cancellable, /* IN */
                                        GAsyncReadyCallback  callback,    /* IN */
                                        gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_set_target)(connection,
	                              channel,
	                              target,
	                              cancellable,
	                              callback,
	                              user_data);
	EXIT;
}

/**
 * pk_connection_channel_set_target_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_set_target_finish" RPC.
 *
 * Sets the channels target.  This may only be set before the channel has
 * started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_set_target_finish (PkConnection  *connection, /* IN */
                                         GAsyncResult  *result,     /* IN */
                                         GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_set_target)(connection,
	                                    result,
	                                    error);
	RETURN(ret);
}

/**
 * pk_connection_channel_set_working_dir_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_set_working_dir" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_set_working_dir_cb (GObject      *source,    /* IN */
                                          GAsyncResult *result,    /* IN */
                                          gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_set_working_dir_finish(PK_CONNECTION(source),
	                                                            result,
	                                                            sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_set_working_dir:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_set_working_dir" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Sets the targets working directory.  This may only be set before the
 * channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_set_working_dir (PkConnection  *connection,  /* IN */
                                       gint           channel,     /* IN */
                                       const gchar   *working_dir, /* IN */
                                       GError       **error)       /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_set_working_dir);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_channel_set_working_dir_async(connection,
	                                            channel,
	                                            working_dir,
	                                            NULL,
	                                            pk_connection_channel_set_working_dir_cb,
	                                            &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_set_working_dir_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_set_working_dir_async" RPC.
 *
 * Sets the targets working directory.  This may only be set before the
 * channel has started.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_set_working_dir_async (PkConnection        *connection,  /* IN */
                                             gint                 channel,     /* IN */
                                             const gchar         *working_dir, /* IN */
                                             GCancellable        *cancellable, /* IN */
                                             GAsyncReadyCallback  callback,    /* IN */
                                             gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_set_working_dir)(connection,
	                                   channel,
	                                   working_dir,
	                                   cancellable,
	                                   callback,
	                                   user_data);
	EXIT;
}

/**
 * pk_connection_channel_set_working_dir_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_set_working_dir_finish" RPC.
 *
 * Sets the targets working directory.  This may only be set before the
 * channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_set_working_dir_finish (PkConnection  *connection, /* IN */
                                              GAsyncResult  *result,     /* IN */
                                              GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_set_working_dir)(connection,
	                                         result,
	                                         error);
	RETURN(ret);
}

/**
 * pk_connection_channel_start_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_start" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_start_cb (GObject      *source,    /* IN */
                                GAsyncResult *result,    /* IN */
                                gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_start_finish(PK_CONNECTION(source),
	                                                  result,
	                                                  sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_start:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_start" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Start the channel. If required, the process will be spawned.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_start (PkConnection  *connection, /* IN */
                             gint           channel,    /* IN */
                             GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_start);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_channel_start_async(connection,
	                                  channel,
	                                  NULL,
	                                  pk_connection_channel_start_cb,
	                                  &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_start_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_start_async" RPC.
 *
 * Start the channel. If required, the process will be spawned.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_start_async (PkConnection        *connection,  /* IN */
                                   gint                 channel,     /* IN */
                                   GCancellable        *cancellable, /* IN */
                                   GAsyncReadyCallback  callback,    /* IN */
                                   gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_start)(connection,
	                         channel,
	                         cancellable,
	                         callback,
	                         user_data);
	EXIT;
}

/**
 * pk_connection_channel_start_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_start_finish" RPC.
 *
 * Start the channel. If required, the process will be spawned.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_start_finish (PkConnection  *connection, /* IN */
                                    GAsyncResult  *result,     /* IN */
                                    GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_start)(connection,
	                               result,
	                               error);
	RETURN(ret);
}

/**
 * pk_connection_channel_stop_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_stop" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_stop_cb (GObject      *source,    /* IN */
                               GAsyncResult *result,    /* IN */
                               gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_stop_finish(PK_CONNECTION(source),
	                                                 result,
	                                                 sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_stop:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_stop" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Stop the channel. If @killpid, the inferior process is terminated.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_stop (PkConnection  *connection, /* IN */
                            gint           channel,    /* IN */
                            GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_stop);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_channel_stop_async(connection,
	                                 channel,
	                                 NULL,
	                                 pk_connection_channel_stop_cb,
	                                 &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_stop_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_stop_async" RPC.
 *
 * Stop the channel. If @killpid, the inferior process is terminated.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_stop_async (PkConnection        *connection,  /* IN */
                                  gint                 channel,     /* IN */
                                  GCancellable        *cancellable, /* IN */
                                  GAsyncReadyCallback  callback,    /* IN */
                                  gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_stop)(connection,
	                        channel,
	                        cancellable,
	                        callback,
	                        user_data);
	EXIT;
}

/**
 * pk_connection_channel_stop_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_stop_finish" RPC.
 *
 * Stop the channel. If @killpid, the inferior process is terminated.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_stop_finish (PkConnection  *connection, /* IN */
                                   GAsyncResult  *result,     /* IN */
                                   GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_stop)(connection,
	                              result,
	                              error);
	RETURN(ret);
}

/**
 * pk_connection_channel_unmute_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "channel_unmute" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_unmute_cb (GObject      *source,    /* IN */
                                 GAsyncResult *result,    /* IN */
                                 gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_channel_unmute_finish(PK_CONNECTION(source),
	                                                   result,
	                                                   sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_channel_unmute:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "channel_unmute" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Resumes delivery of manifest and samples for sources within the channel.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_unmute (PkConnection  *connection, /* IN */
                              gint           channel,    /* IN */
                              GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_unmute);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_channel_unmute_async(connection,
	                                   channel,
	                                   NULL,
	                                   pk_connection_channel_unmute_cb,
	                                   &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_channel_unmute_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "channel_unmute_async" RPC.
 *
 * Resumes delivery of manifest and samples for sources within the channel.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_unmute_async (PkConnection        *connection,  /* IN */
                                    gint                 channel,     /* IN */
                                    GCancellable        *cancellable, /* IN */
                                    GAsyncReadyCallback  callback,    /* IN */
                                    gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_unmute)(connection,
	                          channel,
	                          cancellable,
	                          callback,
	                          user_data);
	EXIT;
}

/**
 * pk_connection_channel_unmute_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "channel_unmute_finish" RPC.
 *
 * Resumes delivery of manifest and samples for sources within the channel.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_channel_unmute_finish (PkConnection  *connection, /* IN */
                                     GAsyncResult  *result,     /* IN */
                                     GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_unmute)(connection,
	                                result,
	                                error);
	RETURN(ret);
}

/**
 * pk_connection_encoder_get_plugin_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "encoder_get_plugin" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_encoder_get_plugin_cb (GObject      *source,    /* IN */
                                     GAsyncResult *result,    /* IN */
                                     gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_encoder_get_plugin_finish(PK_CONNECTION(source),
	                                                       result,
	                                                       sync->params[0],
	                                                       sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_encoder_get_plugin:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "encoder_get_plugin" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the plugin which created the encoder instance.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_encoder_get_plugin (PkConnection  *connection, /* IN */
                                  gint           encoder,    /* IN */
                                  gchar        **plugin,     /* OUT */
                                  GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(encoder_get_plugin);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = plugin;
	pk_connection_encoder_get_plugin_async(connection,
	                                       encoder,
	                                       NULL,
	                                       pk_connection_encoder_get_plugin_cb,
	                                       &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_encoder_get_plugin_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "encoder_get_plugin_async" RPC.
 *
 * Retrieves the plugin which created the encoder instance.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_encoder_get_plugin_async (PkConnection        *connection,  /* IN */
                                        gint                 encoder,     /* IN */
                                        GCancellable        *cancellable, /* IN */
                                        GAsyncReadyCallback  callback,    /* IN */
                                        gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(encoder_get_plugin)(connection,
	                              encoder,
	                              cancellable,
	                              callback,
	                              user_data);
	EXIT;
}

/**
 * pk_connection_encoder_get_plugin_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "encoder_get_plugin_finish" RPC.
 *
 * Retrieves the plugin which created the encoder instance.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_encoder_get_plugin_finish (PkConnection  *connection, /* IN */
                                         GAsyncResult  *result,     /* IN */
                                         gchar        **plugin,     /* OUT */
                                         GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, encoder_get_plugin)(connection,
	                                    result,
	                                    plugin,
	                                    error);
	RETURN(ret);
}

/**
 * pk_connection_manager_add_channel_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "manager_add_channel" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_add_channel_cb (GObject      *source,    /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_manager_add_channel_finish(PK_CONNECTION(source),
	                                                        result,
	                                                        sync->params[0],
	                                                        sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_manager_add_channel:
 * @connection: A #PkConnection.
 * @channel: (out): A location for the channel id.
 * @error: A location for a #GError or %NULL.
 *
 * Synchronous implemenation of the "manager_add_channel" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Adds a channel to the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_add_channel (PkConnection  *connection, /* IN */
                                   gint          *channel,    /* OUT */
                                   GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_add_channel);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = channel;
	pk_connection_manager_add_channel_async(connection,
	                                        NULL,
	                                        pk_connection_manager_add_channel_cb,
	                                        &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_manager_add_channel_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "manager_add_channel_async" RPC.
 *
 * Adds a channel to the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_add_channel_async (PkConnection        *connection,  /* IN */
                                         GCancellable        *cancellable, /* IN */
                                         GAsyncReadyCallback  callback,    /* IN */
                                         gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_add_channel)(connection,
	                               cancellable,
	                               callback,
	                               user_data);
	EXIT;
}

/**
 * pk_connection_manager_add_channel_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "manager_add_channel_finish" RPC.
 *
 * Adds a channel to the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_add_channel_finish (PkConnection  *connection, /* IN */
                                          GAsyncResult  *result,     /* IN */
                                          gint          *channel,    /* OUT */
                                          GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_add_channel)(connection,
	                                     result,
	                                     channel,
	                                     error);
	RETURN(ret);
}

/**
 * pk_connection_manager_add_source_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "manager_add_source" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_add_source_cb (GObject      *source,    /* IN */
                                     GAsyncResult *result,    /* IN */
                                     gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_manager_add_source_finish(PK_CONNECTION(source),
	                                                       result,
	                                                       sync->params[0],
	                                                       sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_manager_add_source:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "manager_add_source" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Create a new source from a plugin in the Agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_add_source (PkConnection  *connection, /* IN */
                                  const gchar   *plugin,     /* IN */
                                  gint          *source,     /* OUT */
                                  GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_add_source);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = source;
	pk_connection_manager_add_source_async(connection,
	                                       plugin,
	                                       NULL,
	                                       pk_connection_manager_add_source_cb,
	                                       &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_manager_add_source_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "manager_add_source_async" RPC.
 *
 * Create a new source from a plugin in the Agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_add_source_async (PkConnection        *connection,  /* IN */
                                        const gchar         *plugin,      /* IN */
                                        GCancellable        *cancellable, /* IN */
                                        GAsyncReadyCallback  callback,    /* IN */
                                        gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_add_source)(connection,
	                              plugin,
	                              cancellable,
	                              callback,
	                              user_data);
	EXIT;
}

/**
 * pk_connection_manager_add_source_finish:
 * @connection: (in): A #PkConnection.
 * @result: (in): A #GAsyncResult.
 * @source: (out): A location for the source id.
 * @error: A location for a #GError or %NULL.
 *
 * Completion of an asynchronous call to the "manager_add_source_finish" RPC.
 *
 * Create a new source from a plugin in the Agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_add_source_finish (PkConnection  *connection, /* IN */
                                         GAsyncResult  *result,     /* IN */
                                         gint          *source,     /* OUT */
                                         GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_add_source)(connection,
	                                    result,
	                                    source,
	                                    error);
	RETURN(ret);
}

/**
 * pk_connection_manager_add_subscription_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "manager_add_subscription" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_add_subscription_cb (GObject      *source,    /* IN */
                                           GAsyncResult *result,    /* IN */
                                           gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_manager_add_subscription_finish(PK_CONNECTION(source),
	                                                             result,
	                                                             sync->params[0],
	                                                             sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_manager_add_subscription:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "manager_add_subscription" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Adds a new subscription to the agent. @buffer_size is the size of the
 * internal buffer in bytes to queue before flushing data to the subscriber.
 * @timeout is the maximum number of milliseconds that should pass before
 * flushing data to the subscriber.
 * 
 * If @buffer_size and @timeout are 0, then no buffering will occur.
 * 
 * @encoder is an optional encoder that can be used to encode the data
 * into a particular format the subscriber is expecting.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_add_subscription (PkConnection  *connection,   /* IN */
                                        gsize          buffer_size,  /* IN */
                                        gsize          timeout,      /* IN */
                                        gint          *subscription, /* OUT */
                                        GError       **error)        /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_add_subscription);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = subscription;
	pk_connection_manager_add_subscription_async(connection,
	                                             buffer_size,
	                                             timeout,
	                                             NULL,
	                                             pk_connection_manager_add_subscription_cb,
	                                             &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_manager_add_subscription_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "manager_add_subscription_async" RPC.
 *
 * Adds a new subscription to the agent. @buffer_size is the size of the
 * internal buffer in bytes to queue before flushing data to the subscriber.
 * @timeout is the maximum number of milliseconds that should pass before
 * flushing data to the subscriber.
 * 
 * If @buffer_size and @timeout are 0, then no buffering will occur.
 * 
 * @encoder is an optional encoder that can be used to encode the data
 * into a particular format the subscriber is expecting.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_add_subscription_async (PkConnection        *connection,  /* IN */
                                              gsize                buffer_size, /* IN */
                                              gsize                timeout,     /* IN */
                                              GCancellable        *cancellable, /* IN */
                                              GAsyncReadyCallback  callback,    /* IN */
                                              gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_add_subscription)(connection,
	                                    buffer_size,
	                                    timeout,
	                                    cancellable,
	                                    callback,
	                                    user_data);
	EXIT;
}

/**
 * pk_connection_manager_add_subscription_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "manager_add_subscription_finish" RPC.
 *
 * Adds a new subscription to the agent. @buffer_size is the size of the
 * internal buffer in bytes to queue before flushing data to the subscriber.
 * @timeout is the maximum number of milliseconds that should pass before
 * flushing data to the subscriber.
 * 
 * If @buffer_size and @timeout are 0, then no buffering will occur.
 * 
 * @encoder is an optional encoder that can be used to encode the data
 * into a particular format the subscriber is expecting.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_add_subscription_finish (PkConnection  *connection,   /* IN */
                                               GAsyncResult  *result,       /* IN */
                                               gint          *subscription, /* OUT */
                                               GError       **error)        /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_add_subscription)(connection,
	                                          result,
	                                          subscription,
	                                          error);
	RETURN(ret);
}

/**
 * pk_connection_manager_get_channels_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "manager_get_channels" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_get_channels_cb (GObject      *source,    /* IN */
                                       GAsyncResult *result,    /* IN */
                                       gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_manager_get_channels_finish(PK_CONNECTION(source),
	                                                         result,
	                                                         sync->params[0],
	                                                         sync->params[1],
	                                                         sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_manager_get_channels:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "manager_get_channels" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the list of channels located within the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_channels (PkConnection  *connection,   /* IN */
                                    gint         **channels,     /* OUT */
                                    gsize         *channels_len, /* OUT */
                                    GError       **error)        /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_get_channels);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = channels;
	sync.params[1] = channels_len;
	pk_connection_manager_get_channels_async(connection,
	                                         NULL,
	                                         pk_connection_manager_get_channels_cb,
	                                         &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_manager_get_channels_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "manager_get_channels_async" RPC.
 *
 * Retrieves the list of channels located within the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_get_channels_async (PkConnection        *connection,  /* IN */
                                          GCancellable        *cancellable, /* IN */
                                          GAsyncReadyCallback  callback,    /* IN */
                                          gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_get_channels)(connection,
	                                cancellable,
	                                callback,
	                                user_data);
	EXIT;
}

/**
 * pk_connection_manager_get_channels_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "manager_get_channels_finish" RPC.
 *
 * Retrieves the list of channels located within the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_channels_finish (PkConnection  *connection,   /* IN */
                                           GAsyncResult  *result,       /* IN */
                                           gint         **channels,     /* OUT */
                                           gsize         *channels_len, /* OUT */
                                           GError       **error)        /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_get_channels)(connection,
	                                      result,
	                                      channels,
	                                      channels_len,
	                                      error);
	RETURN(ret);
}

/**
 * pk_connection_manager_get_hostname_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "manager_get_hostname" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_get_hostname_cb (GObject      *source,    /* IN */
                                       GAsyncResult *result,    /* IN */
                                       gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_manager_get_hostname_finish(PK_CONNECTION(source),
	                                                         result,
	                                                         sync->params[0],
	                                                         sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_manager_get_hostname:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "manager_get_hostname" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the hostname of the system on which perfkit runs.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_hostname (PkConnection  *connection, /* IN */
                                    gchar        **hostname,   /* OUT */
                                    GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_get_hostname);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = hostname;
	pk_connection_manager_get_hostname_async(connection,
	                                         NULL,
	                                         pk_connection_manager_get_hostname_cb,
	                                         &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_manager_get_hostname_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "manager_get_hostname_async" RPC.
 *
 * Retrieves the hostname of the system on which perfkit runs.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_get_hostname_async (PkConnection        *connection,  /* IN */
                                          GCancellable        *cancellable, /* IN */
                                          GAsyncReadyCallback  callback,    /* IN */
                                          gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_get_hostname)(connection,
	                                cancellable,
	                                callback,
	                                user_data);
	EXIT;
}

/**
 * pk_connection_manager_get_hostname_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "manager_get_hostname_finish" RPC.
 *
 * Retrieves the hostname of the system on which perfkit runs.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_hostname_finish (PkConnection  *connection, /* IN */
                                           GAsyncResult  *result,     /* IN */
                                           gchar        **hostname,   /* OUT */
                                           GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_get_hostname)(connection,
	                                      result,
	                                      hostname,
	                                      error);
	RETURN(ret);
}

/**
 * pk_connection_manager_get_plugins_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "manager_get_plugins" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_get_plugins_cb (GObject      *source,    /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_manager_get_plugins_finish(PK_CONNECTION(source),
	                                                        result,
	                                                        sync->params[0],
	                                                        sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_manager_get_plugins:
 * @connection: A #PkConnection.
 * @plugins: (out) (transfer full) (array zero-terminated=1) (element-type utf8):
 *   A location to store a %NULL terminated array of strings containing
 *   the names of the plugins.
 * @error: (out): A location for a #GError or %NULL.
 *
 * Synchronous implemenation of the "manager_get_plugins" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the list of available plugins within the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_plugins (PkConnection   *connection, /* IN */
                                   gchar        ***plugins,    /* OUT */
                                   GError        **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_get_plugins);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = plugins;
	pk_connection_manager_get_plugins_async(connection,
	                                        NULL,
	                                        pk_connection_manager_get_plugins_cb,
	                                        &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_manager_get_plugins_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "manager_get_plugins_async" RPC.
 *
 * Retrieves the list of available plugins within the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_get_plugins_async (PkConnection        *connection,  /* IN */
                                         GCancellable        *cancellable, /* IN */
                                         GAsyncReadyCallback  callback,    /* IN */
                                         gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_get_plugins)(connection,
	                               cancellable,
	                               callback,
	                               user_data);
	EXIT;
}

/**
 * pk_connection_manager_get_plugins_finish:
 * @connection: (in): A #PkConnection.
 * @result: (in): A #GAsyncResult.
 * @plugins: (out) (transfer full) (array zero-terminated=1) (element-type utf8):
 *   A location to store a %NULL terminated array of strings containing
 *   the names of the plugins.
 * @error: (out): A location for a #GError or %NULL.
 *
 * Completion of an asynchronous call to the "manager_get_plugins_finish" RPC.
 *
 * Retrieves the list of available plugins within the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_plugins_finish (PkConnection   *connection, /* IN */
                                          GAsyncResult   *result,     /* IN */
                                          gchar        ***plugins,    /* OUT */
                                          GError        **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_get_plugins)(connection,
	                                     result,
	                                     plugins,
	                                     error);
	RETURN(ret);
}

/**
 * pk_connection_manager_get_sources_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "manager_get_sources" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_get_sources_cb (GObject      *source,    /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_manager_get_sources_finish(PK_CONNECTION(source),
	                                                        result,
	                                                        sync->params[0],
	                                                        sync->params[1],
	                                                        sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_manager_get_sources:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "manager_get_sources" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the list of sources located within the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_sources (PkConnection  *connection,  /* IN */
                                   gint         **sources,     /* OUT */
                                   gsize         *sources_len, /* OUT */
                                   GError       **error)       /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_get_sources);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = sources;
	sync.params[1] = sources_len;
	pk_connection_manager_get_sources_async(connection,
	                                        NULL,
	                                        pk_connection_manager_get_sources_cb,
	                                        &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_manager_get_sources_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "manager_get_sources_async" RPC.
 *
 * Retrieves the list of sources located within the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_get_sources_async (PkConnection        *connection,  /* IN */
                                         GCancellable        *cancellable, /* IN */
                                         GAsyncReadyCallback  callback,    /* IN */
                                         gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_get_sources)(connection,
	                               cancellable,
	                               callback,
	                               user_data);
	EXIT;
}

/**
 * pk_connection_manager_get_sources_finish:
 * @connection: A #PkConnection.
 * @result: (in): A #GAsyncResult.
 * @sources: (out) (array length=sources_len) (element-type gint) (transfer full):
 *   A location to store the array of available sources.
 * @sources_len: (out): A location to store the number of sources.
 * @error: (out): A location to store a #GError or %NULL.
 *
 * Completion of an asynchronous call to the "manager_get_sources_finish" RPC.
 *
 * Retrieves the list of sources located within the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_sources_finish (PkConnection  *connection,  /* IN */
                                          GAsyncResult  *result,      /* IN */
                                          gint         **sources,     /* OUT */
                                          gsize         *sources_len, /* OUT */
                                          GError       **error)       /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_get_sources)(connection,
	                                     result,
	                                     sources,
	                                     sources_len,
	                                     error);
	RETURN(ret);
}

/**
 * pk_connection_manager_get_subscriptions_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "manager_get_subscriptions" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_get_subscriptions_cb (GObject      *source,    /* IN */
                                            GAsyncResult *result,    /* IN */
                                            gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_manager_get_subscriptions_finish(PK_CONNECTION(source),
	                                                              result,
	                                                              sync->params[0],
	                                                              sync->params[1],
	                                                              sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_manager_get_subscriptions:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "manager_get_subscriptions" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieve the list of subscriptions to the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_subscriptions (PkConnection  *connection,        /* IN */
                                         gint         **subscriptions,     /* OUT */
                                         gsize         *subscriptions_len, /* OUT */
                                         GError       **error)             /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_get_subscriptions);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = subscriptions;
	sync.params[1] = subscriptions_len;
	pk_connection_manager_get_subscriptions_async(connection,
	                                              NULL,
	                                              pk_connection_manager_get_subscriptions_cb,
	                                              &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_manager_get_subscriptions_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "manager_get_subscriptions_async" RPC.
 *
 * Retrieve the list of subscriptions to the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_get_subscriptions_async (PkConnection        *connection,  /* IN */
                                               GCancellable        *cancellable, /* IN */
                                               GAsyncReadyCallback  callback,    /* IN */
                                               gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_get_subscriptions)(connection,
	                                     cancellable,
	                                     callback,
	                                     user_data);
	EXIT;
}

/**
 * pk_connection_manager_get_subscriptions_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "manager_get_subscriptions_finish" RPC.
 *
 * Retrieve the list of subscriptions to the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_subscriptions_finish (PkConnection  *connection,        /* IN */
                                                GAsyncResult  *result,            /* IN */
                                                gint         **subscriptions,     /* OUT */
                                                gsize         *subscriptions_len, /* OUT */
                                                GError       **error)             /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_get_subscriptions)(connection,
	                                           result,
	                                           subscriptions,
	                                           subscriptions_len,
	                                           error);
	RETURN(ret);
}

/**
 * pk_connection_manager_get_version_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "manager_get_version" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_get_version_cb (GObject      *source,    /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_manager_get_version_finish(PK_CONNECTION(source),
	                                                        result,
	                                                        sync->params[0],
	                                                        sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_manager_get_version:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "manager_get_version" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the version of the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_version (PkConnection  *connection, /* IN */
                                   gchar        **version,    /* OUT */
                                   GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_get_version);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = version;
	pk_connection_manager_get_version_async(connection,
	                                        NULL,
	                                        pk_connection_manager_get_version_cb,
	                                        &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_manager_get_version_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "manager_get_version_async" RPC.
 *
 * Retrieves the version of the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_get_version_async (PkConnection        *connection,  /* IN */
                                         GCancellable        *cancellable, /* IN */
                                         GAsyncReadyCallback  callback,    /* IN */
                                         gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_get_version)(connection,
	                               cancellable,
	                               callback,
	                               user_data);
	EXIT;
}

/**
 * pk_connection_manager_get_version_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "manager_get_version_finish" RPC.
 *
 * Retrieves the version of the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_version_finish (PkConnection  *connection, /* IN */
                                          GAsyncResult  *result,     /* IN */
                                          gchar        **version,    /* OUT */
                                          GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_get_version)(connection,
	                                     result,
	                                     version,
	                                     error);
	RETURN(ret);
}

/**
 * pk_connection_manager_ping_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "manager_ping" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_ping_cb (GObject      *source,    /* IN */
                               GAsyncResult *result,    /* IN */
                               gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_manager_ping_finish(PK_CONNECTION(source),
	                                                 result,
	                                                 sync->params[0],
	                                                 sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_manager_ping:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "manager_ping" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Pings the agent over the RPC protocol to determine one-way latency.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_ping (PkConnection  *connection, /* IN */
                            GTimeVal      *tv,         /* OUT */
                            GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_ping);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = tv;
	pk_connection_manager_ping_async(connection,
	                                 NULL,
	                                 pk_connection_manager_ping_cb,
	                                 &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_manager_ping_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "manager_ping_async" RPC.
 *
 * Pings the agent over the RPC protocol to determine one-way latency.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_ping_async (PkConnection        *connection,  /* IN */
                                  GCancellable        *cancellable, /* IN */
                                  GAsyncReadyCallback  callback,    /* IN */
                                  gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_ping)(connection,
	                        cancellable,
	                        callback,
	                        user_data);
	EXIT;
}

/**
 * pk_connection_manager_ping_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "manager_ping_finish" RPC.
 *
 * Pings the agent over the RPC protocol to determine one-way latency.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_ping_finish (PkConnection  *connection, /* IN */
                                   GAsyncResult  *result,     /* IN */
                                   GTimeVal      *tv,         /* OUT */
                                   GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_ping)(connection,
	                              result,
	                              tv,
	                              error);
	RETURN(ret);
}

/**
 * pk_connection_manager_remove_channel_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "manager_remove_channel" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_remove_channel_cb (GObject      *source,    /* IN */
                                         GAsyncResult *result,    /* IN */
                                         gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_manager_remove_channel_finish(PK_CONNECTION(source),
	                                                           result,
	                                                           sync->params[0],
	                                                           sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_manager_remove_channel:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "manager_remove_channel" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Removes a channel from the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_remove_channel (PkConnection  *connection, /* IN */
                                      gint           channel,    /* IN */
                                      gboolean      *removed,    /* OUT */
                                      GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_remove_channel);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = removed;
	pk_connection_manager_remove_channel_async(connection,
	                                           channel,
	                                           NULL,
	                                           pk_connection_manager_remove_channel_cb,
	                                           &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_manager_remove_channel_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "manager_remove_channel_async" RPC.
 *
 * Removes a channel from the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_remove_channel_async (PkConnection        *connection,  /* IN */
                                            gint                 channel,     /* IN */
                                            GCancellable        *cancellable, /* IN */
                                            GAsyncReadyCallback  callback,    /* IN */
                                            gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_remove_channel)(connection,
	                                  channel,
	                                  cancellable,
	                                  callback,
	                                  user_data);
	EXIT;
}

/**
 * pk_connection_manager_remove_channel_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "manager_remove_channel_finish" RPC.
 *
 * Removes a channel from the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_remove_channel_finish (PkConnection  *connection, /* IN */
                                             GAsyncResult  *result,     /* IN */
                                             gboolean      *removed,    /* OUT */
                                             GError       **error)      /* OUT */
{
	gboolean ret;
	gboolean dummy;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_remove_channel)(connection,
	                                        result,
	                                        removed ? removed : &dummy,
	                                        error);
	RETURN(ret);
}

/**
 * pk_connection_manager_remove_source_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "manager_remove_source" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_remove_source_cb (GObject      *source,    /* IN */
                                        GAsyncResult *result,    /* IN */
                                        gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_manager_remove_source_finish(PK_CONNECTION(source),
	                                                          result,
	                                                          sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_manager_remove_source:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "manager_remove_source" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Remove a source from the Agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_remove_source (PkConnection  *connection, /* IN */
                                     gint           source,     /* IN */
                                     GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_remove_source);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_manager_remove_source_async(connection,
	                                          source,
	                                          NULL,
	                                          pk_connection_manager_remove_source_cb,
	                                          &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_manager_remove_source_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "manager_remove_source_async" RPC.
 *
 * Remove a source from the Agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_remove_source_async (PkConnection        *connection,  /* IN */
                                           gint                 source,      /* IN */
                                           GCancellable        *cancellable, /* IN */
                                           GAsyncReadyCallback  callback,    /* IN */
                                           gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_remove_source)(connection,
	                                 source,
	                                 cancellable,
	                                 callback,
	                                 user_data);
	EXIT;
}

/**
 * pk_connection_manager_remove_source_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "manager_remove_source_finish" RPC.
 *
 * Remove a source from the Agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_remove_source_finish (PkConnection  *connection, /* IN */
                                            GAsyncResult  *result,     /* IN */
                                            GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_remove_source)(connection,
	                                       result,
	                                       error);
	RETURN(ret);
}

/**
 * pk_connection_manager_remove_subscription_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "manager_remove_subscription" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_remove_subscription_cb (GObject      *source,    /* IN */
                                              GAsyncResult *result,    /* IN */
                                              gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_manager_remove_subscription_finish(PK_CONNECTION(source),
	                                                                result,
	                                                                sync->params[0],
	                                                                sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_manager_remove_subscription:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "manager_remove_subscription" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Removes a subscription from the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_remove_subscription (PkConnection  *connection,   /* IN */
                                           gint           subscription, /* IN */
                                           gboolean      *removed,      /* OUT */
                                           GError       **error)        /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_remove_subscription);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = removed;
	pk_connection_manager_remove_subscription_async(connection,
	                                                subscription,
	                                                NULL,
	                                                pk_connection_manager_remove_subscription_cb,
	                                                &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_manager_remove_subscription_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "manager_remove_subscription_async" RPC.
 *
 * Removes a subscription from the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_remove_subscription_async (PkConnection        *connection,   /* IN */
                                                 gint                 subscription, /* IN */
                                                 GCancellable        *cancellable,  /* IN */
                                                 GAsyncReadyCallback  callback,     /* IN */
                                                 gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_remove_subscription)(connection,
	                                       subscription,
	                                       cancellable,
	                                       callback,
	                                       user_data);
	EXIT;
}

/**
 * pk_connection_manager_remove_subscription_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "manager_remove_subscription_finish" RPC.
 *
 * Removes a subscription from the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_manager_remove_subscription_finish (PkConnection  *connection, /* IN */
                                                  GAsyncResult  *result,     /* IN */
                                                  gboolean      *removed,    /* OUT */
                                                  GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_remove_subscription)(connection,
	                                             result,
	                                             removed,
	                                             error);
	RETURN(ret);
}

/**
 * pk_connection_plugin_get_copyright_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "plugin_get_copyright" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_plugin_get_copyright_cb (GObject      *source,    /* IN */
                                       GAsyncResult *result,    /* IN */
                                       gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_plugin_get_copyright_finish(PK_CONNECTION(source),
	                                                         result,
	                                                         sync->params[0],
	                                                         sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_plugin_get_copyright:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "plugin_get_copyright" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * The plugin copyright.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_plugin_get_copyright (PkConnection  *connection, /* IN */
                                    const gchar   *plugin,     /* IN */
                                    gchar        **copyright,  /* OUT */
                                    GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(plugin_get_copyright);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = copyright;
	pk_connection_plugin_get_copyright_async(connection,
	                                         plugin,
	                                         NULL,
	                                         pk_connection_plugin_get_copyright_cb,
	                                         &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_plugin_get_copyright_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "plugin_get_copyright_async" RPC.
 *
 * The plugin copyright.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_plugin_get_copyright_async (PkConnection        *connection,  /* IN */
                                          const gchar         *plugin,      /* IN */
                                          GCancellable        *cancellable, /* IN */
                                          GAsyncReadyCallback  callback,    /* IN */
                                          gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(plugin_get_copyright)(connection,
	                                plugin,
	                                cancellable,
	                                callback,
	                                user_data);
	EXIT;
}

/**
 * pk_connection_plugin_get_copyright_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "plugin_get_copyright_finish" RPC.
 *
 * The plugin copyright.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_plugin_get_copyright_finish (PkConnection  *connection, /* IN */
                                           GAsyncResult  *result,     /* IN */
                                           gchar        **copyright,  /* OUT */
                                           GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, plugin_get_copyright)(connection,
	                                      result,
	                                      copyright,
	                                      error);
	RETURN(ret);
}

/**
 * pk_connection_plugin_get_description_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "plugin_get_description" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_plugin_get_description_cb (GObject      *source,    /* IN */
                                         GAsyncResult *result,    /* IN */
                                         gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_plugin_get_description_finish(PK_CONNECTION(source),
	                                                           result,
	                                                           sync->params[0],
	                                                           sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_plugin_get_description:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "plugin_get_description" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * The plugin description.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_plugin_get_description (PkConnection  *connection,  /* IN */
                                      const gchar   *plugin,      /* IN */
                                      gchar        **description, /* OUT */
                                      GError       **error)       /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(plugin_get_description);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = description;
	pk_connection_plugin_get_description_async(connection,
	                                           plugin,
	                                           NULL,
	                                           pk_connection_plugin_get_description_cb,
	                                           &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_plugin_get_description_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "plugin_get_description_async" RPC.
 *
 * The plugin description.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_plugin_get_description_async (PkConnection        *connection,  /* IN */
                                            const gchar         *plugin,      /* IN */
                                            GCancellable        *cancellable, /* IN */
                                            GAsyncReadyCallback  callback,    /* IN */
                                            gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(plugin_get_description)(connection,
	                                  plugin,
	                                  cancellable,
	                                  callback,
	                                  user_data);
	EXIT;
}

/**
 * pk_connection_plugin_get_description_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "plugin_get_description_finish" RPC.
 *
 * The plugin description.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_plugin_get_description_finish (PkConnection  *connection,  /* IN */
                                             GAsyncResult  *result,      /* IN */
                                             gchar        **description, /* OUT */
                                             GError       **error)       /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, plugin_get_description)(connection,
	                                        result,
	                                        description,
	                                        error);
	RETURN(ret);
}

/**
 * pk_connection_plugin_get_name_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "plugin_get_name" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_plugin_get_name_cb (GObject      *source,    /* IN */
                                  GAsyncResult *result,    /* IN */
                                  gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_plugin_get_name_finish(PK_CONNECTION(source),
	                                                    result,
	                                                    sync->params[0],
	                                                    sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_plugin_get_name:
 * @connection: A #PkConnection.
 * @plugin: (in): The plugin id.
 * @name: (out) (type utf8): A location for the plugin name.
 *
 * Synchronous implemenation of the "plugin_get_name" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * The plugin name.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_plugin_get_name (PkConnection  *connection, /* IN */
                               const gchar   *plugin,     /* IN */
                               gchar        **name,       /* OUT */
                               GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(plugin_get_name);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = name;
	pk_connection_plugin_get_name_async(connection,
	                                    plugin,
	                                    NULL,
	                                    pk_connection_plugin_get_name_cb,
	                                    &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_plugin_get_name_async:
 * @connection: A #PkConnection.
 * @plugin: (in): The plugin id.
 *
 * Asynchronous implementation of the "plugin_get_name_async" RPC.
 *
 * The plugin name.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_plugin_get_name_async (PkConnection        *connection,  /* IN */
                                     const gchar         *plugin,      /* IN */
                                     GCancellable        *cancellable, /* IN */
                                     GAsyncReadyCallback  callback,    /* IN */
                                     gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(plugin_get_name)(connection,
	                           plugin,
	                           cancellable,
	                           callback,
	                           user_data);
	EXIT;
}

/**
 * pk_connection_plugin_get_name_finish:
 * @connection: A #PkConnection.
 * @name: (out) (type utf8): A location for the plugin name.
 *
 * Completion of an asynchronous call to the "plugin_get_name_finish" RPC.
 *
 * The plugin name.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_plugin_get_name_finish (PkConnection  *connection, /* IN */
                                      GAsyncResult  *result,     /* IN */
                                      gchar        **name,       /* OUT */
                                      GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, plugin_get_name)(connection,
	                                 result,
	                                 name,
	                                 error);
	RETURN(ret);
}

/**
 * pk_connection_plugin_get_plugin_type_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "plugin_get_plugin_type" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_plugin_get_plugin_type_cb (GObject      *source,    /* IN */
                                         GAsyncResult *result,    /* IN */
                                         gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_plugin_get_plugin_type_finish(PK_CONNECTION(source),
	                                                           result,
	                                                           sync->params[0],
	                                                           sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_plugin_get_plugin_type:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "plugin_get_plugin_type" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * The plugin type.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_plugin_get_plugin_type (PkConnection  *connection, /* IN */
                                      const gchar   *plugin,     /* IN */
                                      gint          *type,       /* OUT */
                                      GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(plugin_get_plugin_type);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = type;
	pk_connection_plugin_get_plugin_type_async(connection,
	                                           plugin,
	                                           NULL,
	                                           pk_connection_plugin_get_plugin_type_cb,
	                                           &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_plugin_get_plugin_type_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "plugin_get_plugin_type_async" RPC.
 *
 * The plugin type.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_plugin_get_plugin_type_async (PkConnection        *connection,  /* IN */
                                            const gchar         *plugin,      /* IN */
                                            GCancellable        *cancellable, /* IN */
                                            GAsyncReadyCallback  callback,    /* IN */
                                            gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(plugin_get_plugin_type)(connection,
	                                  plugin,
	                                  cancellable,
	                                  callback,
	                                  user_data);
	EXIT;
}

/**
 * pk_connection_plugin_get_plugin_type_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "plugin_get_plugin_type_finish" RPC.
 *
 * The plugin type.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_plugin_get_plugin_type_finish (PkConnection  *connection, /* IN */
                                             GAsyncResult  *result,     /* IN */
                                             gint          *type,       /* OUT */
                                             GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, plugin_get_plugin_type)(connection,
	                                        result,
	                                        type,
	                                        error);
	RETURN(ret);
}

/**
 * pk_connection_plugin_get_version_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "plugin_get_version" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_plugin_get_version_cb (GObject      *source,    /* IN */
                                     GAsyncResult *result,    /* IN */
                                     gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_plugin_get_version_finish(PK_CONNECTION(source),
	                                                       result,
	                                                       sync->params[0],
	                                                       sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_plugin_get_version:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "plugin_get_version" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * The plugin version.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_plugin_get_version (PkConnection  *connection, /* IN */
                                  const gchar   *plugin,     /* IN */
                                  gchar        **version,    /* OUT */
                                  GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(plugin_get_version);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = version;
	pk_connection_plugin_get_version_async(connection,
	                                       plugin,
	                                       NULL,
	                                       pk_connection_plugin_get_version_cb,
	                                       &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_plugin_get_version_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "plugin_get_version_async" RPC.
 *
 * The plugin version.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_plugin_get_version_async (PkConnection        *connection,  /* IN */
                                        const gchar         *plugin,      /* IN */
                                        GCancellable        *cancellable, /* IN */
                                        GAsyncReadyCallback  callback,    /* IN */
                                        gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(plugin_get_version)(connection,
	                              plugin,
	                              cancellable,
	                              callback,
	                              user_data);
	EXIT;
}

/**
 * pk_connection_plugin_get_version_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "plugin_get_version_finish" RPC.
 *
 * The plugin version.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_plugin_get_version_finish (PkConnection  *connection, /* IN */
                                         GAsyncResult  *result,     /* IN */
                                         gchar        **version,    /* OUT */
                                         GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, plugin_get_version)(connection,
	                                    result,
	                                    version,
	                                    error);
	RETURN(ret);
}

/**
 * pk_connection_source_get_plugin_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "source_get_plugin" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_source_get_plugin_cb (GObject      *source,    /* IN */
                                    GAsyncResult *result,    /* IN */
                                    gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_source_get_plugin_finish(PK_CONNECTION(source),
	                                                      result,
	                                                      sync->params[0],
	                                                      sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_source_get_plugin:
 * @connection: A #PkConnection.
 * @source: (in): The source id.
 * @plugin: (out) (type utf8): A location for the plugin id.
 * @error: (out): A location for a #GError or %NULL.
 *
 * Synchronous implemenation of the "source_get_plugin" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the plugin for which the source originates.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_source_get_plugin (PkConnection  *connection, /* IN */
                                 gint           source,     /* IN */
                                 gchar        **plugin,     /* OUT */
                                 GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(source_get_plugin);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = plugin;
	pk_connection_source_get_plugin_async(connection,
	                                      source,
	                                      NULL,
	                                      pk_connection_source_get_plugin_cb,
	                                      &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_source_get_plugin_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "source_get_plugin_async" RPC.
 *
 * Retrieves the plugin for which the source originates.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_source_get_plugin_async (PkConnection        *connection,  /* IN */
                                       gint                 source,      /* IN */
                                       GCancellable        *cancellable, /* IN */
                                       GAsyncReadyCallback  callback,    /* IN */
                                       gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(source_get_plugin)(connection,
	                             source,
	                             cancellable,
	                             callback,
	                             user_data);
	EXIT;
}

/**
 * pk_connection_source_get_plugin_finish:
 * @connection: A #PkConnection.
 * @plugin: (out) (type utf8): A location for the plugin id.
 * @error: (out): A location for a #GError or %NULL.
 *
 * Completion of an asynchronous call to the "source_get_plugin_finish" RPC.
 *
 * Retrieves the plugin for which the source originates.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_source_get_plugin_finish (PkConnection  *connection, /* IN */
                                        GAsyncResult  *result,     /* IN */
                                        gchar        **plugin,     /* OUT */
                                        GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, source_get_plugin)(connection,
	                                   result,
	                                   plugin,
	                                   error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_add_channel_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "subscription_add_channel" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_add_channel_cb (GObject      *source,    /* IN */
                                           GAsyncResult *result,    /* IN */
                                           gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_subscription_add_channel_finish(PK_CONNECTION(source),
	                                                             result,
	                                                             sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_subscription_add_channel:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "subscription_add_channel" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Adds all sources of @channel to the list of sources for which manifest
 * and samples are delivered to the subscriber.
 * 
 * If @monitor is TRUE, then sources added to @channel will automatically
 * be added to the subscription.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_add_channel (PkConnection  *connection,   /* IN */
                                        gint           subscription, /* IN */
                                        gint           channel,      /* IN */
                                        gboolean       monitor,      /* IN */
                                        GError       **error)        /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_add_channel);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_subscription_add_channel_async(connection,
	                                             subscription,
	                                             channel,
	                                             monitor,
	                                             NULL,
	                                             pk_connection_subscription_add_channel_cb,
	                                             &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_subscription_add_channel_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "subscription_add_channel_async" RPC.
 *
 * Adds all sources of @channel to the list of sources for which manifest
 * and samples are delivered to the subscriber.
 * 
 * If @monitor is TRUE, then sources added to @channel will automatically
 * be added to the subscription.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_add_channel_async (PkConnection        *connection,   /* IN */
                                              gint                 subscription, /* IN */
                                              gint                 channel,      /* IN */
                                              gboolean             monitor,      /* IN */
                                              GCancellable        *cancellable,  /* IN */
                                              GAsyncReadyCallback  callback,     /* IN */
                                              gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(subscription_add_channel)(connection,
	                                    subscription,
	                                    channel,
	                                    monitor,
	                                    cancellable,
	                                    callback,
	                                    user_data);
	EXIT;
}

/**
 * pk_connection_subscription_add_channel_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "subscription_add_channel_finish" RPC.
 *
 * Adds all sources of @channel to the list of sources for which manifest
 * and samples are delivered to the subscriber.
 * 
 * If @monitor is TRUE, then sources added to @channel will automatically
 * be added to the subscription.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_add_channel_finish (PkConnection  *connection, /* IN */
                                               GAsyncResult  *result,     /* IN */
                                               GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, subscription_add_channel)(connection,
	                                          result,
	                                          error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_add_source_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "subscription_add_source" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_add_source_cb (GObject      *source,    /* IN */
                                          GAsyncResult *result,    /* IN */
                                          gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_subscription_add_source_finish(PK_CONNECTION(source),
	                                                            result,
	                                                            sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_subscription_add_source:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "subscription_add_source" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Adds @source to the list of sources for which manifest and samples are
 * delivered to the subscriber.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_add_source (PkConnection  *connection,   /* IN */
                                       gint           subscription, /* IN */
                                       gint           source,       /* IN */
                                       GError       **error)        /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_add_source);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_subscription_add_source_async(connection,
	                                            subscription,
	                                            source,
	                                            NULL,
	                                            pk_connection_subscription_add_source_cb,
	                                            &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_subscription_add_source_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "subscription_add_source_async" RPC.
 *
 * Adds @source to the list of sources for which manifest and samples are
 * delivered to the subscriber.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_add_source_async (PkConnection        *connection,   /* IN */
                                             gint                 subscription, /* IN */
                                             gint                 source,       /* IN */
                                             GCancellable        *cancellable,  /* IN */
                                             GAsyncReadyCallback  callback,     /* IN */
                                             gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(subscription_add_source)(connection,
	                                   subscription,
	                                   source,
	                                   cancellable,
	                                   callback,
	                                   user_data);
	EXIT;
}

/**
 * pk_connection_subscription_add_source_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "subscription_add_source_finish" RPC.
 *
 * Adds @source to the list of sources for which manifest and samples are
 * delivered to the subscriber.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_add_source_finish (PkConnection  *connection, /* IN */
                                              GAsyncResult  *result,     /* IN */
                                              GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, subscription_add_source)(connection,
	                                         result,
	                                         error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_get_buffer_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "subscription_get_buffer" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_get_buffer_cb (GObject      *source,    /* IN */
                                          GAsyncResult *result,    /* IN */
                                          gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_subscription_get_buffer_finish(PK_CONNECTION(source),
	                                                            result,
	                                                            sync->params[0],
	                                                            sync->params[1],
	                                                            sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_subscription_get_buffer:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "subscription_get_buffer" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the buffering timeout and maximum buffer size for the
 * subscription.  If @timeout milliseconds pass or @size bytes are
 * consumed buffering, the data will be delivered to the subscriber.
 * 
 * If @timeout and @size are 0, then no buffering will occur.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_get_buffer (PkConnection  *connection,   /* IN */
                                       gint           subscription, /* IN */
                                       gint          *timeout,      /* OUT */
                                       gint          *size,         /* OUT */
                                       GError       **error)        /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_get_buffer);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = timeout;
	sync.params[1] = size;
	pk_connection_subscription_get_buffer_async(connection,
	                                            subscription,
	                                            NULL,
	                                            pk_connection_subscription_get_buffer_cb,
	                                            &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_subscription_get_buffer_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "subscription_get_buffer_async" RPC.
 *
 * Retrieves the buffering timeout and maximum buffer size for the
 * subscription.  If @timeout milliseconds pass or @size bytes are
 * consumed buffering, the data will be delivered to the subscriber.
 * 
 * If @timeout and @size are 0, then no buffering will occur.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_get_buffer_async (PkConnection        *connection,   /* IN */
                                             gint                 subscription, /* IN */
                                             GCancellable        *cancellable,  /* IN */
                                             GAsyncReadyCallback  callback,     /* IN */
                                             gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(subscription_get_buffer)(connection,
	                                   subscription,
	                                   cancellable,
	                                   callback,
	                                   user_data);
	EXIT;
}

/**
 * pk_connection_subscription_get_buffer_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "subscription_get_buffer_finish" RPC.
 *
 * Retrieves the buffering timeout and maximum buffer size for the
 * subscription.  If @timeout milliseconds pass or @size bytes are
 * consumed buffering, the data will be delivered to the subscriber.
 * 
 * If @timeout and @size are 0, then no buffering will occur.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_get_buffer_finish (PkConnection  *connection, /* IN */
                                              GAsyncResult  *result,     /* IN */
                                              gint          *timeout,    /* OUT */
                                              gint          *size,       /* OUT */
                                              GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, subscription_get_buffer)(connection,
	                                         result,
	                                         timeout,
	                                         size,
	                                         error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_get_created_at_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "subscription_get_created_at" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_get_created_at_cb (GObject      *source,    /* IN */
                                              GAsyncResult *result,    /* IN */
                                              gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_subscription_get_created_at_finish(PK_CONNECTION(source),
	                                                                result,
	                                                                sync->params[0],
	                                                                sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_subscription_get_created_at:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "subscription_get_created_at" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the time at which the subscription was created.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_get_created_at (PkConnection  *connection,   /* IN */
                                           gint           subscription, /* IN */
                                           GTimeVal      *tv,           /* OUT */
                                           GError       **error)        /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_get_created_at);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = tv;
	pk_connection_subscription_get_created_at_async(connection,
	                                                subscription,
	                                                NULL,
	                                                pk_connection_subscription_get_created_at_cb,
	                                                &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_subscription_get_created_at_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "subscription_get_created_at_async" RPC.
 *
 * Retrieves the time at which the subscription was created.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_get_created_at_async (PkConnection        *connection,   /* IN */
                                                 gint                 subscription, /* IN */
                                                 GCancellable        *cancellable,  /* IN */
                                                 GAsyncReadyCallback  callback,     /* IN */
                                                 gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(subscription_get_created_at)(connection,
	                                       subscription,
	                                       cancellable,
	                                       callback,
	                                       user_data);
	EXIT;
}

/**
 * pk_connection_subscription_get_created_at_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "subscription_get_created_at_finish" RPC.
 *
 * Retrieves the time at which the subscription was created.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_get_created_at_finish (PkConnection  *connection, /* IN */
                                                  GAsyncResult  *result,     /* IN */
                                                  GTimeVal      *tv,         /* OUT */
                                                  GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, subscription_get_created_at)(connection,
	                                             result,
	                                             tv,
	                                             error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_get_sources_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "subscription_get_sources" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_get_sources_cb (GObject      *source,    /* IN */
                                           GAsyncResult *result,    /* IN */
                                           gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_subscription_get_sources_finish(PK_CONNECTION(source),
	                                                             result,
	                                                             sync->params[0],
	                                                             sync->params[1],
	                                                             sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_subscription_get_sources:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "subscription_get_sources" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Retrieves the list of sources which are observed by the subscription.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_get_sources (PkConnection  *connection,   /* IN */
                                        gint           subscription, /* IN */
                                        gint         **sources,      /* OUT */
                                        gsize         *sources_len,  /* OUT */
                                        GError       **error)        /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_get_sources);
	pk_connection_sync_init(&sync);
	sync.error = error;
	sync.params[0] = sources;
	sync.params[1] = sources_len;
	pk_connection_subscription_get_sources_async(connection,
	                                             subscription,
	                                             NULL,
	                                             pk_connection_subscription_get_sources_cb,
	                                             &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_subscription_get_sources_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "subscription_get_sources_async" RPC.
 *
 * Retrieves the list of sources which are observed by the subscription.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_get_sources_async (PkConnection        *connection,   /* IN */
                                              gint                 subscription, /* IN */
                                              GCancellable        *cancellable,  /* IN */
                                              GAsyncReadyCallback  callback,     /* IN */
                                              gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(subscription_get_sources)(connection,
	                                    subscription,
	                                    cancellable,
	                                    callback,
	                                    user_data);
	EXIT;
}

/**
 * pk_connection_subscription_get_sources_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "subscription_get_sources_finish" RPC.
 *
 * Retrieves the list of sources which are observed by the subscription.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_get_sources_finish (PkConnection  *connection,  /* IN */
                                               GAsyncResult  *result,      /* IN */
                                               gint         **sources,     /* OUT */
                                               gsize         *sources_len, /* OUT */
                                               GError       **error)       /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, subscription_get_sources)(connection,
	                                          result,
	                                          sources,
	                                          sources_len,
	                                          error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_mute_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "subscription_mute" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_mute_cb (GObject      *source,    /* IN */
                                    GAsyncResult *result,    /* IN */
                                    gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_subscription_mute_finish(PK_CONNECTION(source),
	                                                      result,
	                                                      sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_subscription_mute:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "subscription_mute" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Prevents the subscription from further manifest or sample delivery.  If
 * @drain is set, the current buffer will be flushed.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_mute (PkConnection  *connection,   /* IN */
                                 gint           subscription, /* IN */
                                 gboolean       drain,        /* IN */
                                 GError       **error)        /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_mute);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_subscription_mute_async(connection,
	                                      subscription,
	                                      drain,
	                                      NULL,
	                                      pk_connection_subscription_mute_cb,
	                                      &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_subscription_mute_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "subscription_mute_async" RPC.
 *
 * Prevents the subscription from further manifest or sample delivery.  If
 * @drain is set, the current buffer will be flushed.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_mute_async (PkConnection        *connection,   /* IN */
                                       gint                 subscription, /* IN */
                                       gboolean             drain,        /* IN */
                                       GCancellable        *cancellable,  /* IN */
                                       GAsyncReadyCallback  callback,     /* IN */
                                       gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(subscription_mute)(connection,
	                             subscription,
	                             drain,
	                             cancellable,
	                             callback,
	                             user_data);
	EXIT;
}

/**
 * pk_connection_subscription_mute_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "subscription_mute_finish" RPC.
 *
 * Prevents the subscription from further manifest or sample delivery.  If
 * @drain is set, the current buffer will be flushed.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_mute_finish (PkConnection  *connection, /* IN */
                                        GAsyncResult  *result,     /* IN */
                                        GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, subscription_mute)(connection,
	                                   result,
	                                   error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_remove_channel_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "subscription_remove_channel" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_remove_channel_cb (GObject      *source,    /* IN */
                                              GAsyncResult *result,    /* IN */
                                              gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_subscription_remove_channel_finish(PK_CONNECTION(source),
	                                                                result,
	                                                                sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_subscription_remove_channel:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "subscription_remove_channel" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Removes @channel and all of its sources from the subscription.  This
 * prevents further manifest and sample delivery to the subscriber.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_remove_channel (PkConnection  *connection,   /* IN */
                                           gint           subscription, /* IN */
                                           gint           channel,      /* IN */
                                           GError       **error)        /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_remove_channel);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_subscription_remove_channel_async(connection,
	                                                subscription,
	                                                channel,
	                                                NULL,
	                                                pk_connection_subscription_remove_channel_cb,
	                                                &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_subscription_remove_channel_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "subscription_remove_channel_async" RPC.
 *
 * Removes @channel and all of its sources from the subscription.  This
 * prevents further manifest and sample delivery to the subscriber.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_remove_channel_async (PkConnection        *connection,   /* IN */
                                                 gint                 subscription, /* IN */
                                                 gint                 channel,      /* IN */
                                                 GCancellable        *cancellable,  /* IN */
                                                 GAsyncReadyCallback  callback,     /* IN */
                                                 gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(subscription_remove_channel)(connection,
	                                       subscription,
	                                       channel,
	                                       cancellable,
	                                       callback,
	                                       user_data);
	EXIT;
}

/**
 * pk_connection_subscription_remove_channel_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "subscription_remove_channel_finish" RPC.
 *
 * Removes @channel and all of its sources from the subscription.  This
 * prevents further manifest and sample delivery to the subscriber.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_remove_channel_finish (PkConnection  *connection, /* IN */
                                                  GAsyncResult  *result,     /* IN */
                                                  GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, subscription_remove_channel)(connection,
	                                             result,
	                                             error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_remove_source_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "subscription_remove_source" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_remove_source_cb (GObject      *source,    /* IN */
                                             GAsyncResult *result,    /* IN */
                                             gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_subscription_remove_source_finish(PK_CONNECTION(source),
	                                                               result,
	                                                               sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_subscription_remove_source:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "subscription_remove_source" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Removes @source from the subscription.  This prevents further manifest
 * and sample delivery to the subscriber.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_remove_source (PkConnection  *connection,   /* IN */
                                          gint           subscription, /* IN */
                                          gint           source,       /* IN */
                                          GError       **error)        /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_remove_source);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_subscription_remove_source_async(connection,
	                                               subscription,
	                                               source,
	                                               NULL,
	                                               pk_connection_subscription_remove_source_cb,
	                                               &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_subscription_remove_source_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "subscription_remove_source_async" RPC.
 *
 * Removes @source from the subscription.  This prevents further manifest
 * and sample delivery to the subscriber.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_remove_source_async (PkConnection        *connection,   /* IN */
                                                gint                 subscription, /* IN */
                                                gint                 source,       /* IN */
                                                GCancellable        *cancellable,  /* IN */
                                                GAsyncReadyCallback  callback,     /* IN */
                                                gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(subscription_remove_source)(connection,
	                                      subscription,
	                                      source,
	                                      cancellable,
	                                      callback,
	                                      user_data);
	EXIT;
}

/**
 * pk_connection_subscription_remove_source_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "subscription_remove_source_finish" RPC.
 *
 * Removes @source from the subscription.  This prevents further manifest
 * and sample delivery to the subscriber.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_remove_source_finish (PkConnection  *connection, /* IN */
                                                 GAsyncResult  *result,     /* IN */
                                                 GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, subscription_remove_source)(connection,
	                                            result,
	                                            error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_set_buffer_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "subscription_set_buffer" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_set_buffer_cb (GObject      *source,    /* IN */
                                          GAsyncResult *result,    /* IN */
                                          gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_subscription_set_buffer_finish(PK_CONNECTION(source),
	                                                            result,
	                                                            sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_subscription_set_buffer:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "subscription_set_buffer" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Sets the buffering timeout and maximum buffer size for the subscription.
 * If @timeout milliseconds pass or @size bytes are consummed buffering,
 * the data will be delivered to the subscriber.
 * 
 * Set @timeout and @size to 0 to disable buffering.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_set_buffer (PkConnection  *connection,   /* IN */
                                       gint           subscription, /* IN */
                                       gint           timeout,      /* IN */
                                       gint           size,         /* IN */
                                       GError       **error)        /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_set_buffer);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_subscription_set_buffer_async(connection,
	                                            subscription,
	                                            timeout,
	                                            size,
	                                            NULL,
	                                            pk_connection_subscription_set_buffer_cb,
	                                            &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_subscription_set_buffer_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "subscription_set_buffer_async" RPC.
 *
 * Sets the buffering timeout and maximum buffer size for the subscription.
 * If @timeout milliseconds pass or @size bytes are consummed buffering,
 * the data will be delivered to the subscriber.
 * 
 * Set @timeout and @size to 0 to disable buffering.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_set_buffer_async (PkConnection        *connection,   /* IN */
                                             gint                 subscription, /* IN */
                                             gint                 timeout,      /* IN */
                                             gint                 size,         /* IN */
                                             GCancellable        *cancellable,  /* IN */
                                             GAsyncReadyCallback  callback,     /* IN */
                                             gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(subscription_set_buffer)(connection,
	                                   subscription,
	                                   timeout,
	                                   size,
	                                   cancellable,
	                                   callback,
	                                   user_data);
	EXIT;
}

/**
 * pk_connection_subscription_set_buffer_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "subscription_set_buffer_finish" RPC.
 *
 * Sets the buffering timeout and maximum buffer size for the subscription.
 * If @timeout milliseconds pass or @size bytes are consummed buffering,
 * the data will be delivered to the subscriber.
 * 
 * Set @timeout and @size to 0 to disable buffering.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_set_buffer_finish (PkConnection  *connection, /* IN */
                                              GAsyncResult  *result,     /* IN */
                                              GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, subscription_set_buffer)(connection,
	                                         result,
	                                         error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_set_encoder_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "subscription_set_encoder" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_set_encoder_cb (GObject      *source,    /* IN */
                                           GAsyncResult *result,    /* IN */
                                           gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_subscription_set_encoder_finish(PK_CONNECTION(source),
	                                                             result,
	                                                             sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_subscription_set_encoder:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "subscription_set_encoder" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Sets the encoder to use on the output buffers.  Data will be encoded
 * using this encoder before sending to the client.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_set_encoder (PkConnection  *connection,   /* IN */
                                        gint           subscription, /* IN */
                                        gint           encoder,      /* IN */
                                        GError       **error)        /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_set_encoder);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_subscription_set_encoder_async(connection,
	                                             subscription,
	                                             encoder,
	                                             NULL,
	                                             pk_connection_subscription_set_encoder_cb,
	                                             &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_subscription_set_encoder_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "subscription_set_encoder_async" RPC.
 *
 * Sets the encoder to use on the output buffers.  Data will be encoded
 * using this encoder before sending to the client.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_set_encoder_async (PkConnection        *connection,   /* IN */
                                              gint                 subscription, /* IN */
                                              gint                 encoder,      /* IN */
                                              GCancellable        *cancellable,  /* IN */
                                              GAsyncReadyCallback  callback,     /* IN */
                                              gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(subscription_set_encoder)(connection,
	                                    subscription,
	                                    encoder,
	                                    cancellable,
	                                    callback,
	                                    user_data);
	EXIT;
}

/**
 * pk_connection_subscription_set_encoder_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "subscription_set_encoder_finish" RPC.
 *
 * Sets the encoder to use on the output buffers.  Data will be encoded
 * using this encoder before sending to the client.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_set_encoder_finish (PkConnection  *connection, /* IN */
                                               GAsyncResult  *result,     /* IN */
                                               GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, subscription_set_encoder)(connection,
	                                          result,
	                                          error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_set_handlers_cb:
 * @source: A #PkConnection.
 *
 * Callback for the synchronous implementation of the
 * "subscription_set_handlers" RPC.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_set_handlers_cb (GObject      *source,    /* IN */
                                            GAsyncResult *result,    /* IN */
                                            gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_subscription_set_handlers_finish(PK_CONNECTION(source),
	                                                              result,
	                                                              sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_subscription_set_handlers:
 * @connection: A #PkConnection.
 *
 * Synchronous call to set the handler callbacks for a subscription.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_set_handlers (PkConnection    *connection,       /* IN */
                                         gint             subscription,     /* IN */
                                         PkManifestFunc   manifest_func,    /* IN */
                                         gpointer         manifest_data,    /* IN */
                                         GDestroyNotify   manifest_destroy, /* IN */
                                         PkSampleFunc     sample_func,      /* IN */
                                         gpointer         sample_data,      /* IN */
                                         GDestroyNotify   sample_destroy,   /* IN */
                                         GError         **error)            /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_set_handlers);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_subscription_set_handlers_async(connection,
	                                              subscription,
	                                              manifest_func,
	                                              manifest_data,
	                                              manifest_destroy,
	                                              sample_func,
	                                              sample_data,
	                                              sample_destroy,
	                                              NULL,
	                                              pk_connection_subscription_set_handlers_cb,
	                                              &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_subscription_set_handlers_async:
 * @connection: A #PkConnection.
 *
 * Asynchronously requests the "subscription_set_handlers" RPC.
 *
 * Sets the callback functions for handling incoming data from for a
 * subscription.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_set_handlers_async (PkConnection        *connection,       /* IN */
                                               gint                 subscription,     /* IN */
                                               PkManifestFunc       manifest_func,    /* IN */
                                               gpointer             manifest_data,    /* IN */
                                               GDestroyNotify       manifest_destroy, /* IN */
                                               PkSampleFunc         sample_func,      /* IN */
                                               gpointer             sample_data,      /* IN */
                                               GDestroyNotify       sample_destroy,   /* IN */
                                               GCancellable        *cancellable,      /* IN */
                                               GAsyncReadyCallback  callback,         /* IN */
                                               gpointer             user_data)        /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);
	g_return_if_fail(manifest_func != NULL);
	g_return_if_fail(sample_func != NULL);

	ENTRY;
	RPC_ASYNC(subscription_set_handlers)(connection,
	                                     subscription,
	                                     manifest_func,
	                                     manifest_data,
	                                     manifest_destroy,
	                                     sample_func,
	                                     sample_data,
	                                     sample_destroy,
	                                     cancellable,
	                                     callback,
	                                     user_data);
	EXIT;
}

/**
 * pk_connection_subscription_set_handlers_finish:
 * @connection: A #PkConnection.
 *
 * Completes an asynchronous request to the "subscription_set_handlers" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_set_handlers_finish (PkConnection  *connection, /* IN */
                                                GAsyncResult  *result,     /* IN */
                                                GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, subscription_set_handlers)(connection, result, error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_unmute_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #GAsyncResult.
 *
 * Callback to notify a synchronous call to the "subscription_unmute" RPC that it
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_unmute_cb (GObject      *source,    /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(sync != NULL);

	ENTRY;
	sync->result = pk_connection_subscription_unmute_finish(PK_CONNECTION(source),
	                                                        result,
	                                                        sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_subscription_unmute:
 * @connection: A #PkConnection.
 *
 * Synchronous implemenation of the "subscription_unmute" RPC.  Using
 * synchronous RPCs is generally frowned upon.
 *
 * Enables the subscription for manifest and sample delivery.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_unmute (PkConnection  *connection,   /* IN */
                                   gint           subscription, /* IN */
                                   GError       **error)        /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_unmute);
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_subscription_unmute_async(connection,
	                                        subscription,
	                                        NULL,
	                                        pk_connection_subscription_unmute_cb,
	                                        &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_subscription_unmute_async:
 * @connection: A #PkConnection.
 *
 * Asynchronous implementation of the "subscription_unmute_async" RPC.
 *
 * Enables the subscription for manifest and sample delivery.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_unmute_async (PkConnection        *connection,   /* IN */
                                         gint                 subscription, /* IN */
                                         GCancellable        *cancellable,  /* IN */
                                         GAsyncReadyCallback  callback,     /* IN */
                                         gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(subscription_unmute)(connection,
	                               subscription,
	                               cancellable,
	                               callback,
	                               user_data);
	EXIT;
}

/**
 * pk_connection_subscription_unmute_finish:
 * @connection: A #PkConnection.
 *
 * Completion of an asynchronous call to the "subscription_unmute_finish" RPC.
 *
 * Enables the subscription for manifest and sample delivery.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_unmute_finish (PkConnection  *connection, /* IN */
                                          GAsyncResult  *result,     /* IN */
                                          GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, subscription_unmute)(connection,
	                                     result,
	                                     error);
	RETURN(ret);
}

/**
 * pk_connection_connect_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests a connection to the destination.  @callback will
 * be called when the connection has completed or failed.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_connect_async (PkConnection        *connection,  /* IN */
                             GCancellable        *cancellable, /* IN */
                             GAsyncReadyCallback  callback,    /* IN */
                             gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(connect)(connection, cancellable, callback, user_data);
	EXIT;
}

/**
 * pk_connection_connect_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError or %NULL.
 *
 * Asynchronously completes a request to pk_connection_connect_async().
 *
 * Returns: %TRUE if the connection was successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_connect_finish (PkConnection  *connection, /* IN */
                              GAsyncResult  *result,     /* IN */
                              GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, connect) (connection, result, error);
	RETURN(ret);
}

/**
 * pk_connection_connect_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #PkConnectionSync.
 *
 * Asynchronous callback from pk_connection_connect().
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_connect_cb (GObject      *source,    /* IN */
                          GAsyncResult *result,    /* IN */
                          gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	sync->result = pk_connection_connect_finish(PK_CONNECTION(source),
	                                            result, sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_connect:
 * @connection: A #PkConnection.
 * @error: A location for a #GError or %NULL.
 *
 * Synchronously connects to the connection's destination.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_connect (PkConnection  *connection, /* IN */
                       GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_connect_async(connection, NULL,
	                            pk_connection_connect_cb,
	                            &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_disconnect_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously disconnect the connection. @callback must call
 * pk_connection_disconnect_finish().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_disconnect_async (PkConnection        *connection,  /* IN */
                                GCancellable        *cancellable, /* IN */
                                GAsyncReadyCallback  callback,    /* IN */
                                gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(disconnect)(connection, cancellable, callback, user_data);
	EXIT;
}

/**
 * pk_connection_disconnect_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request to disconnect @connection.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pk_connection_disconnect_finish (PkConnection  *connection, /* IN */
                                 GAsyncResult  *result,     /* IN */
                                 GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, disconnect) (connection, result, error);
	RETURN(ret);
}

/**
 * pk_connection_disconnect_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: A #PkConnectionSync.
 *
 * Callback to notify a synchronous call to disconnect that the operation
 * has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_disconnect_cb (GObject      *source,    /* IN */
                             GAsyncResult *result,    /* IN */
                             gpointer      user_data) /* IN */
{
	PkConnectionSync *sync = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	sync->result = pk_connection_disconnect_finish(PK_CONNECTION(source),
	                                               result, sync->error);
	pk_connection_sync_signal(sync);
	EXIT;
}

/**
 * pk_connection_disconnect:
 * @connection: A #PkConnection.
 * @error: A location for a #GError or %NULL.
 *
 * Synchronously disconnects from the connection's destination.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_disconnect (PkConnection  *connection, /* IN */
                          GError       **error)      /* OUT */
{
	PkConnectionSync sync;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	pk_connection_sync_init(&sync);
	sync.error = error;
	pk_connection_disconnect_async(connection, NULL,
	                               pk_connection_disconnect_cb,
	                               &sync);
	pk_connection_sync_wait(&sync);
	pk_connection_sync_destroy(&sync);
	RETURN(sync.result);
}

/**
 * pk_connection_get_uri:
 * @connection: A #PkConnection.
 *
 * Retrieves the URI for the connection.
 *
 * Returns: A string which should not be modified or freed.
 * Side effects: None.
 */
const gchar *
pk_connection_get_uri (PkConnection *connection) /* IN */
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), NULL);
	return connection->priv->uri;
}

/**
 * pk_connection_get_manager:
 * @connection: A #PkConnection.
 *
 * Retrieves the "Manager" object for the connection.  This can be used as
 * a starting point for the Object Model API.
 *
 * Returns: A #PkManager instance.
 * Side effects: None.
 */
PkManager*
pk_connection_get_manager (PkConnection *connection) /* IN */
{
	PkConnectionPrivate *priv;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), NULL);

	ENTRY;
	priv = connection->priv;
	g_static_rw_lock_writer_lock(&priv->rw_lock);
	if (!priv->manager) {
		priv->manager = g_object_new(PK_TYPE_MANAGER,
		                             "connection", connection,
		                             NULL);
	}
	g_static_rw_lock_writer_unlock(&priv->rw_lock);
	RETURN(priv->manager);
}

/**
 * pk_connection_error_quark:
 *
 * The #PkConnection error quark.
 *
 * Returns: A #GQuark for the #PkConnection error domain.
 * Side effects: The error quark is registered if needed.
 */
GQuark
pk_connection_error_quark (void)
{
	return g_quark_from_static_string("pk-connection-error-quark");
}

/**
 * pk_connection_get_protocol_plugin_path:
 *
 * Retrieves the path to where a protocol plugin should exist.
 *
 * Returns: A string which should be freed with g_free().
 * Side effects: None.
 */
static inline gchar*
pk_connection_get_protocol_plugin_path (const gchar *protocol) /* IN */
{
	gchar *plugin_dir;
	gchar *path;

	if (g_getenv("PERFKIT_CONNECTIONS_DIR") != NULL) {
		plugin_dir = g_build_filename(g_getenv("PERFKIT_CONNECTIONS_DIR"), NULL);
	}
	else {
		plugin_dir = g_build_filename(PACKAGE_LIB_DIR, "perfkit",
		                              "connections", NULL);
	}

	path = g_module_build_path(plugin_dir, protocol);
	g_free(plugin_dir);

	return path;
}

/**
 * pk_connection_get_protocol_type:
 * @protocol: The protocol portion of a URI
 *
 * Retrieves the #GType of a #PkConnection implementation for the given
 * protocol.
 *
 * Returns: A valid #GType if successful; otherwise G_TYPE_INVALID.
 * Side effects: A protocol plugin may be loaded.
 */
static inline GType
pk_connection_get_protocol_type (const gchar *protocol) /* IN */
{
	GType (*register_func) (void) = NULL;
	gpointer protocol_ptr;
	GType protocol_type;
	GModule *module = NULL;
	gchar *path;
	gint i;

	/*
	 * Make sure the lookup hash table exists.
	 */
	if (G_UNLIKELY(g_once_init_enter(&protocol_init))) {
		protocol_types = g_hash_table_new_full(g_str_hash, g_str_equal,
		                                       g_free, NULL);
		g_once_init_leave(&protocol_init, TRUE);
	}

	/*
	 * Handle NULL protocols.
	 */
	if (protocol == NULL) {
		goto invalid_protocol;
	}

	/*
	 * Ensure valid characters in the protocol.
	 */
	for (i = 0; protocol[i]; i++) {
		if (protocol[i] < 'A' || protocol[i] > 'z') {
			goto invalid_protocol;
		}
	}

	/*
	 * Check to see if the protocol has been loaded already.
	 */
	 g_static_mutex_lock(&protocol_mutex);
	 protocol_ptr = g_hash_table_lookup(protocol_types, protocol);
	 protocol_type = GPOINTER_TO_INT(protocol_ptr);
	 if (G_UNLIKELY(!protocol_type)) {
	 	path = pk_connection_get_protocol_plugin_path(protocol);

	 	/*
	 	 * Check to see if the protocol plugin exists and is executable.
	 	 */
		if (!g_file_test(path, G_FILE_TEST_IS_EXECUTABLE)) {
			goto cleanup;
		}

		/*
		 * Load the executable module.
		 */
		module = g_module_open(path, G_MODULE_BIND_LAZY);
		if (!module) {
			g_warning("%s: Could not load protocol: %s",
			          G_STRFUNC, g_module_error());
			goto cleanup;
		}

		/*
		 * Retrieve the plugin symbol.
		 */
		if (!g_module_symbol(module, "pk_connection_register",
		                    (gpointer *)&register_func)) {
			g_warning("%s: Module %s is missing pk_connection_register",
			          G_STRFUNC, path);
			goto cleanup;
		}

		/*
		 * Make sure the valid symbol is not NULL.
		 */
		if (!register_func) {
			g_warning("%s: Module %s contains a NULL pk_connection_register",
			          G_STRFUNC, path);
			goto cleanup;
		}

		/*
		 * Retrieve the GType from the registration method and make sure
		 * the module is never unloaded.
		 */
		protocol_type = register_func();
		g_module_make_resident(module);

		/*
		 * Store the type for future lookup.
		 */
	 	g_hash_table_insert(protocol_types, g_strdup(protocol),
	 	                    GINT_TO_POINTER(protocol_type));

	cleanup:
		g_free(path);
		if (module) {
			g_module_close(module);
		}
	}
	g_static_mutex_unlock(&protocol_mutex);

	return protocol_type;

invalid_protocol:
	g_warning("%s: Invalid protocol: %s", G_STRFUNC, protocol);
	return G_TYPE_INVALID;
}

/**
 * pk_connection_new_from_uri:
 * @uri: A uri to the destination.
 *
 * Creates a new instance of #PkConnection using the connection protocol
 * specified in @uri.
 *
 * Returns: A new #PkConnection instance if successful; otherwise %NULL.
 * Side effects: A protocol plugin may be loaded.
 */
PkConnection*
pk_connection_new_from_uri (const gchar *uri) /* IN */
{
	PkConnection *connection = NULL;
	gchar protocol[32] = { 0 };
	GType protocol_type;
	gboolean done = FALSE;
	gint i;

	g_return_val_if_fail(uri != NULL, NULL);

	/*
	 * Determine protocol handler for uri.
	 */
	for (i = 0; uri[i] && i < (sizeof(protocol) - 1); i++) {
		switch (uri[i]) {
		case ':':
			done = TRUE;
			break;
		default:
			protocol[i] = uri[i];
			break;
		}
		if (done) {
			break;
		}
	}
	protocol[i++] = '\0';

	/*
	 * Lookup protocol implementation by name.
	 */
	protocol_type = pk_connection_get_protocol_type(protocol);
	if (protocol_type == G_TYPE_INVALID) {
		return NULL;
	}

	/*
	 * Create instance of PkConnection and pass in the uri.
	 */
	connection = g_object_new(protocol_type, "uri", uri, NULL);

	return connection;
}

/**
 * pk_connection_is_connected:
 * @connection: A #PkConnection.
 *
 * Checks if @connection is currently connected to a destination.
 *
 * Returns: %TRUE if connected; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_is_connected (PkConnection *connection) /* IN */
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);
	return (connection->priv->state == PK_CONNECTION_CONNECTED);
}

/**
 * pk_connection_hash:
 * @connection: A #PkConnection.
 *
 * Converts a #PkConnection to a hash value.  It can be passed
 * to g_hash_table_new() as the hash_func parameter, when using
 * PkConnection<!-- -->'s as keys in a #GHashTable.
 *
 * Returns: a hash value corresponding to the key
 * Side effects: None.
 */
guint
pk_connection_hash (PkConnection *connection) /* IN */
{
	return g_str_hash(connection->priv->uri);
}

/**
 * pk_connection_emit_state_changed:
 * @connection: A #PkConnection.
 * @state: The new #PkConnectionState.
 *
 * Sets the current state of the connection and emits the
 * "state-changed" signal.
 *
 * Returns: None.
 * Side effects: Current state is set and signal observers notified.
 */
void
pk_connection_emit_state_changed (PkConnection      *connection, /* IN */
                                  PkConnectionState  state)      /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	connection->priv->state = state;
	g_signal_emit(connection, signals[STATE_CHANGED], 0, state);
}

void
pk_connection_emit_channel_added (PkConnection *connection, /* IN */
                                  gint          channel)    /* IN */
{
	ENTRY;
	g_signal_emit(connection, signals[CHANNEL_ADDED], 0, channel);
	EXIT;
}

void
pk_connection_emit_channel_removed (PkConnection *connection, /* IN */
                                    gint          channel)    /* IN */
{
	ENTRY;
	g_signal_emit(connection, signals[CHANNEL_REMOVED], 0, channel);
	EXIT;
}

void
pk_connection_emit_source_added (PkConnection *connection, /* IN */
                                 gint          source)     /* IN */
{
	ENTRY;
	g_signal_emit(connection, signals[SOURCE_ADDED], 0, source);
	EXIT;
}

void
pk_connection_emit_source_removed (PkConnection *connection, /* IN */
                                   gint          source)    /* IN */
{
	ENTRY;
	g_signal_emit(connection, signals[SOURCE_REMOVED], 0, source);
	EXIT;
}

void
pk_connection_emit_subscription_added (PkConnection *connection,   /* IN */
                                       gint          subscription) /* IN */
{
	ENTRY;
	g_signal_emit(connection, signals[SUBSCRIPTION_ADDED], 0, subscription);
	EXIT;
}

void
pk_connection_emit_subscription_removed (PkConnection *connection,   /* IN */
                                         gint          subscription) /* IN */
{
	ENTRY;
	g_signal_emit(connection, signals[SUBSCRIPTION_REMOVED], 0, subscription);
	EXIT;
}

void
pk_connection_emit_plugin_added (PkConnection *connection, /* IN */
                                 const gchar  *plugin)     /* IN */
{
	ENTRY;
	g_signal_emit(connection, signals[PLUGIN_ADDED], 0, plugin);
	EXIT;
}

void
pk_connection_emit_plugin_removed (PkConnection *connection, /* IN */
                                   const gchar  *plugin)     /* IN */
{
	ENTRY;
	g_signal_emit(connection, signals[PLUGIN_REMOVED], 0, plugin);
	EXIT;
}

void
pk_connection_emit_encoder_added (PkConnection *connection, /* IN */
                                  gint          encoder)    /* IN */
{
	ENTRY;
	g_signal_emit(connection, signals[ENCODER_ADDED], 0, encoder);
	EXIT;
}

void
pk_connection_emit_encoder_removed (PkConnection *connection, /* IN */
                                    gint          encoder)    /* IN */
{
	ENTRY;
	g_signal_emit(connection, signals[ENCODER_REMOVED], 0, encoder);
	EXIT;
}

/**
 * pk_connection_get_property:
 * @object: A #PkConnection.
 * @prop_id: The registered property identifier.
 * @value: A #GValue to store the resulting value.
 * @pspec: The registered #GParamSpec.
 *
 * Gets the value for the given property denoted by @prop_id.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_get_property (GObject    *object,   /* IN */
                            guint       prop_id,  /* IN */
                            GValue     *value,    /* IN */
                            GParamSpec *pspec)    /* IN */
{
	switch (prop_id) {
	case PROP_URI:
		g_value_set_string(value,
		                   pk_connection_get_uri(PK_CONNECTION(object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * pk_connection_set_property:
 * @object: A #PkConnection.
 * @prop_id: The registered property identifier.
 * @value: A #GValue containing the new property value.
 * @pspec: The registered #GParamSpec.
 *
 * Sets the value for the given property denoted by @Prop_id.
 *
 * Returns: None.
 * Side effects: Varies by property.
 */
static void
pk_connection_set_property (GObject      *object,  /* IN */
                            guint         prop_id, /* IN */
                            const GValue *value,   /* IN */
                            GParamSpec   *pspec)   /* IN */
{
	switch (prop_id) {
	case PROP_URI:
		PK_CONNECTION(object)->priv->uri = g_value_dup_string(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * pk_connection_finalize:
 * @object: A #PkConnection.
 *
 * Releases all memory allocated by the object.
 *
 * Returns: None.
 * Side effects: All object allocations.
 */
static void
pk_connection_finalize (GObject *object) /* IN */
{
	PkConnectionPrivate *priv = PK_CONNECTION(object)->priv;

	g_free(priv->uri);

	G_OBJECT_CLASS(pk_connection_parent_class)->finalize(object);
}

/**
 * pk_connection_class_init:
 * @klass: A #PkConnectionClass.
 *
 * Initializes the class vtable, installs properties and signals.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_class_init (PkConnectionClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pk_connection_finalize;
	object_class->set_property = pk_connection_set_property;
	object_class->get_property = pk_connection_get_property;
	g_type_class_add_private(object_class, sizeof(PkConnectionPrivate));

	/**
	 * PkConnection:uri:
	 *
	 * The "uri" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_URI,
	                                g_param_spec_string("uri",
	                                                    "Uri",
	                                                    "The connection's URI.",
	                                                    NULL,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * PkConnection::state-changed:
	 * @state: The new PkConnectionState.
	 *
	 * The "state-changed" signal.  This signal is emitted when the connection
	 * state has changed.
	 *
	 * See pk_connection_emit_state_changed().
	 */
	signals[STATE_CHANGED] = g_signal_new("state-changed",
	                                      PK_TYPE_CONNECTION,
	                                      G_SIGNAL_RUN_FIRST,
	                                      0, NULL, NULL,
	                                      g_cclosure_marshal_VOID__UINT,
	                                      G_TYPE_NONE, 1, G_TYPE_UINT);

	#define ADD_SIGNAL_INT(_e, _n)                                   \
		signals[_e] = g_signal_new(_n,                               \
	                               PK_TYPE_CONNECTION,               \
	                               G_SIGNAL_RUN_FIRST,               \
	                               0, NULL, NULL,                    \
	                               g_cclosure_marshal_VOID__INT,     \
	                               G_TYPE_NONE, 1, G_TYPE_INT)

	#define ADD_SIGNAL_STRING(_e, _n)                                \
		signals[_e] = g_signal_new(_n,                               \
	                               PK_TYPE_CONNECTION,               \
	                               G_SIGNAL_RUN_FIRST,               \
	                               0, NULL, NULL,                    \
	                               g_cclosure_marshal_VOID__STRING,  \
	                               G_TYPE_NONE, 1, G_TYPE_STRING)

	ADD_SIGNAL_INT(CHANNEL_ADDED, "channel-added");
	ADD_SIGNAL_INT(CHANNEL_REMOVED, "channel-removed");
	ADD_SIGNAL_INT(SOURCE_ADDED, "source-added");
	ADD_SIGNAL_INT(SOURCE_REMOVED, "source-removed");
	ADD_SIGNAL_INT(SUBSCRIPTION_ADDED, "subscription-added");
	ADD_SIGNAL_INT(SUBSCRIPTION_REMOVED, "subscription-removed");
	ADD_SIGNAL_INT(ENCODER_ADDED, "encoder-added");
	ADD_SIGNAL_INT(ENCODER_REMOVED, "encoder-removed");
	ADD_SIGNAL_STRING(PLUGIN_ADDED, "plugin-added");
	ADD_SIGNAL_STRING(PLUGIN_REMOVED, "plugin-removed");
}

/**
 * pk_connection_init:
 * @connection: A #PkConnection.
 *
 * Initializes a newly created instance of #PkConnection.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_init (PkConnection *connection) /* IN */
{
	connection->priv = G_TYPE_INSTANCE_GET_PRIVATE(connection,
	                                               PK_TYPE_CONNECTION,
	                                               PkConnectionPrivate);
}
