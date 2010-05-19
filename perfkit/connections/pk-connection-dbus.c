/* pk-connection-dbus.c
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

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <stdio.h>
#include <string.h>

#include "pk-connection-dbus.h"

/**
 * SECTION:pk-connection-dbus:
 * @title: PkConnectionDBus
 * @short_description: Perfkit client connection over DBus
 *
 * 
 */

#ifndef DISABLE_TRACE
#define TRACE(_m,...)                                               \
    G_STMT_START {                                                  \
        g_log(G_LOG_DOMAIN, (1 << G_LOG_LEVEL_USER_SHIFT),          \
              _m, __VA_ARGS__);                                     \
    } G_STMT_END
#else
#define TRACE(_m,...)
#endif

#define ENTRY TRACE("ENTRY: %s():%d", G_STRFUNC, __LINE__)

#define EXIT                                                        \
    G_STMT_START {                                                  \
        TRACE(" EXIT: %s():%d", G_STRFUNC, __LINE__);               \
        return;                                                     \
    } G_STMT_END

#define RETURN(_r)                                                  \
    G_STMT_START {                                                  \
        TRACE(" EXIT: %s():%d", G_STRFUNC, __LINE__);               \
        return _r;                                                  \
    } G_STMT_END

#define GOTO(_l)                                                    \
    G_STMT_START {                                                  \
        TRACE(" GOTO: %s:%d", #_l, __LINE__);                       \
        goto _l;                                                    \
    } G_STMT_END

#define CASE_RETURN_STR(_l) case _l: return #_l

#define RESULT_IS_VALID(_t)                                         \
    g_simple_async_result_is_valid(                                 \
            G_ASYNC_RESULT((result)),                               \
            G_OBJECT((connection)),                                 \
            pk_connection_dbus_##_t##_async)

#define GET_RESULT_POINTER(_t, _r)                                  \
    ((_t *)g_simple_async_result_get_op_res_gpointer(               \
        G_SIMPLE_ASYNC_RESULT((_r))))

#define SET_ERROR_INVALID_TYPE(_e,_n,_i,_a)                         \
    g_set_error((_e), PK_CONNECTION_DBUS_ERROR,                     \
                PK_CONNECTION_DBUS_ERROR_DBUS,                      \
                "Argument " #_n " expected type %d, got %d",        \
                (_i), (_a))

#define APPEND_BASIC_PARAM(_t, _n)                                  \
    G_STMT_START {                                                  \
        dbus_message_iter_append_basic(&iter, _t, &(_n));           \
    } G_STMT_END

#define APPEND_INT_PARAM(_n)     APPEND_BASIC_PARAM(DBUS_TYPE_INT32, _n)
#define APPEND_UINT_PARAM(_n)    APPEND_BASIC_PARAM(DBUS_TYPE_UINT32, _n)
#define APPEND_INT64_PARAM(_n)   APPEND_BASIC_PARAM(DBUS_TYPE_INT64, _n)
#define APPEND_UINT64_PARAM(_n)  APPEND_BASIC_PARAM(DBUS_TYPE_UINT64, _n)
#define APPEND_LONG_PARAM(_n)    APPEND_BASIC_PARAM(DBUS_TYPE_INT32, _n)
#define APPEND_ULONG_PARAM(_n)   APPEND_BASIC_PARAM(DBUS_TYPE_UINT32, _n)
#define APPEND_DOUBLE_PARAM(_n)  APPEND_BASIC_PARAM(DBUS_TYPE_DOUBLE, _n)
#define APPEND_BOOLEAN_PARAM(_n) APPEND_BASIC_PARAM(DBUS_TYPE_BOOLEAN, _n)

#define APPEND_STRING_PARAM(_n)                                     \
    G_STMT_START {                                                  \
        const gchar *_s = (_n);                                     \
        if (!_s) {                                                  \
            _s = "";                                                \
        }                                                           \
        APPEND_BASIC_PARAM(DBUS_TYPE_STRING, _s);                   \
    } G_STMT_END

#define APPEND_STRV_PARAM(_n)                                       \
    G_STMT_START {                                                  \
        gint _i;                                                    \
        DBusMessageIter sub_iter;                                   \
        dbus_message_iter_open_container(                           \
             &iter, DBUS_TYPE_ARRAY, "s", &sub_iter);               \
        for (_i = 0; (_n)[_i]; _i++) {                              \
            dbus_message_iter_append_basic(&sub_iter,               \
                                           DBUS_TYPE_STRING,        \
                                           &((_n)[_i]));            \
        }                                                           \
        dbus_message_iter_close_container(&iter, &sub_iter);        \
    } G_STMT_END

#define APPEND_VARIANT_PARAM(_n)                                    \
    G_STMT_START {                                                  \
        switch (G_VALUE_TYPE((_n))) {                               \
        case G_TYPE_INT:                                            \
            APPEND_INT_PARAM((_n)->data[0].v_int);                  \
            break;                                                  \
        case G_TYPE_UINT:                                           \
            APPEND_UINT_PARAM((_n)->data[0].v_uint);                \
            break;                                                  \
        case G_TYPE_INT64:                                          \
            APPEND_INT64_PARAM((_n)->data[0].v_int64);              \
            break;                                                  \
        case G_TYPE_UINT64:                                         \
            APPEND_UINT64_PARAM((_n)->data[0].v_uint64);            \
            break;                                                  \
        case G_TYPE_LONG:                                           \
            APPEND_LONG_PARAM((_n)->data[0].v_long);                \
            break;                                                  \
        case G_TYPE_ULONG:                                          \
            APPEND_ULONG_PARAM((_n)->data[0].v_ulong);              \
            break;                                                  \
        case G_TYPE_BOOLEAN:                                        \
            APPEND_BOOLEAN_PARAM((_n)->data[0].v_int);              \
            break;                                                  \
        case G_TYPE_DOUBLE:                                         \
            APPEND_DOUBLE_PARAM((_n)->data[0].v_double);            \
            break;                                                  \
        case G_TYPE_STRING:                                         \
            APPEND_STRING_PARAM((_n)->data[0].v_pointer);           \
            break;                                                  \
        default:                                                    \
            g_error("Cannot encode type into variant: %s",          \
                    g_type_name(G_VALUE_TYPE((_n))));               \
            break;                                                  \
        }                                                           \
    } G_STMT_END

G_DEFINE_TYPE(PkConnectionDBus, pk_connection_dbus, PK_TYPE_CONNECTION)

enum
{
	STATE_INITIAL,
	STATE_CONNECTED,
	STATE_DISCONNECTED,
};

struct _PkConnectionDBusPrivate
{
	DBusConnection *dbus;
	GMutex *mutex;
	gint state;
};

/**
 * pk_connection_dbus_state_to_str:
 * @state: The connection state.
 *
 * Converts a state to the string representation.
 *
 * Returns: None.
 * Side effects: None.
 */
static const gchar *
pk_connection_dbus_state_to_str (gint state)
{
	switch (state) {
	CASE_RETURN_STR(STATE_INITIAL);
	CASE_RETURN_STR(STATE_CONNECTED);
	CASE_RETURN_STR(STATE_DISCONNECTED);
	default:
		return "UNKNOWN";
	}
}

/**
 * pk_connection_dbus_notify:
 * @call: A #DBusPendingCall
 * @user_data: A #GAsyncResult
 *
 * Callback for #DBusPendingCall notifying that a message reply has
 * completed or timed out.
 *
 * Returns: None.
 * Side effects: The asynchronous finish method is dispatched.
 */
static void
pk_connection_dbus_notify (DBusPendingCall *call,      /* IN */
                           gpointer         user_data) /* IN */
{
	GAsyncResult *result = user_data;

	g_return_if_fail(G_IS_ASYNC_RESULT(result));

	ENTRY;
	g_simple_async_result_set_op_res_gpointer(
			G_SIMPLE_ASYNC_RESULT(result),
			dbus_pending_call_ref(call),
			(GDestroyNotify)dbus_pending_call_unref);
	g_simple_async_result_complete_in_idle(G_SIMPLE_ASYNC_RESULT(result));
	EXIT;
}

/**
 * pk_connection_dbus_cancel:
 * @dbus: A #PkConnectionDBus
 * @result: A #GAsyncResult
 *
 * Callback for #GCancellable "cancel' signal.  The #DBusPendingCall
 * associated with the GCancellable will be cancelled.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_dbus_cancel (PkConnectionDBus   *dbus,
                           GSimpleAsyncResult *result)
{
	DBusPendingCall *call;

	g_return_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result));

	ENTRY;
	call = g_simple_async_result_get_op_res_gpointer(result);
	g_assert(call);

	/*
	 * Cancel the request.
	 */
	dbus_pending_call_cancel(call);

	/*
	 * Notify the finish method to complete the asynchronous request if no result
	 * has yet been received.
	 */
	if (!dbus_pending_call_get_completed(call)) {
		g_simple_async_result_set_error(result,
		                                PK_CONNECTION_DBUS_ERROR,
		                                PK_CONNECTION_DBUS_ERROR_DBUS,
		                                "The RPC request was cancelled");
		g_simple_async_result_complete_in_idle(result);
	}
	EXIT;
}

/**
 * pk_connection_dbus_demarshal_strv:
 * @iter: A #DBusMessageIter.
 * @strv: A location for a string array.
 *
 * Demarshal's a string array type into @strv.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
static inline gboolean
pk_connection_dbus_demarshal_strv (DBusMessageIter   *iter, /* IN */
                                   gchar           ***strv) /* OUT */
{
	DBusMessageIter child;
	gboolean ret = FALSE;
	GPtrArray *array;
	gchar *str;

	ENTRY;

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_ARRAY) {
		RETURN(FALSE);
	}

	dbus_message_iter_recurse(iter, &child);
	array = g_ptr_array_new();

	while (dbus_message_iter_get_arg_type(&child) != DBUS_TYPE_INVALID) {
		if (dbus_message_iter_get_arg_type(&child) != DBUS_TYPE_STRING) {
			goto error;
		}
		dbus_message_iter_get_basic(&child, &str);
		g_ptr_array_add(array, g_strdup(str));
		dbus_message_iter_next(&child);
	}

	g_ptr_array_add(array, NULL);
	*strv = g_strdupv((gchar **)array->pdata);
	ret = TRUE;

error:
	g_ptr_array_unref(array);
	RETURN(ret);
}

/**
 * pk_connection_dbus_demarshal_intv:
 * @iter: A #DBusMessageIter.
 * @strv: A location for an int array.
 *
 * Demarshal's an int array type into @strv.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
static inline gboolean
pk_connection_dbus_demarshal_intv (DBusMessageIter  *iter, /* IN */
                                   gint            **intv, /* OUT */
                                   gsize            *len)  /* OUT */
{
	DBusMessageIter child;
	gboolean ret = FALSE;
	GArray *array;
	gint v;

	ENTRY;

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_ARRAY) {
		RETURN(FALSE);
	}

	dbus_message_iter_recurse(iter, &child);
	array = g_array_new(FALSE, FALSE, sizeof(gint));

	while (dbus_message_iter_get_arg_type(&child) != DBUS_TYPE_INVALID) {
		if (dbus_message_iter_get_arg_type(&child) != DBUS_TYPE_INT32) {
			g_array_free(array, TRUE);
			RETURN(FALSE);
		}
		dbus_message_iter_get_basic(&child, &v);
		g_array_append_val(array, v);
	}

	*intv = (gint *)array->data;
	*len = array->len;
	g_array_free(array, FALSE);

	RETURN(ret);
}

/**
 * pk_connection_dbus_connect_async:
 * @connection: A #PkConnectionDBus
 * @cancellable: A #GCancellable
 * @callback: A #GAsyncReadyCallback
 * @user_data: user data for @callback
 *
 * Asynchronously connects to the Agent over DBus.  @callback MUST call
 * pk_connection_dbus_connect_finish().
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_dbus_connect_async (PkConnection        *connection,  /* IN */
                                  GCancellable        *cancellable, /* IN */
                                  GAsyncReadyCallback  callback,    /* IN */
                                  gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(connection),
	                                   callback, user_data,
	                                   pk_connection_dbus_connect_async);
	g_simple_async_result_complete_in_idle(result);
	EXIT;
}

/**
 * pk_connection_dbus_connect_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult
 * @error: A location for a #GError or %NULL.
 *
 * Completes an asynchronous request to connect.  See
 * pk_connection_dbus_connect_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
static gboolean
pk_connection_dbus_connect_finish (PkConnection  *connection, /* IN */
                                   GAsyncResult  *result,     /* IN */
                                   GError       **error)      /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusError db_error = { NULL };
	gboolean ret = FALSE;

	g_return_val_if_fail(PK_IS_CONNECTION_DBUS(connection), FALSE);

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	g_mutex_lock(priv->mutex);

	/*
	 * Only connect if we haven't yet.
	 */
	if (priv->state != STATE_INITIAL) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_STATE,
		            "Cannot connect to host while %s",
		            pk_connection_dbus_state_to_str(priv->state));
		goto unlock;
	}

	/*
	 * Retrieve the session bus.
	 */
	priv->dbus = dbus_bus_get(DBUS_BUS_SESSION, &db_error);
	if (!priv->dbus) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_NOT_AVAILABLE,
		            "Could not connect to DBUS: %s: %s",
		            db_error.name, db_error.message);
		dbus_error_free(&db_error);
		goto unlock;
	}

	/*
	 * Install GMainLoop integration.
	 */
	dbus_connection_setup_with_g_main(priv->dbus, NULL);
	priv->state = STATE_CONNECTED;
	ret = TRUE;

unlock:
	g_mutex_unlock(priv->mutex);
	pk_connection_emit_state_changed(
			connection,
			ret ? PK_CONNECTION_CONNECTED : PK_CONNECTION_FAILED);
	RETURN(ret);
}

/**
 * pk_connection_dbus_disconnect_async:
 * @connection: A #PkConnectionDBus
 * @cancellable: A #GCancellable
 * @callback: A #GAsyncReadyCallback
 * @user_data: user data for @callback
 *
 * Asynchronously requests that the connection to the Agent over DBus be
 * terminated.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_dbus_disconnect_async (PkConnection        *connection,  /* IN */
                                     GCancellable        *cancellable, /* IN */
                                     GAsyncReadyCallback  callback,    /* IN */
                                     gpointer             user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));
	g_return_if_fail(callback != NULL);

	ENTRY;
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_disconnect_async);
	g_simple_async_result_complete_in_idle(result);
	EXIT;
}

/**
 * pk_connection_dbus_disconnect_finish:
 * @connection: A #PkConnection
 * @result: A #GAsyncResult
 * @error: A localtion for a #GError, or %NULL
 *
 * Completes an asynchronous call to disconnect from DBus.  See
 * pk_connection_dbus_disconnect_async().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
static gboolean
pk_connection_dbus_disconnect_finish (PkConnection  *connection, /* IN */
                                      GAsyncResult  *result,     /* IN */
                                      GError       **error)      /* IN */
{
	PkConnectionDBusPrivate *priv;

	g_return_val_if_fail(PK_IS_CONNECTION_DBUS(connection), FALSE);

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;
	g_mutex_lock(priv->mutex);
	if (priv->dbus) {
		dbus_connection_unref(priv->dbus);
		priv->dbus = NULL;
	}
	g_mutex_unlock(priv->mutex);
	pk_connection_emit_state_changed(connection, PK_CONNECTION_DISCONNECTED);
	RETURN(TRUE);
}


static void
pk_connection_dbus_channel_get_args_async (PkConnection        *connection,  /* IN */
                                           gint                 channel,     /* IN */
                                           GCancellable        *cancellable, /* IN */
                                           GAsyncReadyCallback  callback,    /* IN */
                                           gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_get_args_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "GetArgs");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_get_args_finish (PkConnection   *connection, /* IN */
                                            GAsyncResult   *result,     /* IN */
                                            gchar        ***args,       /* OUT */
                                            GError        **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };
	gint args_len = 0;

	g_return_val_if_fail(args != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_get_args), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*args = NULL;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, args, &args_len,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_get_env_async (PkConnection        *connection,  /* IN */
                                          gint                 channel,     /* IN */
                                          GCancellable        *cancellable, /* IN */
                                          GAsyncReadyCallback  callback,    /* IN */
                                          gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_get_env_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "GetEnv");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_get_env_finish (PkConnection   *connection, /* IN */
                                           GAsyncResult   *result,     /* IN */
                                           gchar        ***env,        /* OUT */
                                           GError        **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };
	gint env_len = 0;

	g_return_val_if_fail(env != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_get_env), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*env = NULL;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, env, &env_len,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_get_exit_status_async (PkConnection        *connection,  /* IN */
                                                  gint                 channel,     /* IN */
                                                  GCancellable        *cancellable, /* IN */
                                                  GAsyncReadyCallback  callback,    /* IN */
                                                  gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_get_exit_status_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "GetExitStatus");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_get_exit_status_finish (PkConnection  *connection,  /* IN */
                                                   GAsyncResult  *result,      /* IN */
                                                   gint          *exit_status, /* OUT */
                                                   GError       **error)       /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(exit_status != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_get_exit_status), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*exit_status = 0;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_INT32, exit_status,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_get_kill_pid_async (PkConnection        *connection,  /* IN */
                                               gint                 channel,     /* IN */
                                               GCancellable        *cancellable, /* IN */
                                               GAsyncReadyCallback  callback,    /* IN */
                                               gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_get_kill_pid_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "GetKillPid");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_get_kill_pid_finish (PkConnection  *connection, /* IN */
                                                GAsyncResult  *result,     /* IN */
                                                gboolean      *kill_pid,   /* OUT */
                                                GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(kill_pid != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_get_kill_pid), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*kill_pid = FALSE;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_BOOLEAN, kill_pid,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_get_pid_async (PkConnection        *connection,  /* IN */
                                          gint                 channel,     /* IN */
                                          GCancellable        *cancellable, /* IN */
                                          GAsyncReadyCallback  callback,    /* IN */
                                          gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_get_pid_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "GetPid");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_get_pid_finish (PkConnection  *connection, /* IN */
                                           GAsyncResult  *result,     /* IN */
                                           gint          *pid,        /* OUT */
                                           GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(pid != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_get_pid), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*pid = 0;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_INT32, pid,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_get_pid_set_async (PkConnection        *connection,  /* IN */
                                              gint                 channel,     /* IN */
                                              GCancellable        *cancellable, /* IN */
                                              GAsyncReadyCallback  callback,    /* IN */
                                              gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_get_pid_set_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "GetPidSet");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_get_pid_set_finish (PkConnection  *connection, /* IN */
                                               GAsyncResult  *result,     /* IN */
                                               gboolean      *pid_set,    /* OUT */
                                               GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(pid_set != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_get_pid_set), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*pid_set = FALSE;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_BOOLEAN, pid_set,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_get_sources_async (PkConnection        *connection,  /* IN */
                                              gint                 channel,     /* IN */
                                              GCancellable        *cancellable, /* IN */
                                              GAsyncReadyCallback  callback,    /* IN */
                                              gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_get_sources_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "GetSources");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_get_sources_finish (PkConnection  *connection,  /* IN */
                                               GAsyncResult  *result,      /* IN */
                                               gint         **sources,     /* OUT */
                                               gsize         *sources_len, /* OUT */
                                               GError       **error)       /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };
	gchar **sources_paths = NULL;
	gint sources_paths_len = 0;

	g_return_val_if_fail(sources != NULL, FALSE);
	g_return_val_if_fail(sources_len != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_get_sources), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*sources = NULL;
	*sources_len = 0;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &sources_paths, &sources_paths_len,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	if (sources_paths) {
		gint i;

		*sources = g_new0(gint, sources_paths_len);
		for (i = 0; i < sources_paths_len; i++) {
			sscanf(sources_paths[i],
			       "/org/perfkit/Agent/Source/%d",
			           &((*sources)[i]));
		}
		*sources_len = sources_paths_len;
	}

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_get_state_async (PkConnection        *connection,  /* IN */
                                            gint                 channel,     /* IN */
                                            GCancellable        *cancellable, /* IN */
                                            GAsyncReadyCallback  callback,    /* IN */
                                            gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_get_state_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "GetState");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_get_state_finish (PkConnection  *connection, /* IN */
                                             GAsyncResult  *result,     /* IN */
                                             gint          *state,      /* OUT */
                                             GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(state != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_get_state), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*state = 0;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_INT32, state,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_get_target_async (PkConnection        *connection,  /* IN */
                                             gint                 channel,     /* IN */
                                             GCancellable        *cancellable, /* IN */
                                             GAsyncReadyCallback  callback,    /* IN */
                                             gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_get_target_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "GetTarget");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_get_target_finish (PkConnection  *connection, /* IN */
                                              GAsyncResult  *result,     /* IN */
                                              gchar        **target,     /* OUT */
                                              GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(target != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_get_target), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*target = NULL;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_STRING, target,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	*target = g_strdup(*target);

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_get_working_dir_async (PkConnection        *connection,  /* IN */
                                                  gint                 channel,     /* IN */
                                                  GCancellable        *cancellable, /* IN */
                                                  GAsyncReadyCallback  callback,    /* IN */
                                                  gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_get_working_dir_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "GetWorkingDir");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_get_working_dir_finish (PkConnection  *connection,  /* IN */
                                                   GAsyncResult  *result,      /* IN */
                                                   gchar        **working_dir, /* OUT */
                                                   GError       **error)       /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(working_dir != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_get_working_dir), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*working_dir = NULL;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_STRING, working_dir,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	*working_dir = g_strdup(*working_dir);

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_mute_async (PkConnection        *connection,  /* IN */
                                       gint                 channel,     /* IN */
                                       GCancellable        *cancellable, /* IN */
                                       GAsyncReadyCallback  callback,    /* IN */
                                       gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_mute_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "Mute");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_mute_finish (PkConnection  *connection, /* IN */
                                        GAsyncResult  *result,     /* IN */
                                        GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_mute), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_set_args_async (PkConnection         *connection,  /* IN */
                                           gint                  channel,     /* IN */
                                           gchar               **args,        /* IN */
                                           GCancellable         *cancellable, /* IN */
                                           GAsyncReadyCallback   callback,    /* IN */
                                           gpointer              user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_set_args_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "SetArgs");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);
	APPEND_STRV_PARAM(args);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_set_args_finish (PkConnection  *connection, /* IN */
                                            GAsyncResult  *result,     /* IN */
                                            GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_set_args), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_set_env_async (PkConnection         *connection,  /* IN */
                                          gint                  channel,     /* IN */
                                          gchar               **env,         /* IN */
                                          GCancellable         *cancellable, /* IN */
                                          GAsyncReadyCallback   callback,    /* IN */
                                          gpointer              user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_set_env_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "SetEnv");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);
	APPEND_STRV_PARAM(env);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_set_env_finish (PkConnection  *connection, /* IN */
                                           GAsyncResult  *result,     /* IN */
                                           GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_set_env), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_set_kill_pid_async (PkConnection        *connection,  /* IN */
                                               gint                 channel,     /* IN */
                                               gboolean             kill_pid,    /* IN */
                                               GCancellable        *cancellable, /* IN */
                                               GAsyncReadyCallback  callback,    /* IN */
                                               gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_set_kill_pid_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "SetKillPid");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);
	APPEND_BOOLEAN_PARAM(kill_pid);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_set_kill_pid_finish (PkConnection  *connection, /* IN */
                                                GAsyncResult  *result,     /* IN */
                                                GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_set_kill_pid), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_set_pid_async (PkConnection        *connection,  /* IN */
                                          gint                 channel,     /* IN */
                                          gint                 pid,         /* IN */
                                          GCancellable        *cancellable, /* IN */
                                          GAsyncReadyCallback  callback,    /* IN */
                                          gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_set_pid_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "SetPid");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);
	APPEND_INT_PARAM(pid);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_set_pid_finish (PkConnection  *connection, /* IN */
                                           GAsyncResult  *result,     /* IN */
                                           GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_set_pid), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_set_target_async (PkConnection        *connection,  /* IN */
                                             gint                 channel,     /* IN */
                                             const gchar         *target,      /* IN */
                                             GCancellable        *cancellable, /* IN */
                                             GAsyncReadyCallback  callback,    /* IN */
                                             gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_set_target_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "SetTarget");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);
	APPEND_STRING_PARAM(target);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_set_target_finish (PkConnection  *connection, /* IN */
                                              GAsyncResult  *result,     /* IN */
                                              GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_set_target), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_set_working_dir_async (PkConnection        *connection,  /* IN */
                                                  gint                 channel,     /* IN */
                                                  const gchar         *working_dir, /* IN */
                                                  GCancellable        *cancellable, /* IN */
                                                  GAsyncReadyCallback  callback,    /* IN */
                                                  gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_set_working_dir_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "SetWorkingDir");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);
	APPEND_STRING_PARAM(working_dir);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_set_working_dir_finish (PkConnection  *connection, /* IN */
                                                   GAsyncResult  *result,     /* IN */
                                                   GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_set_working_dir), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_start_async (PkConnection        *connection,  /* IN */
                                        gint                 channel,     /* IN */
                                        GCancellable        *cancellable, /* IN */
                                        GAsyncReadyCallback  callback,    /* IN */
                                        gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_start_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "Start");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_start_finish (PkConnection  *connection, /* IN */
                                         GAsyncResult  *result,     /* IN */
                                         GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_start), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_stop_async (PkConnection        *connection,  /* IN */
                                       gint                 channel,     /* IN */
                                       GCancellable        *cancellable, /* IN */
                                       GAsyncReadyCallback  callback,    /* IN */
                                       gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_stop_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "Stop");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_stop_finish (PkConnection  *connection, /* IN */
                                        GAsyncResult  *result,     /* IN */
                                        GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_stop), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_channel_unmute_async (PkConnection        *connection,  /* IN */
                                         gint                 channel,     /* IN */
                                         GCancellable        *cancellable, /* IN */
                                         GAsyncReadyCallback  callback,    /* IN */
                                         gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_channel_unmute_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Channel");
	dbus_message_set_member(msg, "Unmute");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d",
	                            channel);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_channel_unmute_finish (PkConnection  *connection, /* IN */
                                          GAsyncResult  *result,     /* IN */
                                          GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_unmute), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_encoder_get_plugin_async (PkConnection        *connection,  /* IN */
                                             gint                 encoder,     /* IN */
                                             GCancellable        *cancellable, /* IN */
                                             GAsyncReadyCallback  callback,    /* IN */
                                             gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_encoder_get_plugin_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Encoder");
	dbus_message_set_member(msg, "GetPlugin");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Encoder/%d",
	                            encoder);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_encoder_get_plugin_finish (PkConnection  *connection, /* IN */
                                              GAsyncResult  *result,     /* IN */
                                              gchar        **pluign,     /* OUT */
                                              GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(pluign != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(encoder_get_plugin), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*pluign = NULL;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_STRING, pluign,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	*pluign = g_strdup(*pluign);

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_manager_add_channel_async (PkConnection        *connection,  /* IN */
                                              GCancellable        *cancellable, /* IN */
                                              GAsyncReadyCallback  callback,    /* IN */
                                              gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessage *msg;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_manager_add_channel_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Manager");
	dbus_message_set_member(msg, "AddChannel");
	dbus_message_set_path(msg, "/org/perfkit/Agent/Manager");

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_manager_add_channel_finish (PkConnection  *connection, /* IN */
                                               GAsyncResult  *result,     /* IN */
                                               gint          *channel,    /* OUT */
                                               GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };
	gchar *channel_path = NULL;

	g_return_val_if_fail(channel != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(manager_add_channel), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*channel = 0;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_OBJECT_PATH, &channel_path,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	if (channel_path) {
		sscanf(channel_path,
		       "/org/perfkit/Agent/Manager/%d",
		       channel);
	}

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_manager_add_subscription_async (PkConnection        *connection,  /* IN */
                                                   gsize                buffer_size, /* IN */
                                                   gsize                timeout,     /* IN */
                                                   gint                 encoder,     /* IN */
                                                   GCancellable        *cancellable, /* IN */
                                                   GAsyncReadyCallback  callback,    /* IN */
                                                   gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_manager_add_subscription_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Manager");
	dbus_message_set_member(msg, "AddSubscription");
	dbus_message_set_path(msg, "/org/perfkit/Agent/Manager");

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);
	APPEND_LONG_PARAM(buffer_size);
	APPEND_LONG_PARAM(timeout);
	APPEND_INT_PARAM(encoder);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_manager_add_subscription_finish (PkConnection  *connection,   /* IN */
                                                    GAsyncResult  *result,       /* IN */
                                                    gint          *subscription, /* OUT */
                                                    GError       **error)        /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };
	gchar *subscription_path = NULL;

	g_return_val_if_fail(subscription != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(manager_add_subscription), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*subscription = 0;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_OBJECT_PATH, &subscription_path,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	if (subscription_path) {
		sscanf(subscription_path,
		       "/org/perfkit/Agent/Manager/%d",
		       subscription);
	}

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_manager_get_channels_async (PkConnection        *connection,  /* IN */
                                               GCancellable        *cancellable, /* IN */
                                               GAsyncReadyCallback  callback,    /* IN */
                                               gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessage *msg;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_manager_get_channels_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Manager");
	dbus_message_set_member(msg, "GetChannels");
	dbus_message_set_path(msg, "/org/perfkit/Agent/Manager");

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_manager_get_channels_finish (PkConnection  *connection,   /* IN */
                                                GAsyncResult  *result,       /* IN */
                                                gint         **channels,     /* OUT */
                                                gsize         *channels_len, /* OUT */
                                                GError       **error)        /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };
	gchar **channels_paths = NULL;
	gint channels_paths_len = 0;

	g_return_val_if_fail(channels != NULL, FALSE);
	g_return_val_if_fail(channels_len != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(manager_get_channels), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*channels = NULL;
	*channels_len = 0;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &channels_paths, &channels_paths_len,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	if (channels_paths) {
		gint i;

		*channels = g_new0(gint, channels_paths_len);
		for (i = 0; i < channels_paths_len; i++) {
			sscanf(channels_paths[i],
			       "/org/perfkit/Agent/Channel/%d",
			           &((*channels)[i]));
		}
		*channels_len = channels_paths_len;
	}

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_manager_get_plugins_async (PkConnection        *connection,  /* IN */
                                              GCancellable        *cancellable, /* IN */
                                              GAsyncReadyCallback  callback,    /* IN */
                                              gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessage *msg;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_manager_get_plugins_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Manager");
	dbus_message_set_member(msg, "GetPlugins");
	dbus_message_set_path(msg, "/org/perfkit/Agent/Manager");

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_manager_get_plugins_finish (PkConnection   *connection, /* IN */
                                               GAsyncResult   *result,     /* IN */
                                               gchar        ***plugins,    /* OUT */
                                               GError        **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };
	gchar **plugins_paths = NULL;
	gint plugins_paths_len = 0;

	g_return_val_if_fail(plugins != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(manager_get_plugins), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*plugins = NULL;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &plugins_paths, &plugins_paths_len,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	if (plugins_paths) {
		gint i;

		*plugins = g_new0(gchar*, plugins_paths_len + 1);
		for (i = 0; i < plugins_paths_len; i++) {
			sscanf(plugins_paths[i],
			       "/org/perfkit/Agent/Plugin/%as",
			           &((*plugins)[i]));
		}
	}

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_manager_get_version_async (PkConnection        *connection,  /* IN */
                                              GCancellable        *cancellable, /* IN */
                                              GAsyncReadyCallback  callback,    /* IN */
                                              gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessage *msg;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_manager_get_version_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Manager");
	dbus_message_set_member(msg, "GetVersion");
	dbus_message_set_path(msg, "/org/perfkit/Agent/Manager");

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_manager_get_version_finish (PkConnection  *connection, /* IN */
                                               GAsyncResult  *result,     /* IN */
                                               gchar        **version,    /* OUT */
                                               GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(version != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(manager_get_version), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*version = NULL;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_STRING, version,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	*version = g_strdup(*version);

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_manager_ping_async (PkConnection        *connection,  /* IN */
                                       GCancellable        *cancellable, /* IN */
                                       GAsyncReadyCallback  callback,    /* IN */
                                       gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessage *msg;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_manager_ping_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Manager");
	dbus_message_set_member(msg, "Ping");
	dbus_message_set_path(msg, "/org/perfkit/Agent/Manager");

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_manager_ping_finish (PkConnection  *connection, /* IN */
                                        GAsyncResult  *result,     /* IN */
                                        GTimeVal      *tv,         /* OUT */
                                        GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };
	gchar *tv_str = NULL;

	g_return_val_if_fail(tv != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(manager_ping), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	memset(tv, 0, sizeof(GTimeVal));

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_STRING, &tv_str,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	g_time_val_from_iso8601(tv_str, tv);

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_manager_remove_channel_async (PkConnection        *connection,  /* IN */
                                                 gint                 channel,     /* IN */
                                                 GCancellable        *cancellable, /* IN */
                                                 GAsyncReadyCallback  callback,    /* IN */
                                                 gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_manager_remove_channel_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Manager");
	dbus_message_set_member(msg, "RemoveChannel");
	dbus_message_set_path(msg, "/org/perfkit/Agent/Manager");

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);
	APPEND_INT_PARAM(channel);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_manager_remove_channel_finish (PkConnection  *connection, /* IN */
                                                  GAsyncResult  *result,     /* IN */
                                                  gboolean      *removed,    /* OUT */
                                                  GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(removed != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(manager_remove_channel), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*removed = FALSE;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_BOOLEAN, removed,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_manager_remove_subscription_async (PkConnection        *connection,   /* IN */
                                                      gint                 subscription, /* IN */
                                                      GCancellable        *cancellable,  /* IN */
                                                      GAsyncReadyCallback  callback,     /* IN */
                                                      gpointer             user_data)    /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_manager_remove_subscription_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Manager");
	dbus_message_set_member(msg, "RemoveSubscription");
	dbus_message_set_path(msg, "/org/perfkit/Agent/Manager");

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);
	APPEND_INT_PARAM(subscription);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_manager_remove_subscription_finish (PkConnection  *connection, /* IN */
                                                       GAsyncResult  *result,     /* IN */
                                                       gboolean      *removed,    /* OUT */
                                                       GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(removed != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(manager_remove_subscription), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*removed = FALSE;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_BOOLEAN, removed,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_plugin_create_encoder_async (PkConnection        *connection,  /* IN */
                                                const gchar         *plugin,      /* IN */
                                                GCancellable        *cancellable, /* IN */
                                                GAsyncReadyCallback  callback,    /* IN */
                                                gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_plugin_create_encoder_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Plugin");
	dbus_message_set_member(msg, "CreateEncoder");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Plugin/%s",
	                            plugin);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_plugin_create_encoder_finish (PkConnection  *connection, /* IN */
                                                 GAsyncResult  *result,     /* IN */
                                                 gint          *encoder,    /* OUT */
                                                 GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };
	gchar *encoder_path = NULL;

	g_return_val_if_fail(encoder != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(plugin_create_encoder), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*encoder = 0;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_OBJECT_PATH, &encoder_path,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	if (encoder_path) {
		sscanf(encoder_path,
		       "/org/perfkit/Agent/Plugin/%d",
		       encoder);
	}

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_plugin_create_source_async (PkConnection        *connection,  /* IN */
                                               const gchar         *plugin,      /* IN */
                                               GCancellable        *cancellable, /* IN */
                                               GAsyncReadyCallback  callback,    /* IN */
                                               gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_plugin_create_source_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Plugin");
	dbus_message_set_member(msg, "CreateSource");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Plugin/%s",
	                            plugin);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_plugin_create_source_finish (PkConnection  *connection, /* IN */
                                                GAsyncResult  *result,     /* IN */
                                                gint          *source,     /* OUT */
                                                GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };
	gchar *source_path = NULL;

	g_return_val_if_fail(source != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(plugin_create_source), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*source = 0;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_OBJECT_PATH, &source_path,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	if (source_path) {
		sscanf(source_path,
		       "/org/perfkit/Agent/Plugin/%d",
		       source);
	}

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_plugin_get_copyright_async (PkConnection        *connection,  /* IN */
                                               const gchar         *plugin,      /* IN */
                                               GCancellable        *cancellable, /* IN */
                                               GAsyncReadyCallback  callback,    /* IN */
                                               gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_plugin_get_copyright_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Plugin");
	dbus_message_set_member(msg, "GetCopyright");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Plugin/%s",
	                            plugin);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_plugin_get_copyright_finish (PkConnection  *connection, /* IN */
                                                GAsyncResult  *result,     /* IN */
                                                gchar        **copyright,  /* OUT */
                                                GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(copyright != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(plugin_get_copyright), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*copyright = NULL;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_STRING, copyright,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	*copyright = g_strdup(*copyright);

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_plugin_get_description_async (PkConnection        *connection,  /* IN */
                                                 const gchar         *plugin,      /* IN */
                                                 GCancellable        *cancellable, /* IN */
                                                 GAsyncReadyCallback  callback,    /* IN */
                                                 gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_plugin_get_description_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Plugin");
	dbus_message_set_member(msg, "GetDescription");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Plugin/%s",
	                            plugin);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_plugin_get_description_finish (PkConnection  *connection,  /* IN */
                                                  GAsyncResult  *result,      /* IN */
                                                  gchar        **description, /* OUT */
                                                  GError       **error)       /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(description != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(plugin_get_description), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*description = NULL;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_STRING, description,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	*description = g_strdup(*description);

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_plugin_get_name_async (PkConnection        *connection,  /* IN */
                                          const gchar         *plugin,      /* IN */
                                          GCancellable        *cancellable, /* IN */
                                          GAsyncReadyCallback  callback,    /* IN */
                                          gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_plugin_get_name_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Plugin");
	dbus_message_set_member(msg, "GetName");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Plugin/%s",
	                            plugin);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_plugin_get_name_finish (PkConnection  *connection, /* IN */
                                           GAsyncResult  *result,     /* IN */
                                           gchar        **name,       /* OUT */
                                           GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(name != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(plugin_get_name), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*name = NULL;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_STRING, name,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	*name = g_strdup(*name);

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_plugin_get_plugin_type_async (PkConnection        *connection,  /* IN */
                                                 const gchar         *plugin,      /* IN */
                                                 GCancellable        *cancellable, /* IN */
                                                 GAsyncReadyCallback  callback,    /* IN */
                                                 gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_plugin_get_plugin_type_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Plugin");
	dbus_message_set_member(msg, "GetType");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Plugin/%s",
	                            plugin);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_plugin_get_plugin_type_finish (PkConnection  *connection, /* IN */
                                                  GAsyncResult  *result,     /* IN */
                                                  gint          *type,       /* OUT */
                                                  GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(type != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(plugin_get_plugin_type), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*type = 0;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_INT32, type,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_plugin_get_version_async (PkConnection        *connection,  /* IN */
                                             const gchar         *plugin,      /* IN */
                                             GCancellable        *cancellable, /* IN */
                                             GAsyncReadyCallback  callback,    /* IN */
                                             gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_plugin_get_version_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Plugin");
	dbus_message_set_member(msg, "GetVersion");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Plugin/%s",
	                            plugin);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_plugin_get_version_finish (PkConnection  *connection, /* IN */
                                              GAsyncResult  *result,     /* IN */
                                              gchar        **version,    /* OUT */
                                              GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(version != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(plugin_get_version), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*version = NULL;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_STRING, version,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	*version = g_strdup(*version);

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_source_get_plugin_async (PkConnection        *connection,  /* IN */
                                            gint                 source,      /* IN */
                                            GCancellable        *cancellable, /* IN */
                                            GAsyncReadyCallback  callback,    /* IN */
                                            gpointer             user_data)   /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_source_get_plugin_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Source");
	dbus_message_set_member(msg, "GetPlugin");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Source/%d",
	                            source);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_source_get_plugin_finish (PkConnection  *connection, /* IN */
                                             GAsyncResult  *result,     /* IN */
                                             gchar        **plugin,     /* OUT */
                                             GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;
	DBusError dbus_error = { 0 };
	gchar *plugin_path = NULL;

	g_return_val_if_fail(plugin != NULL, FALSE);
	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(source_get_plugin), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */
	*plugin = NULL;

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}

	/*
	 * Process message arguments.
	 */
	if (!dbus_message_get_args(msg,
	                           &dbus_error,

	                           DBUS_TYPE_OBJECT_PATH, &plugin_path,
	                           DBUS_TYPE_INVALID)) {
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		GOTO(finish);
	}

	if (plugin_path) {
		sscanf(plugin_path,
		       "/org/perfkit/Agent/Plugin/%as",
		       plugin);
	}

	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_subscription_add_channel_async (PkConnection        *connection,   /* IN */
                                                   gint                 subscription, /* IN */
                                                   gint                 channel,      /* IN */
                                                   gboolean             monitor,      /* IN */
                                                   GCancellable        *cancellable,  /* IN */
                                                   GAsyncReadyCallback  callback,     /* IN */
                                                   gpointer             user_data)    /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_subscription_add_channel_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Subscription");
	dbus_message_set_member(msg, "AddChannel");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Subscription/%d",
	                            subscription);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);
	APPEND_INT_PARAM(channel);
	APPEND_BOOLEAN_PARAM(monitor);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_subscription_add_channel_finish (PkConnection  *connection, /* IN */
                                                    GAsyncResult  *result,     /* IN */
                                                    GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(subscription_add_channel), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_subscription_add_source_async (PkConnection        *connection,   /* IN */
                                                  gint                 subscription, /* IN */
                                                  gint                 source,       /* IN */
                                                  GCancellable        *cancellable,  /* IN */
                                                  GAsyncReadyCallback  callback,     /* IN */
                                                  gpointer             user_data)    /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_subscription_add_source_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Subscription");
	dbus_message_set_member(msg, "AddSource");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Subscription/%d",
	                            subscription);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);
	APPEND_INT_PARAM(source);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_subscription_add_source_finish (PkConnection  *connection, /* IN */
                                                   GAsyncResult  *result,     /* IN */
                                                   GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(subscription_add_source), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_subscription_mute_async (PkConnection        *connection,   /* IN */
                                            gint                 subscription, /* IN */
                                            gboolean             drain,        /* IN */
                                            GCancellable        *cancellable,  /* IN */
                                            GAsyncReadyCallback  callback,     /* IN */
                                            gpointer             user_data)    /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_subscription_mute_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Subscription");
	dbus_message_set_member(msg, "Mute");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Subscription/%d",
	                            subscription);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);
	APPEND_BOOLEAN_PARAM(drain);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_subscription_mute_finish (PkConnection  *connection, /* IN */
                                             GAsyncResult  *result,     /* IN */
                                             GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(subscription_mute), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_subscription_remove_channel_async (PkConnection        *connection,   /* IN */
                                                      gint                 subscription, /* IN */
                                                      gint                 channel,      /* IN */
                                                      GCancellable        *cancellable,  /* IN */
                                                      GAsyncReadyCallback  callback,     /* IN */
                                                      gpointer             user_data)    /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_subscription_remove_channel_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Subscription");
	dbus_message_set_member(msg, "RemoveChannel");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Subscription/%d",
	                            subscription);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);
	APPEND_INT_PARAM(channel);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_subscription_remove_channel_finish (PkConnection  *connection, /* IN */
                                                       GAsyncResult  *result,     /* IN */
                                                       GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(subscription_remove_channel), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_subscription_remove_source_async (PkConnection        *connection,   /* IN */
                                                     gint                 subscription, /* IN */
                                                     gint                 source,       /* IN */
                                                     GCancellable        *cancellable,  /* IN */
                                                     GAsyncReadyCallback  callback,     /* IN */
                                                     gpointer             user_data)    /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_subscription_remove_source_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Subscription");
	dbus_message_set_member(msg, "RemoveSource");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Subscription/%d",
	                            subscription);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);
	APPEND_INT_PARAM(source);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_subscription_remove_source_finish (PkConnection  *connection, /* IN */
                                                      GAsyncResult  *result,     /* IN */
                                                      GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(subscription_remove_source), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_subscription_set_buffer_async (PkConnection        *connection,   /* IN */
                                                  gint                 subscription, /* IN */
                                                  gint                 timeout,      /* IN */
                                                  gint                 size,         /* IN */
                                                  GCancellable        *cancellable,  /* IN */
                                                  GAsyncReadyCallback  callback,     /* IN */
                                                  gpointer             user_data)    /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_subscription_set_buffer_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Subscription");
	dbus_message_set_member(msg, "SetBuffer");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Subscription/%d",
	                            subscription);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);
	APPEND_INT_PARAM(timeout);
	APPEND_INT_PARAM(size);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_subscription_set_buffer_finish (PkConnection  *connection, /* IN */
                                                   GAsyncResult  *result,     /* IN */
                                                   GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(subscription_set_buffer), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


static void
pk_connection_dbus_subscription_unmute_async (PkConnection        *connection,   /* IN */
                                              gint                 subscription, /* IN */
                                              GCancellable        *cancellable,  /* IN */
                                              GAsyncReadyCallback  callback,     /* IN */
                                              gpointer             user_data)    /* IN */
{
	PkConnectionDBusPrivate *priv;
	DBusPendingCall *call = NULL;
	GSimpleAsyncResult *result;
	DBusMessageIter iter;
	DBusMessage *msg;
	gchar *dbus_path;

	g_return_if_fail(PK_IS_CONNECTION_DBUS(connection));

	ENTRY;
	priv = PK_CONNECTION_DBUS(connection)->priv;

	/*
	 * Allocate DBus message.
	 */
	msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
	g_assert(msg);

	/*
	 * Create asynchronous connection handle.
	 */
	result = g_simple_async_result_new(
			G_OBJECT(connection), callback, user_data,
			pk_connection_dbus_subscription_unmute_async);

	/*
	 * Wire cancellable if needed.
	 */
	if (cancellable) {
		g_cancellable_connect(cancellable,
		                      G_CALLBACK(pk_connection_dbus_cancel),
		                      g_object_ref(result), g_object_unref);
	}

	/*
	 * Build the DBus message.
	 */
	dbus_message_set_destination(msg, "org.perfkit.Agent");
	dbus_message_set_interface(msg, "org.perfkit.Agent.Subscription");
	dbus_message_set_member(msg, "Unmute");
	dbus_path = g_strdup_printf("/org/perfkit/Agent/Subscription/%d",
	                            subscription);
	dbus_message_set_path(msg, dbus_path);
	g_free(dbus_path);

	/*
	 * Add message parameters.
	 */
	dbus_message_iter_init_append(msg, &iter);

	/*
	 * Send message to agent and schedule to be notified of the result.
	 */
	if (!dbus_connection_send_with_reply(priv->dbus, msg, &call, -1)) {
		g_warning("Error dispatching message to %s/%s",
		          dbus_message_get_path(msg),
		          dbus_message_get_member(msg));
		dbus_message_unref(msg);
		EXIT;
	}

	/*
	 * Get notified when the reply is received or timeout expires.
	 */
	dbus_pending_call_set_notify(call, pk_connection_dbus_notify,
	                             result, g_object_unref);

	/*
	 * Release resources.
	 */
	dbus_message_unref(msg);
	EXIT;
}


static gboolean
pk_connection_dbus_subscription_unmute_finish (PkConnection  *connection, /* IN */
                                               GAsyncResult  *result,     /* IN */
                                               GError       **error)      /* OUT */
{
	DBusPendingCall *call;
	DBusMessage *msg;
	gboolean ret = FALSE;
	gchar *error_str = NULL;

	g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(result), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(subscription_unmute), FALSE);
	g_return_val_if_fail((call = GET_RESULT_POINTER(DBusPendingCall, result)),
	                     FALSE);

	/*
	 * Clear out params.
	 */

	/*
	 * Check if call was cancelled.
	 */
	if (!(msg = dbus_pending_call_steal_reply(call))) {
		g_simple_async_result_propagate_error(
				G_SIMPLE_ASYNC_RESULT(result),
				error);
		goto finish;
	}

	/*
	 * Check if response is an error.
	 */
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_ERROR) {
		dbus_message_get_args(msg, NULL,
		                      DBUS_TYPE_STRING, &error_str,
		                      DBUS_TYPE_INVALID);
		g_set_error(error, PK_CONNECTION_DBUS_ERROR,
		            PK_CONNECTION_DBUS_ERROR_DBUS,
		            "%s: %s",
		            dbus_message_get_error_name(msg),
		            error_str);
		goto finish;
	}


	ret = TRUE;

finish:
	dbus_message_unref(msg);
	g_object_unref(result);
	RETURN(ret);
}


/**
 * pk_connection_dbus_finalize:
 * @object: A #PkConnectionDBus.
 *
 * Releases all memory allocated by the #PkConnectionDBus instance.
 *
 * Returns: None.
 * Side effects: DBus connection is released.
 */
static void
pk_connection_dbus_finalize (GObject *object)
{
	PkConnectionDBusPrivate *priv;

	priv = PK_CONNECTION_DBUS(object)->priv;

	if (priv->dbus) {
		dbus_connection_unref(priv->dbus);
	}

	G_OBJECT_CLASS(pk_connection_dbus_parent_class)->finalize(object);
}

/**
 * pk_connection_dbus_class_init:
 * @klass: A #PkConnectionDBusClass
 *
 * Initializes the vtable for the #PkConnectionClass.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_dbus_class_init (PkConnectionDBusClass *klass)
{
	GObjectClass *object_class;
	PkConnectionClass *connection_class;

	object_class = G_OBJECT_CLASS(klass);
	connection_class = PK_CONNECTION_CLASS(klass);

	object_class->finalize = pk_connection_dbus_finalize;
	g_type_class_add_private(object_class, sizeof(PkConnectionDBusPrivate));

	#define OVERRIDE_VTABLE(_n) G_STMT_START {                                \
            connection_class->_n##_async = pk_connection_dbus_##_n##_async;   \
            connection_class->_n##_finish = pk_connection_dbus_##_n##_finish; \
        } G_STMT_END

	OVERRIDE_VTABLE(channel_get_args);
	OVERRIDE_VTABLE(channel_get_env);
	OVERRIDE_VTABLE(channel_get_exit_status);
	OVERRIDE_VTABLE(channel_get_kill_pid);
	OVERRIDE_VTABLE(channel_get_pid);
	OVERRIDE_VTABLE(channel_get_pid_set);
	OVERRIDE_VTABLE(channel_get_sources);
	OVERRIDE_VTABLE(channel_get_state);
	OVERRIDE_VTABLE(channel_get_target);
	OVERRIDE_VTABLE(channel_get_working_dir);
	OVERRIDE_VTABLE(channel_mute);
	OVERRIDE_VTABLE(channel_set_args);
	OVERRIDE_VTABLE(channel_set_env);
	OVERRIDE_VTABLE(channel_set_kill_pid);
	OVERRIDE_VTABLE(channel_set_pid);
	OVERRIDE_VTABLE(channel_set_target);
	OVERRIDE_VTABLE(channel_set_working_dir);
	OVERRIDE_VTABLE(channel_start);
	OVERRIDE_VTABLE(channel_stop);
	OVERRIDE_VTABLE(channel_unmute);
	OVERRIDE_VTABLE(connect);
	OVERRIDE_VTABLE(disconnect);
	OVERRIDE_VTABLE(encoder_get_plugin);
	OVERRIDE_VTABLE(manager_add_channel);
	OVERRIDE_VTABLE(manager_add_subscription);
	OVERRIDE_VTABLE(manager_get_channels);
	OVERRIDE_VTABLE(manager_get_plugins);
	OVERRIDE_VTABLE(manager_get_version);
	OVERRIDE_VTABLE(manager_ping);
	OVERRIDE_VTABLE(manager_remove_channel);
	OVERRIDE_VTABLE(manager_remove_subscription);
	OVERRIDE_VTABLE(plugin_create_encoder);
	OVERRIDE_VTABLE(plugin_create_source);
	OVERRIDE_VTABLE(plugin_get_copyright);
	OVERRIDE_VTABLE(plugin_get_description);
	OVERRIDE_VTABLE(plugin_get_name);
	OVERRIDE_VTABLE(plugin_get_plugin_type);
	OVERRIDE_VTABLE(plugin_get_version);
	OVERRIDE_VTABLE(source_get_plugin);
	OVERRIDE_VTABLE(subscription_add_channel);
	OVERRIDE_VTABLE(subscription_add_source);
	OVERRIDE_VTABLE(subscription_mute);
	OVERRIDE_VTABLE(subscription_remove_channel);
	OVERRIDE_VTABLE(subscription_remove_source);
	OVERRIDE_VTABLE(subscription_set_buffer);
	OVERRIDE_VTABLE(subscription_unmute);

	#undef ADD_RPC
}

/**
 * pk_connection_dbus_init:
 * @dbus: A #PkConnectionDBus.
 *
 * Initializes a new instance of #PkConnectionDBus.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_connection_dbus_init (PkConnectionDBus *dbus)
{
	dbus->priv = G_TYPE_INSTANCE_GET_PRIVATE(dbus,
	                                         PK_TYPE_CONNECTION_DBUS,
	                                         PkConnectionDBusPrivate);
	dbus->priv->mutex = g_mutex_new();
}

/**
 * pk_connection_dbus_error_quark:
 *
 * Retrieves the #GQuark representing the #PkConnectionDBus error domain.
 *
 * Returns: A #GQuark.
 * Side effects: The error quark may be registered.
 */
GQuark
pk_connection_dbus_error_quark (void)
{
	return g_quark_from_string("pk-connection-dbus-error-quark");
}

/**
 * pk_connection_register:
 *
 * Module entry point.  Retrieves the #GType for the PkConnectionDBus class.
 *
 * Returns: A #GType.
 * Side effects: None.
 */
G_MODULE_EXPORT GType
pk_connection_register (void)
{
	return PK_TYPE_CONNECTION_DBUS;
}
