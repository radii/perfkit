#include <stdlib.h>
#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

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
test_TestSuite_sample_delivery (void)
{
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

gint
main (gint   argc,
      gchar *argv[])
{
	static GPid pid = 0;
	static gchar *args[] = {
		"./perfkit-daemon",
		"--stdout",
		"-c", "daemon.conf",
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

	g_type_init();
	g_test_init(&argc, &argv, NULL);

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
	g_test_add_func("/TestSuite/stop",
	                test_TestSuite_stop);
	g_test_add_func("/TestSuite/sample_delivery",
	                test_TestSuite_sample_delivery);

	/* Kill the daemon */
	res = g_test_run();
	kill(pid, SIGINT);
	return res;
}
