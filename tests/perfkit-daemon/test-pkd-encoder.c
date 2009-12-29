#include <perfkit-daemon/perfkit-daemon.h>

extern void pkd_manifest_set_source_id (PkdManifest *m, gint i);

#define SETUP_MANIFEST(m) G_STMT_START {                  \
    (m) = pkd_manifest_new();                             \
    pkd_manifest_set_source_id((m), 3);                   \
    pkd_manifest_append((m), "id", G_TYPE_UINT);          \
    pkd_manifest_append((m), "qps", G_TYPE_UINT);         \
    g_assert_cmpint(pkd_manifest_get_n_rows((m)), ==, 2); \
} G_STMT_END

static void
test_PkdEncoder_encode_manifest (void)
{
	PkdManifest *m;
	gchar *buf = NULL;
	gsize len = 0;

	SETUP_MANIFEST(m);
	g_assert(pkd_encoder_encode_manifest(NULL, m, &buf, &len));

	g_assert_cmpint(len, ==, 13);
	g_assert_cmpint(buf[0], ==, 3);           /* Source ID */
	g_assert_cmpint(buf[1], ==, 1);           /* Key Compression Enabled */
	g_assert_cmpint(buf[2], ==, 1);           /* Row 1 */
	g_assert_cmpint(buf[3], ==, G_TYPE_UINT); /* Row 1 Type */
	g_assert_cmpstr(&buf[4], ==, "id");       /* Row 1 Name */
	g_assert_cmpint(buf[7], ==, 2);           /* Row 2 */
	g_assert_cmpint(buf[8], ==, G_TYPE_UINT); /* Row 2 Type */
	g_assert_cmpstr(&buf[9], ==, "qps");      /* Row 2 Name */
}

gint
main (gint    argc,
      gchar  *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/PkdEncoder/encode_manifest",
	                test_PkdEncoder_encode_manifest);

	return g_test_run();
}
