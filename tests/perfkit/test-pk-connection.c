#include <perfkit/perfkit.h>

static void
test_PkConnection_new_from_uri (void)
{
	PkConnection *conn;

	conn = pk_connection_new_from_uri("dbus://");
	g_assert(conn);
	g_object_unref(conn);
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_setenv("PK_PROTOCOLS_DIR", PK_PROTOCOLS_DIR, FALSE);

	g_type_init();
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/PkConnection/new_from_uri",
	                test_PkConnection_new_from_uri);

	return g_test_run();
}
