gboolean
pka_channel_get_state_dbus (PkaChannel       *channel,
                            PkaChannelState  *state,
                            GError          **error)
{
	*state = pka_channel_get_state(channel);
	return TRUE;
}

gboolean
pka_channel_get_pid_dbus (PkaChannel  *channel,
                          GPid        *pid,
                          GError     **error)
{
	*pid = pka_channel_get_pid(channel);
	return TRUE;
}

gboolean
pka_channel_get_target_dbus (PkaChannel  *channel,
                             gchar      **target,
                             GError     **error)
{
	*target = g_strdup(pka_channel_get_target(channel));
	return TRUE;
}

gboolean
pka_channel_get_working_dir_dbus (PkaChannel  *channel,
                                  gchar      **working_dir,
                                  GError     **error)
{
	*working_dir = g_strdup(pka_channel_get_working_dir(channel));
	return TRUE;
}

gboolean
pka_channel_get_args_dbus (PkaChannel   *channel,
                           gchar      ***args,
                           GError      **error)
{
	*args = g_strdupv(pka_channel_get_args(channel));
	return TRUE;
}

static gboolean
pka_channel_get_env_dbus (PkaChannel   *channel,
                          gchar      ***env,
                          GError      **error)
{
	*env = g_strdupv(pka_channel_get_env(channel));
	return TRUE;
}
