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
com_dronelabs_Perfkit_Channel_add_source (DBusGProxy *proxy, const char* IN_source_info_path, char** OUT_source_path, GError **error)

{
  return dbus_g_proxy_call (proxy, "AddSource", error, DBUS_TYPE_G_OBJECT_PATH, IN_source_info_path, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, OUT_source_path, G_TYPE_INVALID);
}

typedef void (*com_dronelabs_Perfkit_Channel_add_source_reply) (DBusGProxy *proxy, char *OUT_source_path, GError *error, gpointer userdata);

static void
com_dronelabs_Perfkit_Channel_add_source_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char* OUT_source_path;
  dbus_g_proxy_end_call (proxy, call, &error, DBUS_TYPE_G_OBJECT_PATH, &OUT_source_path, G_TYPE_INVALID);
  (*(com_dronelabs_Perfkit_Channel_add_source_reply)data->cb) (proxy, OUT_source_path, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
com_dronelabs_Perfkit_Channel_add_source_async (DBusGProxy *proxy, const char* IN_source_info_path, com_dronelabs_Perfkit_Channel_add_source_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "AddSource", com_dronelabs_Perfkit_Channel_add_source_async_callback, stuff, _dbus_glib_async_data_free, DBUS_TYPE_G_OBJECT_PATH, IN_source_info_path, G_TYPE_INVALID);
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
com_dronelabs_Perfkit_Channel_stop (DBusGProxy *proxy, const gboolean IN_killpid, GError **error)

{
  return dbus_g_proxy_call (proxy, "Stop", error, G_TYPE_BOOLEAN, IN_killpid, G_TYPE_INVALID, G_TYPE_INVALID);
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
com_dronelabs_Perfkit_Channel_stop_async (DBusGProxy *proxy, const gboolean IN_killpid, com_dronelabs_Perfkit_Channel_stop_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "Stop", com_dronelabs_Perfkit_Channel_stop_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_BOOLEAN, IN_killpid, G_TYPE_INVALID);
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
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
com_dronelabs_Perfkit_Channel_get_state (DBusGProxy *proxy, gint* OUT_state, GError **error)

{
  return dbus_g_proxy_call (proxy, "GetState", error, G_TYPE_INVALID, G_TYPE_INT, OUT_state, G_TYPE_INVALID);
}

typedef void (*com_dronelabs_Perfkit_Channel_get_state_reply) (DBusGProxy *proxy, gint OUT_state, GError *error, gpointer userdata);

static void
com_dronelabs_Perfkit_Channel_get_state_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  gint OUT_state;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INT, &OUT_state, G_TYPE_INVALID);
  (*(com_dronelabs_Perfkit_Channel_get_state_reply)data->cb) (proxy, OUT_state, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
com_dronelabs_Perfkit_Channel_get_state_async (DBusGProxy *proxy, com_dronelabs_Perfkit_Channel_get_state_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "GetState", com_dronelabs_Perfkit_Channel_get_state_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
com_dronelabs_Perfkit_Channel_get_pid (DBusGProxy *proxy, gint* OUT_pid, GError **error)

{
  return dbus_g_proxy_call (proxy, "GetPid", error, G_TYPE_INVALID, G_TYPE_INT, OUT_pid, G_TYPE_INVALID);
}

typedef void (*com_dronelabs_Perfkit_Channel_get_pid_reply) (DBusGProxy *proxy, gint OUT_pid, GError *error, gpointer userdata);

static void
com_dronelabs_Perfkit_Channel_get_pid_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  gint OUT_pid;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INT, &OUT_pid, G_TYPE_INVALID);
  (*(com_dronelabs_Perfkit_Channel_get_pid_reply)data->cb) (proxy, OUT_pid, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
com_dronelabs_Perfkit_Channel_get_pid_async (DBusGProxy *proxy, com_dronelabs_Perfkit_Channel_get_pid_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "GetPid", com_dronelabs_Perfkit_Channel_get_pid_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
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
com_dronelabs_Perfkit_Channel_get_working_dir (DBusGProxy *proxy, char ** OUT_working_dir, GError **error)

{
  return dbus_g_proxy_call (proxy, "GetWorkingDir", error, G_TYPE_INVALID, G_TYPE_STRING, OUT_working_dir, G_TYPE_INVALID);
}

typedef void (*com_dronelabs_Perfkit_Channel_get_working_dir_reply) (DBusGProxy *proxy, char * OUT_working_dir, GError *error, gpointer userdata);

static void
com_dronelabs_Perfkit_Channel_get_working_dir_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char * OUT_working_dir;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &OUT_working_dir, G_TYPE_INVALID);
  (*(com_dronelabs_Perfkit_Channel_get_working_dir_reply)data->cb) (proxy, OUT_working_dir, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
com_dronelabs_Perfkit_Channel_get_working_dir_async (DBusGProxy *proxy, com_dronelabs_Perfkit_Channel_get_working_dir_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "GetWorkingDir", com_dronelabs_Perfkit_Channel_get_working_dir_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
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
com_dronelabs_Perfkit_Channel_get_env (DBusGProxy *proxy, char *** OUT_env, GError **error)

{
  return dbus_g_proxy_call (proxy, "GetEnv", error, G_TYPE_INVALID, G_TYPE_STRV, OUT_env, G_TYPE_INVALID);
}

typedef void (*com_dronelabs_Perfkit_Channel_get_env_reply) (DBusGProxy *proxy, char * *OUT_env, GError *error, gpointer userdata);

static void
com_dronelabs_Perfkit_Channel_get_env_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char ** OUT_env;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRV, &OUT_env, G_TYPE_INVALID);
  (*(com_dronelabs_Perfkit_Channel_get_env_reply)data->cb) (proxy, OUT_env, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
com_dronelabs_Perfkit_Channel_get_env_async (DBusGProxy *proxy, com_dronelabs_Perfkit_Channel_get_env_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "GetEnv", com_dronelabs_Perfkit_Channel_get_env_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
com_dronelabs_Perfkit_Channel_remove_source (DBusGProxy *proxy, const gint IN_source_id, GError **error)

{
  return dbus_g_proxy_call (proxy, "RemoveSource", error, G_TYPE_INT, IN_source_id, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*com_dronelabs_Perfkit_Channel_remove_source_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
com_dronelabs_Perfkit_Channel_remove_source_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(com_dronelabs_Perfkit_Channel_remove_source_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
com_dronelabs_Perfkit_Channel_remove_source_async (DBusGProxy *proxy, const gint IN_source_id, com_dronelabs_Perfkit_Channel_remove_source_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "RemoveSource", com_dronelabs_Perfkit_Channel_remove_source_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INT, IN_source_id, G_TYPE_INVALID);
}
#endif /* defined DBUS_GLIB_CLIENT_WRAPPERS_com_dronelabs_Perfkit_Channel */

G_END_DECLS
