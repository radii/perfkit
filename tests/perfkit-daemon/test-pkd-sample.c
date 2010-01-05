#include <perfkit-daemon/perfkit-daemon.h>
#include <cut-n-paste/egg-buffer.h>

static void
test_PkdSample_new (void)
{
	PkdSample *s;

	s = pkd_sample_new();
	g_assert(s);
	pkd_sample_unref(s);
}

static void
test_PkdSample_append_string (void)
{
	EggBuffer *b;
	PkdSample *s;
	guchar *buf;
	gsize len;
	gint f;
	guint t;
	gchar *c;

	s = pkd_sample_new();
	pkd_sample_append_string(s, 3, "testing");
	pkd_sample_append_string(s, 1, "buffer");
	pkd_sample_get_data(s, (gchar **)&buf, &len);
	b = egg_buffer_new_from_data(buf, len);
	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 3);
	g_assert_cmpint(t, ==, EGG_BUFFER_STRING);
	egg_buffer_read_string(b, &c);
	g_assert_cmpstr(c, ==, "testing");
	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 1);
	g_assert_cmpint(t, ==, EGG_BUFFER_STRING);
	egg_buffer_read_string(b, &c);
	g_assert_cmpstr(c, ==, "buffer");
	egg_buffer_unref(b);
	pkd_sample_unref(s);
}

static void
test_PkdSample_append_int (void)
{
	EggBuffer *b;
	PkdSample *s;
	guchar *buf;
	gsize len;
	gint f, i;
	guint t;

	s = pkd_sample_new();
	pkd_sample_append_int(s, 1, 1);
	pkd_sample_append_int(s, 2, -1);
	pkd_sample_append_int(s, 3, 0);
	pkd_sample_append_int(s, 4, G_MAXINT);
	pkd_sample_append_int(s, 5, G_MININT);
	pkd_sample_get_data(s, (gchar **)&buf, &len);
	b = egg_buffer_new_from_data(buf, len);

	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 1);
	g_assert_cmpint(t, ==, EGG_BUFFER_INT);
	egg_buffer_read_int(b, &i);
	g_assert_cmpint(i, ==, 1);

	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 2);
	g_assert_cmpint(t, ==, EGG_BUFFER_INT);
	egg_buffer_read_int(b, &i);
	g_assert_cmpint(i, ==, -1);

	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 3);
	g_assert_cmpint(t, ==, EGG_BUFFER_INT);
	egg_buffer_read_int(b, &i);
	g_assert_cmpint(i, ==, 0);

	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 4);
	g_assert_cmpint(t, ==, EGG_BUFFER_INT);
	egg_buffer_read_int(b, &i);
	g_assert_cmpint(i, ==, G_MAXINT);

	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 5);
	g_assert_cmpint(t, ==, EGG_BUFFER_INT);
	egg_buffer_read_int(b, &i);
	g_assert_cmpint(i, ==, G_MININT);

	egg_buffer_unref(b);
	pkd_sample_unref(s);
}

static void
test_PkdSample_append_uint (void)
{
	EggBuffer *b;
	PkdSample *s;
	guchar *buf;
	gsize len;
	gint f;
	guint t, i;

	s = pkd_sample_new();
	pkd_sample_append_int(s, 1, 1);
	pkd_sample_append_int(s, 2, 3);
	pkd_sample_append_int(s, 3, 0);
	pkd_sample_append_int(s, 4, G_MAXUINT);
	pkd_sample_get_data(s, (gchar **)&buf, &len);
	b = egg_buffer_new_from_data(buf, len);

	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 1);
	g_assert_cmpint(t, ==, EGG_BUFFER_UINT);
	egg_buffer_read_int(b, &i);
	g_assert_cmpint(i, ==, 1);

	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 2);
	g_assert_cmpint(t, ==, EGG_BUFFER_UINT);
	egg_buffer_read_int(b, &i);
	g_assert_cmpint(i, ==, 3);

	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 3);
	g_assert_cmpint(t, ==, EGG_BUFFER_UINT);
	egg_buffer_read_int(b, &i);
	g_assert_cmpint(i, ==, 0);

	egg_buffer_read_tag(b, &f, &t);
	g_assert_cmpint(f, ==, 4);
	g_assert_cmpint(t, ==, EGG_BUFFER_UINT);
	egg_buffer_read_int(b, &i);
	g_assert_cmpint(i, ==, G_MAXUINT);

	egg_buffer_unref(b);
	pkd_sample_unref(s);
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/PkdSample/new", test_PkdSample_new);
	g_test_add_func("/PkdSample/append_int", test_PkdSample_append_int);
	g_test_add_func("/PkdSample/append_string", test_PkdSample_append_string);
	g_test_add_func("/PkdSample/append_uint", test_PkdSample_append_uint);

	return g_test_run();
}
