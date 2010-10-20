/* ppg-task.h
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

#ifndef __PPG_TASK_H__
#define __PPG_TASK_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PPG_TYPE_TASK            (ppg_task_get_type())
#define PPG_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_TASK, PpgTask))
#define PPG_TASK_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_TASK, PpgTask const))
#define PPG_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_TASK, PpgTaskClass))
#define PPG_IS_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_TASK))
#define PPG_IS_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_TASK))
#define PPG_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_TASK, PpgTaskClass))

typedef struct _PpgTask        PpgTask;
typedef struct _PpgTaskClass   PpgTaskClass;
typedef struct _PpgTaskPrivate PpgTaskPrivate;

struct _PpgTask
{
	GInitiallyUnowned parent;

	/*< private >*/
	PpgTaskPrivate *priv;
};

struct _PpgTaskClass
{
	GInitiallyUnownedClass parent_class;

	void (*run) (PpgTask *task);
};

GType    ppg_task_get_type          (void) G_GNUC_CONST;
void     ppg_task_schedule          (PpgTask      *task);
void     ppg_task_finish            (PpgTask      *task);
void     ppg_task_finish_with_error (PpgTask      *task,
                                     const GError *error);
gboolean ppg_task_is_error          (PpgTask      *task);
void     ppg_task_get_error         (PpgTask      *task,
                                     GError      **error);

G_END_DECLS

#endif /* __PPG_TASK_H__ */
