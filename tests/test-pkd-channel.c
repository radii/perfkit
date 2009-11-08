#include "pkd-channel.h"
#include "pkd-channels.h"
#include "pkd-runtime.h"

static PkdChannel*
get_channel (void)
{
	PkdChannels *channels;
	PkdChannel  *channel;

	channels = g_object_new (PKD_TYPE_CHANNELS, NULL);
	return pkd_channels_add (channels);
}

static void
test_PkdChannel_target (void)
{
	PkdChannel *channel = get_channel ();
	gchar *target;

	g_assert (PKD_IS_CHANNEL (channel));

	g_object_set (channel, "target", "/bin/ls", NULL);
	g_object_get (channel, "target", &target, NULL);
	g_assert_cmpstr (target, ==, "/bin/ls");

	pkd_channel_set_target (channel, "/bin/ls2");
	g_assert_cmpstr (pkd_channel_get_target (channel), ==, "/bin/ls2");
}

static void
test_PkdChannel_dir (void)
{
	PkdChannel *channel = get_channel ();
	gchar *dir;

	g_assert (PKD_IS_CHANNEL (channel));

	g_object_set (channel, "dir", "/tmp", NULL);
	g_object_get (channel, "dir", &dir, NULL);
	g_assert_cmpstr (dir, ==, "/tmp");

	pkd_channel_set_dir (channel, "/sys");
	g_assert_cmpstr (pkd_channel_get_dir (channel), ==, "/sys");
}

static void
test_PkdChannel_pid (void)
{
	PkdChannel *channel = get_channel ();
	GPid pid;

	g_assert (PKD_IS_CHANNEL (channel));

	pid = 1234;

	g_object_set (channel, "pid", pid, NULL);
	g_object_get (channel, "pid", &pid, NULL);
	g_assert_cmpint (pid, ==, 1234);

	pkd_channel_set_pid (channel, 4567);
	g_assert_cmpint (pkd_channel_get_pid (channel), ==, 4567);
}

static void
test_PkdChannel_env (void)
{
	PkdChannel *channel = get_channel ();
	gchar **env;
	static gchar *lenv[] = { "LD_PRELOAD=haxer.so", NULL };

	pkd_channel_set_env (channel, (const gchar**)lenv);
	env = (gchar**)pkd_channel_get_env (channel);
	g_assert_cmpstr (lenv[0], ==, env[0]);
	g_assert_cmpstr (lenv[1], ==, env[1]);
}

static void
test_PkdChannel_args (void)
{
	PkdChannel *channel = get_channel ();
	gchar **args;
	static gchar *largs[] = { "--help", NULL };

	pkd_channel_set_env (channel, (const gchar**)largs);
	args = (gchar**)pkd_channel_get_env (channel);
	g_assert_cmpstr (largs[0], ==, args[0]);
	g_assert_cmpstr (largs[1], ==, args[1]);
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_type_init ();
	g_test_init (&argc, &argv, NULL);

	pkd_runtime_initialize (TRUE);

	g_test_add_func ("/PkdChannel/target", test_PkdChannel_target);
	g_test_add_func ("/PkdChannel/dir", test_PkdChannel_dir);
	g_test_add_func ("/PkdChannel/pid", test_PkdChannel_pid);
	g_test_add_func ("/PkdChannel/env", test_PkdChannel_env);
	g_test_add_func ("/PkdChannel/args", test_PkdChannel_args);

	return g_test_run ();
}
