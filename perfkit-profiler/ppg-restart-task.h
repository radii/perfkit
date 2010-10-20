/* ppg-restart-task.h
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

#ifndef __PPG_RESTART_TASK_H__
#define __PPG_RESTART_TASK_H__

#include "ppg-task.h"

G_BEGIN_DECLS

#define PPG_TYPE_RESTART_TASK            (ppg_restart_task_get_type())
#define PPG_RESTART_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_RESTART_TASK, PpgRestartTask))
#define PPG_RESTART_TASK_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_RESTART_TASK, PpgRestartTask const))
#define PPG_RESTART_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_RESTART_TASK, PpgRestartTaskClass))
#define PPG_IS_RESTART_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_RESTART_TASK))
#define PPG_IS_RESTART_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_RESTART_TASK))
#define PPG_RESTART_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_RESTART_TASK, PpgRestartTaskClass))

typedef struct _PpgRestartTask        PpgRestartTask;
typedef struct _PpgRestartTaskClass   PpgRestartTaskClass;
typedef struct _PpgRestartTaskPrivate PpgRestartTaskPrivate;

struct _PpgRestartTask
{
	PpgTask parent;

	/*< private >*/
	PpgRestartTaskPrivate *priv;
};

struct _PpgRestartTaskClass
{
	PpgTaskClass parent_class;
};

GType ppg_restart_task_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PPG_RESTART_TASK_H__ */
