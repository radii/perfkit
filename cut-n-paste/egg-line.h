/* egg-line.h
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

#ifndef __EGG_LINE_H__
#define __EGG_LINE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define EGG_TYPE_LINE             (egg_line_get_type ())
#define EGG_LINE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_LINE, EggLine))
#define EGG_LINE_CONST(obj)       (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_LINE, EggLine const))
#define EGG_LINE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  EGG_TYPE_LINE, EggLineClass))
#define EGG_IS_LINE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_LINE))
#define EGG_IS_LINE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  EGG_TYPE_LINE))
#define EGG_LINE_GET_CLASS(obj)	  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EGG_TYPE_LINE, EggLineClass))

typedef struct _EggLine         EggLine;
typedef struct _EggLineClass    EggLineClass;
typedef struct _EggLinePrivate  EggLinePrivate;
typedef struct _EggLineEntry    EggLineEntry;

typedef enum
{
	EGG_LINE_OK,
	EGG_LINE_BAD_ARGS,
	EGG_LINE_FAILURE,
} EggLineStatus;

typedef EggLineEntry* (*EggLineGenerator) (EggLine      *line,
                                           const gchar  *text,
                                           gchar       **end);
typedef EggLineStatus (*EggLineCallback)  (EggLine      *line,
                                           gchar       **args);

struct _EggLine
{
	GObject parent;

	/*< private >*/
	EggLinePrivate *priv;
};

struct _EggLineClass
{
	GObjectClass parent_class;
};

struct _EggLineEntry
{
	gchar             *name;
	EggLineGenerator   generator;
	EggLineCallback    callback;
	gchar             *help;
	gchar             *usage;
};

GType    egg_line_get_type    (void) G_GNUC_CONST;
EggLine* egg_line_new         (void);
void     egg_line_execute     (EggLine            *line,
                               const gchar        *text);
void     egg_line_run         (EggLine            *line);
void     egg_line_set_entries (EggLine            *line,
                               EggLineEntry       *entries);
void     egg_line_set_prompt  (EggLine            *line,
                               const gchar        *prompt);
void     egg_line_show_help   (EggLine            *line,
                               const EggLineEntry *entries);

G_END_DECLS

#endif /* __EGG_LINE_H__ */
