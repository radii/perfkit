/* ppg-session-task.h
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

#ifndef __PPG_SESSION_TASK_H__
#define __PPG_SESSION_TASK_H__

#include "ppg-session.h"
#include "ppg-task.h"

G_BEGIN_DECLS

#define PPG_TYPE_SESSION_TASK            (ppg_session_task_get_type())
#define PPG_SESSION_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_SESSION_TASK, PpgSessionTask))
#define PPG_SESSION_TASK_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_SESSION_TASK, PpgSessionTask const))
#define PPG_SESSION_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_SESSION_TASK, PpgSessionTaskClass))
#define PPG_IS_SESSION_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_SESSION_TASK))
#define PPG_IS_SESSION_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_SESSION_TASK))
#define PPG_SESSION_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_SESSION_TASK, PpgSessionTaskClass))

typedef struct _PpgSessionTask        PpgSessionTask;
typedef struct _PpgSessionTaskClass   PpgSessionTaskClass;
typedef struct _PpgSessionTaskPrivate PpgSessionTaskPrivate;

struct _PpgSessionTask
{
	PpgTask parent;

	/*< private >*/
	PpgSessionTaskPrivate *priv;
};

struct _PpgSessionTaskClass
{
	PpgTaskClass parent_class;
};

GType       ppg_session_task_get_type    (void) G_GNUC_CONST;
void        ppg_session_task_set_session (PpgSessionTask *task,
                                          PpgSession     *session);
PpgSession* ppg_session_task_get_session (PpgSessionTask *task);

G_END_DECLS

#endif /* __PPG_SESSION_TASK_H__ */
