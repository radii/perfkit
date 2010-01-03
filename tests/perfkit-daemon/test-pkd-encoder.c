#include <perfkit-daemon/perfkit-daemon.h>
#include <gdatetime.h>
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
	GDateTime *dt;
	GTimeVal tv;
	gint tp, dp, dh, dm, ds;
	guint tj, dj;
	guint64 tu, du;
	struct {
		gint    period :  5;
		guint   julian : 22;
		guint64 usec   : 37;
	} t = {0, 0, 0};

	SETUP_MANIFEST(m);
	g_assert(pkd_encoder_encode_manifest(NULL, m, &buf, &len));

	g_assert_cmpint(len, ==, 23);
	g_assert_cmpint(buf[0], ==, host_nbo);     /* Network Byte Order */
	g_assert_cmpint(buf[1], ==, 3);            /* Source ID */
	g_assert_cmpint(buf[2], ==, 1);            /* Key Compression Enabled */


	/*
	 * Verify the 64-bit timestamp.  I figured that since we are going
	 * to include 64-bits for a timestamp, we should make pretty damn
	 * good use of those bits.  See the PkdEncoder for more information.
	 *
	 * The timestamp structure is as follows:
	 *
	 *   julian period  :  5  ; The Julian Epoch (0 for current)
	 *   julian day num : 22  ; The day within the julian period
	 *   timekeeping    : 37  ; Timekeeping within day (in usec)
	 *
	 */
	pkd_manifest_get_timeval(m, &tv);
	dt = g_date_time_new_from_timeval(&tv);
	g_date_time_get_julian(dt, &dp, &dj, &dh, &dm, &ds);
	memcpy(&t, &buf[3], sizeof(t));
	tp = t.period;
	tj = t.julian;
	tu = t.usec;
	du = ((dh * 60 * 60) + (dm * 60) + ds) * G_TIME_SPAN_SECOND;
	du += g_date_time_get_microsecond(dt);
	g_assert_cmpint(tp, ==, dp);
	g_assert_cmpint(tj, ==, dj);
	g_assert_cmpint(tu, ==, du);
	g_assert_cmpint(dh, ==, (du / G_TIME_SPAN_HOUR));
	g_assert_cmpint(dm, ==, (du % G_TIME_SPAN_HOUR) / G_TIME_SPAN_MINUTE);
	g_assert_cmpint(ds, ==, (du % G_TIME_SPAN_MINUTE) / G_TIME_SPAN_SECOND);
	g_date_time_unref(dt);

	g_assert_cmpint(buf[11], ==, 1);            /* Row 1 */
	g_assert_cmpint(buf[12], ==, G_TYPE_UINT);  /* Row 1 Type */
	g_assert_cmpstr(&buf[13], ==, "bps");       /* Row 1 Name */
	g_assert_cmpint(buf[17], ==, 2);            /* Row 2 */
	g_assert_cmpint(buf[18], ==, G_TYPE_UINT); /* Row 2 Type */
	g_assert_cmpstr(&buf[19], ==, "qps");      /* Row 2 Name */
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
