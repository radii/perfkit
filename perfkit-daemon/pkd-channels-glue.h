#define DBUS_PKD_CHANNELS_PREFIX "/com/dronelabs/Perfkit/Channels/"
	
static gboolean pkd_channels_add_dbus (PkdChannels  *channels,
                                       gchar       **path,
                                       GError      **error);
static gboolean pkd_channels_remove_dbus (PkdChannels  *channels,
                                          gchar        *path,
                                          GError      **error);
