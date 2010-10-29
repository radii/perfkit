/* ppg-about-dialog.h
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

#ifndef __PPG_ABOUT_DIALOG_H__
#define __PPG_ABOUT_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PPG_TYPE_ABOUT_DIALOG            (ppg_about_dialog_get_type())
#define PPG_ABOUT_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_ABOUT_DIALOG, PpgAboutDialog))
#define PPG_ABOUT_DIALOG_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_ABOUT_DIALOG, PpgAboutDialog const))
#define PPG_ABOUT_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_ABOUT_DIALOG, PpgAboutDialogClass))
#define PPG_IS_ABOUT_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_ABOUT_DIALOG))
#define PPG_IS_ABOUT_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_ABOUT_DIALOG))
#define PPG_ABOUT_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_ABOUT_DIALOG, PpgAboutDialogClass))

typedef struct _PpgAboutDialog        PpgAboutDialog;
typedef struct _PpgAboutDialogClass   PpgAboutDialogClass;
typedef struct _PpgAboutDialogPrivate PpgAboutDialogPrivate;

struct _PpgAboutDialog
{
	GtkWindow parent;

	/*< private >*/
	PpgAboutDialogPrivate *priv;
};

struct _PpgAboutDialogClass
{
	GtkWindowClass parent_class;
};

GType ppg_about_dialog_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PPG_ABOUT_DIALOG_H__ */
