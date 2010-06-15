/* pkg-plugin-page.h
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

#if !defined (__PERFKIT_GUI_INSIDE__) && !defined (PERFKIT_GUI_COMPILATION)
#error "Only <perfkit-gui/perfkit-gui.h> can be included directly."
#endif

#ifndef __PKG_PLUGIN_PAGE_H__
#define __PKG_PLUGIN_PAGE_H__

#include <gtk/gtk.h>
#include <perfkit/perfkit.h>

G_BEGIN_DECLS

#define PKG_TYPE_PLUGIN_PAGE            (pkg_plugin_page_get_type())
#define PKG_PLUGIN_PAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_PLUGIN_PAGE, PkgPluginPage))
#define PKG_PLUGIN_PAGE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_PLUGIN_PAGE, PkgPluginPage const))
#define PKG_PLUGIN_PAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKG_TYPE_PLUGIN_PAGE, PkgPluginPageClass))
#define PKG_IS_PLUGIN_PAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKG_TYPE_PLUGIN_PAGE))
#define PKG_IS_PLUGIN_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKG_TYPE_PLUGIN_PAGE))
#define PKG_PLUGIN_PAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKG_TYPE_PLUGIN_PAGE, PkgPluginPageClass))

typedef struct _PkgPluginPage        PkgPluginPage;
typedef struct _PkgPluginPageClass   PkgPluginPageClass;
typedef struct _PkgPluginPagePrivate PkgPluginPagePrivate;

struct _PkgPluginPage
{
	GtkAlignment parent;

	/*< private >*/
	PkgPluginPagePrivate *priv;
};

struct _PkgPluginPageClass
{
	GtkAlignmentClass parent_class;
};

GType      pkg_plugin_page_get_type (void) G_GNUC_CONST;
GtkWidget* pkg_plugin_page_new      (PkConnection  *connection,
                                     const gchar   *plugin);
void       pkg_plugin_page_reload   (PkgPluginPage *page);

G_END_DECLS

#endif /* __PKG_PLUGIN_PAGE_H__ */
