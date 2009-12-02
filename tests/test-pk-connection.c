#include "pk-connection.h"
#include "pk-connection-dbus.h"

static void
test_PkConnection_new_for_uri (void)
{
	PkConnection *conn;

	conn = pk_connection_new_for_uri ("dbus://");
	g_assert (conn != NULL);
	g_object_unref (conn);
}

static void
test_PkConnection_connect (void)
{
	PkConnection *conn;

	conn = pk_connection_new_for_uri ("dbus://");
	g_assert (conn != NULL);
	g_assert (pk_connection_connect (conn, NULL));
	g_object_unref (conn);
}

static void
test_PkConnection_disconnect (void)
{
	PkConnection *conn;

	conn = pk_connection_new_for_uri ("dbus://");
	g_assert (conn != NULL);
	g_assert (pk_connection_connect (conn, NULL));
	pk_connection_disconnect (conn);
	g_object_unref (conn);
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_type_init ();
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/PkConnection/new_for_uri",
	                 test_PkConnection_new_for_uri);
	g_test_add_func ("/PkConnection/connect",
	                 test_PkConnection_connect);
	g_test_add_func ("/PkConnection/disconnect",
	                 test_PkConnection_disconnect);

	return g_test_run ();
}
