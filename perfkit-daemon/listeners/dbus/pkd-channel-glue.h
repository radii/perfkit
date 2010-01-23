gboolean
pkd_channel_get_state_dbus (PkdChannel       *channel,
                            PkdChannelState  *state,
                            GError          **error)
{
	*state = pkd_channel_get_state(channel);
	return TRUE;
}

gboolean
pkd_channel_get_pid_dbus (PkdChannel  *channel,
                          GPid        *pid,
                          GError     **error)
{
	*pid = pkd_channel_get_pid(channel);
	return TRUE;
}

gboolean
pkd_channel_get_target_dbus (PkdChannel  *channel,
                             gchar      **target,
                             GError     **error)
{
	*target = g_strdup(pkd_channel_get_target(channel));
	return TRUE;
}

gboolean
pkd_channel_get_working_dir_dbus (PkdChannel  *channel,
                                  gchar      **working_dir,
                                  GError     **error)
{
	*working_dir = g_strdup(pkd_channel_get_working_dir(channel));
	return TRUE;
}

gboolean
pkd_channel_get_args_dbus (PkdChannel   *channel,
                           gchar      ***args,
                           GError      **error)
{
	*args = g_strdupv(pkd_channel_get_args(channel));
	return TRUE;
}

static gboolean
pkd_channel_get_env_dbus (PkdChannel   *channel,
                          gchar      ***env,
                          GError      **error)
{
	*env = g_strdupv(pkd_channel_get_env(channel));
	return TRUE;
}
