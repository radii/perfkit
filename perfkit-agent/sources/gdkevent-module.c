/* gdkevent-module.c
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

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib/gprintf.h>

static void
gdkevent_handle_destroy (GdkEvent   *event,
                         GIOChannel *channel)
{
	gchar buffer[32];

	g_snprintf(buffer, sizeof(buffer), "%d|%p\n",
	           GDK_DESTROY,
	           event->any.window);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
}

static void
gdkevent_handle_expose (GdkEvent   *event,
                        GIOChannel *channel)
{
	gchar buffer[128];

	g_snprintf(buffer, sizeof(buffer), "%d|%p|%d|%d|%d|%d\n",
	           GDK_EXPOSE,
	           event->expose.window,
	           event->expose.area.x,
	           event->expose.area.y,
	           event->expose.area.width,
	           event->expose.area.height);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
}

static void
gdkevent_handle_motion_notfiy (GdkEvent   *event,
                               GIOChannel *channel)
{
	gchar buffer[128];

	g_snprintf(buffer, sizeof(buffer), "%d|%p|%0.1f|%0.1f|%u\n",
	           GDK_MOTION_NOTIFY,
	           event->motion.window,
	           event->motion.x,
	           event->motion.y,
	           event->motion.state);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
}

static void
gdkevent_handle_button (GdkEvent   *event,
                        GIOChannel *channel)
{
	gchar buffer[128];

	g_snprintf(buffer, sizeof(buffer), "%d|%p|%u|%0.1f|%0.1f|%u\n",
	           event->any.type,
	           event->button.window,
	           event->button.button,
	           event->button.x,
	           event->button.y,
	           event->button.state);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
}

static void
gdkevent_handle_key (GdkEvent   *event,
                     GIOChannel *channel)
{
	gchar buffer[128];

	g_snprintf(buffer, sizeof(buffer), "%d|%p|%u|%u\n",
	           event->any.type,
	           event->key.window,
	           event->key.keyval,
	           event->key.state);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
}

static void
gdkevent_handle_crossing (GdkEvent   *event,
                          GIOChannel *channel)
{
	gchar buffer[128];

	g_snprintf(buffer, sizeof(buffer), "%d|%p|%u|%u\n",
	           event->any.type,
	           event->crossing.window,
	           event->crossing.mode,
	           event->crossing.state);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
}

static void
gdkevent_handle_focus (GdkEvent   *event,
                       GIOChannel *channel)
{
	gchar buffer[128];

	g_snprintf(buffer, sizeof(buffer), "%d|%p|%u\n",
	           event->any.type,
	           event->focus_change.window,
	           event->focus_change.in);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
}

static void
gdkevent_handle_configure (GdkEvent   *event,
                           GIOChannel *channel)
{
	gchar buffer[128];

	g_snprintf(buffer, sizeof(buffer), "%d|%p|%d|%d|%d|%d\n",
	           event->any.type,
	           event->configure.window,
	           event->configure.x,
	           event->configure.y,
	           event->configure.width,
	           event->configure.height);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
}

static void
gdkevent_handle_any (GdkEvent   *event,
                     GIOChannel *channel)
{
	gchar buffer[32];

	g_snprintf(buffer, sizeof(buffer), "%d|%p\n",
	           event->any.type,
	           event->configure.window);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
}

static void
gdkevent_dispatcher (GdkEvent *event,
                     gpointer  data)
{
	GIOChannel *channel = data;

	switch (event->type) {
	case GDK_NOTHING:
		break;
	case GDK_DELETE:
		gdkevent_handle_any(event, channel);
		break;
	case GDK_DESTROY:
		gdkevent_handle_destroy(event, channel);
		break;
	case GDK_EXPOSE:
		gdkevent_handle_expose(event, channel);
		break;
	case GDK_MOTION_NOTIFY:
		gdkevent_handle_motion_notfiy(event, channel);
		break;
	case GDK_BUTTON_PRESS:
	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
	case GDK_BUTTON_RELEASE:
		gdkevent_handle_button(event, channel);
		break;
	case GDK_KEY_PRESS:
	case GDK_KEY_RELEASE:
		gdkevent_handle_key(event, channel);
		break;
	case GDK_ENTER_NOTIFY:
	case GDK_LEAVE_NOTIFY:
		gdkevent_handle_crossing(event, channel);
		break;
	case GDK_FOCUS_CHANGE:
		gdkevent_handle_focus(event, channel);
		break;
	case GDK_CONFIGURE:
		gdkevent_handle_configure(event, channel);
		break;
	case GDK_MAP:
	case GDK_UNMAP:
		gdkevent_handle_any(event, channel);
		break;
	default:
		/*
		 * TODO: Handle more of these specificaly.
		 */
		gdkevent_handle_any(event, channel);
		break;
	}

	gtk_main_do_event(event);
}

gint
gtk_module_init (gint   argc,
                 gchar *argv[])
{
	GIOChannel *channel = NULL;

	channel = g_io_channel_unix_new(0);
	gdk_event_handler_set(gdkevent_dispatcher, channel,
	                      (GDestroyNotify)g_io_channel_unref);
	return 0;
}
