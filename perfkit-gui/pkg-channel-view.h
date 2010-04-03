/* pkg-channel-view.h
 *
 * Copyright (C) 2010 Christian Hergert
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

#if !defined (__PERFKIT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-gui/perfkit-gui.h> can be included directly."
#endif

#ifndef __PKG_CHANNEL_VIEW_H__
#define __PKG_CHANNEL_VIEW_H__

#include <gtk/gtk.h>
#include <perfkit/perfkit.h>

G_BEGIN_DECLS

#define PKG_TYPE_CHANNEL_VIEW            (pkg_channel_view_get_type())
#define PKG_CHANNEL_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_CHANNEL_VIEW, PkgChannelView))
#define PKG_CHANNEL_VIEW_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_CHANNEL_VIEW, PkgChannelView const))
#define PKG_CHANNEL_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKG_TYPE_CHANNEL_VIEW, PkgChannelViewClass))
#define PKG_IS_CHANNEL_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKG_TYPE_CHANNEL_VIEW))
#define PKG_IS_CHANNEL_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKG_TYPE_CHANNEL_VIEW))
#define PKG_CHANNEL_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKG_TYPE_CHANNEL_VIEW, PkgChannelViewClass))

typedef struct _PkgChannelView        PkgChannelView;
typedef struct _PkgChannelViewClass   PkgChannelViewClass;
typedef struct _PkgChannelViewPrivate PkgChannelViewPrivate;

struct _PkgChannelView
{
	GtkVBox parent;

	/*< private >*/
	PkgChannelViewPrivate *priv;
};

struct _PkgChannelViewClass
{
	GtkVBoxClass parent_class;
};

GType       pkg_channel_view_get_type    (void) G_GNUC_CONST;
GtkWidget*  pkg_channel_view_new         (void);
void        pkg_channel_view_set_channel (PkgChannelView *channel_view,
                                          PkChannel      *channel);
PkChannel*  pkg_channel_view_get_channel (PkgChannelView *channel_view) G_GNUC_PURE;

G_END_DECLS

#endif /* __PKG_CHANNEL_VIEW_H__ */
