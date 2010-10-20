/* ppg-timer-tool-item.h
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

#ifndef __PPG_TIMER_TOOL_ITEM_H__
#define __PPG_TIMER_TOOL_ITEM_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PPG_TYPE_TIMER_TOOL_ITEM            (ppg_timer_tool_item_get_type())
#define PPG_TIMER_TOOL_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_TIMER_TOOL_ITEM, PpgTimerToolItem))
#define PPG_TIMER_TOOL_ITEM_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_TIMER_TOOL_ITEM, PpgTimerToolItem const))
#define PPG_TIMER_TOOL_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_TIMER_TOOL_ITEM, PpgTimerToolItemClass))
#define PPG_IS_TIMER_TOOL_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_TIMER_TOOL_ITEM))
#define PPG_IS_TIMER_TOOL_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_TIMER_TOOL_ITEM))
#define PPG_TIMER_TOOL_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_TIMER_TOOL_ITEM, PpgTimerToolItemClass))

typedef struct _PpgTimerToolItem        PpgTimerToolItem;
typedef struct _PpgTimerToolItemClass   PpgTimerToolItemClass;
typedef struct _PpgTimerToolItemPrivate PpgTimerToolItemPrivate;

struct _PpgTimerToolItem
{
	GtkToolItem parent;

	/*< private >*/
	PpgTimerToolItemPrivate *priv;
};

struct _PpgTimerToolItemClass
{
	GtkToolItemClass parent_class;
};

GType ppg_timer_tool_item_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PPG_TIMER_TOOL_ITEM_H__ */
