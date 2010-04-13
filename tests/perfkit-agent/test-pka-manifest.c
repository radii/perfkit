#include <perfkit-agent/perfkit-agent.h>

static void
test_PkaManifest_new (void)
{
	PkaManifest *m;

	m = pka_manifest_new();
	g_assert(m);
	pka_manifest_ref(m);
	pka_manifest_unref(m);
	pka_manifest_unref(m);
}

static void
test_PkaManifest_basic (void)
{
	PkaManifest *m;
	gint r, i;

	m = pka_manifest_new();
	pka_manifest_append(m, "id", G_TYPE_INT);
	pka_manifest_append(m, "name", G_TYPE_STRING);
	pka_manifest_append(m, "qps", G_TYPE_ULONG);
	pka_manifest_append(m, "dbl", G_TYPE_DOUBLE);

	g_assert_cmpint(pka_manifest_get_n_rows(m), ==, 4);
	g_assert_cmpstr(pka_manifest_get_row_name(m, 1), ==, "id");
	g_assert_cmpstr(pka_manifest_get_row_name(m, 2), ==, "name");
	g_assert_cmpstr(pka_manifest_get_row_name(m, 3), ==, "qps");
	g_assert_cmpstr(pka_manifest_get_row_name(m, 4), ==, "dbl");
	g_assert_cmpint(pka_manifest_get_row_type(m, 1), ==, G_TYPE_INT);
	g_assert_cmpint(pka_manifest_get_row_type(m, 2), ==, G_TYPE_STRING);
	g_assert_cmpint(pka_manifest_get_row_type(m, 3), ==, G_TYPE_ULONG);
	g_assert_cmpint(pka_manifest_get_row_type(m, 4), ==, G_TYPE_DOUBLE);
}

static void
test_PkaManifest_timeval (void)
{
	PkaManifest *m;
	GTimeVal tv1, tv2;

	m = pka_manifest_new();
	g_usleep(G_USEC_PER_SEC / 10);
	g_get_current_time(&tv1);
	pka_manifest_set_timeval(m, &tv1);
	pka_manifest_get_timeval(m, &tv2);
	g_assert_cmpint(tv1.tv_sec, ==, tv2.tv_sec);
	g_assert_cmpint(tv1.tv_usec, ==, tv2.tv_usec);

	pka_manifest_unref(m);
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/PkaManifest/new", test_PkaManifest_new);
	g_test_add_func("/PkaManifest/timeval", test_PkaManifest_timeval);
	g_test_add_func("/PkaManifest/basic", test_PkaManifest_basic);

	return g_test_run();
}
