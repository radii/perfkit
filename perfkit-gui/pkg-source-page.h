/* pkg-source-page.h
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

#ifndef __PKG_SOURCE_PAGE_H__
#define __PKG_SOURCE_PAGE_H__

#include <gtk/gtk.h>
#include <perfkit/perfkit.h>

G_BEGIN_DECLS

#define PKG_TYPE_SOURCE_PAGE            (pkg_source_page_get_type())
#define PKG_SOURCE_PAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_SOURCE_PAGE, PkgSourcePage))
#define PKG_SOURCE_PAGE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_SOURCE_PAGE, PkgSourcePage const))
#define PKG_SOURCE_PAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKG_TYPE_SOURCE_PAGE, PkgSourcePageClass))
#define PKG_IS_SOURCE_PAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKG_TYPE_SOURCE_PAGE))
#define PKG_IS_SOURCE_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKG_TYPE_SOURCE_PAGE))
#define PKG_SOURCE_PAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKG_TYPE_SOURCE_PAGE, PkgSourcePageClass))

typedef struct _PkgSourcePage        PkgSourcePage;
typedef struct _PkgSourcePageClass   PkgSourcePageClass;
typedef struct _PkgSourcePagePrivate PkgSourcePagePrivate;

struct _PkgSourcePage
{
	GtkAlignment parent;

	/*< private >*/
	PkgSourcePagePrivate *priv;
};

struct _PkgSourcePageClass
{
	GtkAlignmentClass parent_class;
};

GType      pkg_source_page_get_type (void) G_GNUC_CONST;
GtkWidget* pkg_source_page_new      (PkConnection  *connection,
                                     gint           source);
void       pkg_source_page_reload   (PkgSourcePage *page);

G_END_DECLS

#endif /* __PKG_SOURCE_PAGE_H__ */
