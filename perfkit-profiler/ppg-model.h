/* ppg-model.h
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

#ifndef __PPG_MODEL_H__
#define __PPG_MODEL_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PPG_TYPE_MODEL            (ppg_model_get_type())
#define PPG_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_MODEL, PpgModel))
#define PPG_MODEL_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_MODEL, PpgModel const))
#define PPG_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_MODEL, PpgModelClass))
#define PPG_IS_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_MODEL))
#define PPG_IS_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_MODEL))
#define PPG_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_MODEL, PpgModelClass))

typedef struct _PpgModel        PpgModel;
typedef struct _PpgModelClass   PpgModelClass;
typedef struct _PpgModelPrivate PpgModelPrivate;
typedef struct _PpgIter         PpgIter;
typedef enum   _PpgResolution   PpgResolution;

struct _PpgIter
{
	PpgModel *model;
	gdouble   begin;
	gdouble   end;
	gdouble   pos;
	gdouble   lower;
	gdouble   upper;

	gpointer  user_data;
	gpointer  user_data1;
	gpointer  user_data2;
};

struct _PpgModel
{
	GInitiallyUnowned parent;

	/*< private >*/
	PpgModelPrivate *priv;
};

struct _PpgModelClass
{
	GInitiallyUnownedClass parent_class;
};

enum _PpgResolution
{
	PPG_RESOLUTION_FULL,
	PPG_RESOLUTION_MILLISECOND,
	PPG_RESOLUTION_SECOND,
	PPG_RESOLUTION_MINUTE,
};

GType    ppg_model_get_type    (void) G_GNUC_CONST;
gboolean ppg_model_get_iter_at (PpgModel      *model,
                                PpgIter       *iter,
                                gdouble        begin,
                                gdouble        end,
                                PpgResolution  resolution);
gboolean ppg_model_iter_next   (PpgModel      *model,
                                PpgIter       *iter);
void     ppg_model_get         (PpgModel      *model,
                                PpgIter       *iter,
                                gint           row,
                                gdouble       *offset,
                                gdouble       *value);
void     ppg_model_get_bounds  (PpgModel      *model,
                                PpgIter       *iter,
                                gdouble       *lower,
                                gdouble       *upper);

G_END_DECLS

#endif /* __PPG_MODEL_H__ */
