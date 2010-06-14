/* pkg-closures.c
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "pkg-closures.h"
#include "pkg-log.h"

PkgChannelCall*
pkg_channel_call_new (PkConnection *connection, /* IN */
                      gint          channel,    /* IN */
                      gpointer      user_data)  /* IN */
{
	PkgChannelCall *call;

	ENTRY;
	call = g_slice_new0(PkgChannelCall);
	call->connection = g_object_ref(connection);
	call->channel = channel;
	call->user_data = user_data;
	RETURN(call);
}

void
pkg_channel_call_free (PkgChannelCall *call) /* IN */
{
	ENTRY;
	g_object_unref(call->connection);
	g_slice_free(PkgChannelCall, call);
	EXIT;
}

PkgSourceCall*
pkg_source_call_new (PkConnection *connection, /* IN */
                     gint          source,     /* IN */
                     gpointer      user_data)  /* IN */
{
	PkgSourceCall *call;

	ENTRY;
	call = g_slice_new0(PkgSourceCall);
	call->connection = g_object_ref(connection);
	call->source = source;
	call->user_data = user_data;
	RETURN(call);
}

void
pkg_source_call_free (PkgSourceCall *call) /* IN */
{
	ENTRY;
	g_object_unref(call->connection);
	g_slice_free(PkgSourceCall, call);
	EXIT;
}

PkgSubscriptionCall*
pkg_subscription_call_new (PkConnection *connection,   /* IN */
                           gint          subscription, /* IN */
                           gpointer      user_data)    /* IN */
{
	PkgSubscriptionCall *call;

	ENTRY;
	call = g_slice_new0(PkgSubscriptionCall);
	call->connection = g_object_ref(connection);
	call->subscription = subscription;
	call->user_data = user_data;
	RETURN(call);
}

void
pkg_subscription_call_free (PkgSubscriptionCall *call) /* IN */
{
	ENTRY;
	g_object_unref(call->connection);
	g_slice_free(PkgSubscriptionCall, call);
	EXIT;
}
