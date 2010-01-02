#include <perfkit-daemon/perfkit-daemon.h>
#include <string.h>

extern void pkd_manifest_set_source_id (PkdManifest *m, gint i);
extern void pkd_sample_set_source_id   (PkdSample   *s, gint i);

#define SETUP_MANIFEST(m) G_STMT_START {                  \
    (m) = pkd_manifest_new();                             \
    pkd_manifest_set_source_id((m), 3);                   \
    pkd_manifest_append((m), "bps", G_TYPE_UINT);         \
    pkd_manifest_append((m), "qps", G_TYPE_UINT);         \
    g_assert_cmpint(pkd_manifest_get_n_rows((m)), ==, 2); \
} G_STMT_END

#define SETUP_SAMPLE(s,m,i,v) G_STMT_START {              \
	PkdSampleWriter w;                                    \
    (s) = pkd_sample_new();                               \
    pkd_sample_set_source_id((s), 5);                     \
    pkd_sample_writer_init(&w, (m), (s));                 \
    pkd_sample_writer_integer(&w, 1, (i));                \
    pkd_sample_writer_integer(&w, 2, (v));                \
    pkd_sample_writer_finish(&w);                         \
    memset(&w, 0, sizeof(w));                             \
} G_STMT_END

static void
test_PkdEncoder_encode_manifest (void)
{
	PkdManifest *m;
	gchar *buf = NULL;
	gsize len = 0;
	gboolean host_nbo = (G_BYTE_ORDER == G_BIG_ENDIAN);

	SETUP_MANIFEST(m);
	g_assert(pkd_encoder_encode_manifest(NULL, m, &buf, &len));

	g_assert_cmpint(len, ==, 15);
	g_assert_cmpint(buf[0], ==, host_nbo);     /* Network Byte Order */
	g_assert_cmpint(buf[1], ==, 3);            /* Source ID */
	g_assert_cmpint(buf[2], ==, 1);            /* Key Compression Enabled */
	g_assert_cmpint(buf[3], ==, 1);            /* Row 1 */
	g_assert_cmpint(buf[4], ==, G_TYPE_UINT);  /* Row 1 Type */
	g_assert_cmpstr(&buf[5], ==, "bps");       /* Row 1 Name */
	g_assert_cmpint(buf[9], ==, 2);            /* Row 2 */
	g_assert_cmpint(buf[10], ==, G_TYPE_UINT); /* Row 2 Type */
	g_assert_cmpstr(&buf[11], ==, "qps");      /* Row 2 Name */
}

static void
test_PkdEncoder_encode_samples (void)
{
	PkdManifest *m;
	PkdSample *samples[3];
	gchar *buf = NULL;
	gsize len = 0;
	gint i;

	SETUP_MANIFEST(m);
	SETUP_SAMPLE(samples[0], m, 55, 88);
	SETUP_SAMPLE(samples[1], m, 66, 99);
	SETUP_SAMPLE(samples[2], m, 77, 11);

	g_assert(pkd_encoder_encode_samples(NULL, &samples[0], 3, &buf, &len));
	g_assert_cmpint(len, ==, 48);

	g_assert_cmpint((*(gint *)&buf[0]), ==, 12);    /* Sample 1   Length */
	g_assert_cmpint(buf[4], ==, 5);                 /* Sample 1   Source ID */
	g_assert_cmpint(buf[5], ==, 1);                 /* Sample 1   ID Compression */
	g_assert_cmpint(buf[6], ==, 1);                 /* Sample 1:1 Manifest Index */
	g_assert_cmpint(*((gint *)&buf[7]), ==, 55);    /* Sample 1:1 Value */
	g_assert_cmpint(buf[11], ==, 2);                /* Sample 1:2 Manifest Index */
	g_assert_cmpint(*((gint *)&buf[12]), ==, 88);   /* Sample 1:2 Value */

	g_assert_cmpint((*(gint *)&buf[16]), ==, 12);   /* Sample 2   Length */
	g_assert_cmpint(buf[20], ==, 5);                /* Sample 2   Source ID */
	g_assert_cmpint(buf[21], ==, 1);                /* Sample 2   ID Compression */
	g_assert_cmpint(buf[22], ==, 1);                /* Sample 2:1 Manifest Index */
	g_assert_cmpint(*((gint *)&buf[23]), ==, 66);   /* Sample 2:1 Value */
	g_assert_cmpint(buf[27], ==, 2);                /* Sample 2:2 Manifest Index */
	g_assert_cmpint(*((gint *)&buf[28]), ==, 99);   /* Sample 2:2 Value */

	g_assert_cmpint((*(gint *)&buf[32]), ==, 12);   /* Sample 3   Length */
	g_assert_cmpint(buf[36], ==, 5);                /* Sample 3   Source ID */
	g_assert_cmpint(buf[37], ==, 1);                /* Sample 3   ID Compression */
	g_assert_cmpint(buf[38], ==, 1);                /* Sample 3:1 Manifest Index */
	g_assert_cmpint(*((gint *)&buf[39]), ==, 77);   /* Sample 3:1 Value */
	g_assert_cmpint(buf[43], ==, 2);                /* Sample 3:2 Manifest Index */
	g_assert_cmpint(*((gint *)&buf[44]), ==, 11);   /* Sample 3:2 Value */
}

gint
main (gint    argc,
      gchar  *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/PkdEncoder/encode_manifest",
	                test_PkdEncoder_encode_manifest);
	g_test_add_func("/PkdEncoder/encode_samples",
	                test_PkdEncoder_encode_samples);

	return g_test_run();
}
