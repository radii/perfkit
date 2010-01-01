#include <perfkit-daemon/perfkit-daemon.h>

static void
test_PkdSample_new (void)
{
	PkdSample *s;

	s = pkd_sample_new();
	g_assert(s);
	pkd_sample_unref(s);
}

static void
test_PkdSampleWriter_basic (void)
{
	PkdSampleWriter sw;
	PkdSample *s;
	PkdManifest *m;
	gchar *buf = NULL;
	gsize buflen = 0;

	m = pkd_manifest_new();
	pkd_manifest_append(m, "ID", G_TYPE_INT);
	pkd_manifest_append(m, "Bytes/Sec", G_TYPE_INT);
	pkd_manifest_append(m, "Format", G_TYPE_STRING);

	s = pkd_sample_new();
	pkd_sample_writer_init(&sw, m, s);
	pkd_sample_writer_integer(&sw, 1, 13);
	pkd_sample_writer_integer(&sw, 2, 42);
	pkd_sample_writer_string(&sw, 3, "kB/sec");
	pkd_sample_writer_finish(&sw);
	pkd_sample_get_data(s, &buf, &buflen);
	g_assert(buf);
	g_assert_cmpint(buflen, ==, 19);

	/* Ensure ID compression is enabled */
	g_assert_cmpint(buf[0], ==, TRUE);

	/* Ensure index 1 */
	g_assert_cmpint(buf[1], ==, 1);

	/* Ensure sample value in host format */
	g_assert_cmpint(*((gint*)(buf + 2)), ==, 13);

	/* Ensure next index */
	g_assert_cmpint(buf[6], ==, 2);

	/* Ensure second sample in host format */
	g_assert_cmpint(*((gint*)(buf + 7)), ==, 42);

	/* Ensure next index */
	g_assert_cmpint(buf[11], ==, 3);

	/* Ensure inline string */
	g_assert_cmpstr(&buf[12], ==, "kB/sec");
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/PkdSample/new", test_PkdSample_new);
	g_test_add_func("/PkdSampleWriter/basic", test_PkdSampleWriter_basic);

	return g_test_run();
}
