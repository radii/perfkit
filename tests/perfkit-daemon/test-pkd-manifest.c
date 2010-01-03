#include <perfkit-daemon/perfkit-daemon.h>

static void
test_PkdManifest_new (void)
{
	PkdManifest *m;

	m = pkd_manifest_new();
	g_assert(m);
	pkd_manifest_ref(m);
	pkd_manifest_unref(m);
	pkd_manifest_unref(m);
}

static void
test_PkdManifestWriter_basic (void)
{
	PkdManifest *m;
	gint r, i;

	m = pkd_manifest_new();
	pkd_manifest_append(m, "id", G_TYPE_INT);
	pkd_manifest_append(m, "name", G_TYPE_STRING);
	pkd_manifest_append(m, "qps", G_TYPE_ULONG);

	g_assert_cmpint(pkd_manifest_get_n_rows(m), ==, 3);
	g_assert_cmpstr(pkd_manifest_get_row_name(m, 1), ==, "id");
	g_assert_cmpstr(pkd_manifest_get_row_name(m, 2), ==, "name");
	g_assert_cmpstr(pkd_manifest_get_row_name(m, 3), ==, "qps");
	g_assert_cmpint(pkd_manifest_get_row_type(m, 1), ==, G_TYPE_INT);
	g_assert_cmpint(pkd_manifest_get_row_type(m, 2), ==, G_TYPE_STRING);
	g_assert_cmpint(pkd_manifest_get_row_type(m, 3), ==, G_TYPE_ULONG);
}

static void
test_PkdManifest_timeval (void)
{
	PkdManifest *m;
	GTimeVal tv1, tv2;

	m = pkd_manifest_new();
	g_usleep(G_USEC_PER_SEC / 10);
	g_get_current_time(&tv1);
	pkd_manifest_set_timeval(m, &tv1);
	pkd_manifest_get_timeval(m, &tv2);
	g_assert_cmpint(tv1.tv_sec, ==, tv2.tv_sec);
	g_assert_cmpint(tv1.tv_usec, ==, tv2.tv_usec);

	pkd_manifest_unref(m);
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/PkdManifest/new", test_PkdManifest_new);
	g_test_add_func("/PkdManifest/timeval", test_PkdManifest_timeval);
	g_test_add_func("/PkdManifestWriter/basic", test_PkdManifestWriter_basic);

	return g_test_run();
}
