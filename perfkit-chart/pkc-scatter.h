/* pkc-scatter.h
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if !defined (__PERFKIT_CHART_INSIDE__) && !defined (PERFKIT_CHART_COMPILATION)
#error "Only <perfkit-chart/perfkit-chart.h> can be included directly."
#endif

#ifndef __PKC_SCATTER_H__
#define __PKC_SCATTER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PKC_TYPE_SCATTER            (pkc_scatter_get_type())
#define PKC_SCATTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKC_TYPE_SCATTER, PkcScatter))
#define PKC_SCATTER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKC_TYPE_SCATTER, PkcScatter const))
#define PKC_SCATTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKC_TYPE_SCATTER, PkcScatterClass))
#define PKC_IS_SCATTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKC_TYPE_SCATTER))
#define PKC_IS_SCATTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKC_TYPE_SCATTER))
#define PKC_SCATTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKC_TYPE_SCATTER, PkcScatterClass))

typedef struct _PkcScatter        PkcScatter;
typedef struct _PkcScatterClass   PkcScatterClass;
typedef struct _PkcScatterPrivate PkcScatterPrivate;

struct _PkcScatter
{
	GtkAlignment parent;

	/*< private >*/
	PkcScatterPrivate *priv;
};

struct _PkcScatterClass
{
	GtkAlignmentClass parent_class;
};

GType          pkc_scatter_get_type        (void) G_GNUC_CONST;
GtkWidget*     pkc_scatter_new             (void);
GtkAdjustment* pkc_scatter_get_xadjustment (PkcScatter *scatter);
GtkAdjustment* pkc_scatter_get_yadjustment (PkcScatter *scatter);

G_END_DECLS

#endif /* __PKC_SCATTER_H__ */
