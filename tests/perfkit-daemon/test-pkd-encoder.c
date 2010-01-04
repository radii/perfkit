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

#define ASSERT_TIMEVAL(b,tv) G_STMT_START {                                  \
    GDateTime *dt;                                                           \
    gint tp, dp, dh, dm, ds;                                                 \
    guint tj, dj;                                                            \
    guint64 tu, du;                                                          \
    struct {                                                                 \
        gint    period :  5;                                                 \
        guint   julian : 22;                                                 \
        guint64 usec   : 37;                                                 \
    } t = {0, 0, 0};                                                         \
                                                                             \
	dt = g_date_time_new_from_timeval((tv));                                 \
	g_date_time_get_julian(dt, &dp, &dj, &dh, &dm, &ds);                     \
	memcpy(&t, b, sizeof(t));                                                \
	tp = t.period;                                                           \
	tj = t.julian;                                                           \
	tu = t.usec;                                                             \
	du = ((dh * 60 * 60) + (dm * 60) + ds) * G_TIME_SPAN_SECOND;             \
	du += g_date_time_get_microsecond(dt);                                   \
	g_assert_cmpint(tp, ==, dp);                                             \
	g_assert_cmpint(tj, ==, dj);                                             \
	g_assert_cmpint(tu, ==, du);                                             \
	g_assert_cmpint(dh, ==, (du / G_TIME_SPAN_HOUR));                        \
	g_assert_cmpint(dm, ==, (du % G_TIME_SPAN_HOUR) / G_TIME_SPAN_MINUTE);   \
	g_assert_cmpint(ds, ==, (du % G_TIME_SPAN_MINUTE) / G_TIME_SPAN_SECOND); \
	g_date_time_unref(dt);                                                   \
} G_STMT_END

enum
{
	VERSION_1 = 1,
};

static void
test_PkdEncoder_encode_manifest (void)
{
	PkdManifest *m;
	gchar *buf = NULL;
	gsize len = 0;
	gboolean host_nbo = (G_BYTE_ORDER == G_BIG_ENDIAN);
	GTimeVal tv;

	SETUP_MANIFEST(m);
	g_assert(pkd_encoder_encode_manifest(NULL, m, &buf, &len));

	g_assert_cmpint(len, ==, 24);
	g_assert_cmpint(buf[0], ==, VERSION_1);    /* Version Check */
	g_assert_cmpint(buf[1], ==, host_nbo);     /* Network Byte Order */
	g_assert_cmpint(buf[2], ==, 3);            /* Source ID */
	g_assert_cmpint(buf[3], ==, 1);            /* Key Compression Enabled */
	pkd_manifest_get_timeval(m, &tv);
	ASSERT_TIMEVAL(&buf[4], &tv);
	g_assert_cmpint(buf[12], ==, 1);            /* Row 1 */
	g_assert_cmpint(buf[13], ==, G_TYPE_UINT);  /* Row 1 Type */
	g_assert_cmpstr(&buf[14], ==, "bps");       /* Row 1 Name */
	g_assert_cmpint(buf[18], ==, 2);            /* Row 2 */
	g_assert_cmpint(buf[19], ==, G_TYPE_UINT);  /* Row 2 Type */
	g_assert_cmpstr(&buf[20], ==, "qps");       /* Row 2 Name */
}

static void
test_PkdEncoder_encode_samples (void)
{
	PkdManifest *m;
	PkdSample *samples[3];
	gchar *buf = NULL;
	gsize len = 0;
	gint i;
	GTimeVal tv;

	SETUP_MANIFEST(m);
	SETUP_SAMPLE(samples[0], m, 55, 88);
	SETUP_SAMPLE(samples[1], m, 66, 99);
	SETUP_SAMPLE(samples[2], m, 77, 11);

	g_assert(pkd_encoder_encode_samples(NULL, &samples[0], 3, &buf, &len));
	g_assert_cmpint(len, ==, 73);

	g_assert_cmpint(buf[0], ==, VERSION_1);         /* Version Flag */
	g_assert_cmpint((*(gint *)&buf[1]), ==, 12);    /* Sample 1   Length */
	g_assert_cmpint(buf[5], ==, 5);                 /* Sample 1   Source ID */
	pkd_sample_get_timeval(samples[0], &tv);
	ASSERT_TIMEVAL(&buf[6], &tv);
	g_assert_cmpint(buf[14], ==, 1);                /* Sample 1   ID Compression */
	g_assert_cmpint(buf[15], ==, 1);                /* Sample 1:1 Manifest Index */
	g_assert_cmpint(*((gint *)&buf[16]), ==, 55);   /* Sample 1:1 Value */
	g_assert_cmpint(buf[20], ==, 2);                /* Sample 1:2 Manifest Index */
	g_assert_cmpint(*((gint *)&buf[21]), ==, 88);   /* Sample 1:2 Value */

	g_assert_cmpint((*(gint *)&buf[25]), ==, 12);   /* Sample 2   Length */
	g_assert_cmpint(buf[29], ==, 5);                /* Sample 2   Source ID */
	pkd_sample_get_timeval(samples[1], &tv);
	ASSERT_TIMEVAL(&buf[30], &tv);
	g_assert_cmpint(buf[38], ==, 1);                /* Sample 2   ID Compression */
	g_assert_cmpint(buf[39], ==, 1);                /* Sample 2:1 Manifest Index */
	g_assert_cmpint(*((gint *)&buf[40]), ==, 66);   /* Sample 2:1 Value */
	g_assert_cmpint(buf[44], ==, 2);                /* Sample 2:2 Manifest Index */
	g_assert_cmpint(*((gint *)&buf[45]), ==, 99);   /* Sample 2:2 Value */

	g_assert_cmpint((*(gint *)&buf[49]), ==, 12);   /* Sample 3   Length */
	g_assert_cmpint(buf[53], ==, 5);                /* Sample 3   Source ID */
	pkd_sample_get_timeval(samples[2], &tv);
	ASSERT_TIMEVAL(&buf[54], &tv);
	g_assert_cmpint(buf[62], ==, 1);                /* Sample 3   ID Compression */
	g_assert_cmpint(buf[63], ==, 1);                /* Sample 3:1 Manifest Index */
	g_assert_cmpint(*((gint *)&buf[64]), ==, 77);   /* Sample 3:1 Value */
	g_assert_cmpint(buf[68], ==, 2);                /* Sample 3:2 Manifest Index */
	g_assert_cmpint(*((gint *)&buf[69]), ==, 11);   /* Sample 3:2 Value */
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
