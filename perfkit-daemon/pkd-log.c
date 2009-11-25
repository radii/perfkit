
/* pkd-log.h
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

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <string.h>
#include <unistd.h>

#include "pkd-log.h"

static GIOChannel *channel   = NULL;
static gboolean    do_stdout = FALSE;

static void
pkd_log_func (const gchar    *log_domain,
              GLogLevelFlags  log_level,
              const gchar    *message,
              gpointer        user_data)
{
	time_t     t;
	struct tm  tt;
	gchar      ftime[32],
			   hostname[64],
			  *buffer;
	GPid       pid;

	if (!channel && !do_stdout)
		return;

	memset (&tt, 0, sizeof (tt));
	memset (&hostname, 0, sizeof (hostname));

	t = time (NULL);
	tt = *localtime (&t);
	strftime (ftime, sizeof (ftime), "%b %d %X", &tt);
	gethostname (hostname, sizeof (hostname));
	pid = (GPid)getpid ();
	buffer = g_strdup_printf ("%s %s %s[%lu]: %s\n",
	                          ftime,
	                          hostname,
	                          log_domain,
	                          (gulong)pid,
	                          message);

	if (do_stdout)
		g_print ("%s", buffer);

	if (channel) {
		g_io_channel_write_chars (channel,
		                          buffer,
		                          -1,
		                          NULL,
		                          NULL);
		g_io_channel_flush (channel, NULL);
	}

	g_free (buffer);
}

void
pkd_log_init (gboolean     use_stdout,
              const gchar *filename)
{
	do_stdout = use_stdout;

	if (filename)
		channel = g_io_channel_new_file (filename, "a", NULL);

	g_log_set_handler (G_LOG_DOMAIN,
	                   G_LOG_LEVEL_MASK,
	                   pkd_log_func,
	                   NULL);
}
