/* egg-fmt.c
 *
 * Copyright (c) 2009 Christian Hergert <chris@dronelabs.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib-object.h>
#include <glib/gprintf.h>

#include "egg-fmt.h"

typedef struct
{
	gchar *name;
	GType  type;
} Column;

void
egg_fmt_iter_init (EggFmtIter     *iter,
                   EggFmtIterNext  next,
                   ...)
{
	GList   *columns = NULL,
	        *liter;
	Column  *column;
	va_list  argv;
	gchar   *name;
	gint     i;

	g_return_if_fail (iter != NULL);
	g_return_if_fail (next != NULL);

	/* clear memory */
	memset (iter, 0, sizeof (EggFmtIter));
	iter->next = next;

	/* build a list of column info */
	va_start (argv, next);
	name = va_arg (argv, gchar*);
	while (name != NULL) {
		column = g_new (Column, 1);
		column->name = g_strdup (name);
		column->type = va_arg (argv, GType);
		columns = g_list_append (columns, column);
		name = va_arg (argv, gchar*);
	}
	va_end (argv);

	/* store column info */
	iter->n_columns = g_list_length (columns);
	iter->column_names = g_new0 (gchar*, iter->n_columns + 1);
	iter->column_values = g_new0 (GValue, iter->n_columns + 1);
	for (liter = columns, i = 0; liter; liter = liter->next, i++) {
		iter->column_names [i] = ((Column*)liter->data)->name;
		g_value_init (&iter->column_values [i], ((Column*)liter->data)->type);
	}

	/* free allocated state */
	g_list_foreach (columns, (GFunc)g_free, NULL);
	g_list_free (columns);
}

void
egg_fmt_csv (EggFmtIter *iter,
             gpointer    user_data,
             ...)
{
	FILE   *file = stdout;
	GType   type;
	GValue *val;
	gint    i;

	g_return_if_fail (iter != NULL);
	g_return_if_fail (iter->next != NULL);

	/* TODO: Parse va_list for options */

	while (iter->next (iter, user_data)) {
		for (i = 0; i < iter->n_columns; i++) {
			val = &iter->column_values [i];
			type = G_VALUE_TYPE (val);

			/* print out based on type */
			if (type == G_TYPE_INT)
				g_fprintf (file, "%d", g_value_get_int (val));
			else if (type == G_TYPE_UINT)
				g_fprintf (file, "%u", g_value_get_uint (val));
			else if (type == G_TYPE_STRING)
				g_fprintf (file, "%s", g_value_get_string (val));

			if (i + 1 < iter->n_columns)
				g_fprintf (file, ",");
			else
				g_fprintf (file, "\n");

			/* reset early, we are done with value */
			g_value_unset (val);
			g_value_init (val, type);
		}
	}
}

void
egg_fmt_table (EggFmtIter *iter,
               gpointer    user_data,
               ...)
{
	FILE     *file = stdout;
	GList    *rows = NULL,
	         *liter;
	GType     type;
	GValue   *val;
	gint      i,
	          j,
	          len,
	         *widths;
	gchar   **strings;

	g_return_if_fail (iter != NULL);
	g_return_if_fail (iter->next != NULL);

	/* create cache for widths */
	widths = g_new0 (int, iter->n_columns);

	while (iter->next (iter, user_data)) {
		strings = g_new0 (gchar*, iter->n_columns + 1);
		for (i = 0; i < iter->n_columns; i++) {
			val = &iter->column_values [i];
			type = G_VALUE_TYPE (val);

			if (type == G_TYPE_INT)
				strings [i] = g_strdup_printf ("%d", g_value_get_int (val));
			else if (type == G_TYPE_UINT)
				strings [i] = g_strdup_printf ("%u", g_value_get_uint (val));
			else if (type == G_TYPE_STRING)
				strings [i] = g_strdup_printf ("%s", g_value_get_string (val));
			else if (type == G_TYPE_STRV)
				strings [i] = g_strjoinv (" ", g_value_get_boxed (val));

			g_value_unset (val);
			g_value_init (val, type);
		}
		rows = g_list_prepend (rows, strings);
	}

	/* reverse our list into proper order */
	rows = g_list_reverse (rows);

	/* calculate largest widths */
	for (liter = rows; liter; liter = liter->next) {
		strings = liter->data;
		for (i = 0; i < iter->n_columns; i++) {
			len = strings [i] ? strlen (strings [i]) : 0;
			if (len > widths [i])
				widths [i] = len;
		}
	}

	/* print out header top row */
	for (i = 0; i < iter->n_columns; i++) {
		/* first time through, make sure titles are smaller or
		 * expand column size. */
		len = strlen (iter->column_names [i]);
		if (widths [i] < len)
			widths [i] = len;

		/* print block data */
		g_fprintf (file, "+-");
		for (j = 0; j < widths [i]; j++)
			g_fprintf (file, "-");
		g_fprintf (file, "-");
	}
	g_fprintf (file, "+\n");

	/* print out header titles */
	for (i = 0; i < iter->n_columns; i++) {
		g_fprintf (file, "| %s ", iter->column_names [i]);
		len = widths [i] - strlen (iter->column_names [i]);
		for (j = 0; j < len; j++)
			g_fprintf (file, " ");
	}
	g_fprintf (file, "|\n");

	/* print out header bottom row */
	for (i = 0; i < iter->n_columns; i++) {
		g_fprintf (file, "+-");
		for (j = 0; j < widths [i]; j++)
			g_fprintf (file, "-");
		g_fprintf (file, "-");
	}
	g_fprintf (file, "+\n");

	/* print out the values */
	for (liter = rows; liter; liter = liter->next) {
		strings = liter->data;

		for (i = 0; i < iter->n_columns; i++) {
			len = widths [i] - strlen (strings [i]);
			g_fprintf (file, "| %s ", strings [i]);
			for (j = 0; j < len; j++)
				g_fprintf (file, " ");
		}

		g_fprintf (file, "|\n");
	}

	/* print out footer row */
	for (i = 0; i < iter->n_columns; i++) {
		g_fprintf (file, "+-");
		for (j = 0; j < widths [i]; j++)
			g_fprintf (file, "-");
		g_fprintf (file, "-");
	}
	g_fprintf (file, "+\n");



	g_free (widths);
	g_list_foreach (rows, (GFunc)g_strfreev, NULL);
}

void
egg_fmt_html_table (EggFmtIter *iter,
                    gpointer    user_data,
                    ...)
{
	FILE   *file = stdout;
	gint    i;
	gchar  *escaped;
	GValue *val;
	GType   type;

	g_return_if_fail (iter != NULL);
	g_return_if_fail (iter->next != NULL);

	g_fprintf (file,
	           "<table>\n"
	           "  <thead>\n"
	           "    <tr>\n");

	for (i = 0; i < iter->n_columns; i++) {
		escaped = g_markup_escape_text (iter->column_names [i], -1);
		g_fprintf (file, "      <th>%s</th>\n", escaped);
		g_free (escaped);
	}

	g_fprintf (file,
	           "    </tr>\n"
	           "  </thead>\n"
	           "  <tbody>\n");

	while (iter->next (iter, user_data)) {
		g_fprintf (file, "    <tr>\n");
		for (i = 0; i < iter->n_columns; i++) {
			g_fprintf (file, "      <td>");
			val = &iter->column_values [i];
			type = G_VALUE_TYPE (val);
			if (type == G_TYPE_INT)
				g_fprintf (file, "%d", g_value_get_int (val));
			else if (type == G_TYPE_UINT)
				g_fprintf (file, "%u", g_value_get_uint (val));
			else if (type == G_TYPE_LONG)
				g_fprintf (file, "%ld", g_value_get_long (val));
			else if (type == G_TYPE_ULONG)
				g_fprintf (file, "%lu", g_value_get_ulong (val));
			else if (type == G_TYPE_STRING)
				g_fprintf (file, "%s", g_value_get_string (val));
			g_fprintf (file, "</td>\n");
			g_value_unset (val);
			g_value_init (val, type);
		}
		g_fprintf (file, "    </tr>\n");
	}

	g_fprintf (file,
	           "  </tbody>\n"
	           "</table>\n");
}
