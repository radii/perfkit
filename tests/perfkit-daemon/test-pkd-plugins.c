#define PERFKIT_COMPILATION
#include <perfkit-daemon/pkd-plugins.h>

static void
test_PkdPlugins_init (void)
{
	pkd_plugins_init();
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_setenv("PERFKIT_SOURCES_DIR", ".", FALSE);
	g_setenv("PERFKIT_ENCODERS_DIR", ".", FALSE);
	g_setenv("PERFKIT_LISTENERS_DIR", ".", FALSE);

	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/PkdPlugins/init", test_PkdPlugins_init);

	return g_test_run();
}
