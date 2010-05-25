/* pka-source-simple.h
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

#include "pka-source-simple.h"

/**
 * SECTION:pka-source-simple
 * @title: PkaSourceSimple
 * @short_description: 
 *
 * Section overview.
 */

G_DEFINE_TYPE(PkaSourceSimple, pka_source_simple, PKA_TYPE_SOURCE)

struct _PkaSourceSimplePrivate
{
	pthread_mutex_t       mutex;
	pthread_cond_t        cond;
	struct timespec       freq;
	struct timespec       timeout;
	gboolean              dedicated;
	gboolean              running;
	GThread              *thread;
	GClosure             *sample;
	GClosure             *spawn;
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
 * pka_source_simple_new:
 *
 * Creates a new instance of #PkaSourceSimple.
 *
 * Returns: the newly created instance of #PkaSourceSimple.
 * Side effects: None.
 */
PkaSource*
pka_source_simple_new (void)
{
	return g_object_new(PKA_TYPE_SOURCE_SIMPLE, NULL);
}

/**
 * pka_source_simple_new_full:
 * @callback: A #PkaSourceSimpleFunc.
 * @spawn_callback: A callback up on channel spawn.
 *
 * Creates a new #PkaSource instance using the callbacks provided.  This source
 * can run in either of two modes depending on the requirements.  If the
 * "use-thread" property is set to %TRUE, then a thread will be spawned and
 * dedicated to calling the callback on regular schedule with as little latency
 * as possible.  If the "use-thread" property is %FALSE, the default, then
 * a shared thread with other sources will be used in a co-routine fashion.
 *
 * Returns: The newly created #PkaSource instance.  This should be freed with
 *   g_object_unref().
 * Side effects: None.
 */
PkaSource*
pka_source_simple_new_full (PkaSourceSimpleFunc  callback,
                            PkaSourceSimpleSpawn spawn_callback,
                            gpointer             user_data,
                            GDestroyNotify       notify)
{
	PkaSource *source;

	source = g_object_new(PKA_TYPE_SOURCE_SIMPLE, NULL);
	pka_source_simple_set_sample_callback(PKA_SOURCE_SIMPLE(source),
	                                      callback, user_data, notify);
	pka_source_simple_set_spawn_callback(PKA_SOURCE_SIMPLE(source),
	                                     spawn_callback, user_data, NULL);

	return source;
}

/**
 * pka_source_simple_set_callback_closure:
 * @source: A #PkaSourceSimple.
 *
 * Sets the closure to be invoked when the sampling timeout has occurred.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_simple_set_sample_closure (PkaSourceSimple *source,
                                      GClosure        *closure)
{
	g_return_if_fail(PKA_IS_SOURCE_SIMPLE(source));
	g_return_if_fail(source->priv->sample == NULL);

	source->priv->sample = g_closure_ref(closure);
	g_closure_sink(closure);
}

/**
 * pka_source_simple_set_sample_callback:
 * @source: A #PkaSourceSimple.
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
pka_source_simple_set_sample_callback (PkaSourceSimple     *source,
                                       PkaSourceSimpleFunc  callback,
                                       gpointer             user_data,
                                       GDestroyNotify       notify)
{
	GClosure *closure;

	g_return_if_fail(PKA_IS_SOURCE_SIMPLE(source));
	g_return_if_fail(callback != NULL);
	g_return_if_fail(source->priv->sample == NULL);

	closure = g_cclosure_new(G_CALLBACK(callback), user_data,
	                         (GClosureNotify)notify);
	g_closure_set_marshal(closure, g_cclosure_marshal_VOID__VOID);
	pka_source_simple_set_sample_closure(source, closure);
}

void
pka_source_simple_set_spawn_closure (PkaSourceSimple *source,
                                     GClosure        *closure)
{
	g_return_if_fail(PKA_IS_SOURCE_SIMPLE(source));
	g_return_if_fail(source->priv->spawn == NULL);

	source->priv->spawn = g_closure_ref(closure);
	g_closure_sink(closure);
}

void
pka_source_simple_set_spawn_callback (PkaSourceSimple      *source,
                                      PkaSourceSimpleSpawn  spawn,
                                      gpointer              user_data,
                                      GDestroyNotify        notify)
{
	GClosure *closure;

	g_return_if_fail(PKA_IS_SOURCE_SIMPLE(source));
	g_return_if_fail(spawn != NULL);
	g_return_if_fail(source->priv->spawn == NULL);

	closure = g_cclosure_new(G_CALLBACK(spawn), user_data,
	                         (GClosureNotify)notify);
	g_closure_set_marshal(closure, g_cclosure_marshal_VOID__POINTER);
	pka_source_simple_set_spawn_closure(source, closure);
}

/**
 * pka_source_simple_set_use_thread:
 * @source: A #PkaSourceSimple.
 * @use_thread: If a dedicated thread should be used.
 *
 * Sets if the source should use its own dedicated thread for sampling.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_simple_set_use_thread (PkaSourceSimple *source,
                                  gboolean         use_thread)
{
	g_return_if_fail(PKA_IS_SOURCE_SIMPLE(source));
	g_return_if_fail(source->priv->thread == NULL);
	g_return_if_fail(source->priv->running == FALSE);

	source->priv->dedicated = use_thread;
}

/**
 * pka_source_simple_get_use_thread:
 * @source: A #PkaSourceSimple.
 *
 * Retrieves if the source should use a dedicated thread.
 *
 * Returns: %TRUE if a dedicated thread is to be used; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_source_simple_get_use_thread (PkaSourceSimple *source)
{
	g_return_val_if_fail(PKA_IS_SOURCE_SIMPLE(source), FALSE);
	return source->priv->dedicated;
}

/**
 * pka_source_simple_update:
 * @source: A #PkaSourceSimple.
 *
 * Updates the next timeout to now + frequency.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
pka_source_simple_update (PkaSourceSimple *source)
{
	PkaSourceSimplePrivate *priv = source->priv;
	struct timespec ts, *freq = &priv->freq;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts.tv_nsec += freq->tv_nsec;
	ts.tv_sec += ts.tv_nsec / G_NSEC_PER_SEC;
	ts.tv_nsec %= G_NSEC_PER_SEC;
	ts.tv_sec += freq->tv_sec;
	priv->timeout = ts;
}

static inline void
pka_source_simple_invoke (PkaSourceSimple *source)
{
	PkaSourceSimplePrivate *priv = source->priv;
	GValue params = {0};

	/*
	 * Update when our next timeout should be. This is done before invoking
	 * the closure to reduce drift.
	 */
	pka_source_simple_update(source);

	/*
	 * Invoke the routine closure.
	 */
	g_value_init(&params, PKA_TYPE_SOURCE_SIMPLE);
	g_value_set_object(&params, source);
	g_closure_invoke(priv->sample, NULL, 1, &params, NULL);
	g_value_unset(&params);
}

/**
 * pka_source_simple_wait:
 * @source: A #PkaSource.
 *
 * This method will block until the sources timeout has occurred or the
 * condition has been signaled by another thread.
 *
 * Returns: %TRUE if the source is not finished.
 * Side effects: None.
 */
static inline gboolean
pka_source_simple_wait (PkaSourceSimple *source,
                        pthread_cond_t  *wait_cond,
                        pthread_mutex_t *wait_mutex)
{
	PkaSourceSimplePrivate *priv = source->priv;

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
pka_source_simple_compare_func (gconstpointer a,
                                gconstpointer b)
{
	PkaSourceSimple **sa = (PkaSourceSimple **)a;
	PkaSourceSimple **sb = (PkaSourceSimple **)b;
	PkaSourceSimplePrivate *pa = (*sa)->priv;
	PkaSourceSimplePrivate *pb = (*sb)->priv;

	return timespec_compare(&pa->timeout, &pb->timeout);
}

static inline gboolean
pka_source_simple_is_ready (PkaSourceSimple *source)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (timespec_compare(&ts, &source->priv->timeout) >= 0);
}

static gpointer
pka_source_simple_shared_worker (gpointer user_data)
{
	PkaSourceSimple *source;
	gboolean dirty;
	gint i;

	pthread_mutex_lock(&mutex);

next:
	if (!sources->len) {
		pthread_cond_wait(&cond, &mutex);
		goto next;
	} else {
		source = g_ptr_array_index(sources, 0);
		if (!pka_source_simple_wait(source, &cond, &mutex)) {
			goto unlock_and_finish;
		}
	}

	dirty = FALSE;

	for (i = 0; i < sources->len; i++) {
		source = g_ptr_array_index(sources, i);
		if (pka_source_simple_is_ready(source)) {
			pka_source_simple_invoke(source);
			dirty = TRUE;
		} else break;
	}

	if (dirty) {
		g_ptr_array_sort(sources, pka_source_simple_compare_func);
	}

	goto next;

unlock_and_finish:
	pthread_mutex_unlock(&mutex);

	return NULL;
}

static gpointer
pka_source_simple_worker (gpointer user_data)
{
	PkaSourceSimple *source = user_data;
	PkaSourceSimplePrivate *priv = source->priv;

	pthread_mutex_lock(&priv->mutex);
	while (pka_source_simple_wait(source, &priv->cond, &priv->mutex)) {
		pka_source_simple_invoke(source);
	}
	pthread_mutex_unlock(&priv->mutex);

	return NULL;
}

static inline void
pka_source_simple_init_pthreads (pthread_mutex_t *init_mutex,
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
pka_source_simple_notify_started (PkaSource    *source,
                                  PkaSpawnInfo *spawn_info)
{
	PkaSourceSimplePrivate *priv = PKA_SOURCE_SIMPLE(source)->priv;
	GError *error = NULL;

	if (priv->spawn) {
		GValue params[2] = {{0}};

		g_value_init(&params[0], PKA_TYPE_SOURCE);
		g_value_init(&params[1], G_TYPE_POINTER);
		g_value_set_object(&params[0], source);
		g_value_set_pointer(&params[1], spawn_info);
		g_closure_invoke(priv->spawn, NULL, 2, &params[0], NULL);
		g_value_unset(&params[0]);
		g_value_unset(&params[1]);
	}

	if (priv->dedicated) {
		g_assert(!priv->thread && !priv->running);
		priv->running = TRUE;
		priv->thread = g_thread_create(pka_source_simple_worker,
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
	g_ptr_array_sort(sources, pka_source_simple_compare_func);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

static void
pka_source_simple_notify_stopped (PkaSource *source)
{
	PkaSourceSimplePrivate *priv = PKA_SOURCE_SIMPLE(source)->priv;

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
 * pka_source_simple_set_frequency:
 * @source: A #PkaSourceSimple.
 *
 * Sets the frequency for which @source should sample.  @frequency should
 * contain the number of seconds and microseconds between each sampling
 * occurrance.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_source_simple_set_frequency (PkaSourceSimple *source,
                                 const GTimeVal  *frequency)
{
	g_return_if_fail(PKA_IS_SOURCE_SIMPLE(source));
	source->priv->freq.tv_sec = frequency->tv_sec;
	source->priv->freq.tv_nsec = frequency->tv_usec * 1000;
}

static void
pka_source_simple_finalize (GObject *object)
{
	PkaSourceSimplePrivate *priv = PKA_SOURCE_SIMPLE(object)->priv;

	pthread_cond_destroy(&priv->cond);
	pthread_mutex_destroy(&priv->mutex);

	if (priv->sample) {
		g_closure_unref(priv->sample);
	}

	G_OBJECT_CLASS(pka_source_simple_parent_class)->finalize(object);
}

static void
pka_source_simple_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_FREQ:
		g_value_set_pointer(value, &PKA_SOURCE_SIMPLE(object)->priv->freq);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pka_source_simple_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
	switch (prop_id) {
	case PROP_USE_THREAD:
		pka_source_simple_set_use_thread(PKA_SOURCE_SIMPLE(object),
		                                 g_value_get_boolean(value));
		break;
	case PROP_FREQ:
		pka_source_simple_set_frequency(PKA_SOURCE_SIMPLE(object),
		                                g_value_get_pointer(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pka_source_simple_class_init (PkaSourceSimpleClass *klass)
{
	GObjectClass *object_class;
	PkaSourceClass *source_class;
	GError *error = NULL;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pka_source_simple_finalize;
	object_class->set_property = pka_source_simple_set_property;
	object_class->get_property = pka_source_simple_get_property;
	g_type_class_add_private(object_class, sizeof(PkaSourceSimplePrivate));

	source_class = PKA_SOURCE_CLASS(klass);
	source_class->notify_started = pka_source_simple_notify_started;
	source_class->notify_stopped = pka_source_simple_notify_stopped;

	/**
	 * PkaSourceSimple:use-thread
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
	 * PkaSourceSimple:frequency
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
	 * PkaSourceSimple:cleanup:
	 *
	 * The "cleanup" signal. This signal is emitted after the source has
	 * been stopped.
	 */
	signals[CLEANUP] = g_signal_new("cleanup",
	                                PKA_TYPE_SOURCE_SIMPLE,
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
	pka_source_simple_init_pthreads(&mutex, &cond);

	/*
	 * Initialize global data fields.
	 */
	sources = g_ptr_array_new();
	running = TRUE;

	/*
	 * Spawn the worker thread.
	 */
	thread = g_thread_create(pka_source_simple_shared_worker,
	                         NULL, FALSE, &error);
	if (!thread) {
		g_error("Failed to initialize monitor thread: %s", error->message);
		g_error_free(error);
	}
}

static void
pka_source_simple_init (PkaSourceSimple *source)
{
	source->priv = G_TYPE_INSTANCE_GET_PRIVATE(source,
	                                           PKA_TYPE_SOURCE_SIMPLE,
	                                           PkaSourceSimplePrivate);
	source->priv->freq.tv_sec = 1;

	/*
	 * Initialize dedicated worker threading synchronization.
	 */
	pka_source_simple_init_pthreads(&source->priv->mutex,
	                                &source->priv->cond);
}
