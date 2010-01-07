/* egg-buffer.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "egg-buffer.h"

/**
 * SECTION:egg-buffer
 * @title: EggBuffer
 * @short_description: Protobuf inspired encoder
 *
 * #EggEncoder is an encoder that can write data into a buffer using the
 * same sort of formats as Google's Protocol Buffers.  This is just the work
 * that encodes the data in and out of a buffer.  For a implementation of
 * of Protocol Buffers, you would need a message compiler that uses this
 * to do the writing and reading.
 */

struct _EggBuffer
{
	volatile gint ref_count;

	GByteArray *ar;
	gsize       pos;
};

static void
egg_buffer_destroy (EggBuffer *buffer)
{
	g_byte_array_unref(buffer->ar);
}

/**
 * egg_buffer_new:
 *
 * Creates a new instance of #EggBuffer.
 *
 * Returns: The newly created instance of #EggBuffer.
 *
 * Side effects: None.
 */
EggBuffer*
egg_buffer_new (void)
{
	EggBuffer *buffer;

	buffer = g_slice_new0(EggBuffer);
	buffer->ar = g_byte_array_sized_new(32);
	buffer->ref_count = 1;

	return buffer;
}

/**
 * egg_buffer_new_from_data:
 * @data: A buffer of encoded data.
 * @len: The length of @data.
 *
 * Creates a new instance of #EggBuffer using the encoded buffer contained
 * within @data.
 *
 * Returns: The newly created instance of #EggBuffer.
 *
 * Side effects: None.
 */
EggBuffer*
egg_buffer_new_from_data (const guint8 *data,
                          gsize         len)
{
	EggBuffer *buffer;

	buffer = g_slice_new0(EggBuffer);
	buffer->ref_count = 1;
	buffer->ar = g_byte_array_sized_new(len);
	g_byte_array_append(buffer->ar, data, len);

	return buffer;
}

/**
 * egg_buffer_write_int:
 * @buffer: An #EggBuffer.
 * @i: An integer to encode.
 *
 * Encodes the integer @i onto the end of the buffer.
 */
void
egg_buffer_write_int (EggBuffer *buffer,
                      gint       i)
{
	egg_buffer_write_uint(buffer, ((i << 1) ^ (i >> 31)));
}

/**
 * egg_buffer_write_int64:
 * @buffer: An #EggBuffer.
 * @i: A 64-bit signed integer to encoded.
 *
 * Encodes the 64-bit signed integer @i using zigzag encoding.
 *
 * Side effects: None.
 */
void
egg_buffer_write_int64 (EggBuffer *buffer,
                        gint64     i)
{
	egg_buffer_write_uint64(buffer, ((i << 1) ^ (i >> 63)));
}

/**
 * egg_buffer_write_uint:
 * @buffer: An #EggBuffer.
 * @i: an unsigned integer to encode.
 *
 * Encodes the unsigned integer @i onto the end of the buffer.
 *
 * Side effects: None.
 */
void
egg_buffer_write_uint (EggBuffer *buffer,
                       guint      i)
{
	guint8 b;

	g_return_if_fail(buffer != NULL);

	/*
	 * NOTES:
	 *
	 *   This encodes the integer starting from the least significant byte
	 *   to the most significant byte.  The Most-Significant-Bit of each
	 *   byte is used to indicate if there are more bytes following.  Bit-On
	 *   means that there is another byte.  Bit-Off means it is the last byte.
	 *
	 */

	do {
		b = ((i > 0x7F) << 7) | (i & 0x7F);
		g_byte_array_append(buffer->ar, &b, 1);
		i >>= 7;
	} while (i > 0);
}

/**
 * egg_buffer_write_uint64:
 * @buffer: An #EggBuffer.
 * @i: A 64-bit unsigned integer to encode.
 *
 * Encodes the 64-bit unsigned integer @i onto the end of the buffer.
 *
 * Side effects: None.
 */
void
egg_buffer_write_uint64 (EggBuffer *buffer,
                         guint64    i)
{
	guint8 b;

	g_return_if_fail(buffer != NULL);

	do {
		b = ((i > 0x7F) << 7) | (i & 0x7F);
		g_byte_array_append(buffer->ar, &b, 1);
		i >>= 7;
	} while (i > 0);
}

/**
 * egg_buffer_write_string:
 * @buffer: An #EggBuffer.
 * @s: A string to append to the buffer or %NULL.
 *
 * Appends string @s to the buffer.
 *
 * Side effects: None.
 */
void
egg_buffer_write_string (EggBuffer   *buffer,
                         const gchar *s)
{
	gsize l;

	g_return_if_fail(buffer != NULL);

	l = s ? strlen(s) : 0;

	/*
	 * Write the string length (No NULL byte).
	 */
	egg_buffer_write_uint(buffer, l);

	/*
	 * Write the string if needed.
	 */
	if (s) {
		g_byte_array_append(buffer->ar, (guint8 *)s, l);
	}
}

/**
 * egg_buffer_write_boolean:
 * @buffer: An #EggBuffer.
 * @b: A #gboolean.
 *
 * Appends the boolean value @b to the buffer.
 *
 * Side effects: None.
 */
void
egg_buffer_write_boolean (EggBuffer *buffer,
                          gboolean   b)
{
	guint8 b_;

	g_return_if_fail(buffer != NULL);

	b_ = b ? 0x01 : 0x00;
	g_byte_array_append(buffer->ar, &b_, 1);
}

/**
 * egg_buffer_write_double:
 * @buffer: An #EggBuffer.
 * @d: A #gdouble to encode.
 *
 * Encodes the double @d onto the end of the buffer.
 *
 * Side effects: None.
 */
void
egg_buffer_write_double (EggBuffer *buffer,
                         gdouble    d)
{
	gdouble d_;

	g_return_if_fail(buffer != NULL);

	/*
	 * Doubles are stored as 64-bit blobs in Little-Endian format.
	 */
	d_ = GUINT64_TO_LE(d);
	g_byte_array_append(buffer->ar, ((guint8 *)&d_), 8);
}

/**
 * egg_buffer_write_float:
 * @buffer: An #EggBuffer.
 * @f: A #gfixed to encode.
 *
 * Encodes the fixed @f onto the end of the buffer.
 *
 * Side effects: None.
 */
void
egg_buffer_write_float (EggBuffer *buffer,
                        gfloat     f)
{
	gfloat f_;

	g_return_if_fail(buffer != NULL);

	/*
	 * Floats are stored as 32-bit blobs in Little-Endian format.
	 */
	f_ = GUINT32_TO_LE(f);
	g_byte_array_append(buffer->ar, ((guint8 *)&f_), 4);
}

/**
 * egg_buffer_write_tag:
 * @buffer: An #EggBuffer
 * @field: The field identifier.
 * @tag: An #EggBufferTag
 *
 * Writes the tag onto the end of the buffer.  This is the same as:
 *
 * [[
 * egg_buffer_write_uint(buffer, (field << 3) | tag);
 * ]]
 *
 * Side effects: None.
 */
void
egg_buffer_write_tag (EggBuffer    *buffer,
                      guint         field,
                      EggBufferTag  tag)
{
	g_return_if_fail(buffer != NULL);
	g_return_if_fail(tag == 0 || tag == 1 || tag == 2 || tag == 5);
	egg_buffer_write_uint(buffer, (field << 3) | tag);
}

/**
 * egg_buffer_write_data:
 * @buffer: An #EggBuffer.
 * @data: A buffer to write.
 * @len: The length of the buffer.
 *
 * Writes @len bytes of @data to the #EggBuffer.
 *
 * Side effects: None.
 */
void
egg_buffer_write_data (EggBuffer    *buffer,
                       const guint8 *data,
                       gsize         len)
{
	g_return_if_fail(buffer != NULL);
	g_return_if_fail(len <= G_MAXUINT);

	egg_buffer_write_uint(buffer, len);
	g_byte_array_append(buffer->ar, data, len);
}

/**
 * egg_buffer_get_buffer:
 * @buffer: An #EggBuffer.
 * @data: A location for the buffer.
 * @len: A location for the buffer length.
 *
 * Retrieves the internal buffer for the #EggBuffer.  This value should not be
 * modified or freed.
 */
void
egg_buffer_get_buffer (EggBuffer     *buffer,
                       const guint8 **data,
                       gsize         *len)
{
	g_return_if_fail(buffer != NULL);
	g_return_if_fail(data != NULL);
	g_return_if_fail(len != NULL);

	*len = buffer->ar->len;
	*data = buffer->ar->data;
}

/**
 * egg_buffer_ref:
 * buffer: A #EggBuffer
 *
 * Atomically increments the reference count of @buffer by one.
 *
 * Returns: The @buffer pointer with its reference count incremented.
 *
 * Side effects: None.
 */
EggBuffer*
egg_buffer_ref (EggBuffer *buffer)
{
	g_return_val_if_fail(buffer != NULL, NULL);
	g_return_val_if_fail(buffer->ref_count > 0, NULL);

	g_atomic_int_inc(&buffer->ref_count);

	return buffer;
}

/**
 * egg_buffer_unref:
 * buffer: A #EggBuffer
 *
 * Atomically decrements the reference count of @buffer by one.
 * When the reference count reaches zero, the structures resources as well as
 * the structure are freed.
 *
 * Returns: The @buffer pointer with its reference count incremented.
 *
 * Side effects: None.
 */
void
egg_buffer_unref (EggBuffer *buffer)
{
	g_return_if_fail(buffer != NULL);
	g_return_if_fail(buffer->ref_count > 0);

	if (g_atomic_int_dec_and_test(&buffer->ref_count)) {
		egg_buffer_destroy(buffer);
		g_slice_free(EggBuffer, buffer);
	}
}

/**
 * egg_buffer_read_uint:
 * @buffer: An #EggBuffer.
 * @i: A location to store a #guint.
 *
 * Reads the next #guint value from the buffer starting at the current
 * offset.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects: None.
 */
gboolean
egg_buffer_read_uint (EggBuffer *buffer,
                      guint     *i)
{
	guint u = 0, o = 0;
	guint8 b = 0;

	g_return_val_if_fail(buffer != NULL, FALSE);
	g_return_val_if_fail(i != NULL, FALSE);

	/*
	 * NOTES:
	 *
	 *   Read each byte in the varint off the buffer.  The last byte is
	 *   denoted by the Most-Significant-Bit being Off.
	 *
	 */

	do {
		/*
		 * Ensure there is space to read and we didn't overflow 32-bit.
		 */
		if ((buffer->pos >= buffer->ar->len) || (o > 28))
			return FALSE;

		b = buffer->ar->data[buffer->pos++];
		u |= ((b & 0x7F) << o);
		o += 7;
	} while ((b & 0x80) != 0);

	*i = u;

	return TRUE;
}

/**
 * egg_buffer_read_uint64:
 * @buffer: An #EggBuffer.
 * @i: A location to store a #guint64.
 *
 * Reads the next #guint64 value from the buffer starting at the current
 * offset.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects: None.
 */
gboolean
egg_buffer_read_uint64 (EggBuffer *buffer,
                        guint64   *i)
{
	guint o = 0;
	guint8 b = 0;
	guint64 u = 0;

	g_return_val_if_fail(buffer != NULL, FALSE);
	g_return_val_if_fail(i != NULL, FALSE);

	/*
	 * NOTES:
	 *
	 *   Read each byte in the varint off the buffer.  The last byte is
	 *   denoted by the Most-Significant-Bit being Off.
	 *
	 */

	do {
		/*
		 * Ensure there is space to read and we didn't overflow 64-bit.
		 */
		if ((buffer->pos >= buffer->ar->len) || (o > 63))
			return FALSE;

		b = buffer->ar->data[buffer->pos++];
		u |= ((b & 0x7F) << o);
		o += 7;
	} while ((b & 0x80) != 0);

	*i = u;

	return TRUE;
}

/**
 * egg_buffer_read_boolean:
 * @buffer: An #EggBuffer.
 * @b: A location to store a #gboolean.
 *
 * Reads the next boolean value from the buffer starting at the current
 * offset.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects: None.
 */
gboolean
egg_buffer_read_boolean (EggBuffer *buffer,
                         gboolean  *b)
{
	g_return_val_if_fail(buffer != NULL, FALSE);
	g_return_val_if_fail(b != NULL, FALSE);

	return egg_buffer_read_uint(buffer, (guint *)b);
}

/**
 * egg_buffer_read_string:
 * @buffer: An #EggBuffer.
 * @s: A location for a string.
 *
 * Reads the next string from the buffer starting at the current offset.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects: None.
 */
gboolean
egg_buffer_read_string (EggBuffer  *buffer,
                        gchar     **s)
{
	guint u;
	gchar *m;

	g_return_val_if_fail(buffer != NULL, FALSE);
	g_return_val_if_fail(s != NULL, FALSE);

	/*
	 * NOTES:
	 *
	 *   Strings are implemented as a varint length followed by the string
	 *   with no NULL byte.  Therefore, we allocate a new string, memcpy
	 *   the string over and set the NULL byte.
	 *
	 */

	/*
	 * Get the length of the string (minus NULL byte).
	 */
	if (!egg_buffer_read_uint(buffer, &u))
		return FALSE;

	/*
	 * Get the string contents if there is enough data.
	 */
	if ((buffer->pos + u) <= buffer->ar->len) {
		m = g_malloc(u + 1);
		memcpy(m, &buffer->ar->data[buffer->pos], u);
		m[u] = '\0';

		*s = m;
		buffer->pos += u;

		return TRUE;
	}

	return FALSE;
}

/**
 * egg_buffer_read_int:
 * @buffer: An #EggBuffer.
 * @i: An location to store a #gint.
 *
 * Reads the next integer from the #EggBuffer and stores it in @i.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects: None.
 */
gboolean
egg_buffer_read_int (EggBuffer *buffer,
                     gint      *i)
{
	guint u;

	g_return_val_if_fail(buffer != NULL, FALSE);
	g_return_val_if_fail(i != NULL, FALSE);

	if (!egg_buffer_read_uint(buffer, &u))
		return FALSE;

	*i = (u >> 1) ^ -(u & 0x1);

	return TRUE;
}

/**
 * egg_buffer_read_int64:
 * @buffer: An #EggBuffer.
 * @i: An location to store a #gint64.
 *
 * Reads the next 64-bit integer from the #EggBuffer and stores it in @i.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects: None.
 */
gboolean
egg_buffer_read_int64 (EggBuffer *buffer,
                       gint64    *i)
{
	guint64 u;

	g_return_val_if_fail(buffer != NULL, FALSE);
	g_return_val_if_fail(i != NULL, FALSE);

	if (!egg_buffer_read_uint64(buffer, &u))
		return FALSE;

	*i = (u >> 1) ^ -(u & 0x1);

	return TRUE;
}

/**
 * egg_buffer_read_tag:
 * @buffer: An #EggBuffer.
 * @field: A location for the field identifier.
 * @tag: A location for the #EggBufferTag.
 *
 * Reads the tag from the current offset of the buffer.  A tag consists
 * of an #EggBufferTag type xor'd with a 3-bit shifted field offset.
 *
 * Returns: %TRUE on success; otherwise %FALSE.
 *
 * Side effects: None.
 */
gboolean
egg_buffer_read_tag (EggBuffer    *buffer,
                     guint        *field,
                     EggBufferTag *tag)
{
	guint u = 0;

	g_return_val_if_fail(buffer != NULL, FALSE);
	g_return_val_if_fail(field != NULL, FALSE);
	g_return_val_if_fail(tag != NULL, FALSE);

	if (!egg_buffer_read_uint(buffer, &u))
		return FALSE;

	*field = ((u & ~0x7) >> 3);
	*tag = (u & 0x07);

	return TRUE;
}

/**
 * egg_buffer_read_data:
 * @buffer: An #EggBuffer
 * @data: A location for the resulting data.
 * @len: A location for the resulting data length.
 *
 * Reads the upcoming data blob from the buffer.  The caller should free the
 * data returned with g_free() when it is no longer needed.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects: None.
 */
gboolean
egg_buffer_read_data (EggBuffer  *buffer,
                      guint8    **data,
                      gsize      *len)
{
	guint u = 0;
	guint8 *m;

	g_return_val_if_fail(buffer != NULL, FALSE);
	g_return_val_if_fail(data != NULL, FALSE);
	g_return_val_if_fail(len != NULL, FALSE);

	if (!egg_buffer_read_uint(buffer, &u))
		return FALSE;

	if ((buffer->pos + u) <= buffer->ar->len) {
		m = g_malloc(u);
		memcpy(m, &buffer->ar->data[buffer->pos], u);
		m[u] = '\0';

		*data = m;
		*len = u;
		buffer->pos += u;

		return TRUE;
	}

	return FALSE;
}

/**
 * egg_buffer_read_double:
 * @buffer: An #EggBuffer.
 * @d: A location for a #gdouble.
 *
 * Reads the next 8 bytes from the buffer as a double.  Doubles are encoded
 * in Little-Endian format.  It will be converted to the host byte order.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects: None.
 */
gboolean
egg_buffer_read_double (EggBuffer *buffer,
                        gdouble   *d)
{
	gdouble d_;

	g_return_val_if_fail(buffer != NULL, FALSE);
	g_return_val_if_fail(d != NULL, FALSE);

	if ((buffer->pos + 8) <= buffer->ar->len) {
		d_ = *((gdouble *)&buffer->ar->data[buffer->pos]);
		*d = GUINT64_FROM_LE(d_);
		return TRUE;
	}

	return FALSE;
}

/**
 * egg_buffer_read_float:
 * @buffer: An #EggBuffer.
 * @f: A location for a #gfloat.
 *
 * Reads the next 4 bytes of the buffer as a 32-bit, Little-Endian encoded
 * float.  The byte-order is adjusted to the host byte-order.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects: None.
 */
gboolean
egg_buffer_read_float (EggBuffer *buffer,
                       gfloat    *f)
{
	gfloat f_;

	g_return_val_if_fail(buffer != NULL, FALSE);
	g_return_val_if_fail(f != NULL, FALSE);

	if ((buffer->pos + 8) <= buffer->ar->len) {
		f_ = *((gfloat *)&buffer->ar->data[buffer->pos]);
		*f = GUINT32_FROM_LE(f_);
		return TRUE;
	}

	return FALSE;
}

GType
egg_buffer_get_type (void)
{
	static GType type_id = 0;
	GType _type_id;

	if (g_once_init_enter((gsize *)&type_id)) {
		_type_id = g_boxed_type_register_static("EggBuffer",
		                                        (GBoxedCopyFunc)egg_buffer_ref,
		                                        (GBoxedFreeFunc)egg_buffer_unref);
		g_once_init_leave((gsize *)&type_id, _type_id);
	}

	return type_id;
}
