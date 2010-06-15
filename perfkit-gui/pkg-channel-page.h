/* pkg-channel-page.h
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

#ifndef __PKG_CHANNEL_PAGE_H__
#define __PKG_CHANNEL_PAGE_H__

#include <gtk/gtk.h>
#include <perfkit/perfkit.h>

G_BEGIN_DECLS

#define PKG_TYPE_CHANNEL_PAGE            (pkg_channel_page_get_type())
#define PKG_CHANNEL_PAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_CHANNEL_PAGE, PkgChannelPage))
#define PKG_CHANNEL_PAGE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_CHANNEL_PAGE, PkgChannelPage const))
#define PKG_CHANNEL_PAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKG_TYPE_CHANNEL_PAGE, PkgChannelPageClass))
#define PKG_IS_CHANNEL_PAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKG_TYPE_CHANNEL_PAGE))
#define PKG_IS_CHANNEL_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKG_TYPE_CHANNEL_PAGE))
#define PKG_CHANNEL_PAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKG_TYPE_CHANNEL_PAGE, PkgChannelPageClass))

typedef struct _PkgChannelPage        PkgChannelPage;
typedef struct _PkgChannelPageClass   PkgChannelPageClass;
typedef struct _PkgChannelPagePrivate PkgChannelPagePrivate;

struct _PkgChannelPage
{
	GtkAlignment parent;

	/*< private >*/
	PkgChannelPagePrivate *priv;
};

struct _PkgChannelPageClass
{
	GtkAlignmentClass parent_class;
};

GType      pkg_channel_page_get_type (void) G_GNUC_CONST;
GtkWidget* pkg_channel_page_new      (PkConnection   *connection,
                                      gint            channel);
void       pkg_channel_page_reload   (PkgChannelPage *page);

G_END_DECLS

#endif /* __PKG_CHANNEL_PAGE_H__ */
