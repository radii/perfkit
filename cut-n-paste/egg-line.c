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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <readline/readline.h>
#include <readline/history.h>

#include "egg-line.h"

struct _EggLinePrivate
{
	EggLineCommand *commands;
	gchar          *prompt;
	gboolean        quit;
};

static EggLineCommand empty[] = {
	{ NULL }
};

enum
{
	SIGNAL_MISSING,
	SIGNAL_LAST
};

G_DEFINE_TYPE (EggLine, egg_line, G_TYPE_OBJECT)

static guint    signals [SIGNAL_LAST];
static EggLine *current = NULL;

static void
egg_line_finalize (GObject *object)
{
	G_OBJECT_CLASS (egg_line_parent_class)->finalize (object);
}

static void
egg_line_class_init (EggLineClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = egg_line_finalize;
	g_type_class_add_private (object_class, sizeof (EggLinePrivate));

	/**
	 * EggLine::missing:
	 * @text: a string containing the entered text
	 *
	 * The "missing" signal.
	 */
	signals [SIGNAL_MISSING] = g_signal_new ("missing",
		                                     EGG_TYPE_LINE,
		                                     G_SIGNAL_RUN_LAST,
		                                     0,
		                                     NULL,
		                                     NULL,
		                                     g_cclosure_marshal_VOID__STRING,
		                                     G_TYPE_NONE,
		                                     1,
		                                     G_TYPE_STRING);
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

static gchar*
egg_line_generator (const gchar *text,
                    gint         state)
{
	EggLineCommand  *command;
	static gint      list_index,
	                 len   = 0,
	                 argc  = 0;
	const gchar     *name;
	gchar           *tmp,
	               **argv  = NULL,
	               **largv = NULL;

	if (!current || !text || !current->priv->commands)
		return NULL;

	command = egg_line_resolve (current, rl_line_buffer, &argc, &argv);
	largv = argv;

	if (command) {
		if (command->generator)
			command = command->generator (current, &argc, &argv);
		else
			command = empty;
	}
	else {
		command = current->priv->commands;
	}

	if (argv && argv [0])
		tmp = g_strdup (argv [0]);
	else
		tmp = g_strdup ("");

	g_strfreev (largv);

	if (!state)
		list_index = 0;

	len = strlen (tmp);

	while (NULL != (name = command [list_index].name)) {
		list_index++;
		if ((g_ascii_strncasecmp (name, tmp, len) == 0)) {
			return g_strdup (name);
		}
	}

	return NULL;
}

static gchar**
egg_line_completion (const gchar *text,
                     gint         start,
                     gint         end)
{
	return rl_completion_matches (text, egg_line_generator);
}

/**
 * egg_line_new:
 *
 * Creates a new instance of #EggLine.
 *
 * Return value: the newly created #EggLine instance.
 */
EggLine*
egg_line_new (void)
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
 * egg_line_set_commands:
 * @line: A #EggLine
 * @entries: A %NULL terminated array of #EggLineCommand
 *
 * Sets the top-level set of #EggLineCommand<!-- -->'s to be completed
 * during runtime.
 */
void
egg_line_set_commands (EggLine              *line,
                       const EggLineCommand *entries)
{
	g_return_if_fail (EGG_IS_LINE (line));
	line->priv->commands = (EggLineCommand*) entries;
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
	EggLineStatus     result;
	EggLineCommand   *command;
	GError           *error = NULL;
	gchar           **argv  = NULL;
	gint              argc  = 0;

	g_return_if_fail (EGG_IS_LINE (line));
	g_return_if_fail (text != NULL);

	command = egg_line_resolve (line, text, &argc, &argv);

	if (command && command->callback) {
		result = command->callback (line, argc, argv, &error);
		switch (result) {
		case EGG_LINE_STATUS_OK:
			break;
		case EGG_LINE_STATUS_BAD_ARGS:
			egg_line_show_usage (line, command);
			break;
		case EGG_LINE_STATUS_FAILURE:
			g_printerr ("EGG_LINE_ERROR: %s\n", error->message);
			g_error_free (error);
			break;
		default:
			break;
		}
	}
	else if (command && command->usage) {
		egg_line_show_usage (line, command);
	}
	else {
		g_signal_emit (line, signals [SIGNAL_MISSING], 0, text);
	}

	g_strfreev (argv);
}

/**
 * egg_line_execute_file:
 * @line: An #EggLine
 * @filename: a filename
 *
 * 
 */
void
egg_line_execute_file (EggLine     *line,
                       const gchar *filename)
{
	GIOChannel *channel;
	GError     *error      = NULL;
	gchar      *str_return = NULL;

	g_return_if_fail (EGG_IS_LINE (line));

	if (!(channel = g_io_channel_new_file (filename, "r", &error))) {
		g_printerr ("%s\n", error->message);
		g_error_free (error);
		return;
	}

	while (G_IO_STATUS_NORMAL == g_io_channel_read_line (channel,
	                                                     &str_return,
	                                                     NULL,
	                                                     NULL,
	                                                     NULL)) {
	    g_strstrip (str_return);

	    if (!g_str_has_prefix (str_return, "#") && strlen (str_return))
	    	egg_line_execute (line, str_return);
	    g_free (str_return);
	}

	g_io_channel_unref (channel);
}

/**
 * egg_line_resolve:
 * @line: An #EggLine
 * @text: command text
 *
 * Resolves a command and arguments for @text.
 *
 * Return value: the instance of #EggLineCommand.  This value should not be
 *   modified or freed.
 */
EggLineCommand*
egg_line_resolve (EggLine       *line,
                  const gchar   *text,
                  gint          *argc,
                  gchar       ***argv)
{
	EggLineCommand  *command = NULL,
	                *tmp     = NULL,
	                *result  = NULL;
	gchar          **largv   = NULL,
	               **origv   = NULL;
	gint             largc   = 0,
	                 i;
	GError          *error   = NULL;

	g_return_val_if_fail (EGG_IS_LINE (line), NULL);
	g_return_val_if_fail (text != NULL, NULL);

	if (argc)
		*argc = 0;

	if (argv)
		*argv = NULL;

	if (strlen (text) == 0)
		return NULL;

	if (!g_shell_parse_argv (text, &largc, &largv, &error)) {
		g_printerr ("%s\n", error->message);
		g_error_free (error);
		return NULL;
	}

	command = line->priv->commands;
	origv = largv;

	for (i = 0; largv [0] && command [i].name;) {
		if (g_str_equal (largv [0], command [i].name)) {
			if (command [i].generator) {
				tmp = command [i].generator (line, &largc, &largv);
			}

			result = &command [i];
			command = tmp ? tmp : empty;

			i = 0;
			largv = &largv [1];
			largc--;
		}
		else i++;
	}

	if (argv)
		*argv = largv ? g_strdupv (largv) : NULL;

	if (argc)
		*argc = largc;

	g_strfreev (origv);

	return result;
}

/**
 * egg_line_show_usage:
 * @line: An #EggLine
 * @command: An #EggLineCommand
 *
 * Shows command usage for @command.
 */
void
egg_line_show_usage (EggLine              *line,
                     const EggLineCommand *command)
{
	g_return_if_fail (EGG_IS_LINE (line));
	g_return_if_fail (command != NULL);

	g_print ("usage: %s\n", command->usage ? command->usage : "");
}
