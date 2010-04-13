#define PERFKIT_COMPILATION
#include <perfkit-agent/pka-plugins.h>

static void
test_PkaPlugins_init (void)
{
	pka_plugins_init();
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_setenv("PERFKIT_SOURCES_DIR", ".", FALSE);
	g_setenv("PERFKIT_ENCODERS_DIR", ".", FALSE);
	g_setenv("PERFKIT_LISTENERS_DIR", ".", FALSE);

	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/PkaPlugins/init", test_PkaPlugins_init);

	return g_test_run();
}
