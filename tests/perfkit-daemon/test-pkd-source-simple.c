#include <perfkit-daemon/perfkit-daemon.h>

extern void pkd_source_notify_started (PkdSource *, PkdSpawnInfo *);
extern void pkd_source_notify_stopped (PkdSource *);

static void
test_PkdSourceSimple_threaded_cb (PkdSourceSimple *source,
                                  gpointer         user_data)
{
	(*((gint *)user_data))++;
}

static void
test_PkdSourceSimple_threaded (void)
{
	PkdSourceSimple *source;
	GTimeVal freq = {0, 500000};
	PkdSpawnInfo info = {0};
	gint i = 1;

	source = g_object_new(PKD_TYPE_SOURCE_SIMPLE, "use-thread", TRUE, NULL);
	g_assert_cmpint(pkd_source_simple_get_use_thread(source), ==, TRUE);
	pkd_source_simple_set_callback(source, test_PkdSourceSimple_threaded_cb, &i, NULL);
	pkd_source_simple_set_frequency(source, &freq);
	pkd_source_notify_started(PKD_SOURCE(source), &info);
	g_usleep(2 * G_USEC_PER_SEC);
	pkd_source_notify_stopped(PKD_SOURCE(source));
	g_assert_cmpint(i, >=, 4); /* not perfect, but meh */

	g_object_unref(source);
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_thread_init(NULL);
	g_type_init();
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/PkdSourceSimple/threaded", test_PkdSourceSimple_threaded);

	return g_test_run();
}
