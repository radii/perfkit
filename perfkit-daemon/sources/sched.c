/* sched.c
 *
 * Copyright (C) 2010 Andrew Stiegmann <andrew.stiegmann@gmail.com>
 *               2010 Christian Hergert <chris@dronelabs.com>
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

#include <egg-time.h>
#include <stdlib.h>
#include <string.h>
#include <perfkit-daemon/perfkit-daemon.h>

typedef struct
{
	PkdManifest *manifest;
	gchar       *filename;
	GPid         pid;
} Sched;

/*
Entries looks like this...
---------------------------------------------------------
se.exec_start                      :     613312708.681735
se.vruntime                        :      57042775.413011
.
. A bunch of similar stuff here.
.
clock-delta                        :                  409
*/

typedef enum
{
	SCHED_UNKNOWN = 0,
	SCHED_INT,
	SCHED_DOUBLE,
} SchedType;

typedef union
{
	gint i;
	gdouble d;
} SchedValue;

typedef struct
{
	gchar      *name;
	SchedType   type;
	SchedValue  val;
} SchedEntry;

static inline gchar*
sched_read (Sched *sched)
{
	gchar *contents = NULL;
	GError *error = NULL;

	if (G_UNLIKELY(!sched->filename)) {
		sched->filename = g_strdup_printf("/proc/%d/sched", sched->pid);
		g_assert(sched->filename);
	}

	if (!g_file_get_contents(sched->filename, &contents, NULL, &error)) {
		g_warning("sched: Failed to read: %s: %s",
		          sched->filename, error->message);
		g_error_free(error);
	}

	return contents;
}

static inline gboolean
has_decimal (gchar *str)
{
	gchar *ptr = str;

	for(; *ptr; ptr++) {
		if (*ptr == '.') 
			return TRUE;
	}

	return FALSE;
}

/*
 * Given a pre-allocated array of SchedEntry, fill each entry until
 * we either run out of entries or space in the array, which ever one
 * happens first.  Note that if the names and types in the SchedEntry are
 * already filled out we will not fill them out again.  This is done to
 * save time and because the names and value types probably won't change, 
 * but their values will.
 */
static gboolean
sched_parse (Sched      *sched,
             GArray     *entries)
{
	static GRegex *regex     = NULL;
	gchar         *contents  = NULL;
	GMatchInfo    *matchInfo = NULL;

	if (g_once_init_enter ((gsize*) &regex)) {
		GRegex *tmp;

		tmp = g_regex_new("^([\\w\\.\\-]+)\\s*:\\s*([\\d\\.]+)$",
		                  G_REGEX_CASELESS | G_REGEX_MULTILINE,
		                  G_REGEX_MATCH_NOTBOL | G_REGEX_MATCH_NOTEOL,
		                  NULL);
		g_once_init_leave ((gsize *)&regex, (gsize)tmp);
	}

	if (!(contents = sched_read(sched))) {
		g_warning("sched: Error reading scheduler file.");
		return FALSE;
	}

	if (!g_regex_match(regex, contents, 0, &matchInfo)) {
		g_warning("sched: No match in scheduler file.");
		return FALSE;
	}

	for (; g_match_info_matches(matchInfo); g_match_info_next(matchInfo, NULL)) {
		gint matches = g_match_info_get_match_count(matchInfo);

		if (matches == 3) {
			gchar *val = g_match_info_fetch(matchInfo, 2);
			SchedEntry se = {0};

			/* Don't populate the names if they already exist */
			if (!se.name) {
				se.name = g_match_info_fetch(matchInfo, 1);	
			}

			/* Same practice with the type */
			if (se.type == SCHED_UNKNOWN) {
				se.type = has_decimal(val) ? SCHED_DOUBLE : SCHED_INT;
			}

			switch (se.type) {
			case SCHED_INT:
				se.val.i = atoi(val);
				break;
			case SCHED_DOUBLE:
				se.val.d = g_strtod(val, NULL);
				break;
			case SCHED_UNKNOWN:
			default:
				se.val.i = -1;  // Signals error.
				se.type = SCHED_UNKNOWN;
				break;
			}

			g_array_append_val(entries, se);
			g_free(val);
		} 
	}

	g_match_info_free(matchInfo);
	g_free(contents);

	return TRUE;
}

static inline void
populate_manifest (PkdManifest *m,
                   GArray      *entries)
{
	SchedEntry *ent;
	int i;

	for (i = 0; i < entries->len; i++) {
		ent = &g_array_index(entries, SchedEntry, i);

		switch (ent->type) {
		case SCHED_INT:
			pkd_manifest_append(m, ent->name, G_TYPE_INT);
			break;
		case SCHED_DOUBLE:
			//pkd_manifest_append(m, ent->name, G_TYPE_DOUBLE);
			pkd_manifest_append(m, ent->name, G_TYPE_INT);
			break;
		case SCHED_UNKNOWN:
		default:
			g_warn_if_reached();
			break;
		}
	}
}

static inline void
populate_sample (PkdSample *s,
                 GArray    *entries)
{
	SchedEntry *ent;
	int i;

	for (i = 0; i < entries->len; i++) {
		ent = &g_array_index(entries, SchedEntry, i);

		switch (ent->type) {
		case SCHED_INT:
			pkd_sample_append_uint(s, i + 1, ent->val.i);
			break;
		case SCHED_DOUBLE:
			//pkd_sample_append_double(s, i + 1, ent->val.d);
			pkd_sample_append_uint(s, i + 1, ent->val.d);
			break;
		case SCHED_UNKNOWN:
		default:
			g_warn_if_reached();
			break;
		}
	}
}

static void
sched_sample (PkdSourceSimple *source,
              gpointer         user_data)
{
	Sched *sched = user_data;
	SchedEntry *ent;
	GArray *entries;
	PkdSample *s;
	gint linesRead, i;

	entries = g_array_sized_new(FALSE, FALSE, sizeof(SchedEntry), 64);

	if (!(linesRead = sched_parse(sched, entries))) {
		goto cleanup;
	}

	if (G_UNLIKELY(!sched->manifest)) {
		sched->manifest = pkd_manifest_new();
		populate_manifest(sched->manifest, entries);
		pkd_source_deliver_manifest(PKD_SOURCE(source), sched->manifest);
	}

	s = pkd_sample_new();
	populate_sample(s, entries);
	pkd_source_deliver_sample(PKD_SOURCE(source), s);
	pkd_sample_unref(s);

cleanup:
	for (i = 0; i < entries->len; i++) {
		ent = &g_array_index(entries, SchedEntry, i);
		g_free(ent->name);
	}

	g_array_unref(entries);
}

static void
sched_spawn (PkdSourceSimple *source,
             PkdSpawnInfo    *spawn_info,
             gpointer         user_data)
{
	Sched *sched = user_data;
	sched->pid = spawn_info->pid;
}

static void
sched_free (gpointer data)
{
	Sched *sched = data;

	if (sched->manifest) {
		pkd_manifest_unref(sched->manifest);
	}

	g_slice_free(Sched, data);
}

PkdSource*
sched_new (void)
{
	Sched *sched;

	sched = g_slice_new0(Sched);
	return pkd_source_simple_new_full(sched_sample,
	                                  sched_spawn,
	                                  sched,
	                                  sched_free);
}

const PkdStaticSourceInfo pkd_source_plugin = {
	.uid         = "Scheduler",
	.name        = "scheduler Data Source",
	.description = "This source provides scheduler usage of a target process.",
	.version     = "0.1.0",
	.factory     = sched_new,
};
