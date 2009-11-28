/* memory.c
 * 
 * Copyright (C) 2009 Christian Hergert
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

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "plugin.h"

typedef struct
{
	gint size;
	gint resident;
	gint share;
	gint text;
	gint lib;
	gint data;
	gint dt;
} proc_pid_statm;

static gboolean
proc_pid_statm_read (proc_pid_statm *pstat,
                     GPid            pid)
{
	gint     fd;
	gchar    path[64];
	gchar    buffer[64];

	memset (path, 0, sizeof (path));
	snprintf (path, sizeof (path), "/proc/%d/statm", pid);

	fd = open (path, O_RDONLY);
	if (fd < 0)
		return FALSE;

	if (read (fd, buffer, sizeof (buffer) < 1)) {
		close (fd);
		return FALSE;
	}

	sscanf (buffer,
	        "%d %d %d %d %d %d %d",
	        &pstat->size,
	        &pstat->resident,
	        &pstat->share,
	        &pstat->text,
	        &pstat->lib,
	        &pstat->data,
	        &pstat->dt);

	close (fd);

	return TRUE;
}

static PkdSample*
memory_generate_sample (PkdSourceSimple  *source,
                        gpointer          user_data,
                        GError          **error)
{
	PkdSample      *sample;
	proc_pid_statm  info;
	GPid            pid;

	pid = (GPid)GPOINTER_TO_INT (user_data);

	memset (&info, 0, sizeof (proc_pid_statm));
	if (!proc_pid_statm_read (&info, pid))
		return FALSE;

	sample = pkd_sample_sized_new (24);

	/* Header (Version, Pad x3) */
	pkd_sample_write_char (sample, 0x01);
	pkd_sample_write_char (sample, 0x00);
	pkd_sample_write_char (sample, 0x00);
	pkd_sample_write_char (sample, 0x00);

	/* Sample data */
	pkd_sample_write_int (sample, info.size);
	pkd_sample_write_int (sample, info.resident);
	pkd_sample_write_int (sample, info.share);
	pkd_sample_write_int (sample, info.text);
	pkd_sample_write_int (sample, info.data);

	return sample;
}

PKD_SOURCE_SIMPLE_REGISTER ("memory", memory_generate_sample)
