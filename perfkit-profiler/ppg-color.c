/* ppg-color.c
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

#include <string.h>

#include "ppg-color.h"

static const gchar *default_colors[] = {
	"#3465a4",
	"#4e9a06",
	"#cc0000",
	"#f57900",
	"#555753",
	"#75507b",
	"#c35c00",
	"#729fcf",
	"#c17d11",
	"#fcaf3e",
	NULL
};

static gchar **colors   = NULL;
static guint   n_colors = 0;

void
ppg_color_init (void)
{
	colors = g_strdupv((gchar **)default_colors);
	n_colors = g_strv_length(colors);
}

void
ppg_color_iter_init (PpgColorIter *iter)
{
	g_return_if_fail(iter != NULL);

	memset(iter, 0, sizeof *iter);
	gdk_color_parse(colors[0], &iter->color);
	clutter_color_from_string(&iter->clutter_color, colors[0]);
}

void
ppg_color_iter_next (PpgColorIter *iter)
{
	const gchar *color;

	g_return_if_fail(iter != NULL);

	iter->mod++;
	color = colors[iter->mod % n_colors];
	gdk_color_parse(color, &iter->color);
	clutter_color_from_string(&iter->clutter_color, color);
}
