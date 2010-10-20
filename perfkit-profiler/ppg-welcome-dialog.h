/* ppg-welcome-dialog.h
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

#ifndef __PPG_WELCOME_DIALOG_H__
#define __PPG_WELCOME_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PPG_TYPE_WELCOME_DIALOG            (ppg_welcome_dialog_get_type())
#define PPG_WELCOME_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_WELCOME_DIALOG, PpgWelcomeDialog))
#define PPG_WELCOME_DIALOG_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_WELCOME_DIALOG, PpgWelcomeDialog const))
#define PPG_WELCOME_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_WELCOME_DIALOG, PpgWelcomeDialogClass))
#define PPG_IS_WELCOME_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_WELCOME_DIALOG))
#define PPG_IS_WELCOME_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_WELCOME_DIALOG))
#define PPG_WELCOME_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_WELCOME_DIALOG, PpgWelcomeDialogClass))

typedef struct _PpgWelcomeDialog        PpgWelcomeDialog;
typedef struct _PpgWelcomeDialogClass   PpgWelcomeDialogClass;
typedef struct _PpgWelcomeDialogPrivate PpgWelcomeDialogPrivate;

struct _PpgWelcomeDialog
{
	GtkWindow parent;

	/*< private >*/
	PpgWelcomeDialogPrivate *priv;
};

struct _PpgWelcomeDialogClass
{
	GtkWindowClass parent_class;
};

GType ppg_welcome_dialog_get_type (void) G_GNUC_CONST;
guint ppg_welcome_dialog_count    (void);

G_END_DECLS

#endif /* __PPG_WELCOME_DIALOG_H__ */
