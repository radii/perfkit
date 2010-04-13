#define PERFKIT_COMPILATION
#include <perfkit-agent/pka-log.h>

static void
test_PkaLog_basic (void)
{
	pka_log_init(FALSE, NULL);
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/PkaLog/basic", test_PkaLog_basic);

	return g_test_run();
}
