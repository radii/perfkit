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

#include <perfkit/perfkit.h>

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
typedef struct _PpgModelIter    PpgModelIter;
typedef enum   _PpgResolution   PpgResolution;

typedef void (*PpgModelValueFunc) (PpgModel     *model,
                                   PpgModelIter *iter,
                                   gint          key,
                                   GValue       *value,
                                   gpointer      user_data);

struct _PpgModelIter
{
	guint32 stamp;
	gdouble time;

	/*< private >*/
	gpointer user_data;
	gpointer user_data2;
	gpointer user_data3;
	gpointer user_data4;
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

GType    ppg_model_get_type         (void) G_GNUC_CONST;
void     ppg_model_add_mapping      (PpgModel          *model,
                                     gint               key,
                                     const gchar       *field,
                                     GType              expected_type);
void     ppg_model_add_mapping_func (PpgModel          *model,
                                     gint               key,
                                     PpgModelValueFunc  func,
                                     gpointer           user_data);
gboolean ppg_model_get_iter_at      (PpgModel          *model,
                                     PpgModelIter      *iter,
                                     gdouble            begin,
                                     gdouble            end,
                                     PpgResolution      resolution);
gboolean ppg_model_get_iter_first   (PpgModel          *model,
                                     PpgModelIter      *iter);
void     ppg_model_get              (PpgModel          *model,
                                     PpgModelIter      *iter,
                                     gint               first_key,
                                     ...);
void     ppg_model_get_value        (PpgModel          *model,
                                     PpgModelIter      *iter,
                                     gint               key,
                                     GValue            *value);
void     ppg_model_insert_manifest  (PpgModel          *model,
                                     PkManifest        *manifest);
void     ppg_model_insert_sample    (PpgModel          *model,
                                     PkManifest        *manifest,
                                     PkSample          *sample);
gboolean ppg_model_iter_next        (PpgModel          *model,
                                     PpgModelIter      *iter);

G_END_DECLS

#endif /* __PPG_MODEL_H__ */
