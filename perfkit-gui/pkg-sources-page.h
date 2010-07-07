/* pkg-sources-page.h
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

#ifndef __PKG_SOURCES_PAGE_H__
#define __PKG_SOURCES_PAGE_H__

#include "pkg-page.h"

G_BEGIN_DECLS

#define PKG_TYPE_SOURCES_PAGE            (pkg_sources_page_get_type())
#define PKG_SOURCES_PAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_SOURCES_PAGE, PkgSourcesPage))
#define PKG_SOURCES_PAGE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_SOURCES_PAGE, PkgSourcesPage const))
#define PKG_SOURCES_PAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKG_TYPE_SOURCES_PAGE, PkgSourcesPageClass))
#define PKG_IS_SOURCES_PAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKG_TYPE_SOURCES_PAGE))
#define PKG_IS_SOURCES_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKG_TYPE_SOURCES_PAGE))
#define PKG_SOURCES_PAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKG_TYPE_SOURCES_PAGE, PkgSourcesPageClass))

typedef struct _PkgSourcesPage        PkgSourcesPage;
typedef struct _PkgSourcesPageClass   PkgSourcesPageClass;
typedef struct _PkgSourcesPagePrivate PkgSourcesPagePrivate;

struct _PkgSourcesPage
{
	GtkAlignment parent;

	/*< private >*/
	PkgSourcesPagePrivate *priv;
};

struct _PkgSourcesPageClass
{
	GtkAlignmentClass parent_class;
};

GType      pkg_sources_page_get_type (void) G_GNUC_CONST;
GtkWidget* pkg_sources_page_new      (void);

G_END_DECLS

#endif /* __PKG_SOURCES_PAGE_H__ */
