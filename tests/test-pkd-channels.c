#include "pkd-channels.h"
#include "pkd-runtime.h"

static void
test_PkdChannels_add (void)
{
	PkdChannels *channels;
	PkdChannel  *channel;

	channels = g_object_new (PKD_TYPE_CHANNELS, NULL);
	channel = pkd_channels_add (channels);
	g_assert (channel != NULL);
	g_object_unref (channels);
}

static void
test_PkdChannels_remove (void)
{
	PkdChannels *channels;
	PkdChannel  *channel;

	channels = g_object_new (PKD_TYPE_CHANNELS, NULL);
	channel = pkd_channels_add (channels);
	g_assert (channel != NULL);
	pkd_channels_remove (channels, channel);
	g_object_unref (channels);
}

gint
main (gint   argc,
      gchar *argv[])
{
	g_type_init ();
	g_test_init (&argc, &argv, NULL);

	pkd_runtime_initialize (TRUE);

	g_test_add_func ("/PkdChannels/add", test_PkdChannels_add);
	g_test_add_func ("/PkdChannels/remove", test_PkdChannels_remove);

	return g_test_run ();
}
