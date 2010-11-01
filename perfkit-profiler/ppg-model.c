/* ppg-model.c
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

#include <egg-time.h>
#include <gobject/gvaluecollector.h>
#include <math.h>
#include <perfkit/perfkit.h>
#include <string.h>

#include "ppg-log.h"
#include "ppg-model.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Model"

/**
 * SECTION:ppg-model
 *
 * #PpgModel will start out as a specific implementation for storing data
 * and accessing it in an iterator based fashion. As this continues, I
 * really hope to start abstracting it into a few backends (such as memory
 * and mmap'd on disk) for particular scenarios.
 *
 * I want to re-iterate, this is a very crappy first attempt to do this
 * so don't hold me to it while I figure this out.
 *
 * -- Christian
 */

G_DEFINE_TYPE(PpgModel, ppg_model, G_TYPE_INITIALLY_UNOWNED)

typedef struct
{
	gint key;
	gchar *name;
	GType expected_type;
	PpgModelValueFunc func;
	gpointer user_data;
} Mapping;

struct _PpgModelPrivate
{
	guint32     stamp;

	PkManifest *manifest;        /* Current manifest */
	guint32     sample_count;    /* Samples on current manifest */
	GPtrArray  *manifests;       /* All manifests */
	GHashTable *mappings;        /* Key to name mappings */
	GHashTable *next_manifests;  /* Manifest->NextManifest mappings */
	GPtrArray  *samples;         /* All samples */
};

static void
mapping_get_value (Mapping *mapping,
                   PpgModel *model,
                   PpgModelIter *iter,
                   PkManifest *manifest,
                   PkSample *sample,
                   GValue  *value)
{
	GType type;
	gint row;

	g_return_if_fail(mapping != NULL);
	g_return_if_fail(manifest != NULL);
	g_return_if_fail(sample != NULL);
	g_return_if_fail(value != NULL);

	if (mapping->func) {
		mapping->func(model, iter, mapping->key, value, mapping->user_data);
	} else {
		/*
		 * FIXME: Probably should cache these somewhere per manifest.
		 */
		row = pk_manifest_get_row_id(manifest, mapping->name);
		if (mapping->expected_type) {
			type = pk_manifest_get_row_type(manifest, row);
			if (type != mapping->expected_type) {
				g_critical("Incoming type %s does not match %s",
				           g_type_name(type),
				           g_type_name(mapping->expected_type));
				return;
			}
		}
		if (!pk_sample_get_value(sample, row, value)) {
			g_value_init(value, pk_manifest_get_row_type(manifest, row));
		}
	}
}

/**
 * ppg_model_finalize:
 * @object: (in): A #PpgModel.
 *
 * Finalizer for a #PpgModel instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_model_finalize (GObject *object)
{
	PpgModelPrivate *priv = PPG_MODEL(object)->priv;

	g_ptr_array_foreach(priv->manifests, (GFunc)pk_manifest_unref, NULL);
	g_ptr_array_unref(priv->manifests);

	g_ptr_array_foreach(priv->samples, (GFunc)pk_sample_unref, NULL);
	g_ptr_array_unref(priv->samples);

	g_hash_table_unref(priv->mappings);
	g_hash_table_unref(priv->next_manifests);

	G_OBJECT_CLASS(ppg_model_parent_class)->finalize(object);
}

/**
 * ppg_model_class_init:
 * @klass: (in): A #PpgModelClass.
 *
 * Initializes the #PpgModelClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_model_class_init (PpgModelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_model_finalize;
	g_type_class_add_private(object_class, sizeof(PpgModelPrivate));
}

/**
 * ppg_model_init:
 * @model: (in): A #PpgModel.
 *
 * Initializes the newly created #PpgModel instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_model_init (PpgModel *model)
{
	PpgModelPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(model, PPG_TYPE_MODEL,
	                                   PpgModelPrivate);
	model->priv = priv;

	priv->stamp = g_random_int();
	priv->manifests = g_ptr_array_new();
	priv->samples = g_ptr_array_new();
	priv->mappings = g_hash_table_new_full(g_int_hash, g_int_equal,
	                                       NULL, g_free);
	priv->next_manifests = g_hash_table_new(g_direct_hash, g_direct_equal);
}

void
ppg_model_insert_manifest (PpgModel   *model,
                           PkManifest *manifest)
{
	PpgModelPrivate *priv;

	g_return_if_fail(PPG_IS_MODEL(model));
	g_return_if_fail(manifest != NULL);

	priv = model->priv;

	/*
	 * Add the mapping from the current sample to the new sample; but only
	 * if we actually received a sample on this manifest.
	 */
	if (priv->manifest && priv->sample_count) {
		g_hash_table_insert(priv->next_manifests, priv->manifest, manifest);
	}

	/*
	 * FIXME: Use insertion sort to order based on timing.
	 */
	priv->manifest = manifest;
	priv->sample_count = 0;
	g_ptr_array_add(priv->manifests, pk_manifest_ref(manifest));
}

void
ppg_model_insert_sample (PpgModel   *model,
                         PkManifest *manifest,
                         PkSample   *sample)
{
	PpgModelPrivate *priv;

	g_return_if_fail(PPG_IS_MODEL(model));
	g_return_if_fail(manifest != NULL);

	priv = model->priv;

	g_assert(manifest == priv->manifest);

	/*
	 * FIXME: Use insertion sort to order based on timestamp.
	 */
	priv->sample_count++;
	g_ptr_array_add(priv->samples, pk_sample_ref(sample));
}

void
ppg_model_add_mapping (PpgModel    *model,
                       gint         key,
                       const gchar *field,
                       GType        expected_type)
{
	PpgModelPrivate *priv;
	Mapping *mapping;

	g_return_if_fail(PPG_IS_MODEL(model));

	priv = model->priv;

	mapping = g_new0(Mapping, 1);
	mapping->key = key;
	mapping->name = g_strdup(field);
	mapping->expected_type = expected_type;

	g_hash_table_insert(priv->mappings, &mapping->key, mapping);
}

void
ppg_model_add_mapping_func (PpgModel *model,
                            gint key,
                            PpgModelValueFunc func,
                            gpointer user_data)
{
	PpgModelPrivate *priv;
	Mapping *mapping;

	g_return_if_fail(PPG_IS_MODEL(model));

	priv = model->priv;

	mapping = g_new0(Mapping, 1);
	mapping->key = key;
	mapping->func = func;
	mapping->user_data = user_data;

	g_hash_table_insert(priv->mappings, &mapping->key, mapping);
}

void
ppg_model_get_valist (PpgModel *model,
                      PpgModelIter *iter,
                      gint first_key,
                      va_list args)
{
	PpgModelPrivate *priv;
	PkManifest *manifest;
	PkSample *sample;
	Mapping *mapping;
	GValue value = { 0 };
	gchar *error = NULL;
	gint key;

	g_return_if_fail(PPG_IS_MODEL(model));
	g_return_if_fail(iter != NULL);

	priv = model->priv;
	key = first_key;

	do {
		manifest = (PkManifest *)iter->user_data;
		sample = (PkSample *)iter->user_data2;
		mapping = g_hash_table_lookup(priv->mappings, &key);

		if (!mapping) {
			g_critical("Cannot find mapping for key %d. "
			           "Make sure to include a -1 sentinel.",
			           key);
			return;
		}

		g_assert(manifest);
		g_assert(sample);
		g_assert(mapping);

		mapping_get_value(mapping, model, iter, manifest, sample, &value);
		G_VALUE_LCOPY(&value, args, 0, &error);
		if (error) {
			g_warning("%s:%s", G_STRFUNC, error);
			g_free(error);
			error = NULL;
		}
		g_value_unset(&value);
	} while (-1 != (key = va_arg(args, gint)));
}

void
ppg_model_get (PpgModel *model,
               PpgModelIter *iter,
               gint first_key,
               ...)
{
	va_list args;

	va_start(args, first_key);
	ppg_model_get_valist(model, iter, first_key, args);
	va_end(args);
}

void
ppg_model_get_value (PpgModel *model,
                     PpgModelIter *iter,
                     gint key,
                     GValue *value)
{
	PpgModelPrivate *priv;
	PkManifest *manifest;
	PkSample *sample;
	Mapping *mapping;

	g_return_if_fail(PPG_IS_MODEL(model));
	g_return_if_fail(iter != NULL);
	g_return_if_fail(value != NULL);

	priv = model->priv;

	g_assert(iter->stamp == priv->stamp);

	manifest = (PkManifest *)iter->user_data;
	sample = (PkSample *)iter->user_data2;
	mapping = g_hash_table_lookup(priv->mappings, &key);

	g_assert(manifest);
	g_assert(sample);
	g_assert(mapping);

	mapping_get_value(mapping, model, iter, manifest, sample, value);
}

static inline gdouble
ppg_model_make_relative (PpgModel *model,
                         PkSample *sample)
{
	PpgModelPrivate *priv = model->priv;
	PkManifest *manifest;
	struct timespec ts_manifest;
	struct timespec ts_sample;
	struct timespec z;

	manifest = g_ptr_array_index(priv->manifests, 0);
	pk_manifest_get_timespec(manifest, &ts_manifest);
	pk_sample_get_timespec(sample, &ts_sample);
	timespec_subtract(&ts_sample, &ts_manifest, &z);
	return z.tv_sec + (z.tv_nsec / 1000000000.0);
}

gboolean
ppg_model_iter_next (PpgModel     *model,
                     PpgModelIter *iter)
{
	PpgModelPrivate *priv;
	PkManifest *manifest;
	PkManifest *next_manifest;
	PkSample *sample;
	struct timespec ts_next;
	struct timespec ts_sample;
	gsize idx;

	g_return_val_if_fail(PPG_IS_MODEL(model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	priv = model->priv;

	g_assert(iter->stamp == priv->stamp);

	manifest = iter->user_data;

	/*
	 * Increment to next position in sample array.
	 */
	idx = GPOINTER_TO_INT(iter->user_data3) + 1;
	if (G_UNLIKELY(idx >= priv->samples->len)) {
		return FALSE;
	}

	iter->user_data3 = GINT_TO_POINTER(idx);
	sample = g_ptr_array_index(priv->samples, idx);
	iter->user_data2 = sample;
	iter->time = ppg_model_make_relative(model, sample);

	/*
	 * The sample is guaranteed to be within either the current manifest
	 * or the following manifest since we discard manifests that contained
	 * no samples.
	 */
	if ((next_manifest = g_hash_table_lookup(priv->next_manifests, manifest))) {
		pk_manifest_get_timespec(next_manifest, &ts_next);
		pk_sample_get_timespec(sample, &ts_sample);
		if (timespec_compare(&ts_sample, &ts_next) >= 0) {
			iter->user_data = next_manifest;
		}
	}

	return TRUE;
}

gboolean
ppg_model_get_iter_first (PpgModel     *model,
                          PpgModelIter *iter)
{
	PpgModelPrivate *priv;

	g_return_val_if_fail(PPG_IS_MODEL(model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	priv = model->priv;

	if (priv->manifests->len) {
		if (priv->samples->len) {
			memset(iter, 0, sizeof *iter);
			iter->stamp = priv->stamp;
			iter->user_data = g_ptr_array_index(priv->manifests, 0);
			iter->user_data2 = g_ptr_array_index(priv->samples, 0);
			iter->user_data3 = 0;
			iter->time = ppg_model_make_relative(model, iter->user_data2);
			return TRUE;
		}
	}

	return FALSE;
}

gboolean
ppg_model_get_iter_at (PpgModel      *model,
                       PpgModelIter  *iter,
                       gdouble        begin,
                       gdouble        end,
                       PpgResolution  resolution) /* UNUSED */
{
	g_return_val_if_fail(PPG_IS_MODEL(model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);
	g_return_val_if_fail(begin >= 0.0, FALSE);
	g_return_val_if_fail(end >= 0.0, FALSE);

	if ((begin == 0.0) && (end == 0.0)) {
		return ppg_model_get_iter_first(model, iter);
	}

	/*
	 * TODO: Binary search for begin time.
	 */

	return FALSE;
}
