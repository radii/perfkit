/* ppg-record-action.h
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

#if !defined (__PERFKIT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-profiler/perfkit-profiler.h> can be included directly."
#endif

#ifndef __PPG_RECORD_ACTION_H__
#define __PPG_RECORD_ACTION_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PPG_TYPE_RECORD_ACTION            (ppg_record_action_get_type())
#define PPG_RECORD_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_RECORD_ACTION, PpgRecordAction))
#define PPG_RECORD_ACTION_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_RECORD_ACTION, PpgRecordAction const))
#define PPG_RECORD_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_RECORD_ACTION, PpgRecordActionClass))
#define PPG_IS_RECORD_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_RECORD_ACTION))
#define PPG_IS_RECORD_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_RECORD_ACTION))
#define PPG_RECORD_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_RECORD_ACTION, PpgRecordActionClass))

typedef struct _PpgRecordAction        PpgRecordAction;
typedef struct _PpgRecordActionClass   PpgRecordActionClass;
typedef struct _PpgRecordActionPrivate PpgRecordActionPrivate;

struct _PpgRecordAction
{
	GtkToggleAction parent;

	/*< private >*/
	PpgRecordActionPrivate *priv;
};

struct _PpgRecordActionClass
{
	GtkToggleActionClass parent_class;
};

GType ppg_record_action_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PPG_RECORD_ACTION_H__ */
