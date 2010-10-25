/* ppg-monitor.c
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

#include <ctype.h>
#include <glib/gi18n.h>
#include <sys/sysinfo.h>
#include <stdlib.h>
#include <uber.h>

#include "ppg-color.h"
#include "ppg-monitor.h"

#define MAX_CPU 64

typedef struct
{
	guint    len;
	gdouble  total       [MAX_CPU];
	gdouble  freq        [MAX_CPU];
	glong    last_user   [MAX_CPU];
	glong    last_idle   [MAX_CPU];
	glong    last_system [MAX_CPU];
	glong    last_nice   [MAX_CPU];
} CpuInfo;

typedef struct
{
	gdouble mem_free;
	gdouble swap_free;
} MemInfo;

static CpuInfo  cpu_info = { 0 };
static MemInfo  mem_info = { 0 };
static gboolean shutdown = FALSE;

static void
ppg_monitor_init_cpu (void)
{
#if __linux__
	cpu_info.len = get_nprocs();
#else
#error "Only linux is currently supported"
#endif
}

static void
ppg_monitor_next_cpu (void)
{
	gchar cpu[32] = { 0 };
	glong user;
	glong sys;
	glong nice_;
	glong idle;
	glong user_calc;
	glong sys_calc;
	glong nice_calc;
	glong idle_calc;
	gchar *buf = NULL;
	glong total;
	gchar *line;
	gint ret;
	gint id;
	gint i;

	if (G_UNLIKELY(!g_file_get_contents("/proc/stat", &buf, NULL, NULL))) {
		return;
	}

	line = buf;
	for (i = 0; buf[i]; i++) {
		if (buf[i] == '\n') {
			buf[i] = '\0';
			if (g_str_has_prefix(line, "cpu")) {
				if (isdigit(line[3])) {
					user = nice_ = sys = idle = id = 0;
					ret = sscanf(line, "%s %ld %ld %ld %ld",
								 cpu, &user, &nice_, &sys, &idle);
					if (ret != 5) {
						goto next;
					}
					ret = sscanf(cpu, "cpu%d", &id);
					if (ret != 1 || id < 0 || id >= cpu_info.len) {
						goto next;
					}
					user_calc = user - cpu_info.last_user[id];
					nice_calc = nice_ - cpu_info.last_nice[id];
					sys_calc = sys - cpu_info.last_system[id];
					idle_calc = idle - cpu_info.last_idle[id];
					total = user_calc + nice_calc + sys_calc + idle_calc;
					cpu_info.total[id] = (user_calc + nice_calc + sys_calc) / (gfloat)total * 100.;
					cpu_info.last_user[id] = user;
					cpu_info.last_nice[id] = nice_;
					cpu_info.last_idle[id] = idle;
					cpu_info.last_system[id] = sys;
				}
			} else {
				/* CPU info comes first. Skip further lines. */
				break;
			}
		  next:
			line = &buf[i + 1];
		}
	}

	g_free(buf);
}

static void
ppg_monitor_next_cpu_freq (void)
{
	glong max;
	glong cur;
	gboolean ret;
	gchar *buf;
	gchar *path;
	gint i;

	g_return_if_fail(cpu_info.len > 0);

	/*
	 * Get current frequencies.
	 */
	for (i = 0; i < cpu_info.len; i++) {
		/*
		 * Get max frequency.
		 */
		path = g_strdup_printf("/sys/devices/system/cpu/cpu%d"
		                       "/cpufreq/scaling_max_freq", i);
		ret = g_file_get_contents(path, &buf, NULL, NULL);
		g_free(path);
		if (!ret) {
			continue;
		}
		max = atoi(buf);
		g_free(buf);

		/*
		 * Get current frequency.
		 */
		path = g_strdup_printf("/sys/devices/system/cpu/cpu%d/"
		                       "cpufreq/scaling_cur_freq", i);
		ret = g_file_get_contents(path, &buf, NULL, NULL);
		g_free(path);
		if (!ret) {
			continue;
		}
		cur = atoi(buf);
		g_free(buf);

		/*
		 * Store frequency percentage.
		 */
		cpu_info.freq[i] = (gfloat)cur / (gfloat)max * 100.0;
	}
}

static void
ppg_monitor_next_mem (void)
{
	gdouble mem_total = 0;
	gdouble mem_free = 0;
	gdouble swap_total = 0;
	gdouble swap_free = 0;
	gdouble cached = 0;
	gchar *buf;
	gchar *line;
	gint i;

	if (!g_file_get_contents("/proc/meminfo", &buf, NULL, NULL)) {
		g_critical("Failed to open /proc/meminfo");
		return;
	}

	line = buf;

	for (i = 0; buf[i]; i++) {
		if (buf[i] == '\n') {
			buf[i] = '\0';
			if (g_str_has_prefix(line, "MemTotal:")) {
				if (sscanf(line, "MemTotal: %lf", &mem_total) != 1) {
					g_warning("Failed to read MemTotal");
					goto error;
				}
			} else if (g_str_has_prefix(line, "MemFree:")) {
				if (sscanf(line, "MemFree: %lf", &mem_free) != 1) {
					g_warning("Failed to read MemFree");
					goto error;
				}
			} else if (g_str_has_prefix(line, "SwapTotal:")) {
				if (sscanf(line, "SwapTotal: %lf", &swap_total) != 1) {
					g_warning("Failed to read SwapTotal");
					goto error;
				}
			} else if (g_str_has_prefix(line, "SwapFree:")) {
				if (sscanf(line, "SwapFree: %lf", &swap_free) != 1) {
					g_warning("Failed to read SwapFree");
					goto error;
				}
			} else if (g_str_has_prefix(line, "Cached:")) {
				if (sscanf(line, "Cached: %lf", &cached) != 1) {
					g_warning("Failed to read Cached");
					goto error;
				}
			}
			line = &buf[i + 1];
		}
	}

	mem_info.mem_free = (mem_total - cached - mem_free) / mem_total;
	mem_info.swap_free = (swap_total - swap_free) / swap_total;

  error:
	g_free(buf);
}

static gpointer
ppg_monitor_thread (gpointer data)
{
	while (!g_atomic_int_get((gint *)&shutdown)) {
		ppg_monitor_next_cpu();
		ppg_monitor_next_cpu_freq();
		ppg_monitor_next_mem();
		g_usleep(G_USEC_PER_SEC);
	}
	return NULL;
}

void
ppg_monitor_init (void)
{
	static gsize initialized = FALSE;
	GError *error = NULL;

	if (G_UNLIKELY(g_once_init_enter(&initialized))) {
		ppg_monitor_init_cpu();
		if (!g_thread_create(ppg_monitor_thread, NULL, FALSE, &error)) {
			g_critical("Failed to create monitor thread: %s", error->message);
			g_error_free(error);
		}
		g_once_init_leave(&initialized, TRUE);
	}
}

static gboolean
ppg_monitor_get_cpu (UberLineGraph *graph,
                     guint          line,
                     gdouble       *value,
                     gpointer       user_data)
{
	g_return_val_if_fail(line > 0, FALSE);
	g_return_val_if_fail(line <= cpu_info.len * 2, FALSE);

	if (line <= cpu_info.len) {
		*value = cpu_info.total[line - 1];
	} else {
		*value = cpu_info.freq[(line / 2)- 1];
	}

	return TRUE;
}

static gboolean
ppg_monitor_get_mem (UberLineGraph *graph,
                     guint          line,
                     gdouble       *value,
                     gpointer       user_data)
{
	switch (line) {
	case 1:
		*value = mem_info.mem_free;
		break;
	case 2:
		*value = mem_info.swap_free;
		break;
	default:
		g_assert_not_reached();
	}

	return TRUE;
}

static inline gboolean
has_freq_scaling (gint cpu)
{
	gboolean ret = FALSE;
	gchar *path;

#if __linux__
	path = g_strdup_printf("/sys/devices/system/cpu/cpu%d/cpufreq", cpu);
	ret = g_file_test(path, G_FILE_TEST_IS_DIR);
	g_free(path);
#else
#error "Your platform is not supported"
#endif

	return ret;
}

GtkWidget*
ppg_monitor_cpu_new (void)
{
	const UberRange range = { 0.0, 100.0, 100.0 };
	const gdouble dashes[] = { 1.0, 4.0 };
	PpgColorIter iter;
	GtkWidget *graph;
	GtkWidget *l;
	gchar *title;
	gint i;
	gint line;

	graph = g_object_new(UBER_TYPE_LINE_GRAPH,
	                     "autoscale", FALSE,
	                     "range", &range,
	                     "format", UBER_GRAPH_FORMAT_PERCENT,
	                     "visible", TRUE,
	                     NULL);
	uber_line_graph_set_data_func(UBER_LINE_GRAPH(graph),
	                              ppg_monitor_get_cpu,
	                              NULL, NULL);

	/*
	 * Add regular lines.
	 */
	ppg_color_iter_init(&iter);
	for (i = 0; i < cpu_info.len; i++) {
		title = g_strdup_printf("CPU%d", i + 1);
		l = g_object_new(UBER_TYPE_LABEL,
		                 "color", &iter.color,
		                 "text", title,
		                 "visible", TRUE,
		                 NULL);
		uber_line_graph_add_line(UBER_LINE_GRAPH(graph),
		                         &iter.color,
		                         UBER_LABEL(l));
		g_free(title);
		ppg_color_iter_next(&iter);
	}

	/*
	 * Add cpu frequency lines.
	 */
	ppg_color_iter_init(&iter);
	for (i = 0; i < cpu_info.len; i++) {
		if (has_freq_scaling(i)) {
			line = uber_line_graph_add_line(UBER_LINE_GRAPH(graph),
			                                &iter.color, NULL);
			uber_line_graph_set_line_dash(UBER_LINE_GRAPH(graph), line,
			                              dashes, G_N_ELEMENTS(dashes), 0);
		}
		ppg_color_iter_next(&iter);
	}

	return graph;
}

GtkWidget*
ppg_monitor_mem_new (void)
{
	UberRange range = { 0.0, 100.0, 100.0 };
	PpgColorIter iter;
	GtkWidget *graph;
	UberLabel *l;

	graph = g_object_new(UBER_TYPE_LINE_GRAPH,
	                     "format", UBER_GRAPH_FORMAT_PERCENT,
	                     "range", &range,
	                     "visible", TRUE,
	                     NULL);
	uber_line_graph_set_data_func(UBER_LINE_GRAPH(graph),
	                              ppg_monitor_get_mem,
	                              NULL, NULL);

	ppg_color_iter_init(&iter);
	l = g_object_new(UBER_TYPE_LABEL,
	                 "color", &iter.color,
	                 "text", _("Memory Free"),
	                 "visible", TRUE,
	                 NULL);
	uber_line_graph_add_line(UBER_LINE_GRAPH(graph), &iter.color, l);

	ppg_color_iter_next(&iter);
	l = g_object_new(UBER_TYPE_LABEL,
	                 "color", &iter.color,
	                 "text", _("Swap Free"),
	                 "visible", TRUE,
	                 NULL);
	uber_line_graph_add_line(UBER_LINE_GRAPH(graph), &iter.color, l);

	return graph;
}
