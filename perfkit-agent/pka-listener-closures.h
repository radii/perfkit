/* pka-listener-closures.h
 *
 * Copyright 2010 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PKA_LISTENER_CLOSURES_H__
#define __PKA_LISTENER_CLOSURES_H__

#include <glib.h>

#include "pka-log.h"
typedef struct
{
	gint channel;
	gint source;
} ChannelAddSourceCall;

typedef struct
{
	gint channel;
} ChannelGetArgsCall;

typedef struct
{
	gint channel;
} ChannelGetEnvCall;

typedef struct
{
	gint channel;
} ChannelGetExitStatusCall;

typedef struct
{
	gint channel;
} ChannelGetKillPidCall;

typedef struct
{
	gint channel;
} ChannelGetPidCall;

typedef struct
{
	gint channel;
} ChannelGetPidSetCall;

typedef struct
{
	gint channel;
} ChannelGetSourcesCall;

typedef struct
{
	gint channel;
} ChannelGetStateCall;

typedef struct
{
	gint channel;
} ChannelGetTargetCall;

typedef struct
{
	gint channel;
} ChannelGetWorkingDirCall;

typedef struct
{
	gint channel;
} ChannelMuteCall;

typedef struct
{
	gint channel;
	gchar **args;
} ChannelSetArgsCall;

typedef struct
{
	gint channel;
	gchar **env;
} ChannelSetEnvCall;

typedef struct
{
	gint channel;
	gboolean kill_pid;
} ChannelSetKillPidCall;

typedef struct
{
	gint channel;
	gint pid;
} ChannelSetPidCall;

typedef struct
{
	gint channel;
	gchar *target;
} ChannelSetTargetCall;

typedef struct
{
	gint channel;
	gchar *working_dir;
} ChannelSetWorkingDirCall;

typedef struct
{
	gint channel;
} ChannelStartCall;

typedef struct
{
	gint channel;
} ChannelStopCall;

typedef struct
{
	gint channel;
} ChannelUnmuteCall;

typedef struct
{
} DisconnectCall;

typedef struct
{
	gint encoder;
} EncoderGetPluginCall;

typedef struct
{
} ManagerAddChannelCall;

typedef struct
{
	gchar *plugin;
} ManagerAddSourceCall;

typedef struct
{
	gsize buffer_size;
	gsize timeout;
} ManagerAddSubscriptionCall;

typedef struct
{
} ManagerGetChannelsCall;

typedef struct
{
} ManagerGetPluginsCall;

typedef struct
{
} ManagerGetSourcesCall;

typedef struct
{
} ManagerGetVersionCall;

typedef struct
{
} ManagerPingCall;

typedef struct
{
	gint channel;
} ManagerRemoveChannelCall;

typedef struct
{
	gint source;
} ManagerRemoveSourceCall;

typedef struct
{
	gint subscription;
} ManagerRemoveSubscriptionCall;

typedef struct
{
	gchar *plugin;
} PluginGetCopyrightCall;

typedef struct
{
	gchar *plugin;
} PluginGetDescriptionCall;

typedef struct
{
	gchar *plugin;
} PluginGetNameCall;

typedef struct
{
	gchar *plugin;
} PluginGetPluginTypeCall;

typedef struct
{
	gchar *plugin;
} PluginGetVersionCall;

typedef struct
{
	gint source;
} SourceGetPluginCall;

typedef struct
{
	gint subscription;
	gint channel;
	gboolean monitor;
} SubscriptionAddChannelCall;

typedef struct
{
	gint subscription;
	gint source;
} SubscriptionAddSourceCall;

typedef struct
{
	gint subscription;
	gboolean drain;
} SubscriptionMuteCall;

typedef struct
{
	gint subscription;
	gint channel;
} SubscriptionRemoveChannelCall;

typedef struct
{
	gint subscription;
	gint source;
} SubscriptionRemoveSourceCall;

typedef struct
{
	gint subscription;
	gint timeout;
	gint size;
} SubscriptionSetBufferCall;

typedef struct
{
	gint subscription;
	gint encoder;
} SubscriptionSetEncoderCall;

typedef struct
{
	gint subscription;
} SubscriptionUnmuteCall;

void
ChannelAddSourceCall_Free (ChannelAddSourceCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelAddSourceCall, call);
	EXIT;
}

void
ChannelGetArgsCall_Free (ChannelGetArgsCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelGetArgsCall, call);
	EXIT;
}

void
ChannelGetEnvCall_Free (ChannelGetEnvCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelGetEnvCall, call);
	EXIT;
}

void
ChannelGetExitStatusCall_Free (ChannelGetExitStatusCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelGetExitStatusCall, call);
	EXIT;
}

void
ChannelGetKillPidCall_Free (ChannelGetKillPidCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelGetKillPidCall, call);
	EXIT;
}

void
ChannelGetPidCall_Free (ChannelGetPidCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelGetPidCall, call);
	EXIT;
}

void
ChannelGetPidSetCall_Free (ChannelGetPidSetCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelGetPidSetCall, call);
	EXIT;
}

void
ChannelGetSourcesCall_Free (ChannelGetSourcesCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelGetSourcesCall, call);
	EXIT;
}

void
ChannelGetStateCall_Free (ChannelGetStateCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelGetStateCall, call);
	EXIT;
}

void
ChannelGetTargetCall_Free (ChannelGetTargetCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelGetTargetCall, call);
	EXIT;
}

void
ChannelGetWorkingDirCall_Free (ChannelGetWorkingDirCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelGetWorkingDirCall, call);
	EXIT;
}

void
ChannelMuteCall_Free (ChannelMuteCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelMuteCall, call);
	EXIT;
}

void
ChannelSetArgsCall_Free (ChannelSetArgsCall *call) /* IN */
{
	ENTRY;
	g_strfreev(call->args);
	g_slice_free(ChannelSetArgsCall, call);
	EXIT;
}

void
ChannelSetEnvCall_Free (ChannelSetEnvCall *call) /* IN */
{
	ENTRY;
	g_strfreev(call->env);
	g_slice_free(ChannelSetEnvCall, call);
	EXIT;
}

void
ChannelSetKillPidCall_Free (ChannelSetKillPidCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelSetKillPidCall, call);
	EXIT;
}

void
ChannelSetPidCall_Free (ChannelSetPidCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelSetPidCall, call);
	EXIT;
}

void
ChannelSetTargetCall_Free (ChannelSetTargetCall *call) /* IN */
{
	ENTRY;
	g_free(call->target);
	g_slice_free(ChannelSetTargetCall, call);
	EXIT;
}

void
ChannelSetWorkingDirCall_Free (ChannelSetWorkingDirCall *call) /* IN */
{
	ENTRY;
	g_free(call->working_dir);
	g_slice_free(ChannelSetWorkingDirCall, call);
	EXIT;
}

void
ChannelStartCall_Free (ChannelStartCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelStartCall, call);
	EXIT;
}

void
ChannelStopCall_Free (ChannelStopCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelStopCall, call);
	EXIT;
}

void
ChannelUnmuteCall_Free (ChannelUnmuteCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ChannelUnmuteCall, call);
	EXIT;
}

void
DisconnectCall_Free (DisconnectCall *call) /* IN */
{
	ENTRY;
	g_slice_free(DisconnectCall, call);
	EXIT;
}

void
EncoderGetPluginCall_Free (EncoderGetPluginCall *call) /* IN */
{
	ENTRY;
	g_slice_free(EncoderGetPluginCall, call);
	EXIT;
}

void
ManagerAddChannelCall_Free (ManagerAddChannelCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ManagerAddChannelCall, call);
	EXIT;
}

void
ManagerAddSourceCall_Free (ManagerAddSourceCall *call) /* IN */
{
	ENTRY;
	g_free(call->plugin);
	g_slice_free(ManagerAddSourceCall, call);
	EXIT;
}

void
ManagerAddSubscriptionCall_Free (ManagerAddSubscriptionCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ManagerAddSubscriptionCall, call);
	EXIT;
}

void
ManagerGetChannelsCall_Free (ManagerGetChannelsCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ManagerGetChannelsCall, call);
	EXIT;
}

void
ManagerGetPluginsCall_Free (ManagerGetPluginsCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ManagerGetPluginsCall, call);
	EXIT;
}

void
ManagerGetSourcesCall_Free (ManagerGetSourcesCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ManagerGetSourcesCall, call);
	EXIT;
}

void
ManagerGetVersionCall_Free (ManagerGetVersionCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ManagerGetVersionCall, call);
	EXIT;
}

void
ManagerPingCall_Free (ManagerPingCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ManagerPingCall, call);
	EXIT;
}

void
ManagerRemoveChannelCall_Free (ManagerRemoveChannelCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ManagerRemoveChannelCall, call);
	EXIT;
}

void
ManagerRemoveSourceCall_Free (ManagerRemoveSourceCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ManagerRemoveSourceCall, call);
	EXIT;
}

void
ManagerRemoveSubscriptionCall_Free (ManagerRemoveSubscriptionCall *call) /* IN */
{
	ENTRY;
	g_slice_free(ManagerRemoveSubscriptionCall, call);
	EXIT;
}

void
PluginGetCopyrightCall_Free (PluginGetCopyrightCall *call) /* IN */
{
	ENTRY;
	g_free(call->plugin);
	g_slice_free(PluginGetCopyrightCall, call);
	EXIT;
}

void
PluginGetDescriptionCall_Free (PluginGetDescriptionCall *call) /* IN */
{
	ENTRY;
	g_free(call->plugin);
	g_slice_free(PluginGetDescriptionCall, call);
	EXIT;
}

void
PluginGetNameCall_Free (PluginGetNameCall *call) /* IN */
{
	ENTRY;
	g_free(call->plugin);
	g_slice_free(PluginGetNameCall, call);
	EXIT;
}

void
PluginGetPluginTypeCall_Free (PluginGetPluginTypeCall *call) /* IN */
{
	ENTRY;
	g_free(call->plugin);
	g_slice_free(PluginGetPluginTypeCall, call);
	EXIT;
}

void
PluginGetVersionCall_Free (PluginGetVersionCall *call) /* IN */
{
	ENTRY;
	g_free(call->plugin);
	g_slice_free(PluginGetVersionCall, call);
	EXIT;
}

void
SourceGetPluginCall_Free (SourceGetPluginCall *call) /* IN */
{
	ENTRY;
	g_slice_free(SourceGetPluginCall, call);
	EXIT;
}

void
SubscriptionAddChannelCall_Free (SubscriptionAddChannelCall *call) /* IN */
{
	ENTRY;
	g_slice_free(SubscriptionAddChannelCall, call);
	EXIT;
}

void
SubscriptionAddSourceCall_Free (SubscriptionAddSourceCall *call) /* IN */
{
	ENTRY;
	g_slice_free(SubscriptionAddSourceCall, call);
	EXIT;
}

void
SubscriptionMuteCall_Free (SubscriptionMuteCall *call) /* IN */
{
	ENTRY;
	g_slice_free(SubscriptionMuteCall, call);
	EXIT;
}

void
SubscriptionRemoveChannelCall_Free (SubscriptionRemoveChannelCall *call) /* IN */
{
	ENTRY;
	g_slice_free(SubscriptionRemoveChannelCall, call);
	EXIT;
}

void
SubscriptionRemoveSourceCall_Free (SubscriptionRemoveSourceCall *call) /* IN */
{
	ENTRY;
	g_slice_free(SubscriptionRemoveSourceCall, call);
	EXIT;
}

void
SubscriptionSetBufferCall_Free (SubscriptionSetBufferCall *call) /* IN */
{
	ENTRY;
	g_slice_free(SubscriptionSetBufferCall, call);
	EXIT;
}

void
SubscriptionSetEncoderCall_Free (SubscriptionSetEncoderCall *call) /* IN */
{
	ENTRY;
	g_slice_free(SubscriptionSetEncoderCall, call);
	EXIT;
}

void
SubscriptionUnmuteCall_Free (SubscriptionUnmuteCall *call) /* IN */
{
	ENTRY;
	g_slice_free(SubscriptionUnmuteCall, call);
	EXIT;
}

ChannelAddSourceCall*
ChannelAddSourceCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelAddSourceCall));
}

ChannelGetArgsCall*
ChannelGetArgsCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelGetArgsCall));
}

ChannelGetEnvCall*
ChannelGetEnvCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelGetEnvCall));
}

ChannelGetExitStatusCall*
ChannelGetExitStatusCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelGetExitStatusCall));
}

ChannelGetKillPidCall*
ChannelGetKillPidCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelGetKillPidCall));
}

ChannelGetPidCall*
ChannelGetPidCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelGetPidCall));
}

ChannelGetPidSetCall*
ChannelGetPidSetCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelGetPidSetCall));
}

ChannelGetSourcesCall*
ChannelGetSourcesCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelGetSourcesCall));
}

ChannelGetStateCall*
ChannelGetStateCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelGetStateCall));
}

ChannelGetTargetCall*
ChannelGetTargetCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelGetTargetCall));
}

ChannelGetWorkingDirCall*
ChannelGetWorkingDirCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelGetWorkingDirCall));
}

ChannelMuteCall*
ChannelMuteCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelMuteCall));
}

ChannelSetArgsCall*
ChannelSetArgsCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelSetArgsCall));
}

ChannelSetEnvCall*
ChannelSetEnvCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelSetEnvCall));
}

ChannelSetKillPidCall*
ChannelSetKillPidCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelSetKillPidCall));
}

ChannelSetPidCall*
ChannelSetPidCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelSetPidCall));
}

ChannelSetTargetCall*
ChannelSetTargetCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelSetTargetCall));
}

ChannelSetWorkingDirCall*
ChannelSetWorkingDirCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelSetWorkingDirCall));
}

ChannelStartCall*
ChannelStartCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelStartCall));
}

ChannelStopCall*
ChannelStopCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelStopCall));
}

ChannelUnmuteCall*
ChannelUnmuteCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ChannelUnmuteCall));
}

DisconnectCall*
DisconnectCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(DisconnectCall));
}

EncoderGetPluginCall*
EncoderGetPluginCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(EncoderGetPluginCall));
}

ManagerAddChannelCall*
ManagerAddChannelCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ManagerAddChannelCall));
}

ManagerAddSourceCall*
ManagerAddSourceCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ManagerAddSourceCall));
}

ManagerAddSubscriptionCall*
ManagerAddSubscriptionCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ManagerAddSubscriptionCall));
}

ManagerGetChannelsCall*
ManagerGetChannelsCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ManagerGetChannelsCall));
}

ManagerGetPluginsCall*
ManagerGetPluginsCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ManagerGetPluginsCall));
}

ManagerGetSourcesCall*
ManagerGetSourcesCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ManagerGetSourcesCall));
}

ManagerGetVersionCall*
ManagerGetVersionCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ManagerGetVersionCall));
}

ManagerPingCall*
ManagerPingCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ManagerPingCall));
}

ManagerRemoveChannelCall*
ManagerRemoveChannelCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ManagerRemoveChannelCall));
}

ManagerRemoveSourceCall*
ManagerRemoveSourceCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ManagerRemoveSourceCall));
}

ManagerRemoveSubscriptionCall*
ManagerRemoveSubscriptionCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(ManagerRemoveSubscriptionCall));
}

PluginGetCopyrightCall*
PluginGetCopyrightCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(PluginGetCopyrightCall));
}

PluginGetDescriptionCall*
PluginGetDescriptionCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(PluginGetDescriptionCall));
}

PluginGetNameCall*
PluginGetNameCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(PluginGetNameCall));
}

PluginGetPluginTypeCall*
PluginGetPluginTypeCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(PluginGetPluginTypeCall));
}

PluginGetVersionCall*
PluginGetVersionCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(PluginGetVersionCall));
}

SourceGetPluginCall*
SourceGetPluginCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(SourceGetPluginCall));
}

SubscriptionAddChannelCall*
SubscriptionAddChannelCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(SubscriptionAddChannelCall));
}

SubscriptionAddSourceCall*
SubscriptionAddSourceCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(SubscriptionAddSourceCall));
}

SubscriptionMuteCall*
SubscriptionMuteCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(SubscriptionMuteCall));
}

SubscriptionRemoveChannelCall*
SubscriptionRemoveChannelCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(SubscriptionRemoveChannelCall));
}

SubscriptionRemoveSourceCall*
SubscriptionRemoveSourceCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(SubscriptionRemoveSourceCall));
}

SubscriptionSetBufferCall*
SubscriptionSetBufferCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(SubscriptionSetBufferCall));
}

SubscriptionSetEncoderCall*
SubscriptionSetEncoderCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(SubscriptionSetEncoderCall));
}

SubscriptionUnmuteCall*
SubscriptionUnmuteCall_Create (void)
{
	ENTRY;
	RETURN(g_slice_new0(SubscriptionUnmuteCall));
}

#endif /* __PKA_LISTENER_CLOSURES_H__ */