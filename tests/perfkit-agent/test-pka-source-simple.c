#include <perfkit-agent/perfkit-agent.h>

extern void pka_source_notify_started (PkaSource *, PkaSpawnInfo *);
extern void pka_source_notify_stopped (PkaSource *);

static void
test_PkaSourceSimple_threaded_cb (PkaSourceSimple *source,
                                  gpointer         user_data)
{
	(*((gint *)user_data))++;
}

/*
 * Tests the dedicated threaded worker.
 */
static void
test_PkaSourceSimple_threaded (void)
{
	PkaSourceSimple *source;
	GTimeVal freq = {0, 500000};
	PkaSpawnInfo info = {0};
	gint i = 1;

	source = g_object_new(PKA_TYPE_SOURCE_SIMPLE, "use-thread", TRUE, NULL);
	g_assert_cmpint(pka_source_simple_get_use_thread(source), ==, TRUE);
	pka_source_simple_set_sample_callback(source, test_PkaSourceSimple_threaded_cb, &i, NULL);
	pka_source_simple_set_frequency(source, &freq);
	pka_source_notify_started(PKA_SOURCE(source), &info);
	g_usleep(2 * G_USEC_PER_SEC);
	pka_source_notify_stopped(PKA_SOURCE(source));
	g_assert_cmpint(i, >=, 4); /* not perfect, but meh */

	g_object_unref(source);
}

/*
 * Tests the shared threaded worker.
 */
static void
test_PkaSourceSimple_shared (void)
{
	PkaSourceSimple *source;
	GTimeVal freq = {0, 500000};
	PkaSpawnInfo info = {0};
	gint i = 1;

	source = g_object_new(PKA_TYPE_SOURCE_SIMPLE, "use-thread", FALSE, NULL);
	g_assert_cmpint(pka_source_simple_get_use_thread(source), ==, FALSE);
	pka_source_simple_set_sample_callback(source, test_PkaSourceSimple_threaded_cb, &i, NULL);
	pka_source_simple_set_frequency(source, &freq);
	pka_source_notify_started(PKA_SOURCE(source), &info);
	g_usleep(2 * G_USEC_PER_SEC);
	pka_source_notify_stopped(PKA_SOURCE(source));
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

	g_test_add_func("/PkaSourceSimple/threaded", test_PkaSourceSimple_threaded);
	g_test_add_func("/PkaSourceSimple/shared", test_PkaSourceSimple_shared);

	return g_test_run();
}
