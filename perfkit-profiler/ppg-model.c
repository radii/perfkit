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

#include <math.h>
#include <string.h>

#include "ppg-model.h"

G_DEFINE_TYPE(PpgModel, ppg_model, G_TYPE_INITIALLY_UNOWNED)

struct _PpgModelPrivate
{
	gint n_rows;
};

gboolean
ppg_model_get_iter_at (PpgModel      *model,
                       PpgIter       *iter,
                       gdouble        begin,
                       gdouble        end,
                       PpgResolution  resolution)
{
	gboolean ret = FALSE;

	g_return_val_if_fail(PPG_IS_MODEL(model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);
	g_return_val_if_fail(begin >= 0.0, FALSE);
	g_return_val_if_fail(resolution == PPG_RESOLUTION_FULL, FALSE);

	memset(iter, 0, sizeof(*iter));

	iter->model = model;
	iter->begin = begin;
	iter->end = end;
	iter->pos = 0.0;

	/*
	 * FIXME: Determine the range for the chunk.
	 */
	iter->lower = 0.0f;
	iter->upper = 20.0f;

	ret = TRUE;

	return ret;
}

gboolean
ppg_model_iter_next (PpgModel *model,
                     PpgIter  *iter)
{
	g_return_val_if_fail(PPG_IS_MODEL(model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	/*
	 * FIXME: Go to next chunk in data set.
	 */

	iter->pos += 5.0;

	return (iter->pos < iter->end);
}

void
ppg_model_get (PpgModel *model,
               PpgIter  *iter,
               gint      row,
               gdouble  *offset,
               gdouble  *value)
{
	PpgModelPrivate *priv;

	g_return_if_fail(PPG_IS_MODEL(model));
	g_return_if_fail(iter != NULL);
	g_return_if_fail(row >= 0);
	g_return_if_fail(row < model->priv->n_rows);
	g_return_if_fail(value != NULL);

	priv = model->priv;

	*value = 0.0;
	*offset = 0.0f;

	/*
	 * FIXME: Get data from data set.
	 */

	*offset = iter->pos;
#if 0
	*value = g_random_double_range(iter->lower, iter->upper);
#else
	{
		gdouble half = (iter->upper - iter->lower) / 2.0;
		*value = cos(iter->pos / 2.0) * half + half;
	}
#endif
}

void
ppg_model_get_bounds (PpgModel *model,
                      PpgIter  *iter,
                      gdouble  *lower,
                      gdouble  *upper)
{
	g_return_if_fail(PPG_IS_MODEL(model));
	g_return_if_fail(iter != NULL);
	g_return_if_fail(lower != NULL);
	g_return_if_fail(upper != NULL);

	*lower = iter->lower;
	*upper = iter->upper;
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
	model->priv = G_TYPE_INSTANCE_GET_PRIVATE(model, PPG_TYPE_MODEL,
	                                          PpgModelPrivate);

	/*
	 * FIXME: hack to test.
	 */
	model->priv->n_rows = 1;
}
