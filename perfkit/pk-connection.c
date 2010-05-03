/* pk-connection.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdio.h>
#include <gmodule.h>

#include "pk-connection.h"
#include "pk-connection-lowlevel.h"

/**
 * SECTION:pk-connection
 * @title: PkConnection
 * @short_description:
 *
 * 
 */

G_DEFINE_TYPE(PkConnection, pk_connection, G_TYPE_OBJECT)

struct _PkConnectionPrivate
{
	gchar             *uri;
	PkConnectionState  state;
};

typedef struct
{
	GMutex    *mutex;
	GCond     *cond;
	gboolean   result;
	GError   **error;
	gpointer   params[16];
} PkConnectionSync;

enum
{
	PROP_0,
	PROP_URI,
};

enum
{
	STATE_CHANGED,
	LAST_SIGNAL
};

static guint         signals[LAST_SIGNAL] = { 0 };
static GStaticMutex  protocol_mutex       = G_STATIC_MUTEX_INIT;
static GHashTable   *protocol_types       = NULL;

#define ENTRY G_STMT_START {                                       \
        g_debug("ENTRY: %s():%d", G_STRFUNC, __LINE__);            \
    } G_STMT_END

#define EXIT G_STMT_START {                                        \
        g_debug("EXIT:  %s():%d", G_STRFUNC, __LINE__);            \
        return;                                                    \
    } G_STMT_END

#define RETURN(_r) G_STMT_START {                                  \
        g_debug("EXIT:  %s():%d", G_STRFUNC, __LINE__);            \
        return _r;                                                 \
    } G_STMT_END

#define RPC_ASYNC(_n)                                              \
    if (!PK_CONNECTION_GET_CLASS(connection)->_n##_async) {        \
        g_warning("%s does not support the #_n RPC.",              \
                  g_type_name(G_TYPE_FROM_INSTANCE(connection)));  \
        EXIT;                                                      \
    }                                                              \
    PK_CONNECTION_GET_CLASS(connection)->_n##_async

#define RPC_FINISH(_r, _n)                                         \
    if (!PK_CONNECTION_GET_CLASS(connection)->_n##_finish) {       \
        g_critical("%s does not support the #_n RPC.",             \
                   g_type_name(G_TYPE_FROM_INSTANCE(connection))); \
        g_set_error(error, PK_CONNECTION_ERROR,                    \
                    PK_CONNECTION_ERROR_NOT_SUPPORTED,             \
                    "The #_n RPC is not supported over your "      \
                    "connection.");                                \
        RETURN(FALSE);                                             \
    }                                                              \
    _r = PK_CONNECTION_GET_CLASS(connection)->_n##_finish

#define CHECK_FOR_RPC(_n) G_STMT_START {                           \
        if ((!PK_CONNECTION_GET_CLASS(connection)->_n##_async) ||  \
            (!PK_CONNECTION_GET_CLASS(connection)->_n##_finish)) { \
            g_set_error(error, PK_CONNECTION_ERROR,                \
                        PK_CONNECTION_ERROR_NOT_SUPPORTED,         \
                        "The #_n RPC is not supported over "       \
                        "your connection.");                       \
            RETURN(FALSE);                                         \
        }                                                          \
    } G_STMT_END

/**
 * pk_connection_async_init:
 * @async: A #PkConnectionSync.
 *
 * Initializes a #PkConnectionSync allocated on the stack.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
pk_connection_async_init (PkConnectionSync *async) /* IN */
{
	memset(async, 0, sizeof(*async));
	async->mutex = g_mutex_new();
	async->cond = g_cond_new();
	g_mutex_lock(async->mutex);
}

/**
 * pk_connection_async_destroy:
 * @async: A #PkConnectionSync.
 *
 * Cleans up dynamically allocated data within the structure.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
pk_connection_async_destroy (PkConnectionSync *async) /* IN */
{
	g_mutex_free(async->mutex);
	g_cond_free(async->cond);
}

/**
 * pk_connection_async_wait:
 * @async: A #PkConnectionSync.
 *
 * Waits for an async operation in a secondary thread to complete.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
pk_connection_async_wait (PkConnectionSync *async) /* IN */
{
	g_cond_wait(async->cond, async->mutex);
}

/**
 * pk_connection_async_signal:
 * @async: A #PkConnectionSync.
 *
 * Signals a thread blocked in pk_connection_async_wait() that an async
 * operation has completed.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
pk_connection_async_signal (PkConnectionSync *async) /* IN */
{
	g_mutex_lock(async->mutex);
	g_cond_signal(async->cond);
	g_mutex_unlock(async->mutex);
}

/**
 * pk_connection_plugin_get_name_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "plugin_get_name" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_plugin_get_name_finish().
 *
 * See pk_connection_plugin_get_name().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_plugin_get_name_async (PkConnection        *connection,  /* IN */
                                     gchar               *plugin,      /* IN */
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_plugin_get_name_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "plugin_get_name" RPC.
 * This should be called from a callback supplied to
 * pk_connection_plugin_get_name_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_plugin_get_name_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_plugin_get_name_async().
 *
 * Callback executed when an asynchronous request for the
 * "plugin_get_name" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_plugin_get_name_cb (GObject      *source,    /* IN */
                                  GAsyncResult *result,    /* IN */
                                  gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_plugin_get_name_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_plugin_get_name:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "plugin_get_name" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_plugin_get_name (PkConnection  *connection, /* IN */
                               gchar         *plugin,     /* IN */
                               gchar        **name,       /* OUT */
                               GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(plugin_get_name);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)name;
	pk_connection_plugin_get_name_async(
			connection,
			plugin,
			NULL,
			pk_connection_plugin_get_name_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_plugin_get_description_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "plugin_get_description" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_plugin_get_description_finish().
 *
 * See pk_connection_plugin_get_description().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_plugin_get_description_async (PkConnection        *connection,  /* IN */
                                            gchar               *plugin,      /* IN */
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_plugin_get_description_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "plugin_get_description" RPC.
 * This should be called from a callback supplied to
 * pk_connection_plugin_get_description_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_plugin_get_description_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_plugin_get_description_async().
 *
 * Callback executed when an asynchronous request for the
 * "plugin_get_description" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_plugin_get_description_cb (GObject      *source,    /* IN */
                                         GAsyncResult *result,    /* IN */
                                         gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_plugin_get_description_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_plugin_get_description:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "plugin_get_description" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_plugin_get_description (PkConnection  *connection,  /* IN */
                                      gchar         *plugin,      /* IN */
                                      gchar        **description, /* OUT */
                                      GError       **error)       /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(plugin_get_description);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)description;
	pk_connection_plugin_get_description_async(
			connection,
			plugin,
			NULL,
			pk_connection_plugin_get_description_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_plugin_get_version_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "plugin_get_version" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_plugin_get_version_finish().
 *
 * See pk_connection_plugin_get_version().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_plugin_get_version_async (PkConnection        *connection,  /* IN */
                                        gchar               *plugin,      /* IN */
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_plugin_get_version_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "plugin_get_version" RPC.
 * This should be called from a callback supplied to
 * pk_connection_plugin_get_version_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_plugin_get_version_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_plugin_get_version_async().
 *
 * Callback executed when an asynchronous request for the
 * "plugin_get_version" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_plugin_get_version_cb (GObject      *source,    /* IN */
                                     GAsyncResult *result,    /* IN */
                                     gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_plugin_get_version_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_plugin_get_version:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "plugin_get_version" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_plugin_get_version (PkConnection  *connection, /* IN */
                                  gchar         *plugin,     /* IN */
                                  gchar        **version,    /* OUT */
                                  GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(plugin_get_version);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)version;
	pk_connection_plugin_get_version_async(
			connection,
			plugin,
			NULL,
			pk_connection_plugin_get_version_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_plugin_get_plugin_type_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "plugin_get_plugin_type" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_plugin_get_plugin_type_finish().
 *
 * See pk_connection_plugin_get_plugin_type().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_plugin_get_plugin_type_async (PkConnection        *connection,  /* IN */
                                            gchar               *plugin,      /* IN */
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_plugin_get_plugin_type_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "plugin_get_plugin_type" RPC.
 * This should be called from a callback supplied to
 * pk_connection_plugin_get_plugin_type_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_plugin_get_plugin_type_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_plugin_get_plugin_type_async().
 *
 * Callback executed when an asynchronous request for the
 * "plugin_get_plugin_type" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_plugin_get_plugin_type_cb (GObject      *source,    /* IN */
                                         GAsyncResult *result,    /* IN */
                                         gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_plugin_get_plugin_type_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_plugin_get_plugin_type:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "plugin_get_plugin_type" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_plugin_get_plugin_type (PkConnection  *connection, /* IN */
                                      gchar         *plugin,     /* IN */
                                      gint          *type,       /* OUT */
                                      GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(plugin_get_plugin_type);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)type;
	pk_connection_plugin_get_plugin_type_async(
			connection,
			plugin,
			NULL,
			pk_connection_plugin_get_plugin_type_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_encoder_set_property_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "encoder_set_property" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_encoder_set_property_finish().
 *
 * See pk_connection_encoder_set_property().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_encoder_set_property_async (PkConnection        *connection,  /* IN */
                                          gint                 encoder,     /* IN */
                                          const gchar         *name,        /* IN */
                                          const GValue        *value,       /* IN */
                                          GCancellable        *cancellable, /* IN */
                                          GAsyncReadyCallback  callback,    /* IN */
                                          gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(encoder_set_property)(connection,
	                                encoder,
	                                name,
	                                value,
	                                cancellable,
	                                callback,
	                                user_data);
	EXIT;
}

/**
 * pk_connection_encoder_set_property_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_encoder_set_property_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "encoder_set_property" RPC.
 * This should be called from a callback supplied to
 * pk_connection_encoder_set_property_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_encoder_set_property_finish (PkConnection  *connection, /* IN */
                                           GAsyncResult  *result,     /* IN */
                                           GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, encoder_set_property)(connection,
	                                      result,
	                                      error);
	RETURN(ret);
}

/**
 * pk_connection_encoder_set_property_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_encoder_set_property_async().
 *
 * Callback executed when an asynchronous request for the
 * "encoder_set_property" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_encoder_set_property_cb (GObject      *source,    /* IN */
                                       GAsyncResult *result,    /* IN */
                                       gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_encoder_set_property_finish(
			PK_CONNECTION(source),
			result,
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_encoder_set_property:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "encoder_set_property" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_encoder_set_property (PkConnection  *connection, /* IN */
                                    gint           encoder,    /* IN */
                                    const gchar   *name,       /* IN */
                                    const GValue  *value,      /* IN */
                                    GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(encoder_set_property);
	pk_connection_async_init(&async);
	async.error = error;
	pk_connection_encoder_set_property_async(
			connection,
			encoder,
			name,
			value,
			NULL,
			pk_connection_encoder_set_property_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_encoder_get_property_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "encoder_get_property" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_encoder_get_property_finish().
 *
 * See pk_connection_encoder_get_property().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_encoder_get_property_async (PkConnection        *connection,  /* IN */
                                          gint                 encoder,     /* IN */
                                          const gchar         *name,        /* IN */
                                          GCancellable        *cancellable, /* IN */
                                          GAsyncReadyCallback  callback,    /* IN */
                                          gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(encoder_get_property)(connection,
	                                encoder,
	                                name,
	                                cancellable,
	                                callback,
	                                user_data);
	EXIT;
}

/**
 * pk_connection_encoder_get_property_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_encoder_get_property_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "encoder_get_property" RPC.
 * This should be called from a callback supplied to
 * pk_connection_encoder_get_property_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_encoder_get_property_finish (PkConnection  *connection, /* IN */
                                           GAsyncResult  *result,     /* IN */
                                           GValue        *value,      /* OUT */
                                           GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, encoder_get_property)(connection,
	                                      result,
	                                      value,
	                                      error);
	RETURN(ret);
}

/**
 * pk_connection_encoder_get_property_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_encoder_get_property_async().
 *
 * Callback executed when an asynchronous request for the
 * "encoder_get_property" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_encoder_get_property_cb (GObject      *source,    /* IN */
                                       GAsyncResult *result,    /* IN */
                                       gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_encoder_get_property_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_encoder_get_property:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "encoder_get_property" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_encoder_get_property (PkConnection  *connection, /* IN */
                                    gint           encoder,    /* IN */
                                    const gchar   *name,       /* IN */
                                    GValue        *value,      /* OUT */
                                    GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(encoder_get_property);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)value;
	pk_connection_encoder_get_property_async(
			connection,
			encoder,
			name,
			NULL,
			pk_connection_encoder_get_property_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_source_set_property_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "source_set_property" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_source_set_property_finish().
 *
 * See pk_connection_source_set_property().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_source_set_property_async (PkConnection        *connection,  /* IN */
                                         gint                 source,      /* IN */
                                         const gchar         *name,        /* IN */
                                         const GValue        *value,       /* IN */
                                         GCancellable        *cancellable, /* IN */
                                         GAsyncReadyCallback  callback,    /* IN */
                                         gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(source_set_property)(connection,
	                               source,
	                               name,
	                               value,
	                               cancellable,
	                               callback,
	                               user_data);
	EXIT;
}

/**
 * pk_connection_source_set_property_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_source_set_property_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "source_set_property" RPC.
 * This should be called from a callback supplied to
 * pk_connection_source_set_property_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_source_set_property_finish (PkConnection  *connection, /* IN */
                                          GAsyncResult  *result,     /* IN */
                                          GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, source_set_property)(connection,
	                                     result,
	                                     error);
	RETURN(ret);
}

/**
 * pk_connection_source_set_property_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_source_set_property_async().
 *
 * Callback executed when an asynchronous request for the
 * "source_set_property" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_source_set_property_cb (GObject      *source,    /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_source_set_property_finish(
			PK_CONNECTION(source),
			result,
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_source_set_property:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "source_set_property" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_source_set_property (PkConnection  *connection, /* IN */
                                   gint           source,     /* IN */
                                   const gchar   *name,       /* IN */
                                   const GValue  *value,      /* IN */
                                   GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(source_set_property);
	pk_connection_async_init(&async);
	async.error = error;
	pk_connection_source_set_property_async(
			connection,
			source,
			name,
			value,
			NULL,
			pk_connection_source_set_property_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_source_get_property_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "source_get_property" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_source_get_property_finish().
 *
 * See pk_connection_source_get_property().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_source_get_property_async (PkConnection        *connection,  /* IN */
                                         gint                 source,      /* IN */
                                         const gchar         *name,        /* IN */
                                         GCancellable        *cancellable, /* IN */
                                         GAsyncReadyCallback  callback,    /* IN */
                                         gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(source_get_property)(connection,
	                               source,
	                               name,
	                               cancellable,
	                               callback,
	                               user_data);
	EXIT;
}

/**
 * pk_connection_source_get_property_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_source_get_property_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "source_get_property" RPC.
 * This should be called from a callback supplied to
 * pk_connection_source_get_property_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_source_get_property_finish (PkConnection  *connection, /* IN */
                                          GAsyncResult  *result,     /* IN */
                                          GValue        *value,      /* OUT */
                                          GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, source_get_property)(connection,
	                                     result,
	                                     value,
	                                     error);
	RETURN(ret);
}

/**
 * pk_connection_source_get_property_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_source_get_property_async().
 *
 * Callback executed when an asynchronous request for the
 * "source_get_property" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_source_get_property_cb (GObject      *source,    /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_source_get_property_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_source_get_property:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "source_get_property" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_source_get_property (PkConnection  *connection, /* IN */
                                   gint           source,     /* IN */
                                   const gchar   *name,       /* IN */
                                   GValue        *value,      /* OUT */
                                   GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(source_get_property);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)value;
	pk_connection_source_get_property_async(
			connection,
			source,
			name,
			NULL,
			pk_connection_source_get_property_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_source_get_plugin_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "source_get_plugin" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_source_get_plugin_finish().
 *
 * See pk_connection_source_get_plugin().
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_source_get_plugin_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "source_get_plugin" RPC.
 * This should be called from a callback supplied to
 * pk_connection_source_get_plugin_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_source_get_plugin_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_source_get_plugin_async().
 *
 * Callback executed when an asynchronous request for the
 * "source_get_plugin" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_source_get_plugin_cb (GObject      *source,    /* IN */
                                    GAsyncResult *result,    /* IN */
                                    gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_source_get_plugin_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_source_get_plugin:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "source_get_plugin" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_source_get_plugin (PkConnection  *connection, /* IN */
                                 gint           source,     /* IN */
                                 gchar        **plugin,     /* OUT */
                                 GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(source_get_plugin);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)plugin;
	pk_connection_source_get_plugin_async(
			connection,
			source,
			NULL,
			pk_connection_source_get_plugin_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_manager_get_channels_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "manager_get_channels" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_manager_get_channels_finish().
 *
 * See pk_connection_manager_get_channels().
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_manager_get_channels_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "manager_get_channels" RPC.
 * This should be called from a callback supplied to
 * pk_connection_manager_get_channels_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_manager_get_channels_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_manager_get_channels_async().
 *
 * Callback executed when an asynchronous request for the
 * "manager_get_channels" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_get_channels_cb (GObject      *source,    /* IN */
                                       GAsyncResult *result,    /* IN */
                                       gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_manager_get_channels_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->params[1],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_manager_get_channels:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "manager_get_channels" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_channels (PkConnection  *connection,   /* IN */
                                    gint         **channels,     /* OUT */
                                    gsize         *channels_len, /* OUT */
                                    GError       **error)        /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_get_channels);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)channels;
	async.params[1] = (gpointer)channels_len;
	pk_connection_manager_get_channels_async(
			connection,
			NULL,
			pk_connection_manager_get_channels_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_manager_get_source_plugins_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "manager_get_source_plugins" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_manager_get_source_plugins_finish().
 *
 * See pk_connection_manager_get_source_plugins().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_get_source_plugins_async (PkConnection        *connection,  /* IN */
                                                GCancellable        *cancellable, /* IN */
                                                GAsyncReadyCallback  callback,    /* IN */
                                                gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_get_source_plugins)(connection,
	                                      cancellable,
	                                      callback,
	                                      user_data);
	EXIT;
}

/**
 * pk_connection_manager_get_source_plugins_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_manager_get_source_plugins_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "manager_get_source_plugins" RPC.
 * This should be called from a callback supplied to
 * pk_connection_manager_get_source_plugins_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_source_plugins_finish (PkConnection   *connection, /* IN */
                                                 GAsyncResult   *result,     /* IN */
                                                 gchar        ***plugins,    /* OUT */
                                                 GError        **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_get_source_plugins)(connection,
	                                            result,
	                                            plugins,
	                                            error);
	RETURN(ret);
}

/**
 * pk_connection_manager_get_source_plugins_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_manager_get_source_plugins_async().
 *
 * Callback executed when an asynchronous request for the
 * "manager_get_source_plugins" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_get_source_plugins_cb (GObject      *source,    /* IN */
                                             GAsyncResult *result,    /* IN */
                                             gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_manager_get_source_plugins_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_manager_get_source_plugins:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "manager_get_source_plugins" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_source_plugins (PkConnection   *connection, /* IN */
                                          gchar        ***plugins,    /* OUT */
                                          GError        **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_get_source_plugins);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)plugins;
	pk_connection_manager_get_source_plugins_async(
			connection,
			NULL,
			pk_connection_manager_get_source_plugins_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_manager_get_version_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "manager_get_version" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_manager_get_version_finish().
 *
 * See pk_connection_manager_get_version().
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_manager_get_version_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "manager_get_version" RPC.
 * This should be called from a callback supplied to
 * pk_connection_manager_get_version_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_manager_get_version_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_manager_get_version_async().
 *
 * Callback executed when an asynchronous request for the
 * "manager_get_version" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_get_version_cb (GObject      *source,    /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_manager_get_version_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_manager_get_version:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "manager_get_version" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_manager_get_version (PkConnection  *connection, /* IN */
                                   gchar        **version,    /* OUT */
                                   GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_get_version);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)version;
	pk_connection_manager_get_version_async(
			connection,
			NULL,
			pk_connection_manager_get_version_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_manager_ping_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "manager_ping" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_manager_ping_finish().
 *
 * See pk_connection_manager_ping().
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_manager_ping_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "manager_ping" RPC.
 * This should be called from a callback supplied to
 * pk_connection_manager_ping_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_manager_ping_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_manager_ping_async().
 *
 * Callback executed when an asynchronous request for the
 * "manager_ping" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_ping_cb (GObject      *source,    /* IN */
                               GAsyncResult *result,    /* IN */
                               gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_manager_ping_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_manager_ping:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "manager_ping" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_manager_ping (PkConnection  *connection, /* IN */
                            GTimeVal      *tv,         /* OUT */
                            GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_ping);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)tv;
	pk_connection_manager_ping_async(
			connection,
			NULL,
			pk_connection_manager_ping_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_manager_add_channel_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "manager_add_channel" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_manager_add_channel_finish().
 *
 * See pk_connection_manager_add_channel().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_add_channel_async (PkConnection        *connection,  /* IN */
                                         const PkSpawnInfo   *spawn_info,  /* IN */
                                         GCancellable        *cancellable, /* IN */
                                         GAsyncReadyCallback  callback,    /* IN */
                                         gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_add_channel)(connection,
	                               spawn_info,
	                               cancellable,
	                               callback,
	                               user_data);
	EXIT;
}

/**
 * pk_connection_manager_add_channel_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_manager_add_channel_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "manager_add_channel" RPC.
 * This should be called from a callback supplied to
 * pk_connection_manager_add_channel_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_manager_add_channel_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_manager_add_channel_async().
 *
 * Callback executed when an asynchronous request for the
 * "manager_add_channel" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_add_channel_cb (GObject      *source,    /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_manager_add_channel_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_manager_add_channel:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "manager_add_channel" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_manager_add_channel (PkConnection       *connection, /* IN */
                                   const PkSpawnInfo  *spawn_info, /* IN */
                                   gint               *channel,    /* OUT */
                                   GError            **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_add_channel);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)channel;
	pk_connection_manager_add_channel_async(
			connection,
			spawn_info,
			NULL,
			pk_connection_manager_add_channel_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_manager_remove_channel_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "manager_remove_channel" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_manager_remove_channel_finish().
 *
 * See pk_connection_manager_remove_channel().
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_manager_remove_channel_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "manager_remove_channel" RPC.
 * This should be called from a callback supplied to
 * pk_connection_manager_remove_channel_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_manager_remove_channel_finish (PkConnection  *connection, /* IN */
                                             GAsyncResult  *result,     /* IN */
                                             GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_remove_channel)(connection,
	                                        result,
	                                        error);
	RETURN(ret);
}

/**
 * pk_connection_manager_remove_channel_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_manager_remove_channel_async().
 *
 * Callback executed when an asynchronous request for the
 * "manager_remove_channel" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_remove_channel_cb (GObject      *source,    /* IN */
                                         GAsyncResult *result,    /* IN */
                                         gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_manager_remove_channel_finish(
			PK_CONNECTION(source),
			result,
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_manager_remove_channel:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "manager_remove_channel" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_manager_remove_channel (PkConnection  *connection, /* IN */
                                      gint           channel,    /* IN */
                                      GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_remove_channel);
	pk_connection_async_init(&async);
	async.error = error;
	pk_connection_manager_remove_channel_async(
			connection,
			channel,
			NULL,
			pk_connection_manager_remove_channel_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_manager_add_subscription_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "manager_add_subscription" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_manager_add_subscription_finish().
 *
 * See pk_connection_manager_add_subscription().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_manager_add_subscription_async (PkConnection        *connection,     /* IN */
                                              gint                 channel,        /* IN */
                                              gsize                buffer_size,    /* IN */
                                              gulong               buffer_timeout, /* IN */
                                              const gchar         *encoder,        /* IN */
                                              GCancellable        *cancellable,    /* IN */
                                              GAsyncReadyCallback  callback,       /* IN */
                                              gpointer             user_data)      /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(manager_add_subscription)(connection,
	                                    channel,
	                                    buffer_size,
	                                    buffer_timeout,
	                                    encoder,
	                                    cancellable,
	                                    callback,
	                                    user_data);
	EXIT;
}

/**
 * pk_connection_manager_add_subscription_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_manager_add_subscription_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "manager_add_subscription" RPC.
 * This should be called from a callback supplied to
 * pk_connection_manager_add_subscription_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_manager_add_subscription_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_manager_add_subscription_async().
 *
 * Callback executed when an asynchronous request for the
 * "manager_add_subscription" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_add_subscription_cb (GObject      *source,    /* IN */
                                           GAsyncResult *result,    /* IN */
                                           gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_manager_add_subscription_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_manager_add_subscription:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "manager_add_subscription" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_manager_add_subscription (PkConnection  *connection,     /* IN */
                                        gint           channel,        /* IN */
                                        gsize          buffer_size,    /* IN */
                                        gulong         buffer_timeout, /* IN */
                                        const gchar   *encoder,        /* IN */
                                        gint          *subscription,   /* OUT */
                                        GError       **error)          /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_add_subscription);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)subscription;
	pk_connection_manager_add_subscription_async(
			connection,
			channel,
			buffer_size,
			buffer_timeout,
			encoder,
			NULL,
			pk_connection_manager_add_subscription_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_manager_remove_subscription_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "manager_remove_subscription" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_manager_remove_subscription_finish().
 *
 * See pk_connection_manager_remove_subscription().
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_manager_remove_subscription_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "manager_remove_subscription" RPC.
 * This should be called from a callback supplied to
 * pk_connection_manager_remove_subscription_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_manager_remove_subscription_finish (PkConnection  *connection, /* IN */
                                                  GAsyncResult  *result,     /* IN */
                                                  GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, manager_remove_subscription)(connection,
	                                             result,
	                                             error);
	RETURN(ret);
}

/**
 * pk_connection_manager_remove_subscription_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_manager_remove_subscription_async().
 *
 * Callback executed when an asynchronous request for the
 * "manager_remove_subscription" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_manager_remove_subscription_cb (GObject      *source,    /* IN */
                                              GAsyncResult *result,    /* IN */
                                              gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_manager_remove_subscription_finish(
			PK_CONNECTION(source),
			result,
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_manager_remove_subscription:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "manager_remove_subscription" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_manager_remove_subscription (PkConnection  *connection,   /* IN */
                                           gint           subscription, /* IN */
                                           GError       **error)        /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(manager_remove_subscription);
	pk_connection_async_init(&async);
	async.error = error;
	pk_connection_manager_remove_subscription_async(
			connection,
			subscription,
			NULL,
			pk_connection_manager_remove_subscription_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_channel_get_args_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "channel_get_args" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_channel_get_args_finish().
 *
 * See pk_connection_channel_get_args().
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_channel_get_args_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "channel_get_args" RPC.
 * This should be called from a callback supplied to
 * pk_connection_channel_get_args_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_channel_get_args_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_channel_get_args_async().
 *
 * Callback executed when an asynchronous request for the
 * "channel_get_args" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_args_cb (GObject      *source,    /* IN */
                                   GAsyncResult *result,    /* IN */
                                   gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_channel_get_args_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_channel_get_args:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "channel_get_args" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_args (PkConnection   *connection, /* IN */
                                gint            channel,    /* IN */
                                gchar        ***args,       /* OUT */
                                GError        **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_args);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)args;
	pk_connection_channel_get_args_async(
			connection,
			channel,
			NULL,
			pk_connection_channel_get_args_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_channel_get_env_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "channel_get_env" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_channel_get_env_finish().
 *
 * See pk_connection_channel_get_env().
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_channel_get_env_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "channel_get_env" RPC.
 * This should be called from a callback supplied to
 * pk_connection_channel_get_env_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_channel_get_env_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_channel_get_env_async().
 *
 * Callback executed when an asynchronous request for the
 * "channel_get_env" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_env_cb (GObject      *source,    /* IN */
                                  GAsyncResult *result,    /* IN */
                                  gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_channel_get_env_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_channel_get_env:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "channel_get_env" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_env (PkConnection   *connection, /* IN */
                               gint            channel,    /* IN */
                               gchar        ***env,        /* OUT */
                               GError        **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_env);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)env;
	pk_connection_channel_get_env_async(
			connection,
			channel,
			NULL,
			pk_connection_channel_get_env_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_channel_get_pid_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "channel_get_pid" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_channel_get_pid_finish().
 *
 * See pk_connection_channel_get_pid().
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_channel_get_pid_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "channel_get_pid" RPC.
 * This should be called from a callback supplied to
 * pk_connection_channel_get_pid_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_pid_finish (PkConnection  *connection, /* IN */
                                      GAsyncResult  *result,     /* IN */
                                      GPid          *pid,        /* OUT */
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
 * pk_connection_channel_get_pid_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_channel_get_pid_async().
 *
 * Callback executed when an asynchronous request for the
 * "channel_get_pid" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_pid_cb (GObject      *source,    /* IN */
                                  GAsyncResult *result,    /* IN */
                                  gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_channel_get_pid_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_channel_get_pid:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "channel_get_pid" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_pid (PkConnection  *connection, /* IN */
                               gint           channel,    /* IN */
                               GPid          *pid,        /* OUT */
                               GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_pid);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)pid;
	pk_connection_channel_get_pid_async(
			connection,
			channel,
			NULL,
			pk_connection_channel_get_pid_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_channel_get_state_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "channel_get_state" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_channel_get_state_finish().
 *
 * See pk_connection_channel_get_state().
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_channel_get_state_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "channel_get_state" RPC.
 * This should be called from a callback supplied to
 * pk_connection_channel_get_state_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_channel_get_state_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_channel_get_state_async().
 *
 * Callback executed when an asynchronous request for the
 * "channel_get_state" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_state_cb (GObject      *source,    /* IN */
                                    GAsyncResult *result,    /* IN */
                                    gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_channel_get_state_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_channel_get_state:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "channel_get_state" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_state (PkConnection  *connection, /* IN */
                                 gint           channel,    /* IN */
                                 gint          *state,      /* OUT */
                                 GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_state);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)state;
	pk_connection_channel_get_state_async(
			connection,
			channel,
			NULL,
			pk_connection_channel_get_state_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_channel_get_target_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "channel_get_target" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_channel_get_target_finish().
 *
 * See pk_connection_channel_get_target().
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_channel_get_target_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "channel_get_target" RPC.
 * This should be called from a callback supplied to
 * pk_connection_channel_get_target_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_channel_get_target_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_channel_get_target_async().
 *
 * Callback executed when an asynchronous request for the
 * "channel_get_target" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_target_cb (GObject      *source,    /* IN */
                                     GAsyncResult *result,    /* IN */
                                     gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_channel_get_target_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_channel_get_target:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "channel_get_target" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_target (PkConnection  *connection, /* IN */
                                  gint           channel,    /* IN */
                                  gchar        **target,     /* OUT */
                                  GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_target);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)target;
	pk_connection_channel_get_target_async(
			connection,
			channel,
			NULL,
			pk_connection_channel_get_target_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_channel_get_working_dir_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "channel_get_working_dir" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_channel_get_working_dir_finish().
 *
 * See pk_connection_channel_get_working_dir().
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_channel_get_working_dir_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "channel_get_working_dir" RPC.
 * This should be called from a callback supplied to
 * pk_connection_channel_get_working_dir_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_channel_get_working_dir_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_channel_get_working_dir_async().
 *
 * Callback executed when an asynchronous request for the
 * "channel_get_working_dir" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_working_dir_cb (GObject      *source,    /* IN */
                                          GAsyncResult *result,    /* IN */
                                          gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_channel_get_working_dir_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_channel_get_working_dir:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "channel_get_working_dir" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_working_dir (PkConnection  *connection,  /* IN */
                                       gint           channel,     /* IN */
                                       gchar        **working_dir, /* OUT */
                                       GError       **error)       /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_working_dir);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)working_dir;
	pk_connection_channel_get_working_dir_async(
			connection,
			channel,
			NULL,
			pk_connection_channel_get_working_dir_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_channel_get_sources_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "channel_get_sources" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_channel_get_sources_finish().
 *
 * See pk_connection_channel_get_sources().
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_channel_get_sources_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "channel_get_sources" RPC.
 * This should be called from a callback supplied to
 * pk_connection_channel_get_sources_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_channel_get_sources_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_channel_get_sources_async().
 *
 * Callback executed when an asynchronous request for the
 * "channel_get_sources" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_get_sources_cb (GObject      *source,    /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_channel_get_sources_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->params[1],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_channel_get_sources:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "channel_get_sources" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_get_sources (PkConnection  *connection,  /* IN */
                                   gint           channel,     /* IN */
                                   gint         **sources,     /* OUT */
                                   gsize         *sources_len, /* OUT */
                                   GError       **error)       /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_get_sources);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)sources;
	async.params[1] = (gpointer)sources_len;
	pk_connection_channel_get_sources_async(
			connection,
			channel,
			NULL,
			pk_connection_channel_get_sources_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_channel_add_source_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "channel_add_source" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_channel_add_source_finish().
 *
 * See pk_connection_channel_add_source().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_add_source_async (PkConnection        *connection,  /* IN */
                                        gint                 channel,     /* IN */
                                        const gchar         *plugin,      /* IN */
                                        GCancellable        *cancellable, /* IN */
                                        GAsyncReadyCallback  callback,    /* IN */
                                        gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_add_source)(connection,
	                              channel,
	                              plugin,
	                              cancellable,
	                              callback,
	                              user_data);
	EXIT;
}

/**
 * pk_connection_channel_add_source_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_channel_add_source_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "channel_add_source" RPC.
 * This should be called from a callback supplied to
 * pk_connection_channel_add_source_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_add_source_finish (PkConnection  *connection, /* IN */
                                         GAsyncResult  *result,     /* IN */
                                         gint          *source,     /* OUT */
                                         GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_add_source)(connection,
	                                    result,
	                                    source,
	                                    error);
	RETURN(ret);
}

/**
 * pk_connection_channel_add_source_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_channel_add_source_async().
 *
 * Callback executed when an asynchronous request for the
 * "channel_add_source" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_add_source_cb (GObject      *source,    /* IN */
                                     GAsyncResult *result,    /* IN */
                                     gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_channel_add_source_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_channel_add_source:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "channel_add_source" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_add_source (PkConnection  *connection, /* IN */
                                  gint           channel,    /* IN */
                                  const gchar   *plugin,     /* IN */
                                  gint          *source,     /* OUT */
                                  GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_add_source);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)source;
	pk_connection_channel_add_source_async(
			connection,
			channel,
			plugin,
			NULL,
			pk_connection_channel_add_source_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_channel_remove_source_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "channel_remove_source" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_channel_remove_source_finish().
 *
 * See pk_connection_channel_remove_source().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_remove_source_async (PkConnection        *connection,  /* IN */
                                           gint                 channel,     /* IN */
                                           gint                 source,      /* IN */
                                           GCancellable        *cancellable, /* IN */
                                           GAsyncReadyCallback  callback,    /* IN */
                                           gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_remove_source)(connection,
	                                 channel,
	                                 source,
	                                 cancellable,
	                                 callback,
	                                 user_data);
	EXIT;
}

/**
 * pk_connection_channel_remove_source_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_channel_remove_source_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "channel_remove_source" RPC.
 * This should be called from a callback supplied to
 * pk_connection_channel_remove_source_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_remove_source_finish (PkConnection  *connection, /* IN */
                                            GAsyncResult  *result,     /* IN */
                                            GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_remove_source)(connection,
	                                       result,
	                                       error);
	RETURN(ret);
}

/**
 * pk_connection_channel_remove_source_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_channel_remove_source_async().
 *
 * Callback executed when an asynchronous request for the
 * "channel_remove_source" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_remove_source_cb (GObject      *source,    /* IN */
                                        GAsyncResult *result,    /* IN */
                                        gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_channel_remove_source_finish(
			PK_CONNECTION(source),
			result,
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_channel_remove_source:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "channel_remove_source" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_remove_source (PkConnection  *connection, /* IN */
                                     gint           channel,    /* IN */
                                     gint           source,     /* IN */
                                     GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_remove_source);
	pk_connection_async_init(&async);
	async.error = error;
	pk_connection_channel_remove_source_async(
			connection,
			channel,
			source,
			NULL,
			pk_connection_channel_remove_source_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_channel_start_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "channel_start" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_channel_start_finish().
 *
 * See pk_connection_channel_start().
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
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_channel_start_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "channel_start" RPC.
 * This should be called from a callback supplied to
 * pk_connection_channel_start_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_channel_start_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_channel_start_async().
 *
 * Callback executed when an asynchronous request for the
 * "channel_start" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_start_cb (GObject      *source,    /* IN */
                                GAsyncResult *result,    /* IN */
                                gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_channel_start_finish(
			PK_CONNECTION(source),
			result,
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_channel_start:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "channel_start" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_start (PkConnection  *connection, /* IN */
                             gint           channel,    /* IN */
                             GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_start);
	pk_connection_async_init(&async);
	async.error = error;
	pk_connection_channel_start_async(
			connection,
			channel,
			NULL,
			pk_connection_channel_start_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_channel_stop_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "channel_stop" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_channel_stop_finish().
 *
 * See pk_connection_channel_stop().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_stop_async (PkConnection        *connection,  /* IN */
                                  gint                 channel,     /* IN */
                                  gboolean             killpid,     /* IN */
                                  GCancellable        *cancellable, /* IN */
                                  GAsyncReadyCallback  callback,    /* IN */
                                  gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_stop)(connection,
	                        channel,
	                        killpid,
	                        cancellable,
	                        callback,
	                        user_data);
	EXIT;
}

/**
 * pk_connection_channel_stop_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_channel_stop_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "channel_stop" RPC.
 * This should be called from a callback supplied to
 * pk_connection_channel_stop_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
 * pk_connection_channel_stop_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_channel_stop_async().
 *
 * Callback executed when an asynchronous request for the
 * "channel_stop" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_stop_cb (GObject      *source,    /* IN */
                               GAsyncResult *result,    /* IN */
                               gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_channel_stop_finish(
			PK_CONNECTION(source),
			result,
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_channel_stop:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "channel_stop" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_stop (PkConnection  *connection, /* IN */
                            gint           channel,    /* IN */
                            gboolean       killpid,    /* IN */
                            GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_stop);
	pk_connection_async_init(&async);
	async.error = error;
	pk_connection_channel_stop_async(
			connection,
			channel,
			killpid,
			NULL,
			pk_connection_channel_stop_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_channel_pause_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "channel_pause" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_channel_pause_finish().
 *
 * See pk_connection_channel_pause().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_pause_async (PkConnection        *connection,  /* IN */
                                   gint                 channel,     /* IN */
                                   GCancellable        *cancellable, /* IN */
                                   GAsyncReadyCallback  callback,    /* IN */
                                   gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_pause)(connection,
	                         channel,
	                         cancellable,
	                         callback,
	                         user_data);
	EXIT;
}

/**
 * pk_connection_channel_pause_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_channel_pause_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "channel_pause" RPC.
 * This should be called from a callback supplied to
 * pk_connection_channel_pause_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_pause_finish (PkConnection  *connection, /* IN */
                                    GAsyncResult  *result,     /* IN */
                                    GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_pause)(connection,
	                               result,
	                               error);
	RETURN(ret);
}

/**
 * pk_connection_channel_pause_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_channel_pause_async().
 *
 * Callback executed when an asynchronous request for the
 * "channel_pause" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_pause_cb (GObject      *source,    /* IN */
                                GAsyncResult *result,    /* IN */
                                gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_channel_pause_finish(
			PK_CONNECTION(source),
			result,
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_channel_pause:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "channel_pause" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_pause (PkConnection  *connection, /* IN */
                             gint           channel,    /* IN */
                             GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_pause);
	pk_connection_async_init(&async);
	async.error = error;
	pk_connection_channel_pause_async(
			connection,
			channel,
			NULL,
			pk_connection_channel_pause_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_channel_unpause_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "channel_unpause" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_channel_unpause_finish().
 *
 * See pk_connection_channel_unpause().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_channel_unpause_async (PkConnection        *connection,  /* IN */
                                     gint                 channel,     /* IN */
                                     GCancellable        *cancellable, /* IN */
                                     GAsyncReadyCallback  callback,    /* IN */
                                     gpointer             user_data)   /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(channel_unpause)(connection,
	                           channel,
	                           cancellable,
	                           callback,
	                           user_data);
	EXIT;
}

/**
 * pk_connection_channel_unpause_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_channel_unpause_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "channel_unpause" RPC.
 * This should be called from a callback supplied to
 * pk_connection_channel_unpause_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_unpause_finish (PkConnection  *connection, /* IN */
                                      GAsyncResult  *result,     /* IN */
                                      GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, channel_unpause)(connection,
	                                 result,
	                                 error);
	RETURN(ret);
}

/**
 * pk_connection_channel_unpause_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_channel_unpause_async().
 *
 * Callback executed when an asynchronous request for the
 * "channel_unpause" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_channel_unpause_cb (GObject      *source,    /* IN */
                                  GAsyncResult *result,    /* IN */
                                  gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_channel_unpause_finish(
			PK_CONNECTION(source),
			result,
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_channel_unpause:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "channel_unpause" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_channel_unpause (PkConnection  *connection, /* IN */
                               gint           channel,    /* IN */
                               GError       **error)      /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(channel_unpause);
	pk_connection_async_init(&async);
	async.error = error;
	pk_connection_channel_unpause_async(
			connection,
			channel,
			NULL,
			pk_connection_channel_unpause_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_subscription_enable_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "subscription_enable" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_subscription_enable_finish().
 *
 * See pk_connection_subscription_enable().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_enable_async (PkConnection        *connection,   /* IN */
                                         gint                 subscription, /* IN */
                                         GCancellable        *cancellable,  /* IN */
                                         GAsyncReadyCallback  callback,     /* IN */
                                         gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(subscription_enable)(connection,
	                               subscription,
	                               cancellable,
	                               callback,
	                               user_data);
	EXIT;
}

/**
 * pk_connection_subscription_enable_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_subscription_enable_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "subscription_enable" RPC.
 * This should be called from a callback supplied to
 * pk_connection_subscription_enable_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_enable_finish (PkConnection  *connection, /* IN */
                                          GAsyncResult  *result,     /* IN */
                                          GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, subscription_enable)(connection,
	                                     result,
	                                     error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_enable_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_subscription_enable_async().
 *
 * Callback executed when an asynchronous request for the
 * "subscription_enable" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_enable_cb (GObject      *source,    /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_subscription_enable_finish(
			PK_CONNECTION(source),
			result,
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_subscription_enable:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "subscription_enable" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_enable (PkConnection  *connection,   /* IN */
                                   gint           subscription, /* IN */
                                   GError       **error)        /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_enable);
	pk_connection_async_init(&async);
	async.error = error;
	pk_connection_subscription_enable_async(
			connection,
			subscription,
			NULL,
			pk_connection_subscription_enable_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_subscription_disable_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "subscription_disable" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_subscription_disable_finish().
 *
 * See pk_connection_subscription_disable().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_disable_async (PkConnection        *connection,   /* IN */
                                          gint                 subscription, /* IN */
                                          GCancellable        *cancellable,  /* IN */
                                          GAsyncReadyCallback  callback,     /* IN */
                                          gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(subscription_disable)(connection,
	                                subscription,
	                                cancellable,
	                                callback,
	                                user_data);
	EXIT;
}

/**
 * pk_connection_subscription_disable_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_subscription_disable_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "subscription_disable" RPC.
 * This should be called from a callback supplied to
 * pk_connection_subscription_disable_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_disable_finish (PkConnection  *connection, /* IN */
                                           GAsyncResult  *result,     /* IN */
                                           GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, subscription_disable)(connection,
	                                      result,
	                                      error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_disable_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_subscription_disable_async().
 *
 * Callback executed when an asynchronous request for the
 * "subscription_disable" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_disable_cb (GObject      *source,    /* IN */
                                       GAsyncResult *result,    /* IN */
                                       gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_subscription_disable_finish(
			PK_CONNECTION(source),
			result,
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_subscription_disable:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "subscription_disable" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_disable (PkConnection  *connection,   /* IN */
                                    gint           subscription, /* IN */
                                    GError       **error)        /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_disable);
	pk_connection_async_init(&async);
	async.error = error;
	pk_connection_subscription_disable_async(
			connection,
			subscription,
			NULL,
			pk_connection_subscription_disable_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_subscription_set_handlers_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "subscription_set_handlers" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_subscription_set_handlers_finish().
 *
 * See pk_connection_subscription_set_handlers().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_set_handlers_async (PkConnection        *connection,   /* IN */
                                               gint                 subscription, /* IN */
                                               GFunc                manifest,     /* IN */
                                               GFunc                sample,       /* IN */
                                               GCancellable        *cancellable,  /* IN */
                                               GAsyncReadyCallback  callback,     /* IN */
                                               gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(subscription_set_handlers)(connection,
	                                     subscription,
	                                     manifest,
	                                     sample,
	                                     cancellable,
	                                     callback,
	                                     user_data);
	EXIT;
}

/**
 * pk_connection_subscription_set_handlers_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_subscription_set_handlers_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "subscription_set_handlers" RPC.
 * This should be called from a callback supplied to
 * pk_connection_subscription_set_handlers_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
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
	RPC_FINISH(ret, subscription_set_handlers)(connection,
	                                           result,
	                                           error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_set_handlers_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_subscription_set_handlers_async().
 *
 * Callback executed when an asynchronous request for the
 * "subscription_set_handlers" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_set_handlers_cb (GObject      *source,    /* IN */
                                            GAsyncResult *result,    /* IN */
                                            gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_subscription_set_handlers_finish(
			PK_CONNECTION(source),
			result,
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_subscription_set_handlers:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "subscription_set_handlers" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_set_handlers (PkConnection  *connection,   /* IN */
                                         gint           subscription, /* IN */
                                         GFunc          manifest,     /* IN */
                                         GFunc          sample,       /* IN */
                                         GError       **error)        /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_set_handlers);
	pk_connection_async_init(&async);
	async.error = error;
	pk_connection_subscription_set_handlers_async(
			connection,
			subscription,
			manifest,
			sample,
			NULL,
			pk_connection_subscription_set_handlers_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_subscription_get_encoder_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable to cancel an async request or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * Asynchronously requests the "subscription_get_encoder" RPC.
 * @callback is executed upon completion of the request.  @callback should
 * call pk_connection_subscription_get_encoder_finish().
 *
 * See pk_connection_subscription_get_encoder().
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_connection_subscription_get_encoder_async (PkConnection        *connection,   /* IN */
                                              gint                 subscription, /* IN */
                                              GCancellable        *cancellable,  /* IN */
                                              GAsyncReadyCallback  callback,     /* IN */
                                              gpointer             user_data)    /* IN */
{
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	RPC_ASYNC(subscription_get_encoder)(connection,
	                                    subscription,
	                                    cancellable,
	                                    callback,
	                                    user_data);
	EXIT;
}

/**
 * pk_connection_subscription_get_encoder_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult received in callback from
 *          pk_connection_subscription_get_encoder_async().
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request of the "subscription_get_encoder" RPC.
 * This should be called from a callback supplied to
 * pk_connection_subscription_get_encoder_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_get_encoder_finish (PkConnection  *connection, /* IN */
                                               GAsyncResult  *result,     /* IN */
                                               gint          *encoder,    /* OUT */
                                               GError       **error)      /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	RPC_FINISH(ret, subscription_get_encoder)(connection,
	                                          result,
	                                          encoder,
	                                          error);
	RETURN(ret);
}

/**
 * pk_connection_subscription_get_encoder_cb:
 * @source: A #PkConnection.
 * @result: A #GAsyncResult.
 * @user_data: User data supplied to pk_connection_subscription_get_encoder_async().
 *
 * Callback executed when an asynchronous request for the
 * "subscription_get_encoder" RPC has been completed.
 *
 * Results are proxied to the thread that is blocking until the request
 * has completed.  The blocking thread is woken up.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_subscription_get_encoder_cb (GObject      *source,    /* IN */
                                           GAsyncResult *result,    /* IN */
                                           gpointer      user_data) /* IN */

{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_subscription_get_encoder_finish(
			PK_CONNECTION(source),
			result,
			async->params[0],
			async->error);
	pk_connection_async_signal(async);
	EXIT;
}

/**
 * pk_connection_subscription_get_encoder:
 * @connection: A #PkConnection
 * @error: A location for a #GError or %NULL.
 *
 * The "subscription_get_encoder" RPC.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pk_connection_subscription_get_encoder (PkConnection  *connection,   /* IN */
                                        gint           subscription, /* IN */
                                        gint          *encoder,      /* OUT */
                                        GError       **error)        /* OUT */
{
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	CHECK_FOR_RPC(subscription_get_encoder);
	pk_connection_async_init(&async);
	async.error = error;
	async.params[0] = (gpointer)encoder;
	pk_connection_subscription_get_encoder_async(
			connection,
			subscription,
			NULL,
			pk_connection_subscription_get_encoder_cb,
			&async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_get_uri:
 * @connection: A #PkConnection.
 *
 * Retrieves the "uri" property.
 *
 * Returns: The uri as a string. This value should not be modified or freed.
 * Side effects: None.
 */
const gchar *
pk_connection_get_uri (PkConnection *connection)
{
	g_return_val_if_fail(PK_IS_CONNECTION(connection), NULL);
	return connection->priv->uri;
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
	RPC_ASYNC(connect) (connection, cancellable, callback, user_data);
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
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_connect_finish(PK_CONNECTION(source),
	                                             result, async->error);
	pk_connection_async_signal(async);
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
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	pk_connection_async_init(&async);
	async.error = error;
	pk_connection_connect_async(connection, NULL,
	                            pk_connection_connect_cb,
	                            &async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
}

/**
 * pk_connection_disconnect_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: user data for @callback.
 *
 * 
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
	RPC_ASYNC(disconnect) (connection, cancellable, callback, user_data);
	EXIT;
}

/**
 * pk_connection_disconnect_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A location for a #GError or %NULL.
 *
 * 
 *
 * Returns: 
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
 * 
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_disconnect_cb (GObject      *source,    /* IN */
                             GAsyncResult *result,    /* IN */
                             gpointer      user_data) /* IN */
{
	PkConnectionSync *async = user_data;

	g_return_if_fail(PK_IS_CONNECTION(source));
	g_return_if_fail(user_data != NULL);

	ENTRY;
	async->result = pk_connection_disconnect_finish(PK_CONNECTION(source),
	                                                result, async->error);
	pk_connection_async_signal(async);
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
	PkConnectionSync async;

	g_return_val_if_fail(PK_IS_CONNECTION(connection), FALSE);

	ENTRY;
	pk_connection_async_init(&async);
	async.error = error;
	pk_connection_disconnect_async(connection, NULL,
	                               pk_connection_disconnect_cb,
	                               &async);
	pk_connection_async_wait(&async);
	pk_connection_async_destroy(&async);
	RETURN(async.result);
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

	if (g_getenv("PK_CONNECTIONS_DIR") != NULL) {
		plugin_dir = g_build_filename(g_getenv("PK_CONNECTIONS_DIR"), NULL);
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
	if (g_once_init_enter((gsize *)&protocol_types)) {
		GHashTable *hash;
		hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
		g_once_init_leave((gsize *)&protocol_types, (gsize)hash);
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
 * @uri: A uri to the destination perfkit-daemon.
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
	                                      G_OBJECT_CLASS_TYPE(object_class),
	                                      G_SIGNAL_RUN_FIRST,
	                                      0, NULL, NULL,
	                                      g_cclosure_marshal_VOID__UINT,
	                                      G_TYPE_NONE, 1, G_TYPE_UINT);
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
