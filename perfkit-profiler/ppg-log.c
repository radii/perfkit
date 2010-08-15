/* ppg-log.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
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

#include <string.h>
#include <unistd.h>

#include "ppg-log.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Log"

typedef struct
{
	GMutex    *mutex;
	GPtrArray *channels;
	GArray    *listeners;
	gchar      hostname[64];
	guint      id_seq;
} PpgLog;

typedef struct
{
	guint      id;
	PpgLogFunc func;
	gpointer   user_data;
} PpgLogListener;

static PpgLog ppg_log = { 0 };

/**
 * ppg_log_get_thread:
 *
 * Retrieves task id for the current thread.  This is only supported on Linux.
 * On other platforms, the current process id is returned.
 *
 * Returns: The task id.
 * Side effects: None.
 */
static inline gint
ppg_log_get_thread (void)
{
#if __linux__
	return (gint)syscall(SYS_gettid);
#else
	return getpid();
#endif /* __linux__ */
}

/**
 * ppg_log_level_str:
 *
 * Retrieves the log level as a string.
 *
 * Returns: The log level string.
 * Side effects: None.
 */
static inline const gchar *
ppg_log_level_str (GLogLevelFlags log_level) /* IN */
{
	#define CASE_LEVEL_STR(_l) case G_LOG_LEVEL_##_l: return #_l
	switch ((long)log_level) {
	CASE_LEVEL_STR(ERROR);
	CASE_LEVEL_STR(CRITICAL);
	CASE_LEVEL_STR(WARNING);
	CASE_LEVEL_STR(MESSAGE);
	CASE_LEVEL_STR(INFO);
	CASE_LEVEL_STR(DEBUG);
	default:
		return "UNKNOWN";
	}
	#undef CASE_LEVEL_STR
}

/**
 * ppg_log_handler:
 *
 * GLog handler to write log entries to configured destinations.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_log_handler (const gchar    *log_domain, /* IN */
                 GLogLevelFlags  log_level,  /* IN */
                 const gchar    *message,    /* IN */
                 gpointer        user_data)  /* IN */
{
	PpgLogListener *listener;
	GIOChannel *channel;
	const gchar *level;
	gchar ftime[32];
	gchar *buffer;
	struct timespec ts;
	struct tm tt;
	time_t t;
	gint i;

	if (G_LIKELY(ppg_log.channels->len)) {
		level = ppg_log_level_str(log_level);
		clock_gettime(CLOCK_REALTIME, &ts);
		t = (time_t)ts.tv_sec;
		tt = *localtime(&t);
		strftime(ftime, sizeof(ftime), "%Y/%m/%d %H:%M:%S", &tt);
		buffer = g_strdup_printf("%s.%04ld  %s: %12s[%d]: %8s: %s",
		                         ftime, ts.tv_nsec / 100000,
		                         ppg_log.hostname, log_domain,
		                         ppg_log_get_thread(), level, message);
		g_mutex_lock(ppg_log.mutex);
		for (i = 0; i < ppg_log.channels->len; i++) {
			channel = g_ptr_array_index(ppg_log.channels, i);
			g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
			g_io_channel_write_chars(channel, "\n", -1, NULL, NULL);
			g_io_channel_flush(channel, NULL);
		}
		for (i = 0; i < ppg_log.listeners->len; i++) {
			listener = &g_array_index(ppg_log.listeners, PpgLogListener, i);
			listener->func(buffer, listener->user_data);
		}
		g_mutex_unlock(ppg_log.mutex);
		g_free(buffer);
	}
}

/**
 * ppg_log_init:
 *
 * Initialize the logging subsystem.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_log_init (gboolean stdout_) /* IN */
{
	struct utsname u;
	GIOChannel *channel;

	/*
	 * Initialize logging structures.
	 */
	ppg_log.mutex = g_mutex_new();
	ppg_log.channels = g_ptr_array_new();
	ppg_log.listeners = g_array_new(FALSE, FALSE, sizeof(PpgLogListener));
	g_ptr_array_set_free_func(ppg_log.channels,
	                          (GDestroyNotify)g_io_channel_unref);

	/*
	 * Get hostname for logs.
	 */
#ifdef __linux__
	uname(&u);
	strncpy(ppg_log.hostname, u.nodename, sizeof(ppg_log.hostname) - 1);
#else
#error "Target platform not supported"
#endif /* __linux__ */

	/*
	 * Configure logging to stdout if necessary.
	 */
	if (stdout_) {
		channel = g_io_channel_unix_new(0);
		g_ptr_array_add(ppg_log.channels, channel);
	}

	/*
	 * Set default logger.
	 */
	g_log_set_default_handler(ppg_log_handler, NULL);
	INFO("Logging system initialized.");
}

/**
 * ppg_log_shutdown:
 *
 * Shuts down the logging subsystem.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_log_shutdown (void)
{
	g_log_set_default_handler(g_log_default_handler, NULL);
	g_ptr_array_free(ppg_log.channels, TRUE);
	g_mutex_free(ppg_log.mutex);
	memset(&ppg_log, 0, sizeof(ppg_log));
}

/**
 * ppg_log_func_name:
 * @func: A #PpgLogFunc
 * @user_data: user data for @func.
 *
 * Registers a log observer.
 *
 * Returns: A unique observer id.
 * Side effects: None.
 */
guint
ppg_log_add_listener (PpgLogFunc func,      /* IN */
                      gpointer   user_data) /* IN */
{
	PpgLogListener listener;

	g_return_val_if_fail(func != NULL, -1);

	listener.func = func;
	listener.user_data = user_data;

	g_mutex_lock(ppg_log.mutex);
	listener.id = ++ppg_log.id_seq;
	g_array_append_val(ppg_log.listeners, listener);
	g_mutex_unlock(ppg_log.mutex);

	return listener.id;
}

/**
 * ppg_log_remove_listener:
 * @: A #guint.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_log_remove_listener (guint listener_id) /* IN */
{
	PpgLogListener *listener;
	gint i;

	g_mutex_lock(ppg_log.mutex);
	for (i = 0; i < ppg_log.listeners->len; i++) {
		listener = &g_array_index(ppg_log.listeners, PpgLogListener, i);
		if (listener->id == listener_id) {
			g_array_remove_index(ppg_log.listeners, i);
			goto unlock;
		}
	}
	g_warning("No log listener matched %u.", listener_id);
  unlock:
	g_mutex_unlock(ppg_log.mutex);
}
