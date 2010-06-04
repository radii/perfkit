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
#include <sys/types.h>
#include <sys/syscall.h>
#endif /* __linux__ */

#include <glib.h>
#include <string.h>
#include <unistd.h>

#include "pka-log.h"

static GPtrArray *channels     = NULL;
static gchar      hostname[64] = "";
static GLogFunc   last_handler = NULL;

G_LOCK_DEFINE(channels_lock);

/**
 * pka_log_get_thread:
 *
 * Retrieves task id for the current thread.  This is only supported on Linux.
 * On other platforms, the current process id is returned.
 *
 * Returns: The task id.
 * Side effects: None.
 */
static inline gint
pka_log_get_thread (void)
{
#if __linux__
	return (gint)syscall(SYS_gettid);
#else
	return getpid();
#endif /* __linux__ */
}

/**
 * pka_log_level_str:
 * @log_level: A #GLogLevelFlags.
 *
 * Retrieves the log level as a string.
 *
 * Returns: A string which shouldn't be modified or freed.
 * Side effects: None.
 */
static inline const gchar *
pka_log_level_str (GLogLevelFlags log_level) /* IN */
{
	#define CASE_LEVEL_STR(_l) case G_LOG_LEVEL_##_l: return #_l
	switch ((long)log_level) {
	CASE_LEVEL_STR(ERROR);
	CASE_LEVEL_STR(CRITICAL);
	CASE_LEVEL_STR(WARNING);
	CASE_LEVEL_STR(MESSAGE);
	CASE_LEVEL_STR(INFO);
	CASE_LEVEL_STR(DEBUG);
	CASE_LEVEL_STR(TRACE);
	default:
		return "UNKNOWN";
	}
	#undef CASE_LEVEL_STR
}

/**
 * pka_log_write_to_channel:
 * @channel: A #GIOChannel.
 * @message: A string log message.
 *
 * Writes @message to @channel and flushes the channel.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_log_write_to_channel (GIOChannel  *channel, /* IN */
                          const gchar *message) /* IN */
{
	g_io_channel_write_chars(channel, message, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
}

/**
 * pka_log_handler:
 * @log_domain: A string containing the log section.
 * @log_level: A #GLogLevelFlags.
 * @message: The string message.
 * @user_data: User data supplied to g_log_set_default_handler().
 *
 * Default log handler that will dispatch log messages to configured logging
 * destinations.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_log_handler (const gchar    *log_domain, /* IN */
                 GLogLevelFlags  log_level,  /* IN */
                 const gchar    *message,    /* IN */
                 gpointer        user_data)  /* IN */
{
	struct timespec ts;
	struct tm tt;
	time_t t;
	const gchar *level;
	gchar ftime[32];
	gchar *buffer;

	if (G_LIKELY(channels->len)) {
		level = pka_log_level_str(log_level);
		clock_gettime(CLOCK_REALTIME, &ts);
		t = (time_t)ts.tv_sec;
		tt = *localtime(&t);
		strftime(ftime, sizeof(ftime), "%Y/%m/%d %X", &tt);
		buffer = g_strdup_printf("%s.%04ld  %s: %12s[%d]: %8s: %s\n",
		                         ftime, ts.tv_nsec / 100000,
		                         hostname, log_domain,
		                         pka_log_get_thread(), level, message);
		G_LOCK(channels_lock);
		g_ptr_array_foreach(channels, (GFunc)pka_log_write_to_channel, buffer);
		G_UNLOCK(channels_lock);
		g_free(buffer);
	}
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
pka_log_init (gboolean     stdout_,  /* IN */
              const gchar *filename) /* IN */
{
	static gsize initialized = FALSE;
	struct utsname u;
	GIOChannel *channel;

	if (g_once_init_enter(&initialized)) {
		channels = g_ptr_array_new();
		if (filename) {
			channel = g_io_channel_new_file(filename, "a", NULL);
			g_ptr_array_add(channels, channel);
		}
		if (stdout_) {
			channel = g_io_channel_unix_new(0);
			g_ptr_array_add(channels, channel);
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
