/* ppg-about-dialog.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ppg-about-dialog.h"

G_DEFINE_TYPE(PpgAboutDialog, ppg_about_dialog, GTK_TYPE_ABOUT_DIALOG)

struct _PpgAboutDialogPrivate
{
	gpointer dummy;
};

static const gchar *authors[] = {
	"Christian Hergert <chris@dronelabs.com>",
	NULL
};

static const gchar *copyright = "Copyright © 2010 Christian Hergert";

static const gchar *license =
"This program is free software: you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation, either version 3 of the License, or\n"
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program.  If not, see <http://www.gnu.org/licenses/>.\n";

static const gchar *website = "http://perfkit.org/";


/**
 * ppg_about_dialog_finalize:
 * @object: A #PpgAboutDialog.
 *
 * Finalizer for a #PpgAboutDialog instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_about_dialog_finalize (GObject *object) /* IN */
{
	G_OBJECT_CLASS(ppg_about_dialog_parent_class)->finalize(object);
}

/**
 * ppg_about_dialog_class_init:
 * @klass: A #PpgAboutDialogClass.
 *
 * Initializes the #PpgAboutDialogClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_about_dialog_class_init (PpgAboutDialogClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_about_dialog_finalize;
	g_type_class_add_private(object_class, sizeof(PpgAboutDialogPrivate));
}

/**
 * ppg_about_dialog_init:
 * @dialog: A #PpgAboutDialog.
 *
 * Initializes the newly created #PpgAboutDialog instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_about_dialog_init (PpgAboutDialog *dialog) /* IN */
{
	dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE(dialog, PPG_TYPE_ABOUT_DIALOG,
	                                           PpgAboutDialogPrivate);

	g_object_set(dialog,
	             "authors", authors,
	             "copyright", copyright,
	             "license", license,
	             "logo-icon-name", "clock",
	             "program-name", PRODUCT_NAME,
	             "version", PACKAGE_VERSION,
	             "website", website,
	             "wrap-license", FALSE,
	             NULL);
}
