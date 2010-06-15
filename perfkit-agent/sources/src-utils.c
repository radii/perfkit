/* src-utils.c
 *
 * Copyright (C) 2010 Andrew Stiegmann <andrew.stiegmann@gmail.com>
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

/*
 * XXX: We should move this to a common utils directory sometime in
 *      the near future.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>

#include "src-utils.h"


/**
 * src_utils_str_tok:
 * @delim: (IN) The delimeter.
 * @*ptr: (IN) The string to tokenize.
 * @Returns: Pointer to the beginning of the string after the token or
 *           NULL if at the end of the string
 *
 * Tokenizes string inline by replacing the delimiting charcter with a
 * \0.
 *
 **/
gchar*
src_utils_str_tok (const gchar delim, /* IN */
                   gchar *ptr) /* IN */
{
   // Would like to use a switch here.
   while(1) {
      if (*ptr == '\0') {
         return NULL;
      } else if (*ptr == delim) {
         *ptr = '\0';
         return ++ptr;
      } else {
         ptr++;
      }
   }
} /* src_utils_str_tok */


/**
 * src_utils_read_file:
 * @*path: (IN) Path to file.
 * @*buffer: (IN/OUT) The buffer to read into.  Can be NULL.
 * @bsize: (IN) The size of the buffer if buffer is not NULL.
 * @Returns: pointer to a buffer containg file data.
 *
 * Will read a file into a given buffer or, if buffer is NULL, allocate
 * a new buffer and read into it.  If you do not give this function a
 * buffer, you are responsible for freeing the buffer when you are done
 * using it.
 *
 **/
gchar*
src_utils_read_file (const gchar *path, /* IN */
            gchar *buffer, /* IN/OUT */
            gssize bsize) /* IN */
{
   gint fd;
   ssize_t bytesRead;

   g_return_val_if_fail(path != NULL, NULL);

   if (buffer != NULL) {
      fd = open(path, O_RDONLY);

      if (fd < 0) {
         return NULL;
      }

      bytesRead = read(fd, buffer, bsize-1);
      close(fd);

      if (bytesRead < 0) {
         return NULL;
      }

      buffer[bytesRead] = '\0';
      return buffer;
   } else {
      gchar *contents;
      g_file_get_contents(path, &contents, NULL, NULL);
      return contents;
   }
} /* src_utils_read_file */
