/* egg-time.h
 *
 * Copyright (c) 2010 Christian Hergert
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

#ifndef __EGG_TIME_H__
#define __EGG_TIME_H__

#include <glib.h>

G_BEGIN_DECLS

#define G_TIME_SPAN_DAY         (G_GINT64_CONSTANT (86400000000))
#define G_TIME_SPAN_HOUR        (G_GINT64_CONSTANT (3600000000))
#define G_TIME_SPAN_MINUTE      (G_GINT64_CONSTANT (60000000))
#define G_TIME_SPAN_SECOND      (G_GINT64_CONSTANT (1000000))
#define G_TIME_SPAN_MILLISECOND (G_GINT64_CONSTANT (1000))

typedef gint64 GTimeSpan;

inline void
g_time_val_to_ticks (GTimeVal  *tv,
                     GTimeSpan *u)
{
	*u = ((((guint64)tv->tv_sec) + G_GINT64_CONSTANT(62135596800)) * 10000000)
	   + (((guint64)tv->tv_usec) * 10);
}

inline void
g_time_val_from_ticks (GTimeVal  *tv,
                       GTimeSpan *u)
{
	tv->tv_usec = ((*u) / 10) % G_TIME_SPAN_SECOND;
	tv->tv_sec = ((*u) / G_TIME_SPAN_SECOND) - G_GINT64_CONSTANT(62135596800);
}

inline void
g_time_val_add_span (GTimeVal  *tv,
                     GTimeSpan *ts)
{
	GTimeSpan u1;

	g_time_val_to_ticks(tv, &u1);
	u1 += *ts;
	g_time_val_from_ticks(tv, &u1);
}

inline void
g_time_val_diff (GTimeVal  *tv1,
                 GTimeVal  *tv2,
                 GTimeSpan *u)
{
	gint64 u1, u2;

	g_time_val_to_ticks(tv1, &u1);
	g_time_val_to_ticks(tv2, &u2);
	*u = (u1 - u2);
}

G_END_DECLS

#endif /* __EGG_TIME_H__ */
