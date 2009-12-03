/* perfkit-support.c
 * 
 * Copyright (C) 2009 Christian Hergert
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <glib/gi18n.h>

#include <perfkit/perfkit.h>
#include <perfkit-daemon/perfkit-daemon.h>

static gchar *opt_filename = NULL;

static GOptionEntry entries[] = {
	{ "filename", 'f', 0, G_OPTION_ARG_FILENAME, &opt_filename,
	  N_("Specify output filename"), N_("FILE") },
	{ NULL }
};

static void
write_kv (GIOChannel  *channel,
          const gchar *key,
          const gchar *value)
{
	g_io_channel_write_chars (channel, key, -1, NULL, NULL);
	g_io_channel_write_chars (channel, " = ", -1, NULL, NULL);
	g_io_channel_write_chars (channel, value, -1, NULL, NULL);
	g_io_channel_write_chars (channel, "\n", -1, NULL, NULL);
}

static void
generate_date (GIOChannel *channel)
{
	gchar     datestr[64];
	time_t    t;
	struct tm tt;

	memset (&tt, 0, sizeof (tt));
	memset (&datestr, 0, sizeof (datestr));
	t = time (NULL);
	tt = *gmtime (&t);
	strftime (datestr, sizeof (datestr), "%Y-%m-%d %H:%M:%S", &tt);
	write_kv (channel, "date", datestr);
}

static void
generate_system_info (GIOChannel *channel)
{
	struct utsname u;

	memset (&u, 0, sizeof (u));
	uname (&u);

	write_kv (channel, "uname.sysname", u.sysname);
	write_kv (channel, "uname.nodename", u.nodename);
	write_kv (channel, "uname.release", u.release);
	write_kv (channel, "uname.version", u.version);
	write_kv (channel, "uname.domainname", u.domainname);
}

static void
generate_channels (GIOChannel *channel)
{
	PkConnection  *conn;
	PkChannels    *channels;
	GList         *list,
	              *iter;
	GError        *error = NULL;
	gchar         *tmp,
	             **tmpv;

	g_io_channel_write_chars (channel, "\n", -1, NULL, NULL);

	if (!(conn = pk_connection_new_for_uri ("dbus://"))) {
		g_io_channel_write_chars (channel, "## Could not access perfkit-daemon!\n",
		                          -1, NULL, NULL);
		goto finish;
	}

	if (!pk_connection_connect (conn, &error)) {
		g_printerr ("Could not talk to perfkit-daemon: %s\n", error->message);
		g_error_free (error);
		return;
	}

	channels = pk_connection_get_channels (conn);
	list = pk_channels_find_all (channels);

	for (iter = list; iter; iter = iter->next) {
		tmp = g_strdup_printf ("[channel-%d]\n",
		                       pk_channel_get_id (iter->data));
		g_io_channel_write_chars (channel, tmp, -1, NULL, NULL);
		g_free (tmp);

		tmp = pk_channel_get_target (iter->data);
		write_kv (channel, "target", tmp);
		g_free (tmp);

		tmpv = pk_channel_get_args (iter->data);
		tmp = g_strjoinv (" ", tmpv);
		write_kv (channel, "args", tmp);
		g_free (tmp);
		g_strfreev (tmpv);

		tmp = pk_channel_get_dir (iter->data);
		write_kv (channel, "dir", tmp);
		g_free (tmp);

		tmpv = pk_channel_get_env (iter->data);
		tmp = g_strjoinv ("', '", tmpv);
		if (tmp && *tmp)
			tmp = g_strdup_printf ("'%s'", tmp); /* Leak */
		write_kv (channel, "env", tmp);
		g_free (tmp);
		g_strfreev (tmpv);

		g_io_channel_write_chars (channel, "\n", -1, NULL, NULL);
	}

	g_list_foreach (list, (GFunc)g_object_unref, NULL);
	g_list_free (list);

finish:
	g_io_channel_write_chars (channel, "\n", -1, NULL, NULL);
}

static gint
generate_support_data (const gchar *filename)
{
	GIOChannel *channel;
	GError     *error = NULL;

	if (g_str_equal (filename, "-")) {
		channel = g_io_channel_unix_new (0);
	}
	else if (!(channel = g_io_channel_new_file (filename, "w", &error))) {
		g_printerr ("%s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_io_channel_write_chars (channel, "[system]\n", -1, NULL, NULL);
	generate_date (channel);
	generate_system_info (channel);
	g_io_channel_write_chars (channel, "\n", -1, NULL, NULL);

	g_io_channel_write_chars (channel, "[perfkit]\n", -1, NULL, NULL);
	write_kv (channel, "lib.version", PK_VERSION_S);
	write_kv (channel, "daemon.version", PKD_VERSION_S);
	g_io_channel_write_chars (channel, "\n", -1, NULL, NULL);

	generate_channels (channel);

	/* TODO */

	if (!g_io_channel_flush (channel, &error)) {
		g_printerr ("%s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_io_channel_close (channel);

	return EXIT_SUCCESS;
}

gint
main (gint   argc,
      gchar *argv[])
{
	GOptionContext *context;
	GError         *error = NULL;
	gchar          *filename,
				    datestr[64];
	time_t          t;
	struct tm       tt;

	/* parse command line arguments */
	context = g_option_context_new ("- Generate support data for Perfkit");
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_printerr ("%s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_type_init ();

	/* determine output filename */
	if (!(filename = opt_filename)) {
		memset (&tt, 0, sizeof (tt));
		memset (&datestr, 0, sizeof (datestr));
		t = time (NULL);
		tt = *gmtime (&t);
		strftime (datestr, sizeof (datestr), "%Y%m%d%H%M%S", &tt);
		filename = g_strdup_printf ("perfkit-support-%s.log", datestr);
	}

	return generate_support_data (filename);
}
