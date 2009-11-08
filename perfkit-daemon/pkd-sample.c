/* pkd-sample.c
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

#include "pkd-sample.h"

/**
 * SECTION:pkd-sample
 * @title: PkdSample
 * @short_description: Source samples
 *
 * #PkdSample is an abstraction for data samples which are recorded from a
 * #PkdSource.
 */

struct _PkdSample
{
	volatile gint ref_count;
};

static void
pkd_sample_destroy (PkdSample *sample)
{
}

static void
pkd_sample_free (PkdSample *sample)
{
	g_slice_free (PkdSample, sample);
}

/**
 * pkd_sample_new:
 *
 * Creates a new instance of #PkdSample.
 *
 * Return value: the newly created #PkdSample instance.
 */
PkdSample*
pkd_sample_new (void)
{
	PkdSample *sample;

	sample = g_slice_new0 (PkdSample);
	sample->ref_count = 1;

	return sample;
}

/**
 * pkd_sample_ref:
 * @sample: A #PkdSample
 *
 * Atomically increments the reference count of @sample by one.
 *
 * Return value: the #PkdSample reference.
 */
PkdSample*
pkd_sample_ref (PkdSample *sample)
{
	g_return_val_if_fail (sample != NULL, NULL);
	g_return_val_if_fail (sample->ref_count > 0, NULL);

	g_atomic_int_inc (&sample->ref_count);
	return sample;
}

/**
 * pkd_sample_unref:
 * @sample: A #PkdSample
 *
 * Atomically decrements the reference count of @sample by one.
 * When the reference count reaches zero, the structure is destroyed and freed.
 */
void
pkd_sample_unref (PkdSample *sample)
{
	g_return_if_fail (sample != NULL);
	g_return_if_fail (sample->ref_count > 0);

	if (g_atomic_int_dec_and_test (&sample->ref_count)) {
		pkd_sample_destroy (sample);
		pkd_sample_free (sample);
	}
}

GType
pkd_sample_get_type (void)
{
	static GType gtype = 0;

	if (g_once_init_enter ((gsize*)&gtype)) {
		GType init_gtype;

		init_gtype = g_boxed_type_register_static ("PkdSample",
		                                           (GBoxedCopyFunc)pkd_sample_ref,
		                                           (GBoxedFreeFunc)pkd_sample_unref);
		g_once_init_leave ((gsize*)&gtype, (gsize)init_gtype);
	}

	return gtype;
}
