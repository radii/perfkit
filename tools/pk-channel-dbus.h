/* Generated by dbus-binding-tool; do not edit! */

#include <glib.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#ifndef _DBUS_GLIB_ASYNC_DATA_FREE
#define _DBUS_GLIB_ASYNC_DATA_FREE
static
#ifdef G_HAVE_INLINE
inline
#endif
void
_dbus_glib_async_data_free (gpointer stuff)
{
	g_slice_free (DBusGAsyncData, stuff);
}
#endif

#ifndef DBUS_GLIB_CLIENT_WRAPPERS_com_dronelabs_Perfkit_Channel
#define DBUS_GLIB_CLIENT_WRAPPERS_com_dronelabs_Perfkit_Channel

static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
com_dronelabs_Perfkit_Channel_get_target (DBusGProxy *proxy, char ** OUT_target, GError **error)

{
  return dbus_g_proxy_call (proxy, "GetTarget", error, G_TYPE_INVALID, G_TYPE_STRING, OUT_target, G_TYPE_INVALID);
}

typedef void (*com_dronelabs_Perfkit_Channel_get_target_reply) (DBusGProxy *proxy, char * OUT_target, GError *error, gpointer userdata);

static void
com_dronelabs_Perfkit_Channel_get_target_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char * OUT_target;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &OUT_target, G_TYPE_INVALID);
  (*(com_dronelabs_Perfkit_Channel_get_target_reply)data->cb) (proxy, OUT_target, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
com_dronelabs_Perfkit_Channel_get_target_async (DBusGProxy *proxy, com_dronelabs_Perfkit_Channel_get_target_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "GetTarget", com_dronelabs_Perfkit_Channel_get_target_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
com_dronelabs_Perfkit_Channel_get_args (DBusGProxy *proxy, char *** OUT_args, GError **error)

{
  return dbus_g_proxy_call (proxy, "GetArgs", error, G_TYPE_INVALID, G_TYPE_STRV, OUT_args, G_TYPE_INVALID);
}

typedef void (*com_dronelabs_Perfkit_Channel_get_args_reply) (DBusGProxy *proxy, char * *OUT_args, GError *error, gpointer userdata);

static void
com_dronelabs_Perfkit_Channel_get_args_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char ** OUT_args;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRV, &OUT_args, G_TYPE_INVALID);
  (*(com_dronelabs_Perfkit_Channel_get_args_reply)data->cb) (proxy, OUT_args, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
com_dronelabs_Perfkit_Channel_get_args_async (DBusGProxy *proxy, com_dronelabs_Perfkit_Channel_get_args_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "GetArgs", com_dronelabs_Perfkit_Channel_get_args_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
com_dronelabs_Perfkit_Channel_start (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "Start", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*com_dronelabs_Perfkit_Channel_start_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
com_dronelabs_Perfkit_Channel_start_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(com_dronelabs_Perfkit_Channel_start_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
com_dronelabs_Perfkit_Channel_start_async (DBusGProxy *proxy, com_dronelabs_Perfkit_Channel_start_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "Start", com_dronelabs_Perfkit_Channel_start_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
com_dronelabs_Perfkit_Channel_stop (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "Stop", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*com_dronelabs_Perfkit_Channel_stop_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
com_dronelabs_Perfkit_Channel_stop_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(com_dronelabs_Perfkit_Channel_stop_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
com_dronelabs_Perfkit_Channel_stop_async (DBusGProxy *proxy, com_dronelabs_Perfkit_Channel_stop_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "Stop", com_dronelabs_Perfkit_Channel_stop_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
com_dronelabs_Perfkit_Channel_pause (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "Pause", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*com_dronelabs_Perfkit_Channel_pause_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
com_dronelabs_Perfkit_Channel_pause_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(com_dronelabs_Perfkit_Channel_pause_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
com_dronelabs_Perfkit_Channel_pause_async (DBusGProxy *proxy, com_dronelabs_Perfkit_Channel_pause_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "Pause", com_dronelabs_Perfkit_Channel_pause_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
com_dronelabs_Perfkit_Channel_unpause (DBusGProxy *proxy, GError **error)

{
  return dbus_g_proxy_call (proxy, "Unpause", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*com_dronelabs_Perfkit_Channel_unpause_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
com_dronelabs_Perfkit_Channel_unpause_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(com_dronelabs_Perfkit_Channel_unpause_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
com_dronelabs_Perfkit_Channel_unpause_async (DBusGProxy *proxy, com_dronelabs_Perfkit_Channel_unpause_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "Unpause", com_dronelabs_Perfkit_Channel_unpause_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
#endif /* defined DBUS_GLIB_CLIENT_WRAPPERS_com_dronelabs_Perfkit_Channel */

G_END_DECLS
