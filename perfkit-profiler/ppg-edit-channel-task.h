/* ppg-edit-channel-task.h
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

#ifndef __PPG_EDIT_CHANNEL_TASK_H__
#define __PPG_EDIT_CHANNEL_TASK_H__

#include "ppg-session-task.h"

G_BEGIN_DECLS

#define PPG_TYPE_EDIT_CHANNEL_TASK            (ppg_edit_channel_task_get_type())
#define PPG_EDIT_CHANNEL_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_EDIT_CHANNEL_TASK, PpgEditChannelTask))
#define PPG_EDIT_CHANNEL_TASK_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_EDIT_CHANNEL_TASK, PpgEditChannelTask const))
#define PPG_EDIT_CHANNEL_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_EDIT_CHANNEL_TASK, PpgEditChannelTaskClass))
#define PPG_IS_EDIT_CHANNEL_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_EDIT_CHANNEL_TASK))
#define PPG_IS_EDIT_CHANNEL_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_EDIT_CHANNEL_TASK))
#define PPG_EDIT_CHANNEL_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_EDIT_CHANNEL_TASK, PpgEditChannelTaskClass))

typedef struct _PpgEditChannelTask        PpgEditChannelTask;
typedef struct _PpgEditChannelTaskClass   PpgEditChannelTaskClass;
typedef struct _PpgEditChannelTaskPrivate PpgEditChannelTaskPrivate;

struct _PpgEditChannelTask
{
	PpgSessionTask parent;

	/*< private >*/
	PpgEditChannelTaskPrivate *priv;
};

struct _PpgEditChannelTaskClass
{
	PpgSessionTaskClass parent_class;
};

GType ppg_edit_channel_task_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PPG_EDIT_CHANNEL_TASK_H__ */
