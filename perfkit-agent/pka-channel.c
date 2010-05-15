/* pka-channel.c
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

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "Channel"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "pka-channel.h"
#include "pka-log.h"
#include "pka-source.h"
#include "pka-subscription.h"

#define ENSURE_STATE(_c, _s, _l)                                            \
    G_STMT_START {                                                          \
    	if ((_c)->priv->state != (PKA_CHANNEL_##_s)) {                      \
    		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,  \
                        "Channel is not in the " #_s " state.");            \
            GOTO(_l);                                                       \
        }                                                                   \
    } G_STMT_END

#define RETURN_FIELD(_ch, _ft, _fi)                                         \
    G_STMT_START {                                                          \
        PkaChannelPrivate *priv;                                            \
        _ft ret;                                                            \
        g_return_val_if_fail(PKA_IS_CHANNEL((_ch)), 0);                     \
        ENTRY;                                                              \
        priv = (_ch)->priv;                                                 \
        g_mutex_lock(priv->mutex);                                          \
        ret = priv->_fi;                                                    \
        g_mutex_unlock(priv->mutex);                                        \
        RETURN(ret);                                                        \
    } G_STMT_END

#define RETURN_FIELD_COPY(_ch, _cf, _fi)                                    \
    G_STMT_START {                                                          \
        PkaChannelPrivate *priv;                                            \
        gpointer ret;                                                       \
        g_return_val_if_fail(PKA_IS_CHANNEL((_ch)), NULL);                  \
        ENTRY;                                                              \
        priv = (_ch)->priv;                                                 \
        g_mutex_lock(priv->mutex);                                          \
        ret = _cf(priv->_fi);                                               \
        g_mutex_unlock(priv->mutex);                                        \
        RETURN(ret);                                                        \
    } G_STMT_END

#define AUTHORIZE_IOCTL(_c, _i, _t, _l)                                     \
    G_STMT_START {                                                          \
    	/* TODO: handle target (_t) */                                      \
        if (!pka_context_is_authorized(_c, PKA_IOCTL_##_i)) {               \
        	g_set_error(error, PKA_CONTEXT_ERROR,                           \
                        PKA_CONTEXT_ERROR_NOT_AUTHORIZED,                   \
                        "Insufficient permissions for operation.");         \
            GOTO(_l);                                                       \
        }                                                                   \
    } G_STMT_END

#define SET_FIELD_COPY(_ch, _fr, _cp, _t)                                   \
    G_STMT_START {                                                          \
        PkaChannelPrivate *priv;                                            \
        gboolean ret = FALSE;                                               \
        ENTRY;                                                              \
        priv = _ch->priv;                                                   \
        AUTHORIZE_IOCTL(context, MODIFY_CHANNEL, _ch, failed);              \
        g_mutex_lock(priv->mutex);                                          \
        ENSURE_STATE(_ch, READY, unlock);                                   \
        DEBUG(Channel, "Setting " #_t " of channel %d on behalf of "        \
              "context %d.", priv->id, pka_context_get_id(context));        \
        _fr(priv->_t);                                                      \
        priv->_t = _cp(_t);                                                 \
        ret = TRUE;                                                         \
      unlock:                                                               \
      	g_mutex_unlock(priv->mutex);                                        \
      failed:                                                               \
      	RETURN(ret);                                                        \
	} G_STMT_END

/**
 * SECTION:pka-channel
 * @title: PkaChannel
 * @short_description: Data source aggregation and process management
 *
 * #PkaChannel encapsulates the logic for spawning a new inferior process or
 * attaching to an existing one.  #PkaSource<!-- -->'s are added to the
 * channel to provide instrumentation into the inferior.
 *
 * #PkaSource implementations will deliver data samples to the channel
 * directly.  The channel will in turn pass them on to any subscriptions that
 * are observing the channel.  This also occurs for #PkaManifest updates.
 */

G_DEFINE_TYPE (PkaChannel, pka_channel, G_TYPE_OBJECT)

/*
 * Internal methods used for management of samples and manifests.
 */
extern void pka_subscription_deliver_sample   (PkaSubscription *subscription,
                                               PkaSample       *sample);
extern void pka_subscription_deliver_manifest (PkaSubscription *subscription,
                                               PkaManifest     *manifest);
extern void pka_sample_set_source_id          (PkaSample       *sample,
                                               gint             source_id);
extern void pka_manifest_set_source_id        (PkaManifest     *manifest,
                                               gint             source_id);
extern void pka_source_notify_started         (PkaSource       *source,
                                               PkaSpawnInfo    *spawn_info);
extern void pka_source_notify_stopped         (PkaSource       *source);
extern void pka_source_notify_muted           (PkaSource       *source);
extern void pka_source_notify_unmuted         (PkaSource       *source);
extern void pka_source_set_channel            (PkaSource       *source,
                                               PkaChannel      *channel);

struct _PkaChannelPrivate
{
	guint         id;         /* Monotonic id for the channel. */

	GMutex       *mutex;
	guint         state;      /* Current channel state. */
	GPtrArray    *subs;       /* Array of subscriptions. */
	GPtrArray    *sources;    /* Array of data sources. */
	GTree        *indexed;    /* Pointer-to-index data source map. */
	GTree        *manifests;  /* Pointer-to-manifest map. */

	GPid          pid;         /* Inferior pid */
	gboolean      pid_set;     /* Pid was attached, not spawned */
	gchar        *working_dir; /* Inferior working directory */
	gchar        *target;      /* Inferior executable */
	gchar       **env;         /* Key=Value environment */
	gchar       **args;        /* Target arguments */
	gboolean      kill_pid;    /* Should inferior be killed upon stop */
	gint          exit_status; /* The inferiors exit status */
};

static guint channel_seq = 0;

/**
 * pka_channel_new:
 *
 * Creates a new instance of #PkaChannel.
 *
 * Return value: the newly created #PkaChannel.
 * Side effects: None.
 */
PkaChannel*
pka_channel_new (void)
{
	ENTRY;
	RETURN(g_object_new(PKA_TYPE_CHANNEL, NULL));
}

/**
 * pka_channel_get_id:
 * @channel: A #PkaChannel
 *
 * Retrieves the channel id.
 *
 * Returns: A guint.
 * Side effects: None.
 */
guint
pka_channel_get_id (PkaChannel *channel) /* IN */
{
	g_return_val_if_fail(PKA_IS_CHANNEL(channel), G_MAXUINT);
	return channel->priv->id;
}

/**
 * pka_channel_get_target:
 * @channel: A #PkaChannel
 *
 * Retrieves the "target" executable for the channel.  If the channel is to
 * connect to an existing process this is %NULL.
 *
 * Returns: A string containing the target.
 *   The value should be freed with g_free().
 * Side effects: None.
 */
gchar*
pka_channel_get_target (PkaChannel *channel) /* IN */
{
	RETURN_FIELD_COPY(channel, g_strdup, target);
}

/**
 * pka_channel_set_target:
 * @channel: A #PkaChannel.
 * @context: A #PkaContext.
 * @target: the target executable.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the target of the #PkaChannel.  If the context does not have
 * permissions, this operation will fail.  The channel also must not
 * have been started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_set_target (PkaChannel   *channel, /* IN */
                        PkaContext   *context, /* IN */
                        const gchar  *target,  /* IN */
                        GError      **error)   /* OUT */
{
	SET_FIELD_COPY(channel, g_free, g_strdup, target);
}

/**
 * pka_channel_get_working_dir:
 * @channel: A #PkaChannel
 *
 * Retrieves the "working-dir" property for the channel.  This is the directory
 * the process should be spawned within.
 *
 * Returns: A string containing the working directory.
 *   The value should be freed with g_free().
 * Side effects: None.
 */
gchar*
pka_channel_get_working_dir (PkaChannel *channel) /* IN */
{
	RETURN_FIELD_COPY(channel, g_strdup, working_dir);
}

/**
 * pka_channel_set_working_dir:
 * @channel: A #PkaChannel.
 * @context: A #PkaContext.
 * @working_dir: the working directory for the target.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the working directory of the inferior process.  If the context
 * does not have permissions, this operation will fail.  Also, this may
 * only be called before the channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_set_working_dir (PkaChannel   *channel,     /* IN */
                             PkaContext   *context,     /* IN */
                             const gchar  *working_dir, /* IN */
                             GError      **error)       /* OUT */
{
	SET_FIELD_COPY(channel, g_free, g_strdup, working_dir);
}

/**
 * pka_channel_get_args:
 * @channel: A #PkaChannel
 *
 * Retrieves the "args" property.
 *
 * Returns: A string array containing the arguments to the process.  This
 *   value should be freed with g_strfreev().
 * Side effects: None.
 */
gchar**
pka_channel_get_args (PkaChannel *channel) /* IN */
{
	RETURN_FIELD_COPY(channel, g_strdupv, args);
}

/**
 * pka_channel_set_args:
 * @channel: A #PkaChannel.
 * @context: A #PkaContext.
 * @args: The target arguments.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the arguments for the target.  If the context does not have
 * permissions, this operation will fail.  Also, this may only be called
 * before the session has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_set_args (PkaChannel  *channel, /* IN */
                      PkaContext  *context, /* IN */
                      gchar      **args,    /* IN */
                      GError     **error)   /* OUT */
{
	SET_FIELD_COPY(channel, g_strfreev, g_strdupv, args);
}

/**
 * pka_channel_get_env:
 * @channel: A #PkaChannel
 *
 * Retrieves the "env" property.
 *
 * Returns: A string array containing the environment variables to set before
 *   the process has started.  The value should be freed with g_free().
 * Side effects: None.
 */
gchar**
pka_channel_get_env (PkaChannel *channel) /* IN */
{
	RETURN_FIELD_COPY(channel, g_strdupv, env);
}

/**
 * pka_channel_set_env:
 * @channel: A #PkaChannel.
 * @context: A #PkaContext.
 * @env: A #GStrv containing the environment.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the environment of the target process.  @args should be a #GStrv
 * containing "KEY=VALUE" strings for the environment.  If the context
 * does not have permissions, this operation will fail.  Also, this may
 * only be called before the channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_set_env (PkaChannel  *channel, /* IN */
                     PkaContext  *context, /* IN */
                     gchar      **env,     /* IN */
                     GError     **error)   /* OUT */
{
	SET_FIELD_COPY(channel, g_strfreev, g_strdupv, env);
}

/**
 * pka_channel_get_pid:
 * @channel: A #PkaChannel
 *
 * Retrieves the pid of the process.  If an existing process is to be monitored
 * this is the pid of that process.  If a new process was to be spawned, after
 * the process has been spawned this will contain its process id.
 *
 * Returns: The process id of the monitored proces.
 *
 * Side effects: None.
 */
GPid
pka_channel_get_pid (PkaChannel *channel)
{
	RETURN_FIELD(channel, GPid, pid);
}

/**
 * pka_channel_set_pid:
 * @channel: A #PkaChannel.
 * @context: A #PkaContext.
 * @pid: A #GPid.
 * @error: A location for #GError, or %NULL.
 *
 * Sets the pid of the process of which to attach.  If set, the channel will
 * not spawn a process, but instead instruct the sources to attach to the
 * existing process.  If the context does not have permissions, this
 * operation will fail.  Also, this may only be called before the channel has
 * been started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_set_pid (PkaChannel  *channel, /* IN */
                     PkaContext  *context, /* IN */
                     GPid         pid,     /* IN */
                     GError     **error)   /* OUT */
{
	PkaChannelPrivate *priv;
	gboolean ret = FALSE;

	ENTRY;
	priv = channel->priv;
	AUTHORIZE_IOCTL(context, MODIFY_CHANNEL, channel, failed);
	g_mutex_lock(priv->mutex);
	ENSURE_STATE(channel, READY, unlock);
	INFO(Channel, "Setting pid of channel %d to %d on behalf of context %d.",
	     priv->id, pid, pka_context_get_id(context));
	priv->pid = pid;
	priv->pid_set = TRUE;
	g_free(priv->target);
	priv->target = NULL;
	ret = TRUE;
  unlock:
	g_mutex_unlock(priv->mutex);
  failed:
	RETURN(ret);
}

/**
 * pka_channel_get_pid_set:
 * @channel: A #PkaChannel.
 *
 * Determines if the pid was set manually, as opposed to set when the inferior
 * process was spawned.  %TRUE indicates that the channel is attaching to an
 * existing process.
 *
 * Returns: %TRUE if the pid was set; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_get_pid_set (PkaChannel *channel) /* IN */
{
    RETURN_FIELD(channel, gboolean, pid_set);
}

/**
 * pka_channel_get_kill_pid:
 * @channel: A #PkaChannel.
 *
 * Retrieves if the inferior process should be killed if the channel is stopped
 * before it has exited.  This value is ignored if the process was not spawned
 * by Perfkit.
 *
 * Returns: %TRUE if the process will be killed upon stopping the channel;
 *   otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_get_kill_pid (PkaChannel *channel) /* IN */
{
	RETURN_FIELD(channel, gboolean, kill_pid);
}

/**
 * pka_channel_set_kill_pid:
 * @channel: A #PkaChannel.
 * @context: A #PkaContext.
 * @kill_pid: If the inferior should be killed when stopping.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the "kill_pid" property.  If set, any process spawned by Perfkit will
 * be killed when the channel is stopped.  This is the default.  If the context
 * does not have permissions, this operation will fail.
 *
 * This may be called at any time.  If the process has already been stopped,
 * this will do nothing.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_set_kill_pid (PkaChannel  *channel,  /* IN */
                          PkaContext  *context,  /* IN */
                          gboolean     kill_pid, /* IN */
                          GError     **error)    /* OUT */
{
	PkaChannelPrivate *priv;
	gboolean ret = FALSE;

	ENTRY;
	priv = channel->priv;
	AUTHORIZE_IOCTL(context, MODIFY_CHANNEL, channel, failed);
	g_mutex_lock(priv->mutex);
	DEBUG(Channel, "Setting kill_pid of channel %d to %s on behalf of "
	               "context %d.", priv->id, kill_pid ? "TRUE" : "FALSE",
	               pka_context_get_id(context));
	priv->kill_pid = kill_pid;
	ret = TRUE;
	g_mutex_unlock(priv->mutex);
  failed:
	RETURN(ret);
}

/**
 * pka_channel_get_exit_status:
 * @channel: A #PkaChannel.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the exit status of the inferior process.  The exit status is
 * only accurate if the process was spawned by @channel and has exited.
 *
 * Returns: The exit status of @channels<!-- -->'s inferior.
 * Side effects: None.
 */
gboolean
pka_channel_get_exit_status (PkaChannel  *channel,     /* IN */
                             gint        *exit_status, /* OUT */
                             GError     **error)       /* OUT */
{
	PkaChannelPrivate *priv;
	gboolean ret = FALSE;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(exit_status != NULL, FALSE);

	ENTRY;
	priv = channel->priv;
	g_mutex_lock(priv->mutex);
	if (priv->state != PKA_CHANNEL_STOPPED) {
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            "The process has not exited yet.");
		GOTO(failed);
	}
	if (priv->pid_set) {
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            "The exit status may only be retrieved from processes "
		            "spawned by Perfkit.");
		GOTO(failed);
	}
	*exit_status = priv->exit_status;
	ret = TRUE;
  failed:
	g_mutex_unlock(priv->mutex);
	RETURN(ret);
}

/**
 * pka_channel_inferior_exited:
 * @pid: A #GPid.
 * @status: The inferior exit status.
 * @channel: A #PkaChannel.
 *
 * Callback up on the child process exiting.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_channel_inferior_exited (GPid        pid,     /* IN */
                             gint        status,  /* IN */
                             PkaChannel *channel) /* IN */
{
	PkaChannelPrivate *priv;

	g_return_if_fail(PKA_IS_CHANNEL(channel));

	ENTRY;
	priv = channel->priv;
	g_mutex_lock(priv->mutex);
	priv->exit_status = status;
	g_mutex_unlock(priv->mutex);
	pka_channel_stop(channel, pka_context_default(), NULL);
	g_object_unref(channel);
	EXIT;
}

/**
 * pka_channel_init_spawn_info_locked:
 * @channel: A #PkaChannel.
 * @spawn_info: A #PkaSpawnInfo.
 * @error: A location for a #GError, or %NULL.
 *
 * Initializes @spawn_info with the current data within @channel.  The sources
 * are notified and allowed to modify the spawn info if needed.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
static gboolean
pka_channel_init_spawn_info_locked (PkaChannel    *channel,    /* IN */
                                    PkaSpawnInfo  *spawn_info, /* IN */
                                    GError       **error)      /* OUT */
{
	PkaChannelPrivate *priv;
	PkaSource *source;
	gboolean ret = TRUE;
	gint i;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(spawn_info != NULL, FALSE);

	ENTRY;
	priv = channel->priv;
	memset(spawn_info, 0, sizeof(*spawn_info));
	spawn_info->target = g_strdup(priv->target);
	spawn_info->working_dir = g_strdup(priv->working_dir);
	spawn_info->env = g_strdupv(priv->env);
	spawn_info->args = g_strdupv(priv->args);
	spawn_info->pid = priv->pid;
	for (i = 0; i < priv->sources->len; i++) {
		source = g_ptr_array_index(priv->sources, i);
		if (!pka_source_modify_spawn_info(source, spawn_info, error)) {
			ret = FALSE;
			GOTO(error);
		}
	}
  error:
	RETURN(ret);
}

/**
 * pka_channel_destroy_spawn_info:
 * @channel: A #PkaChannel.
 * @spawn_info: A #PkaSpawnInfo.
 *
 * Destroys the previously initialized #PkaSpawnInfo.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_channel_destroy_spawn_info (PkaChannel   *channel,    /* IN */
                                PkaSpawnInfo *spawn_info) /* IN */
{
	ENTRY;
	g_free(spawn_info->target);
	g_free(spawn_info->working_dir);
	g_strfreev(spawn_info->args);
	g_strfreev(spawn_info->env);
	EXIT;
}

/**
 * pka_channel_start:
 * @channel: A #PkaChannel
 * @error: A location for a #GError or %NULL
 *
 * Attempts to start the channel.  If the channel was successfully started
 * the attached #PkaSource<!-- -->'s will be notified to start creating
 * samples.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects:
 *   The channels state-machine is altered.
 *   Data sources are notified to start sending manifests and samples.
 */
gboolean
pka_channel_start (PkaChannel  *channel, /* IN */
                   PkaContext  *context, /* IN */
                   GError     **error)   /* OUT */
{
	PkaChannelPrivate *priv;
	PkaSpawnInfo spawn_info = { 0 };
	gboolean ret = FALSE;
	GError *local_error = NULL;
	gchar **argv = NULL;
	gchar *argv_str;
	gchar *env_str;
	gint len, i;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(channel->priv->state == PKA_CHANNEL_READY, FALSE);

	ENTRY;
	priv = channel->priv;
	g_mutex_lock(priv->mutex);
	ENSURE_STATE(channel, READY, unlock);
	INFO(Channel, "Starting channel %d on behalf of context %d.",
	     priv->id, pka_context_get_id(context));

	/*
	 * Allow sources to modify the spawn information.
	 */
	if (!pka_channel_init_spawn_info_locked(channel, &spawn_info, error)) {
		GOTO(unlock);
	}

	/*
	 * Spawn the inferior process only if necessary.
	 */
	if ((!spawn_info.pid) && (spawn_info.target)) {
		/*
		 * Build the spawned argv. Copy in arguments if needed.
		 */
		if (spawn_info.args) {
			len = g_strv_length(spawn_info.args);
			argv = g_new0(gchar*,  len + 2);
			for (i = 0; i < len; i++) {
				argv[i + 1] = g_strdup(spawn_info.args[i]);
			}
		} else {
			argv = g_new0(gchar*, 2);
		}

		/*
		 * Set the executable target.
		 */
		argv[0] = g_strdup(spawn_info.target);

		/*
		 * Determine the working directory.
		 */
		if (!spawn_info.working_dir || strlen(spawn_info.working_dir) == 0) {
			spawn_info.working_dir = g_strdup(g_get_tmp_dir());
		}

		/*
		 * Log the channel arguments.
		 */
		argv_str = spawn_info.args ? g_strjoinv(", ", spawn_info.args)
		                           : g_strdup("");
		env_str = spawn_info.env ? g_strjoinv(", ", spawn_info.env)
		                         : g_strdup("");
		INFO(Channel, "Attempting to spawn channel %d on behalf of context %d.",
		     priv->id, pka_context_get_id(context));
		INFO(Channel, "             Target = %s",
		     spawn_info.target ? spawn_info.target : "");
		INFO(Channel, "          Arguments = %s", argv_str);
		INFO(Channel, "  Working Directory = %s", spawn_info.working_dir);
		INFO(Channel, "        Environment = %s", env_str);
		g_free(argv_str);
		g_free(env_str);

		/*
		 * Attempt to spawn the process.
		 */
		if (!g_spawn_async(spawn_info.working_dir,
		                   argv,
		                   spawn_info.env,
		                   G_SPAWN_SEARCH_PATH |
		                   G_SPAWN_STDERR_TO_DEV_NULL |
		                   G_SPAWN_STDOUT_TO_DEV_NULL |
		                   G_SPAWN_DO_NOT_REAP_CHILD,
		                   NULL,
		                   NULL,
		                   &priv->pid,
		                   &local_error))
		{
			priv->state = PKA_CHANNEL_FAILED;
			WARNING(Channel, "Error starting channel %d: %s",
			        priv->id, local_error->message);
			g_propagate_error(error, local_error);
			GOTO(unlock);
		}

		/*
		 * Register callback upon child exit.
		 */
		g_child_watch_add(priv->pid,
		                  (GChildWatchFunc)pka_channel_inferior_exited,
		                  g_object_ref(channel));

		INFO(Channel, "Channel %d spawned process %d.", priv->id, priv->pid);
	}

	priv->state = PKA_CHANNEL_RUNNING;
	ret = TRUE;

  unlock:
  	g_mutex_unlock(priv->mutex);
  	if (ret) {
		/*
		 * Notify the included data channels of the inferior starting up.
		 */
		g_ptr_array_foreach(priv->sources,
		                    (GFunc)pka_source_notify_started,
		                    &spawn_info);
	}
	pka_channel_destroy_spawn_info(channel, &spawn_info);
	g_strfreev(argv);
	RETURN(ret);
}

/**
 * pka_channel_stop:
 * @channel: A #PkaChannel.
 * @context: A #PkaContext.
 * @error: A location for a #GError or %NULL.
 *
 * Attempts to stop the channel.  If successful, the attached
 * #PkaSource<!-- -->'s will be notified that they should stop sending
 * samples.  After the sources have been notified if @killpid is set
 * the process will be terminated.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects:
 *   The state machine is altered.
 *   Data sources are notified to stop.
 *   The process is terminated if @killpid is set.
 */
gboolean
pka_channel_stop (PkaChannel  *channel, /* IN */
                  PkaContext  *context, /* IN */
                  GError     **error)   /* OUT */
{
	PkaChannelPrivate *priv;
	gboolean ret = TRUE;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	g_mutex_lock(priv->mutex);
	switch (priv->state) {
	case PKA_CHANNEL_RUNNING:
	case PKA_CHANNEL_MUTED:
		INFO(Channel, "Stopping channel %d on behalf of context %d.",
		     priv->id, pka_context_get_id(context));
		priv->state = PKA_CHANNEL_STOPPED;

		/*
		 * Notify sources of channel stopping.
		 */
		g_ptr_array_foreach(priv->sources,
		                    (GFunc)pka_source_notify_stopped,
		                    NULL);

		/*
		 * Kill the process only if settings permit and we spawned the
		 * process outself.
		 */
		if ((priv->kill_pid) && (!priv->pid_set) && (priv->pid)) {
			INFO(Channel, "Channel %d killing process %d.",
			     priv->id, (gint)priv->pid);
			kill(priv->pid, SIGKILL);
		}
		BREAK;
	case PKA_CHANNEL_READY:
		ret = FALSE;
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            "Cannot stop a channel that has not yet been started.");
		BREAK;
	case PKA_CHANNEL_STOPPED:
	case PKA_CHANNEL_FAILED:
		BREAK;
	default:
		g_warn_if_reached();
	}
	g_mutex_unlock(priv->mutex);
	RETURN(ret);
}

/**
 * pka_channel_mute:
 * @channel: A #PkaChannel.
 * @error: A location for a #GError, or %NULL.
 *
 * Attempts to mute the channel.  If successful, the attached
 * #PkaSource<!-- -->'s are notified to mute as well.  Any samples delivered
 * while muted are silently dropped.  Updated manifests, however, are stored
 * for future delivery when unpausing occurs.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects:
 *   The state machine is altered.
 *   Data sources are notified to mute.
 */
gboolean
pka_channel_mute (PkaChannel  *channel, /* IN */
                  PkaContext  *context, /* IN */
                  GError     **error)   /* IN */
{
	PkaChannelPrivate *priv;
	gboolean ret = FALSE;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	g_mutex_lock(priv->mutex);
	switch (priv->state) {
	case PKA_CHANNEL_READY:
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            _("Cannot mute channel; not yet started."));
		BREAK;
	case PKA_CHANNEL_STOPPED:
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            _("Cannot mute channel; channel stopped."));
		BREAK;
	case PKA_CHANNEL_FAILED:
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            _("Cannot mute channel; channel failed to start."));
		BREAK;
	case PKA_CHANNEL_MUTED:
		ret = TRUE;
		BREAK;
	case PKA_CHANNEL_RUNNING:
		priv->state = PKA_CHANNEL_MUTED;
		INFO(Channel, "Muting channel %d on behalf of context %d.",
		     priv->id, pka_context_get_id(context));

		/*
		 * Notify sources that we have muted.
		 */
		g_ptr_array_foreach(priv->sources,
		                    (GFunc)pka_source_notify_muted,
		                    NULL);
		ret = TRUE;
		BREAK;
	default:
		g_warn_if_reached();
	}
	g_mutex_unlock(priv->mutex);
	RETURN(ret);
}

/**
 * pka_channel_traverse_manifests_cb:
 * @source: A #PkaSource
 * @manifest: A #PkaManifest
 * @channel: A #PkaChannel
 *
 * Internal method for use within a GTree foreach to notify all subscriptions
 * of the current manifest.
 *
 * Returns: %FALSE indicating we want to continue tree traversal.
 * Side effects: The manifest is broadcasted to subscriptions.
 */
static gboolean
pka_channel_traverse_manifests_cb (PkaSource   *source,   /* IN */
                                   PkaManifest *manifest, /* IN */
                                   PkaChannel  *channel)  /* IN */
{
	g_ptr_array_foreach(channel->priv->subs,
	                    (GFunc)pka_subscription_deliver_manifest,
	                    manifest);
	return FALSE;
}

/**
 * pka_channel_unmute:
 * @channel: A #PkaChannel.
 * @error: A location for a #GError or %NULL.
 *
 * Attempts to unmute a #PkaChannel.  If successful; the
 * #PkaSource<!-- -->'s will be notified to unmute and continue sending
 * samples.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects:
 *   The state machine is altered.
 *   Data sources are notified to resume.
 */
gboolean
pka_channel_unmute (PkaChannel  *channel, /* IN */
                    PkaContext  *context, /* IN */
                    GError     **error)   /* OUT */
{
	PkaChannelPrivate *priv;
	gboolean ret = FALSE;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);

	ENTRY;
	priv = channel->priv;
	g_mutex_lock(priv->mutex);
	switch (priv->state) {
	case PKA_CHANNEL_MUTED:
		INFO(Channel, "Unpausing channel %d on behalf of context %d.",
		     priv->id, pka_context_get_id(context));
		priv->state = PKA_CHANNEL_RUNNING;

		/*
		 * Send subscriptions current manifest.
		 */
		g_tree_foreach(priv->manifests,
		               (GTraverseFunc)pka_channel_traverse_manifests_cb,
		               channel);

		/*
		 * Enable sources.
		 */
		g_ptr_array_foreach(priv->sources,
		                    (GFunc)pka_source_notify_unmuted,
		                    NULL);

		ret = TRUE;
		BREAK;
	case PKA_CHANNEL_READY:
	case PKA_CHANNEL_RUNNING:
	case PKA_CHANNEL_STOPPED:
	case PKA_CHANNEL_FAILED:
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            "Channel %d is not muted.", priv->id);
		BREAK;
	default:
		g_warn_if_reached();
	}
	g_mutex_unlock(priv->mutex);
	RETURN(ret);
}

/**
 * pka_channel_deliver_sample:
 * @channel: A #PkaChannel
 * @source: A #PkaSource
 * @sample: A #PkaSample
 *
 * Internal method used by #PkaSource to deliver its samples to our channel.
 *
 * Side effects: None.
 */
void
pka_channel_deliver_sample (PkaChannel *channel,
                            PkaSource  *source,
                            PkaSample  *sample)
{
	PkaChannelPrivate *priv;
	gint i, idx;

	g_return_if_fail(channel != NULL);
	g_return_if_fail(source != NULL);
	g_return_if_fail(sample != NULL);

	/*
	 * NOTES:
	 *
	 *   Can we look into dropping this into a Multi-Producer, Single Consumer
	 *   Queue that ships the samples over to the subscription every so often?
	 *   It would be nice not to screw with the sample thread by introducing
	 *   our mutexes.  It could cause collateral damage between sources during
	 *   contention.
	 *
	 */

	priv = channel->priv;

	g_mutex_lock(priv->mutex);

	/*
	 * Drop the sample if we are not currently recording samples.
	 */
	if (priv->state != PKA_CHANNEL_RUNNING) {
		goto unlock;
	}

	/*
	 * Pass the sample to all of the observing subscriptions.
	 */
	for (i = 0; i < priv->subs->len; i++) {
		idx = GPOINTER_TO_INT(g_tree_lookup(priv->indexed, source));
		pka_sample_set_source_id(sample, idx);
		pka_subscription_deliver_sample(priv->subs->pdata[i],
		                                sample);
	}

unlock:
	g_mutex_unlock(priv->mutex);
}

/**
 * pka_channel_deliver_manifest:
 * @channel: A #PkaChannel
 * @source: A #PkaSource
 * @manifest: A #PkaManifest
 *
 * Internal method used by #PkaSource to deliver its manifest updates to our
 * channel.
 *
 * Side effects: None.
 */
void
pka_channel_deliver_manifest (PkaChannel  *channel,
                              PkaSource   *source,
                              PkaManifest *manifest)
{
	PkaChannelPrivate *priv;
	gint idx;
	gint i;

	g_return_if_fail(channel != NULL);
	g_return_if_fail(source != NULL);
	g_return_if_fail(manifest != NULL);

	ENTRY;
	priv = channel->priv;
	g_mutex_lock(priv->mutex);

	/*
	 * NOTES:
	 *
	 *   We always store the most recent copy of the manifest for when
	 *   subscriptions are added.  That way we can notify them of the
	 *   current data manifest before we pass samples.
	 */

	/*
	 * Look up source index and store in manifest.
	 */
	idx = GPOINTER_TO_INT(g_tree_lookup(priv->indexed, source));
	pka_manifest_set_source_id(manifest, idx);

	/*
	 * Store the manifest for future lookup.
	 */
	g_tree_insert(priv->manifests, source, pka_manifest_ref(manifest));

	/*
	 * We are finished for now unless we are active.
	 */
	if (priv->state != PKA_CHANNEL_RUNNING) {
		GOTO(unlock);
	}

	/*
	 * Notify subscriptions of the manifest update.
	 */
	for (i = 0; i < priv->subs->len; i++) {
		pka_subscription_deliver_manifest(priv->subs->pdata[i], manifest);
	}
  unlock:
	g_mutex_unlock(priv->mutex);
	EXIT;
}

/**
 * pka_channel_deliver_manifest_to_subscription:
 * @key: The #GTree key.
 * @value: A #PkaManifest.
 * @data: A #PkaSubscription.
 *
 * Delivers a #PkaManifest to a subscription.
 *
 * Returns: %FALSE to indicate further #GTree traversal.
 * Side effects: None.
 */
static gboolean
pka_channel_deliver_manifest_to_subscription (gpointer key,   /* IN */
                                              gpointer value, /* IN */
                                              gpointer data)  /* IN */
{
	ENTRY;
	pka_subscription_deliver_manifest(data, value);
	RETURN(FALSE);
}

/**
 * pka_channel_add_subscription:
 * @channel: A #PkaChannel
 * @subscription: A #PkaSubscription
 *
 * Internal method for registering a subscription to a channel.
 *
 * Side effects: None.
 */
void
pka_channel_add_subscription (PkaChannel      *channel,      /* IN */
                              PkaSubscription *subscription) /* IN */
{
	PkaChannelPrivate *priv;

	g_return_if_fail(PKA_IS_CHANNEL(channel));
	g_return_if_fail(subscription != NULL);

	ENTRY;
	priv = channel->priv;
	g_mutex_lock(priv->mutex);
	g_ptr_array_add(priv->subs, pka_subscription_ref(subscription));
	if ((priv->state & (PKA_CHANNEL_RUNNING | PKA_CHANNEL_MUTED)) != 0) {
		g_tree_foreach(priv->manifests,
		               pka_channel_deliver_manifest_to_subscription,
		               subscription);
	}
	g_mutex_unlock(priv->mutex);
	EXIT;
}

/**
 * pka_channel_remove_subscription:
 * @channel: A #PkaChannel
 * @subscription: A #PkaSubscription
 *
 * Internal method for unregistering a subscription from a channel.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_channel_remove_subscription (PkaChannel      *channel,      /* IN */
                                 PkaSubscription *subscription) /* IN */
{
	PkaChannelPrivate *priv;

	g_return_if_fail(PKA_IS_CHANNEL(channel));
	g_return_if_fail(subscription != NULL);

	ENTRY;
	priv = channel->priv;
	g_mutex_lock(priv->mutex);
	if (g_ptr_array_remove(priv->subs, subscription)) {
		pka_subscription_unref(subscription);
	}
	g_mutex_unlock(priv->mutex);
	EXIT;
}

/**
 * pka_channel_get_state:
 * @channel: A #PkaChannel.
 *
 * Retrieves the current state of the channel.
 *
 * Returns: The channel state.
 * Side effects: None.
 */
PkaChannelState
pka_channel_get_state (PkaChannel *channel) /* IN */
{
	PkaChannelPrivate *priv;
	PkaChannelState state;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), 0);

	ENTRY;
	priv = channel->priv;
	g_mutex_lock(priv->mutex);
	state = priv->state;
	g_mutex_unlock(priv->mutex);
	RETURN(state);
}

/**
 * pka_channel_finalize:
 * @object: A #PkaChannel.
 *
 * Finalizes a #PkaChannel and frees resources.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_channel_finalize (GObject *object)
{
	PkaChannelPrivate *priv = PKA_CHANNEL(object)->priv;

	ENTRY;
	g_free(priv->target);
	g_free(priv->working_dir);
	g_strfreev(priv->args);
	g_strfreev(priv->env);
	g_ptr_array_foreach(priv->sources, (GFunc)g_object_unref, NULL);
	g_ptr_array_foreach(priv->subs, (GFunc)pka_subscription_unref, NULL);
	g_ptr_array_free(priv->sources, TRUE);
	g_ptr_array_free(priv->subs, TRUE);
	g_mutex_free(priv->mutex);
	G_OBJECT_CLASS(pka_channel_parent_class)->finalize(object);
	EXIT;
}

/**
 * pka_channel_class_init:
 * @klass: A #PkaChannelClass.
 *
 * Initializes the #PkaChannelClass.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_channel_class_init (PkaChannelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pka_channel_finalize;
	g_type_class_add_private(object_class, sizeof(PkaChannelPrivate));
}

/**
 * pka_channel_compare_manifest:
 * @a: A #PkaManifest.
 * @b: A #PkaManifest.
 *
 * Sort compare function to sort manifests.
 *
 * Returns: the sort order.
 * Side effects: None.
 */
static gint
pka_channel_compare_manifest (gconstpointer a, /* IN */
                              gconstpointer b) /* IN */
{
	return (a == b) ? 0 : (a - b);
}

/**
 * pka_channel_init:
 * @channel: A #PkaChannel.
 *
 * Initializes a newly created #PkaChannel instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_channel_init (PkaChannel *channel) /* IN */
{
	ENTRY;
	channel->priv = G_TYPE_INSTANCE_GET_PRIVATE(channel,
	                                            PKA_TYPE_CHANNEL,
	                                            PkaChannelPrivate);
	channel->priv->subs = g_ptr_array_new();
	channel->priv->sources = g_ptr_array_new();
	channel->priv->mutex = g_mutex_new();
	channel->priv->indexed = g_tree_new(pka_channel_compare_manifest);
	channel->priv->manifests = g_tree_new_full(
		(GCompareDataFunc)pka_channel_compare_manifest,
		NULL, NULL,
		(GDestroyNotify)pka_manifest_unref);
	channel->priv->id = g_atomic_int_exchange_and_add((gint *)&channel_seq, 1);
	channel->priv->state = PKA_CHANNEL_READY;
	EXIT;
}

/**
 * pka_channel_error_quark:
 *
 * Retrieves the #PkaChannel error domain #GQuark.
 *
 * Returns: A #GQuark.
 * Side effects: None.
 */
GQuark
pka_channel_error_quark (void)
{
	return g_quark_from_static_string("pka-channel-error-quark");
}
