/* ppg-add-source-dialog.h
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

#ifndef __PPG_ADD_SOURCE_DIALOG_H__
#define __PPG_ADD_SOURCE_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PPG_TYPE_ADD_SOURCE_DIALOG            (ppg_add_source_dialog_get_type())
#define PPG_ADD_SOURCE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_ADD_SOURCE_DIALOG, PpgAddSourceDialog))
#define PPG_ADD_SOURCE_DIALOG_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_ADD_SOURCE_DIALOG, PpgAddSourceDialog const))
#define PPG_ADD_SOURCE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_ADD_SOURCE_DIALOG, PpgAddSourceDialogClass))
#define PPG_IS_ADD_SOURCE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_ADD_SOURCE_DIALOG))
#define PPG_IS_ADD_SOURCE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_ADD_SOURCE_DIALOG))
#define PPG_ADD_SOURCE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_ADD_SOURCE_DIALOG, PpgAddSourceDialogClass))

typedef struct _PpgAddSourceDialog        PpgAddSourceDialog;
typedef struct _PpgAddSourceDialogClass   PpgAddSourceDialogClass;
typedef struct _PpgAddSourceDialogPrivate PpgAddSourceDialogPrivate;

struct _PpgAddSourceDialog
{
	GtkDialog parent;

	/*< private >*/
	PpgAddSourceDialogPrivate *priv;
};

struct _PpgAddSourceDialogClass
{
	GtkDialogClass parent_class;
};

GType ppg_add_source_dialog_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PPG_ADD_SOURCE_DIALOG_H__ */
