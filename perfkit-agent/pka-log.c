/* pka-log.c
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

#ifdef __linux__
#include <sys/utsname.h>
#endif

#include <string.h>
#include <unistd.h>

#include "pka-log.h"

static gboolean    wants_stdout = FALSE;
static GIOChannel *channel      = NULL;
static gchar       hostname[64] = "";
static GLogFunc    last_handler = NULL;

static void
pka_log_handler (const gchar    *log_domain,
                 GLogLevelFlags  log_level,
                 const gchar    *message,
                 gpointer        user_data)
{
	time_t t;
	struct tm tt;
	gchar ftime[32], *buffer;
	GPid pid;
	const gchar *level;

	if (!channel && !wants_stdout) {
		return;
	}

	switch ((log_level & G_LOG_LEVEL_MASK)) {
	case G_LOG_LEVEL_ERROR:
		level = "ERROR";
		break;
	case G_LOG_LEVEL_CRITICAL:
		level = "CRITICAL";
		break;
	case G_LOG_LEVEL_WARNING:
		level = "WARNING";
		break;
	case G_LOG_LEVEL_MESSAGE:
		level = "MESSAGE";
		break;
	case G_LOG_LEVEL_INFO:
		level = "INFO";
		break;
	case G_LOG_LEVEL_DEBUG:
		level = "DEBUG";
		break;
	default:
		g_warn_if_reached();
		level = "UNKNOWN";
		break;
	}

	memset(&tt, 0, sizeof(tt));

	t = time(NULL);
	tt = *localtime(&t);
	strftime(ftime, sizeof(ftime), "%b %d %X", &tt);
	pid = (GPid)getpid();
	buffer = g_strdup_printf("%s %s %s[%lu]: %s: %s\n",
	                         ftime,
	                         hostname,
	                         log_domain,
	                         (gulong)pid,
	                         level,
	                         message);

	if (wants_stdout) {
		g_print("%s", buffer);
	}

	if (channel) {
		g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
		g_io_channel_flush(channel, NULL);
	}

	g_free(buffer);
}

/**
 * pka_log_init:
 * @stdout_: Indicates logging should be written to stdout.
 * @filename: An optional file in which to store logs.
 *
 * Initializes the logging subsystem.
 *
 * Side effects: GLib logging handlers are attached to receive incoming log
 *   messages.  A file-handle is opened for @filename if necessary.
 */
void
pka_log_init (gboolean     stdout_,
              const gchar *filename)
{
	static gsize initialized = FALSE;
	struct utsname  u;

	/*
	 * Only allow initialization of the logging subsystem once.  Create a
	 * new GIOChannel* for easy log writing if needed.
	 */

	if (g_once_init_enter(&initialized)) {
		wants_stdout = stdout_;
		if (filename) {
			channel = g_io_channel_new_file(filename, "a", NULL);
		}

#ifdef __linux__
		uname(&u);
		memcpy(hostname, u.nodename, sizeof(hostname));
#else
#ifdef __APPLE__
		gethostname(hostname, sizeof (hostname));
#else
#error "Target platform not supported"
#endif /* __APPLE__ */
#endif /* __linux__ */

		g_log_set_default_handler(pka_log_handler, NULL);
		g_once_init_leave(&initialized, TRUE);
	}
}

/**
 * pka_log_shutdown:
 *
 * Cleans up after the logging subsystem.
 *
 * Returns: None.
 * Side effects: Logging handler is removed.
 */
void
pka_log_shutdown (void)
{
	if (last_handler) {
		g_log_set_default_handler(last_handler, NULL);
		last_handler = NULL;
	}
}
