#include <perfkit-agent/perfkit-agent.h>
#include <cut-n-paste/egg-buffer.h>

extern void pka_manifest_set_source_id (PkaManifest *m, gint i);
extern void pka_sample_set_source_id   (PkaSample   *s, gint i);

#define SETUP_MANIFEST(m) G_STMT_START {                  \
    (m) = pka_manifest_new();                             \
    pka_manifest_set_source_id((m), 3);                   \
    pka_manifest_append((m), "bps", G_TYPE_UINT);         \
    pka_manifest_append((m), "qps", G_TYPE_UINT);         \
    pka_manifest_append((m), "dbl", G_TYPE_DOUBLE);       \
    g_assert_cmpint(pka_manifest_get_n_rows((m)), ==, 3); \
} G_STMT_END

static void
test_PkaEncoder_encode_manifest (void)
{
	EggBuffer *b;
	PkaManifest *m;
	guint8 *buf = NULL;
	gsize len = 0;
	gint f;
	guint t, u2;
	guint64 u;
	gchar *c;

	SETUP_MANIFEST(m);
	g_assert(pka_encoder_encode_manifest(NULL, m, &buf, &len));
	b = egg_buffer_new_from_data(buf, len);

	/* timestamp */
	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 1);
	g_assert_cmpint(t, ==, EGG_BUFFER_UINT64);
	egg_buffer_read_uint64(b, &u);
	g_assert_cmpint(u, !=, 0); /* FIXME */

	/* resolution */
	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 2);
	g_assert_cmpint(t, ==, EGG_BUFFER_UINT);
	egg_buffer_read_uint(b, &u2);
	g_assert_cmpint(u2, ==, PKA_RESOLUTION_USEC);

	/* source */
	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 3);
	g_assert_cmpint(t, ==, EGG_BUFFER_UINT);
	egg_buffer_read_uint(b, &u2);
	g_assert_cmpint(u2, ==, pka_manifest_get_source_id(m));

	/* columns */
	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 4);
	g_assert_cmpint(t, ==, EGG_BUFFER_REPEATED);

	/* data len */
	egg_buffer_read_uint(b, &u2);
	g_assert_cmpint(u2, ==, 30);

	/* first column */
	egg_buffer_read_uint(b, &u2);
	g_assert_cmpint(u2, ==, 9);
	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 1);
	g_assert_cmpint(t, ==, EGG_BUFFER_UINT);
	egg_buffer_read_uint(b, &u2);
	g_assert_cmpint(u2, ==, 1);
	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 2);
	g_assert_cmpint(t, ==, EGG_BUFFER_ENUM);
	egg_buffer_read_uint(b, &u2);
	g_assert_cmpint(u2, ==, G_TYPE_UINT);
	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 3);
	g_assert_cmpint(t, ==, EGG_BUFFER_STRING);
	egg_buffer_read_string(b, &c);
	g_assert_cmpstr(c, ==, "bps");

	/* second column */
	egg_buffer_read_uint(b, &u2);
	g_assert_cmpint(u2, ==, 9);
	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 1);
	g_assert_cmpint(t, ==, EGG_BUFFER_UINT);
	egg_buffer_read_uint(b, &u2);
	g_assert_cmpint(u2, ==, 2);
	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 2);
	g_assert_cmpint(t, ==, EGG_BUFFER_ENUM);
	egg_buffer_read_uint(b, &u2);
	g_assert_cmpint(u2, ==, G_TYPE_UINT);
	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 3);
	g_assert_cmpint(t, ==, EGG_BUFFER_STRING);
	egg_buffer_read_string(b, &c);
	g_assert_cmpstr(c, ==, "qps");

	/* third column */
	egg_buffer_read_uint(b, &u2);
	g_assert_cmpint(u2, ==, 9);
	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 1);
	g_assert_cmpint(t, ==, EGG_BUFFER_UINT);
	egg_buffer_read_uint(b, &u2);
	g_assert_cmpint(u2, ==, 3);
	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 2);
	g_assert_cmpint(t, ==, EGG_BUFFER_ENUM);
	egg_buffer_read_uint(b, &u2);
	g_assert_cmpint(u2, ==, G_TYPE_DOUBLE);
	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 3);
	g_assert_cmpint(t, ==, EGG_BUFFER_STRING);
	egg_buffer_read_string(b, &c);
	g_assert_cmpstr(c, ==, "dbl");

	egg_buffer_unref(b);
}

static void
test_PkaEncoder_encode_samples (void)
{
	PkaSample *samples[4];
	PkaManifest *m;
	guint8 *buf;
	gsize len;

	SETUP_MANIFEST(m);
	pka_manifest_set_resolution(m, PKA_RESOLUTION_SECOND);
	g_assert(m);

	samples[0] = pka_sample_new();
	samples[1] = pka_sample_new();
	samples[2] = pka_sample_new();
	samples[3] = pka_sample_new();

	/* empty samples test */
	g_assert(pka_encoder_encode_samples(NULL, m, samples, 3, &buf, &len));
	g_assert_cmpint(len, ==, 12);
	g_assert_cmpint(buf[0],  ==, 0x8);
	g_assert_cmpint(buf[1],  ==, 0x0);
	g_assert_cmpint(buf[2],  ==, 0x12);
	g_assert_cmpint(buf[3],  ==, 0x0);
	g_assert_cmpint(buf[4],  ==, 0x8);
	g_assert_cmpint(buf[5],  ==, 0x0);
	g_assert_cmpint(buf[6],  ==, 0x12);
	g_assert_cmpint(buf[7],  ==, 0x0);
	g_assert_cmpint(buf[8],  ==, 0x8);
	g_assert_cmpint(buf[9],  ==, 0x0);
	g_assert_cmpint(buf[10], ==, 0x12);
	g_assert_cmpint(buf[11], ==, 0x0);

	/* single value per sample */
	pka_sample_append_uint(samples[0], 1, 123);
	pka_sample_append_uint(samples[1], 1, 321);
	pka_sample_append_uint(samples[2], 1, 111);
	pka_sample_append_double(samples[3], 3, 123.45);
	g_assert(pka_encoder_encode_samples(NULL, m, samples, 4, &buf, &len));
	g_assert_cmpint(len, ==, 32);
	g_assert_cmpint(buf[0],  ==, 0x8);
	g_assert_cmpint(buf[1],  ==, 0x0);
	g_assert_cmpint(buf[2],  ==, 0x12);
	g_assert_cmpint(buf[3],  ==, 0x2);
	g_assert_cmpint(buf[4],  ==, 0x8);
	g_assert_cmpint(buf[5],  ==, 0x7B);
	g_assert_cmpint(buf[6],  ==, 0x8);
	g_assert_cmpint(buf[7],  ==, 0x0);
	g_assert_cmpint(buf[8],  ==, 0x12);
	g_assert_cmpint(buf[9],  ==, 0x3);
	g_assert_cmpint(buf[10], ==, 0x8);
	g_assert_cmpint(buf[11], ==, 0xC1);
	g_assert_cmpint(buf[12], ==, 0x2);
	g_assert_cmpint(buf[13], ==, 0x8);
	g_assert_cmpint(buf[14], ==, 0x0);
	g_assert_cmpint(buf[15], ==, 0x12);
	g_assert_cmpint(buf[16], ==, 0x2);
	g_assert_cmpint(buf[17], ==, 0x8);
	g_assert_cmpint(buf[18], ==, 0x6F);
	g_assert_cmpint(buf[19], ==, 0x8);
	g_assert_cmpint(buf[20], ==, 0x0);
	g_assert_cmpint(buf[21], ==, 0x12);
	g_assert_cmpint(buf[22], ==, 0x9);
	g_assert_cmpint(buf[23], ==, 0x19);
	g_assert_cmpint(buf[24], ==, 0x0);
	g_assert_cmpint(buf[25], ==, 0x0);
	g_assert_cmpint(buf[26], ==, 0x0);
	g_assert_cmpint(buf[27], ==, 0x0);
	g_assert_cmpint(buf[28], ==, 0x0);
	g_assert_cmpint(buf[29], ==, 0xc0);
	g_assert_cmpint(buf[30], ==, 0x5e);
	g_assert_cmpint(buf[31], ==, 0x40);

	pka_sample_unref(samples[0]);
	pka_sample_unref(samples[1]);
	pka_sample_unref(samples[2]);
	pka_sample_unref(samples[3]);
}

gint
main (gint    argc,
      gchar  *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/PkaEncoder/encode_manifest", test_PkaEncoder_encode_manifest);
	g_test_add_func("/PkaEncoder/encode_samples", test_PkaEncoder_encode_samples);

	return g_test_run();
}
