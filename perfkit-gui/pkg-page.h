/* pkg-page.h
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

#ifndef __PKG_PAGE_H__
#define __PKG_PAGE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PKG_TYPE_PAGE             (pkg_page_get_type())
#define PKG_PAGE(o)               (G_TYPE_CHECK_INSTANCE_CAST((o),    PKG_TYPE_PAGE, PkgPage))
#define PKG_IS_PAGE(o)            (G_TYPE_CHECK_INSTANCE_TYPE((o),    PKG_TYPE_PAGE))
#define PKG_PAGE_GET_INTERFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE((o), PKG_TYPE_PAGE, PkgPageIface))

typedef struct _PkgPage PkgPage;
typedef struct _PkgPageIface PkgPageIface;

struct _PkgPageIface
{
	GTypeInterface parent;

	void (*load)   (PkgPage *page);
	void (*unload) (PkgPage *page);
};

GType pkg_page_get_type (void) G_GNUC_CONST;
void  pkg_page_load     (PkgPage *page);
void  pkg_page_unload   (PkgPage *page);

G_END_DECLS

#endif /* __PKG_PAGE_H__ */
