#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

static gchar *tmpdir = NULL;
static GMainLoop *loop = NULL;

static DBusGConnection*
get_conn (void)
{
	static DBusGConnection *dbus_conn = NULL;
	if (!dbus_conn) {
		dbus_conn = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
		g_assert(dbus_conn);
	}
	return dbus_conn;
}

static DBusGProxy*
get_manager_proxy (void)
{
	return dbus_g_proxy_new_for_name(get_conn(),
	                                 "com.dronelabs.Perfkit",
	                                 "/com/dronelabs/Perfkit/Manager",
	                                 "com.dronelabs.Perfkit.Manager");
}

static DBusGProxy*
get_channel_proxy (const gchar *path)
{
	return dbus_g_proxy_new_for_name(get_conn(),
	                                 "com.dronelabs.Perfkit",
	                                 path,
	                                 "com.dronelabs.Perfkit.Channel");
}

static DBusGProxy*
get_sub_proxy (const gchar *path)
{
	return dbus_g_proxy_new_for_name(get_conn(),
	                                 "com.dronelabs.Perfkit",
	                                 path,
	                                 "com.dronelabs.Perfkit.Subscription");
}

static void
test_TestSuite_manager_ping (void)
{
	DBusGProxy *proxy;
	gchar *result = NULL;
	GError *error = NULL;

	proxy = get_manager_proxy();
	g_assert(proxy);

	if (!dbus_g_proxy_call(proxy, "Ping", &error, G_TYPE_INVALID,
	                       G_TYPE_STRING, &result, G_TYPE_INVALID))
		g_error("%s", error->message);

	g_assert_cmpstr(result, !=, NULL);
	g_assert_cmpstr(result, !=, "");
	g_free(result);

	g_object_unref(proxy);
}

static void
test_TestSuite_create_channel (void)
{
	static gchar *nc_args[] = { "-l", "-p", "10000", NULL };
	DBusGProxy *proxy;
	gchar *path = NULL;
	GError *error = NULL;

	proxy = get_manager_proxy();
	g_assert(proxy);

	if (!dbus_g_proxy_call(proxy, "CreateChannel", &error,
	                       G_TYPE_UINT, 0,
	                       G_TYPE_STRING, g_strdup("/bin/nc"),
	                       G_TYPE_STRV, g_strdupv(nc_args),
	                       G_TYPE_STRV, NULL,
	                       G_TYPE_STRING, g_strdup("/"),
	                       G_TYPE_INVALID,
	                       DBUS_TYPE_G_OBJECT_PATH, &path,
	                       G_TYPE_INVALID))
		g_error("%s", error->message);

	g_assert_cmpstr(path, ==, "/com/dronelabs/Perfkit/Channels/0");
	g_free(path);
	g_object_unref(proxy);
}

static void
test_TestSuite_add_source (void)
{
	DBusGProxy *proxy;
	gchar *path = NULL;
	GError *error = NULL;

	#define MEM_INFO "/com/dronelabs/Perfkit/Plugins/Sources/Memory"

	proxy = get_channel_proxy("/com/dronelabs/Perfkit/Channels/0");
	if (!dbus_g_proxy_call(proxy, "AddSource", &error,
	                       DBUS_TYPE_G_OBJECT_PATH, g_strdup(MEM_INFO),
	                       G_TYPE_INVALID,
	                       DBUS_TYPE_G_OBJECT_PATH, &path,
	                       G_TYPE_INVALID))
	    g_error("%s", error->message);

	g_assert_cmpstr(path, ==, "/com/dronelabs/Perfkit/Sources/0");
	g_free(path);
	g_object_unref(proxy);
}

static void
test_TestSuite_start (void)
{
	DBusGProxy *proxy;
	GError *error = NULL;

	proxy = get_channel_proxy("/com/dronelabs/Perfkit/Channels/0");
	if (!dbus_g_proxy_call(proxy, "Start", &error,
	                       G_TYPE_INVALID,
	                       G_TYPE_INVALID))
	    g_error("%s", error->message);
	g_object_unref(proxy);
}

static void
test_TestSuite_stop (void)
{
	DBusGProxy *proxy;
	GError *error = NULL;

	proxy = get_channel_proxy("/com/dronelabs/Perfkit/Channels/0");
	if (!dbus_g_proxy_call(proxy, "Stop", &error,
	                       G_TYPE_BOOLEAN, TRUE,
	                       G_TYPE_INVALID,
	                       G_TYPE_INVALID))
	    g_error("%s", error->message);
	g_object_unref(proxy);
}

#define HAS_INTERFACE(m,i)   (g_strcmp0(i, dbus_message_get_interface(m)) == 0)
#define IS_MEMBER_NAMED(m,i) (g_strcmp0(i, dbus_message_get_member(m)) == 0)
#define IS_SIGNATURE(m,i)    (g_strcmp0(i, dbus_message_get_signature(m)) == 0)

static guint m_count = 0;
static guint s_count = 0;
static guint c_count = 0;

enum
{
	MSG_UNKNOWN,
	MSG_MANIFEST,
	MSG_SAMPLE,
};

static inline gint
get_msg_type (DBusMessage *msg)
{
	if (IS_MEMBER_NAMED(msg, "Manifest"))
		return MSG_MANIFEST;
	else if (IS_MEMBER_NAMED(msg, "Sample"))
		return MSG_SAMPLE;
	return MSG_UNKNOWN;
}

static DBusHandlerResult
handle_msg (DBusConnection *conn,
            DBusMessage    *msg,
            void           *data)
{
	if (!HAS_INTERFACE(msg, "com.dronelabs.Perfkit.Subscription"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	switch (get_msg_type(msg)) {
	case MSG_MANIFEST:
		m_count ++;
		break;
	case MSG_SAMPLE:
		s_count ++;
		break;
	default:
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
}

static DBusObjectPathVTable subs_vtable = {
	NULL,
	handle_msg,
};

static void
new_conn (DBusServer     *server,
          DBusConnection *conn,
          void           *data)
{
	c_count ++;

	dbus_connection_ref(conn);
	dbus_connection_setup_with_g_main(conn, NULL);
	dbus_connection_register_fallback(conn,
	                                  "/Subscriptions",
	                                  &subs_vtable,
	                                  data);
}

static void
test_TestSuite_subscription (void)
{
	DBusGProxy *proxy, *subproxy;
	GError *error = NULL;
	gchar *path = NULL, *addr = NULL;
	DBusGConnection *conn;
	DBusServer *server;
	GMainContext *context;

	#define SUB_PATH "/Subscriptions/0"
	#define CHANNEL_PATH "/com/dronelabs/Perfkit/Channels/0"

	loop = g_main_loop_new(NULL, FALSE);
	addr = g_strdup_printf("unix:path=%s" G_DIR_SEPARATOR_S "subscription.socket", tmpdir);
	server = dbus_server_listen(addr, NULL);
	dbus_server_setup_with_g_main(server, g_main_context_default());
	dbus_server_set_new_connection_function(server, new_conn, NULL, NULL);
	conn = dbus_g_connection_open(addr, NULL);
	g_assert(conn);

	proxy = get_manager_proxy();
	if (!dbus_g_proxy_call(proxy, "CreateSubscription", &error,
	                       G_TYPE_STRING, g_strdup(addr),
	                       G_TYPE_STRING, g_strdup(SUB_PATH),
	                       DBUS_TYPE_G_OBJECT_PATH, g_strdup(CHANNEL_PATH),
	                       G_TYPE_UINT, 0,
	                       G_TYPE_ULONG, 2000, /* 2 Seconds */
	                       DBUS_TYPE_G_OBJECT_PATH, g_strdup("/"),
	                       G_TYPE_INVALID,
	                       DBUS_TYPE_G_OBJECT_PATH, &path,
	                       G_TYPE_INVALID))
		g_error("%s", error->message);
	g_assert_cmpstr(path, ==, "/com/dronelabs/Perfkit/Subscriptions/0");
	subproxy = get_sub_proxy(path);
	if (!dbus_g_proxy_call(subproxy, "Enable", &error,
	                       G_TYPE_INVALID, G_TYPE_INVALID))
	    g_error("%s", error->message);
	g_free(path);
	g_free(addr);
	g_object_unref(subproxy);
	g_object_unref(proxy);
	dbus_g_connection_unref(conn);

	g_timeout_add_seconds(5, (GSourceFunc)g_main_loop_quit, loop);
	g_main_loop_run(loop);

	g_assert_cmpint(m_count, ==, 1);
	g_assert_cmpint(s_count, >=, 2);
	g_assert_cmpint(c_count, ==, 2);
}

gint
main (gint   argc,
      gchar *argv[])
{
	static GPid pid = 0;
	static gchar *args[] = {
		"./perfkit-daemon",
		"--stdout",
		"-c", "daemon.conf",
		"-l", "/tmp/test-suite.log",
		NULL };
	static gchar *env[] = {
		"PERFKIT_SOURCES_DIR=./sources/.libs",
		"PERFKIT_ENCODERS_DIR=./encoders/.libs",
		"PERFKIT_LISTENERS_DIR=./listeners/.libs",
		NULL,
		NULL
	};

	GError *error = NULL;
	gint res;
	gchar *str;

	g_type_init();
	g_test_init(&argc, &argv, NULL);

	str = g_strdup("/tmp/test-suite-XXXXXX");
	tmpdir = mkdtemp(str);
	env[3] = g_strdup_printf("DBUS_SESSION_BUS_ADDRESS=%s",
	                         g_getenv("DBUS_SESSION_BUS_ADDRESS"));

	/* Start the daemon */
	if (!g_spawn_async(DAEMON_DIR, args, env,
	                   G_SPAWN_STDOUT_TO_DEV_NULL,
	                   NULL, NULL, &pid, &error))
	{
		g_printerr("%s\n", error->message);
		g_error_free(error);
		return EXIT_FAILURE;
	}

	g_usleep(G_USEC_PER_SEC / 2);

	/*
	 * Test various aspects of the daemon.  Order of tests somewhat matters
	 * here so that its easy to determine where things went wrong.
	 */
	g_test_add_func("/TestSuite/manager_ping",
	                test_TestSuite_manager_ping);
	g_test_add_func("/TestSuite/create_channel",
	                test_TestSuite_create_channel);
	g_test_add_func("/TestSuite/add_source",
	                test_TestSuite_add_source);
	g_test_add_func("/TestSuite/start",
	                test_TestSuite_start);
	g_test_add_func("/TestSuite/subscription",
	                test_TestSuite_subscription);
	g_test_add_func("/TestSuite/stop",
	                test_TestSuite_stop);

	/* Kill the daemon */
	res = g_test_run();
	kill(pid, SIGINT);

	/* Cleanup temp folders */
	g_remove(g_strdup_printf("%s/subscription.socket", tmpdir));
	g_remove(tmpdir);

	return res;
}
