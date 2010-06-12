/* pkg-util.h
 *
 * Copyright (C) 2007 David Zeuthen
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "pkg-util.h"

void
pkg_util_get_mix_color (GtkWidget    *widget,
                        GtkStateType  state,
                        gchar        *color_buf,
                        gsize         color_buf_size)
{
	GtkStyle *style;
	GdkColor color = {0};

	g_return_if_fail (GTK_IS_WIDGET (widget));
	g_return_if_fail (color_buf != NULL);

	/* This color business shouldn't be this hard... */
	style = gtk_widget_get_style (widget);
#define BLEND_FACTOR 0.7
	color.red   = style->text[state].red   * BLEND_FACTOR +
				  style->base[state].red   * (1.0 - BLEND_FACTOR);
	color.green = style->text[state].green * BLEND_FACTOR +
				  style->base[state].green * (1.0 - BLEND_FACTOR);
	color.blue  = style->text[state].blue  * BLEND_FACTOR +
				  style->base[state].blue  * (1.0 - BLEND_FACTOR);
#undef BLEND_FACTOR
	snprintf (color_buf,
	          color_buf_size, "#%02x%02x%02x",
	          (color.red >> 8),
	          (color.green >> 8),
	          (color.blue >> 8));
}
