/* pkg-page.c
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "pkg-log.h"
#include "pkg-page.h"

/**
 * pkg_page_load:
 * @page: A #PkgPage.
 *
 * Loads the page.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pkg_page_load (PkgPage *page) /* IN */
{
	g_return_if_fail(PKG_IS_PAGE(page));

	ENTRY;
	PKG_PAGE_GET_INTERFACE(page)->load(page);
	EXIT;
}

/**
 * pkg_page_unload:
 * @page: A #PkgPage.
 *
 * Unloads the page.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pkg_page_unload (PkgPage *page) /* IN */
{
	g_return_if_fail(PKG_IS_PAGE(page));

	ENTRY;
	if (PKG_PAGE_GET_INTERFACE(page)->unload) {
		PKG_PAGE_GET_INTERFACE(page)->unload(page);
	}
	EXIT;
}

GType
pkg_page_get_type (void)
{
	static GType type_id = 0;

	if (g_once_init_enter((gsize *)&type_id)) {
		GType _type_id;
		const GTypeInfo g_type_info = {
			sizeof(PkgPageIface),
			NULL, /* base_init */
			NULL, /* base_finalize */
			NULL, /* class_init */
			NULL, /* class_finalize */
			NULL, /* class_data */
			0,    /* instance_size */
			0,    /* n_preallocs */
			NULL, /* instance_init */
			NULL  /* value_vtable */
		};

		_type_id = g_type_register_static(G_TYPE_INTERFACE,
		                                  "PkgPage",
		                                  &g_type_info,
		                                  0);
		g_type_interface_add_prerequisite(_type_id, GTK_TYPE_WIDGET);
		g_once_init_leave((gsize *)&type_id, _type_id);
	}
	return type_id;
}
