/* src-utils.h
 *
 * Copyright (C) 2010 Andrew Stiegmann (stieg)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published byg
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

#ifndef __SRC_UTILS_H__
#define __SRC_UTILS_H__

#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <unistd.h>

G_BEGIN_DECLS

gchar* src_utils_str_tok(const gchar,
                         gchar*);

gchar* src_utils_read_file(const char*,
                           gchar*,
                           gssize);

G_END_DECLS

#endif /* __SRC_UTILS_H__ */
