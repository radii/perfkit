/* egg-line.c
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 
 * 02110-1301 USA
 */

#include <readline/readline.h>
#include <readline/history.h>

#include "egg-line.h"

struct _EggLinePrivate
{
	EggLineEntry *entries;
	gchar        *prompt;
	gboolean      quit;
};

G_DEFINE_TYPE (EggLine, egg_line, G_TYPE_OBJECT)

static EggLine *current = NULL;

static void
egg_line_finalize (GObject *object)
{
	G_OBJECT_CLASS (egg_line_parent_class)->finalize (object);
}

static void
egg_line_class_init (EggLineClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = egg_line_finalize;

	g_type_class_add_private (object_class, sizeof(EggLinePrivate));
}

static void
egg_line_init (EggLine *line)
{
	line->priv = G_TYPE_INSTANCE_GET_PRIVATE (line,
	                                          EGG_TYPE_LINE,
	                                          EggLinePrivate);
	line->priv->quit = FALSE;
	line->priv->prompt = g_strdup ("> ");
}

static EggLineEntry *last_entries = NULL;

static gchar*
get_last_word (void)
{
	struct readline_state state;
	gboolean in_space = FALSE;
	gchar *buf, *end, *word;

	rl_save_state (&state);

	buf = state.buffer + strlen (state.buffer);

	while (buf > state.buffer && !in_space) {
		if (whitespace (*buf))
			in_space = TRUE;
		buf--;
	}

	end = buf;

	while (buf > state.buffer && !whitespace (*buf))
		buf--;

	if (!in_space)
		return g_strdup ("");

	word = g_malloc0 (end - buf + 2);
	memcpy (word, buf, end - buf + 1);
	g_strstrip (word);

	rl_restore_state (&state);

	return word;
}

static gchar*
egg_line_generator (const gchar *text,
                    gint         state)
{
	EggLineEntry *entries;
	static gint   list_index,
	              len,
	              i;
	gchar        *name,
	             *last_word;

	if (!current || !current->priv->entries)
		return NULL;

	entries = last_entries ? last_entries : current->priv->entries;

	if (!state) {
		list_index = 0;
		len = strlen (text);
	}

	last_word = get_last_word ();

	if (last_entries && entries && last_word) {
		for (i = 0; entries [i].name; i++) {
			if (g_str_equal (entries [i].name, last_word)) {
				if (entries [i].generator)
					entries = entries [i].generator (current, text, NULL);
				else
					entries = NULL;
				break;
			}
		}
	}
	else if (strlen (last_word)) {
		entries = NULL;
	}

	g_free (last_word);

	while (entries && NULL != (name = entries [list_index].name)) {
		list_index++;
		if (g_ascii_strncasecmp (name, text, len) == 0) {
			return g_strdup (name);
		}
	}

	last_entries = entries;

	return NULL;
}

static gchar**
egg_line_completion (const gchar *text,
                     gint         start,
                     gint         end)
{
	gchar **matches = NULL;

	if (start == 0)
		last_entries = NULL;

	matches = rl_completion_matches (text, egg_line_generator);

	if (!last_entries)
		return NULL;

	return matches;
}

/**
 * egg_line_new:
 *
 * Creates a new instance of #EggLine.
 *
 * Return value: the newly created #EggLine instance.
 */
EggLine*
egg_line_new ()
{
	return g_object_new (EGG_TYPE_LINE, NULL);
}

/**
 * egg_line_quit:
 * @line: An #EggLine
 *
 * Quits the readline loop after the current line has completed.
 */
void
egg_line_quit (EggLine *line)
{
	g_return_if_fail (EGG_IS_LINE (line));
	line->priv->quit = TRUE;
}

/**
 * egg_line_run:
 * @line: A #EggLine
 *
 * Blocks running the readline interaction using stdin and stdout.
 */
void
egg_line_run (EggLine *line)
{
	EggLinePrivate *priv;
	gchar          *text;

	g_return_if_fail (EGG_IS_LINE (line));

	current = line;
	priv = line->priv;
	priv->quit = FALSE;

	rl_readline_name = "egg-line";
	rl_attempted_completion_function = egg_line_completion;

	while (!priv->quit) {
		text = readline (priv->prompt);

		if (!text)
			break;

		if (*text) {
			add_history (text);
			egg_line_execute (line, text);
		}
	}

	g_print ("\n");
	current = NULL;
}

/**
 * egg_line_set_entries:
 * @line: A #EggLine
 * @entries: A %NULL terminated array of #EggLineEntry
 *
 * Sets the top-level set of #EggLineEntry<!-- -->'s to be completed
 * during runtime.
 */
void
egg_line_set_entries (EggLine      *line,
                      EggLineEntry *entries)
{
	g_return_if_fail (EGG_IS_LINE (line));
	line->priv->entries = entries;
}

/**
 * egg_line_set_prompt:
 * @line: An #EggLine
 * @prompt: a string containing the prompt
 *
 * Sets the line prompt.
 */
void
egg_line_set_prompt (EggLine     *line,
                     const gchar *prompt)
{
	EggLinePrivate *priv;

	g_return_if_fail (EGG_IS_LINE (line));
	g_return_if_fail (prompt != NULL);

	priv = line->priv;

	if (priv->prompt)
		g_free (priv->prompt);

	priv->prompt = g_strdup (prompt);
}

/**
 * egg_line_execute:
 * @line: An #EggLine
 * @text: the command to execute
 *
 * Executes the command as described by @text.
 */
void
egg_line_execute (EggLine     *line,
                  const gchar *text)
{
	EggLinePrivate  *priv;
	EggLineEntry    *entries,
					*command = NULL;
	gchar          **parts,
				   **tmp;
	gint             i;
	gboolean         found;

	g_return_if_fail (EGG_IS_LINE (line));
	g_return_if_fail (text != NULL);

	priv = line->priv;
	entries = priv->entries;
	tmp = parts = g_strsplit_set (text, " \t", -1);

	for (tmp = parts; *tmp && strlen (*tmp); tmp++) {
		found = FALSE;

		for (i = 0; entries && entries [i].name; i++) {
			if (g_str_equal (entries [i].name, *tmp)) {
				found = TRUE;
				command = &entries [i];
				if (entries [i].generator)
					entries = entries [i].generator (line, "", NULL);
				else
					entries = NULL;
				break;
			}
		}

		if (!found)
			goto finish;
	}

finish:
	if (command && command->callback)
		command->callback (line, tmp);

	g_strfreev (parts);
}
