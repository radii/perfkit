/* egg-fmt.h
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

#ifndef __EGG_FMT_H__
#define __EGG_FMT_H__

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * EggFmtIter:
 *
 * A struct containing necessary information for iteration through
 * data-sets for formatting.
 */
typedef struct _EggFmtIter EggFmtIter;

/**
 * EggFmtIterNext:
 * @iter: A #EggFmtIter
 * @user_data: user data supplied to egg_fmt_iter_init().
 *
 * A method prototype to retreive the next set of values for the iter.
 *
 * Return value: %FALSE if there are no more values.
 */
typedef gboolean (*EggFmtIterNext) (EggFmtIter *iter,
                                    gpointer    user_data);

/**
 * EggFmtFunc:
 * @iter: An #EggFmtIter
 * @user_data: user provided data for callbacks
 * @...: A NULL-terminated list of option/value tuples for the formatter.
 *
 * A method prototype for various formatting implementations.  See
 * egg_fmt_table() and egg_fmt_csv().
 */
typedef void (*EggFmtFunc) (EggFmtIter *iter, gpointer user_data, ...) G_GNUC_NULL_TERMINATED;

struct _EggFmtIter
{
	/* VTable */
	EggFmtIterNext next;
	gpointer       reserved1;
	gpointer       reserved2;
	gpointer       reserved3;

	/* Iter Information */
	gint           n_columns;
	gchar        **column_names;
	GValue        *column_values;
	gboolean       done;

	/* User data for state */
	gpointer       user_data;
	gpointer       user_data2;
	gpointer       user_data3;
	gpointer       user_data4;
};

void egg_fmt_table     (EggFmtIter     *iter,
                        gpointer        user_data,
                        ...) G_GNUC_NULL_TERMINATED;
void egg_fmt_csv       (EggFmtIter     *iter,
                        gpointer        user_data,
                        ...) G_GNUC_NULL_TERMINATED;
void egg_fmt_html_table(EggFmtIter     *iter,
                        gpointer        user_data,
                        ...) G_GNUC_NULL_TERMINATED;
void egg_fmt_iter_init (EggFmtIter     *iter,
                        EggFmtIterNext  next,
                        ...) G_GNUC_NULL_TERMINATED;

G_END_DECLS

#endif /* __EGG_FMT_H__ */
