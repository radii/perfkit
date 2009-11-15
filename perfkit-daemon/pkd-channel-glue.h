static gboolean pkd_channel_get_target_dbus (PkdChannel *channel, gchar **path, GError **error);
static gboolean pkd_channel_get_args_dbus (PkdChannel *channel, gchar ***args, GError **error);
static gboolean pkd_channel_get_env_dbus (PkdChannel *channel, gchar ***env, GError **error);
static gboolean pkd_channel_get_dir_dbus (PkdChannel *channel, gchar **dir, GError **error);
static gboolean pkd_channel_get_pid_dbus (PkdChannel *channel, guint *pid, GError **error);
