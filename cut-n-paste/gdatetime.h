/* gdatetime.h
 *
 * Copyright (C) 2009-2010 Christian Hergert <chris@dronelabs.com>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __G_DATE_TIME_H__
#define __G_DATE_TIME_H__

#include <time.h>
#include <glib.h>

G_BEGIN_DECLS

#define G_TIME_SPAN_DAY         (G_GINT64_CONSTANT (86400000000))
#define G_TIME_SPAN_HOUR        (G_GINT64_CONSTANT (3600000000))
#define G_TIME_SPAN_MINUTE      (G_GINT64_CONSTANT (60000000))
#define G_TIME_SPAN_SECOND      (G_GINT64_CONSTANT (1000000))
#define G_TIME_SPAN_MILLISECOND (G_GINT64_CONSTANT (1000))

typedef struct _GDateTime GDateTime;
typedef gint64            GTimeSpan;

GDateTime *   g_date_time_add                    (GDateTime      *datetime,
                                                  GTimeSpan      *timespan);
GDateTime *   g_date_time_add_days               (GDateTime      *datetime,
                                                  gint            days);
GDateTime *   g_date_time_add_full               (GDateTime      *datetime,
                                                  gint            years,
                                                  gint            months,
                                                  gint            days,
                                                  gint            hours,
                                                  gint            minutes,
                                                  gint            seconds);
GDateTime *   g_date_time_add_hours              (GDateTime      *datetime,
                                                  gint            hours);
GDateTime *   g_date_time_add_milliseconds       (GDateTime      *datetime,
                                                  gint            milliseconds);
GDateTime *   g_date_time_add_minutes            (GDateTime      *datetime,
                                                  gint            minutes);
GDateTime *   g_date_time_add_months             (GDateTime      *datetime,
                                                  gint            months);
GDateTime *   g_date_time_add_seconds            (GDateTime      *datetime,
                                                  gint            seconds);
GDateTime *   g_date_time_add_weeks              (GDateTime      *datetime,
                                                  gint            weeks);
GDateTime *   g_date_time_add_years              (GDateTime      *datetime,
                                                  gint            years);
gint          g_date_time_compare                (gconstpointer   dt1,
                                                  gconstpointer   dt2);
GDateTime *   g_date_time_copy                   (GDateTime      *datetime);
GDateTime *   g_date_time_date                   (GDateTime      *datetime);
void          g_date_time_diff                   (GDateTime      *begin,
                                                  GDateTime      *end,
                                                  GTimeSpan      *timespan);
gboolean      g_date_time_equal                  (gconstpointer   dt1,
                                                  gconstpointer   dt2);
gchar *       g_date_time_format_for_display     (GDateTime      *datetime);
gint          g_date_time_get_day_of_week        (GDateTime      *datetime);
gint          g_date_time_get_day_of_month       (GDateTime      *datetime);
gint          g_date_time_get_day_of_year        (GDateTime      *datetime);
void          g_date_time_get_dmy                (GDateTime      *datetime,
                                                  gint           *day,
                                                  gint           *month,
                                                  gint           *year);
gint          g_date_time_get_hour               (GDateTime      *datetime);
void          g_date_time_get_julian             (GDateTime      *datetime,
                                                  gint           *period,
                                                  gint           *julian,
                                                  gint           *hour,
                                                  gint           *minute,
                                                  gint           *second);
gint          g_date_time_get_microsecond        (GDateTime      *datetime);
gint          g_date_time_get_millisecond        (GDateTime      *datetime);
gint          g_date_time_get_minute             (GDateTime      *datetime);
gint          g_date_time_get_month              (GDateTime      *datetime);
gint          g_date_time_get_second             (GDateTime      *datetime);
void          g_date_time_get_utc_offset         (GDateTime      *datetime,
                                                  GTimeSpan      *timespan);
gint          g_date_time_get_year               (GDateTime      *datetime);
guint         g_date_time_hash                   (gconstpointer   datetime);
gboolean      g_date_time_is_leap_year           (GDateTime      *datetime);
gboolean      g_date_time_is_daylight_savings    (GDateTime      *datetime);
GDateTime *   g_date_time_new_from_date          (gint            year,
                                                  gint            month,
                                                  gint            day);
GDateTime *   g_date_time_new_from_time_t        (time_t          t);
GDateTime *   g_date_time_new_from_timeval       (GTimeVal       *tv);
GDateTime *   g_date_time_new_full               (gint            year,
                                                  gint            month,
                                                  gint            day,
                                                  gint            hour,
                                                  gint            minute,
                                                  gint            second);
GDateTime *   g_date_time_now                    (void);
GDateTime *   g_date_time_parse                  (const gchar    *input);
GDateTime *   g_date_time_parse_with_format      (const gchar    *format,
                                                  const gchar    *input);
gchar *       g_date_time_printf                 (GDateTime      *datetime,
                                                  const gchar    *format);
GDateTime *   g_date_time_ref                    (GDateTime      *datetime);
GDateTime *   g_date_time_to_local               (GDateTime      *datetime);
time_t        g_date_time_to_time_t              (GDateTime      *datetime);
void          g_date_time_to_timeval             (GDateTime      *datetime,
                                                  GTimeVal       *tv);
GDateTime *   g_date_time_to_utc                 (GDateTime      *datetime);
GDateTime *   g_date_time_today                  (void);
void          g_date_time_unref                  (GDateTime      *datetime);
GDateTime *   g_date_time_utc_now                (void);

G_END_DECLS

#endif /* __G_DATE_TIME_H__ */
