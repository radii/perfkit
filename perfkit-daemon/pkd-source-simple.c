/* pkd-source-simple.h
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

#ifndef __linux__
#error "Perfkit has not yet been ported to your platform."
#endif

#include <egg-time.h>
#include <pthread.h>

#include "pkd-source-simple.h"

/**
 * SECTION:pkd-source-simple
 * @title: PkdSourceSimple
 * @short_description: 
 *
 * Section overview.
 */

G_DEFINE_TYPE(PkdSourceSimple, pkd_source_simple, PKD_TYPE_SOURCE)

struct _PkdSourceSimplePrivate
{
	pthread_mutex_t       mutex;
	pthread_cond_t        cond;
	struct timespec       freq;
	struct timespec       timeout;
	gboolean              dedicated;
	gboolean              running;
	GThread              *thread;
	GClosure             *closure;
	PkdSourceSimpleSpawn  spawn_callback;
	gpointer              user_data;
};

enum
{
	PROP_0,
	PROP_USE_THREAD,
	PROP_FREQ,
};

enum
{
	CLEANUP,
	LAST_SIGNAL
};

static pthread_cond_t   cond;
static pthread_mutex_t  mutex;
static gboolean         running = FALSE;
static GPtrArray       *sources = NULL;
static GThread         *thread  = NULL;
static guint            signals[LAST_SIGNAL] = {0};

/**
 * pkd_source_simple_new:
 *
 * Creates a new instance of #PkdSourceSimple.
 *
 * Returns: the newly created instance of #PkdSourceSimple.
 * Side effects: None.
 */
PkdSource*
pkd_source_simple_new (void)
{
	return g_object_new(PKD_TYPE_SOURCE_SIMPLE, NULL);
}

PkdSource*
pkd_source_simple_new_full (PkdSourceSimpleFunc  callback,
                            gpointer             user_data,
                            PkdSourceSimpleSpawn spawn_callback,
                            gboolean             use_thread,
                            GDestroyNotify       notify)
{
	PkdSource *source;

	source = g_object_new(PKD_TYPE_SOURCE_SIMPLE,
	                      "use-thread", use_thread,
	                      NULL);
	pkd_source_simple_set_callback(PKD_SOURCE_SIMPLE(source),
	                               callback, user_data, notify);
	PKD_SOURCE_SIMPLE(source)->priv->spawn_callback = spawn_callback;
	PKD_SOURCE_SIMPLE(source)->priv->user_data = user_data;

	return source;
}

/**
 * pkd_source_simple_set_callback_closure:
 * @source: A #PkdSourceSimple.
 *
 * Sets the closure to be invoked when the sampling timeout has occurred.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pkd_source_simple_set_callback_closure (PkdSourceSimple *source,
                                        GClosure        *closure)
{
	g_return_if_fail(PKD_IS_SOURCE_SIMPLE(source));
	g_return_if_fail(source->priv->closure == NULL);

	source->priv->closure = g_closure_ref(closure);
	g_closure_sink(closure);
}

/**
 * pkd_source_simple_set_callback:
 * @source: A #PkdSourceSimple.
 * @callback: The callback to execute.
 * @user_data: user data for @callback.
 * @notify: A callback to be executed upon destroy or %NULL.
 *
 * Sets the callback to be invoked when the sampling timeout has occurred.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pkd_source_simple_set_callback (PkdSourceSimple     *source,
                                PkdSourceSimpleFunc  callback,
                                gpointer             user_data,
                                GDestroyNotify       notify)
{
	GClosure *closure;

	g_return_if_fail(PKD_IS_SOURCE_SIMPLE(source));
	g_return_if_fail(callback != NULL);
	g_return_if_fail(source->priv->closure == NULL);

	closure = g_cclosure_new(G_CALLBACK(callback), user_data,
	                         (GClosureNotify)notify);
	g_closure_set_marshal(closure, g_cclosure_marshal_VOID__VOID);
	pkd_source_simple_set_callback_closure(source, closure);
}

/**
 * pkd_source_simple_set_use_thread:
 * @source: A #PkdSourceSimple.
 * @use_thread: If a dedicated thread should be used.
 *
 * Sets if the source should use its own dedicated thread for sampling.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pkd_source_simple_set_use_thread (PkdSourceSimple *source,
                                  gboolean         use_thread)
{
	g_return_if_fail(PKD_IS_SOURCE_SIMPLE(source));
	g_return_if_fail(source->priv->thread == NULL);
	g_return_if_fail(source->priv->running == FALSE);

	source->priv->dedicated = use_thread;
}

/**
 * pkd_source_simple_get_use_thread:
 * @source: A #PkdSourceSimple.
 *
 * Retrieves if the source should use a dedicated thread.
 *
 * Returns: %TRUE if a dedicated thread is to be used; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pkd_source_simple_get_use_thread (PkdSourceSimple *source)
{
	g_return_val_if_fail(PKD_IS_SOURCE_SIMPLE(source), FALSE);
	return source->priv->dedicated;
}

/**
 * pkd_source_simple_update:
 * @source: A #PkdSourceSimple.
 *
 * Updates the next timeout to now + frequency.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
pkd_source_simple_update (PkdSourceSimple *source)
{
	PkdSourceSimplePrivate *priv = source->priv;
	struct timespec ts, *freq = &priv->freq;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts.tv_nsec += freq->tv_nsec;
	ts.tv_sec += ts.tv_nsec / G_NSEC_PER_SEC;
	ts.tv_nsec %= G_NSEC_PER_SEC;
	ts.tv_sec += freq->tv_sec;
	priv->timeout = ts;
}

static inline void
pkd_source_simple_invoke (PkdSourceSimple *source)
{
	PkdSourceSimplePrivate *priv = source->priv;
	GValue params = {0};

	/*
	 * Update when our next timeout should be. This is done before invoking
	 * the closure to reduce drift.
	 */
	pkd_source_simple_update(source);

	/*
	 * Invoke the routine closure.
	 */
	g_value_init(&params, PKD_TYPE_SOURCE_SIMPLE);
	g_value_set_object(&params, source);
	g_closure_invoke(priv->closure, NULL, 1, &params, NULL);
	g_value_unset(&params);
}

/**
 * pkd_source_simple_wait:
 * @source: A #PkdSource.
 *
 * This method will block until the sources timeout has occurred or the
 * condition has been signaled by another thread.
 *
 * Returns: %TRUE if the source is not finished.
 * Side effects: None.
 */
static inline gboolean
pkd_source_simple_wait (PkdSourceSimple *source,
                        pthread_cond_t  *wait_cond,
                        pthread_mutex_t *wait_mutex)
{
	PkdSourceSimplePrivate *priv = source->priv;

	pthread_cond_timedwait(wait_cond, wait_mutex, &priv->timeout);
	return (wait_cond == &priv->cond) ? priv->running : running;
}

static inline gint
timespec_compare (const struct timespec *ts1,
                  const struct timespec *ts2)
{
	if ((ts1->tv_sec == ts2->tv_sec) && (ts1->tv_nsec == ts2->tv_nsec))
		return 0;
	else if ((ts1->tv_sec > ts2->tv_sec) ||
	         ((ts1->tv_sec == ts2->tv_sec) &&
	          (ts1->tv_nsec > ts2->tv_nsec)))
		return 1;
	else
		return -1;
}

static gint
pkd_source_simple_compare_func (gconstpointer a,
                                gconstpointer b)
{
	PkdSourceSimplePrivate *priva = PKD_SOURCE_SIMPLE(a)->priv;
	PkdSourceSimplePrivate *privb = PKD_SOURCE_SIMPLE(b)->priv;

	return timespec_compare(&priva->timeout, &privb->timeout);
}

static inline gboolean
pkd_source_simple_is_ready (PkdSourceSimple *source)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (timespec_compare(&ts, &source->priv->timeout) >= 0);
}

static gpointer
pkd_source_simple_shared_worker (gpointer user_data)
{
	PkdSourceSimple *source;
	gboolean dirty;
	gint i;

	pthread_mutex_lock(&mutex);

next:
	if (!sources->len) {
		pthread_cond_wait(&cond, &mutex);
		goto next;
	} else {
		source = g_ptr_array_index(sources, 0);
		if (!pkd_source_simple_wait(source, &cond, &mutex)) {
			goto unlock_and_finish;
		}
	}

	dirty = FALSE;

	for (i = 0; i < sources->len; i++) {
		source = g_ptr_array_index(sources, i);
		if (pkd_source_simple_is_ready(source)) {
			pkd_source_simple_invoke(source);
			dirty = TRUE;
		} else break;
	}

	if (dirty) {
		g_ptr_array_sort(sources, pkd_source_simple_compare_func);
	}

	goto next;

unlock_and_finish:
	pthread_mutex_unlock(&mutex);

	return NULL;
}

static gpointer
pkd_source_simple_worker (gpointer user_data)
{
	PkdSourceSimple *source = user_data;
	PkdSourceSimplePrivate *priv = source->priv;

	pthread_mutex_lock(&priv->mutex);
	while (pkd_source_simple_wait(source, &priv->cond, &priv->mutex)) {
		pkd_source_simple_invoke(source);
	}
	pthread_mutex_unlock(&priv->mutex);

	return NULL;
}

static inline void
pkd_source_simple_init_pthreads (pthread_mutex_t *init_mutex,
                                 pthread_cond_t  *init_cond)
{
	pthread_condattr_t attr;

	/*
	 * Initialize threading synchronization.
	 */
	pthread_mutex_init(init_mutex, NULL);
	pthread_condattr_init(&attr);
	if (pthread_condattr_setclock(&attr, CLOCK_MONOTONIC) != 0) {
		g_error("Failed to initialize monotonic clock!");
	}
	pthread_cond_init(init_cond, &attr);
}

static void
pkd_source_simple_notify_started (PkdSource    *source,
                                  PkdSpawnInfo *spawn_info)
{
	PkdSourceSimplePrivate *priv = PKD_SOURCE_SIMPLE(source)->priv;
	GError *error = NULL;

	if (priv->spawn_callback) {
		priv->spawn_callback(PKD_SOURCE_SIMPLE(source), spawn_info, priv->user_data);
	}

	if (priv->dedicated) {
		g_assert(!priv->thread && !priv->running);
		priv->running = TRUE;
		priv->thread = g_thread_create(pkd_source_simple_worker,
		                               source, FALSE, &error);
		if (!priv->thread) {
			g_critical("Error creating sampling thread: %s."
			           "Attempting best effort with shared worker.",
			           error->message);
			g_error_free(error);
			goto attach_shared;
		}
		return;
	}

attach_shared:
	priv->dedicated = FALSE;
	pthread_mutex_lock(&mutex);
	g_ptr_array_add(sources, source);
	g_ptr_array_sort(sources, pkd_source_simple_compare_func);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

static void
pkd_source_simple_notify_stopped (PkdSource *source)
{
	PkdSourceSimplePrivate *priv = PKD_SOURCE_SIMPLE(source)->priv;

	if (priv->dedicated) {
		pthread_mutex_lock(&priv->mutex);
		priv->running = FALSE;
		g_signal_emit(source, signals[CLEANUP], 0);
		pthread_cond_signal(&priv->cond);
		pthread_mutex_unlock(&priv->mutex);
		return;
	}

	pthread_mutex_lock(&mutex);
	g_ptr_array_remove(sources, source);
	priv->running = FALSE;
	g_signal_emit(source, signals[CLEANUP], 0);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

/**
 * pkd_source_simple_set_frequency:
 * @source: A #PkdSourceSimple.
 *
 * Sets the frequency for which @source should sample.  @frequency should
 * contain the number of seconds and microseconds between each sampling
 * occurrance.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pkd_source_simple_set_frequency (PkdSourceSimple *source,
                                 const GTimeVal  *frequency)
{
	g_return_if_fail(PKD_IS_SOURCE_SIMPLE(source));
	source->priv->freq.tv_sec = frequency->tv_sec;
	source->priv->freq.tv_nsec = frequency->tv_usec * 1000;
}

static void
pkd_source_simple_finalize (GObject *object)
{
	PkdSourceSimplePrivate *priv = PKD_SOURCE_SIMPLE(object)->priv;

	pthread_cond_destroy(&priv->cond);
	pthread_mutex_destroy(&priv->mutex);

	if (priv->closure) {
		g_closure_unref(priv->closure);
	}

	G_OBJECT_CLASS(pkd_source_simple_parent_class)->finalize(object);
}

static void
pkd_source_simple_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_FREQ:
		g_value_set_pointer(value, &PKD_SOURCE_SIMPLE(object)->priv->freq);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pkd_source_simple_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
	switch (prop_id) {
	case PROP_USE_THREAD:
		pkd_source_simple_set_use_thread(PKD_SOURCE_SIMPLE(object),
		                                 g_value_get_boolean(value));
		break;
	case PROP_FREQ:
		pkd_source_simple_set_frequency(PKD_SOURCE_SIMPLE(object),
		                                g_value_get_pointer(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pkd_source_simple_class_init (PkdSourceSimpleClass *klass)
{
	GObjectClass *object_class;
	PkdSourceClass *source_class;
	GError *error = NULL;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pkd_source_simple_finalize;
	object_class->set_property = pkd_source_simple_set_property;
	object_class->get_property = pkd_source_simple_get_property;
	g_type_class_add_private(object_class, sizeof(PkdSourceSimplePrivate));

	source_class = PKD_SOURCE_CLASS(klass);
	source_class->notify_started = pkd_source_simple_notify_started;
	source_class->notify_stopped = pkd_source_simple_notify_stopped;

	/**
	 * PkdSourceSimple:use-thread
	 *
	 * The "use-thread" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_USE_THREAD,
	                                g_param_spec_boolean("use-thread",
	                                                     "UseThread",
	                                                     "Denotes a dedicated thread be used",
	                                                     FALSE,
	                                                     G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * PkdSourceSimple:frequency
	 *
	 * The "frequency" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_FREQ,
	                                g_param_spec_pointer("frequency",
	                                                     "Frequency",
	                                                     "The sampling frequency",
	                                                     G_PARAM_READWRITE));

	/**
	 * PkdSourceSimple:cleanup:
	 *
	 * The "cleanup" signal. This signal is emitted after the source has
	 * been stopped.
	 */
	signals[CLEANUP] = g_signal_new("cleanup",
	                                PKD_TYPE_SOURCE_SIMPLE,
	                                G_SIGNAL_RUN_FIRST,
	                                0,
	                                NULL,
	                                NULL,
	                                g_cclosure_marshal_VOID__VOID,
	                                G_TYPE_NONE,
	                                0);

	/*
	 * Initialize shared worker pthread synchronization.
	 */
	pkd_source_simple_init_pthreads(&mutex, &cond);

	/*
	 * Initialize global data fields.
	 */
	sources = g_ptr_array_new();
	running = TRUE;

	/*
	 * Spawn the worker thread.
	 */
	thread = g_thread_create(pkd_source_simple_shared_worker,
	                         NULL, FALSE, &error);
	if (!thread) {
		g_error("Failed to initialize monitor thread: %s", error->message);
		g_error_free(error);
	}
}

static void
pkd_source_simple_init (PkdSourceSimple *source)
{
	source->priv = G_TYPE_INSTANCE_GET_PRIVATE(source,
	                                           PKD_TYPE_SOURCE_SIMPLE,
	                                           PkdSourceSimplePrivate);
	source->priv->freq.tv_sec = 1;

	/*
	 * Initialize dedicated worker threading synchronization.
	 */
	pkd_source_simple_init_pthreads(&source->priv->mutex,
	                                &source->priv->cond);
}
