/* ppg-prefs-dialog.h
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

#ifndef __PPG_PREFS_DIALOG_H__
#define __PPG_PREFS_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PPG_TYPE_PREFS_DIALOG            (ppg_prefs_dialog_get_type())
#define PPG_PREFS_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_PREFS_DIALOG, PpgPrefsDialog))
#define PPG_PREFS_DIALOG_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_PREFS_DIALOG, PpgPrefsDialog const))
#define PPG_PREFS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_PREFS_DIALOG, PpgPrefsDialogClass))
#define PPG_IS_PREFS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_PREFS_DIALOG))
#define PPG_IS_PREFS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_PREFS_DIALOG))
#define PPG_PREFS_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_PREFS_DIALOG, PpgPrefsDialogClass))

typedef struct _PpgPrefsDialog        PpgPrefsDialog;
typedef struct _PpgPrefsDialogClass   PpgPrefsDialogClass;
typedef struct _PpgPrefsDialogPrivate PpgPrefsDialogPrivate;

struct _PpgPrefsDialog
{
	GtkDialog parent;

	/*< private >*/
	PpgPrefsDialogPrivate *priv;
};

struct _PpgPrefsDialogClass
{
	GtkDialogClass parent_class;
};

GType ppg_prefs_dialog_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PPG_PREFS_DIALOG_H__ */
