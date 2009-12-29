#define PERFKIT_COMPILATION
#include <perfkit-daemon/pkd-log.h>

static void
test_PkdLog_basic (void)
{
	pkd_log_init(FALSE, NULL);
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/PkdLog/basic", test_PkdLog_basic);

	return g_test_run();
}
