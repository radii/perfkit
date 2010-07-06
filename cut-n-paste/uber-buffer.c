/* uber-buffer.c
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>

#include "uber-buffer.h"

#define DEFAULT_SIZE (64)
#ifndef g_realloc_n
#define g_realloc_n(a,b,c) g_realloc(a, b * c)
#endif

/**
 * SECTION:uber-buffer
 * @title: UberBuffer
 * @short_description: A ciricular buffer for #gdouble<!-- -->'s.
 *
 * #UberBuffer is a circular buffer which is designed to store
 * #gdouble<!-- -->'s.  uber_buffer_foreach() is a macro which can
 * be used to iterate through the values in the buffer.
 *
 * The default #gdouble value is -INFINITY.
 */

/**
 * uber_buffer_dispose:
 * @buffer: A #UberBuffer.
 *
 * Cleans up the #UberBuffer instance and frees any allocated resources.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_buffer_dispose (UberBuffer *buffer) /* IN */
{
	g_free(buffer->buffer);
}

/**
 * uber_buffer_clear_range:
 * @buffer: A #UberBuffer.
 * @begin: The beginning index.
 * @end: The ending index.
 *
 * Clears a continguous range of values from the buffer by setting them to the
 * default value (-INFINITY).
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
uber_buffer_clear_range (UberBuffer *buffer, /* IN */
                         gint        begin,  /* IN */
                         gint        end)    /* IN */
{
	gint i;

	g_return_if_fail(buffer != NULL);

	for (i = begin; i < end; i++) {
		buffer->buffer[i] = -INFINITY;
	}
}

/**
 * uber_buffer_new:
 *
 * Creates a new instance of #UberBuffer.
 *
 * Returns: the newly created instance which should be freed with
 *   uber_buffer_unref().
 * Side effects: None.
 */
UberBuffer*
uber_buffer_new (void)
{
	UberBuffer *buffer;

	buffer = g_slice_new0(UberBuffer);
	buffer->ref_count = 1;
	buffer->buffer = g_new(gdouble, DEFAULT_SIZE);
	buffer->len = DEFAULT_SIZE;
	buffer->pos = 0;
	uber_buffer_clear_range(buffer, 0, DEFAULT_SIZE);
	return buffer;
}

/**
 * uber_buffer_set_size:
 * @buffer: A #UberBuffer.
 * @size: The number of elements that @buffer should contain.
 *
 * Resizes the circular buffer.
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_buffer_set_size (UberBuffer *buffer, /* IN */
                      gint        size)   /* IN */
{
	gint count;

	g_return_if_fail(buffer != NULL);
	g_return_if_fail(size > 0);

	if (size == buffer->len) {
		return;
	}
	if (size > buffer->len) {
		buffer->buffer = g_realloc_n(buffer->buffer, size, sizeof(gdouble));
		uber_buffer_clear_range(buffer, buffer->len, size);
		if ((count = buffer->len - buffer->pos)) {
			memmove(&buffer->buffer[size - count],
			        &buffer->buffer[buffer->pos],
			        count * sizeof(gdouble));
			if (size - count > buffer->pos) {
				uber_buffer_clear_range(buffer, 0, size - count - buffer->pos);
			}
		}
		buffer->len = size;
		return;
	}
	if (size >= buffer->pos) {
		memmove(&buffer->buffer[buffer->pos],
		        &buffer->buffer[size],
		        (buffer->len - size) * sizeof(gdouble));
		buffer->buffer = g_realloc_n(buffer->buffer, size, sizeof(gdouble));
		buffer->len = size;
		return;
	}
	memmove(buffer->buffer, &buffer->buffer[buffer->pos - size],
	        size * sizeof(gdouble));
	buffer->buffer = g_realloc_n(buffer->buffer, size, sizeof(gdouble));
	buffer->pos = 0;
	buffer->len = size;
}

/**
 * uber_buffer_append:
 * @buffer: A #UberBuffer.
 * @value: A #gdouble.
 *
 * Appends a new value onto the circular buffer.
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_buffer_append (UberBuffer *buffer, /* IN */
                    gdouble     value)  /* IN */
{
	g_return_if_fail(buffer != NULL);

	buffer->buffer[buffer->pos++] = value;
	if (buffer->pos >= buffer->len) {
		buffer->pos = 0;
	}
}

gdouble
uber_buffer_get_index (UberBuffer *buffer, /* IN */
                       gint        idx)    /* IN */
{
	g_return_val_if_fail(buffer != NULL, -INFINITY);
	g_return_val_if_fail(idx < buffer->len, -INFINITY);

	if (buffer->pos > idx) {
		return buffer->buffer[buffer->pos - idx - 1];
	}
	idx -= buffer->pos;
	return buffer->buffer[buffer->len - idx - 1];
}

/**
 * UberBuffer_ref:
 * @buffer: A #UberBuffer.
 *
 * Atomically increments the reference count of @buffer by one.
 *
 * Returns: A reference to @buffer.
 * Side effects: None.
 */
UberBuffer*
uber_buffer_ref (UberBuffer *buffer) /* IN */
{
	g_return_val_if_fail(buffer != NULL, NULL);
	g_return_val_if_fail(buffer->ref_count > 0, NULL);

	g_atomic_int_inc(&buffer->ref_count);
	return buffer;
}

/**
 * uber_buffer_unref:
 * @buffer: A UberBuffer.
 *
 * Atomically decrements the reference count of @buffer by one.  When the
 * reference count reaches zero, the structure will be destroyed and
 * freed.
 *
 * Returns: None.
 * Side effects: The structure will be freed when the reference count
 *   reaches zero.
 */
void
uber_buffer_unref (UberBuffer *buffer) /* IN */
{
	g_return_if_fail(buffer != NULL);
	g_return_if_fail(buffer->ref_count > 0);

	if (g_atomic_int_dec_and_test(&buffer->ref_count)) {
		uber_buffer_dispose(buffer);
		g_slice_free(UberBuffer, buffer);
	}
}
