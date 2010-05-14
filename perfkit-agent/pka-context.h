/* pka-context.h
 *
 * Copyright 2010 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PKA_CONTEXT_H__
#define __PKA_CONTEXT_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKA_TYPE_CONTEXT  (pka_context_get_type())
#define PKA_CONTEXT_ERROR (pka_context_error_quark())

typedef struct _PkaContext PkaContext;

typedef enum
{
	PKA_CONTEXT_ERROR_NOT_AUTHENTICATED,
	PKA_CONTEXT_ERROR_NOT_AUTHORIZED,
} PkaContextError;

typedef enum
{
	PKA_IOCTL_INVALID = 0,
	PKA_IOCTL_ADD_CHANNEL,
	PKA_IOCTL_ADD_ENCODER,
	PKA_IOCTL_ADD_SOURCE,
	PKA_IOCTL_ADD_SUBSCRIPTION,
	PKA_IOCTL_REMOVE_CHANNEL,
	PKA_IOCTL_REMOVE_ENCODER,
	PKA_IOCTL_REMOVE_SOURCE,
	PKA_IOCTL_REMOVE_SUBSCRIPTION,
} PkaIOControl;

GQuark      pka_context_error_quark      (void) G_GNUC_CONST;
GType       pka_context_get_type         (void) G_GNUC_CONST;
gboolean    pka_context_is_authenticated (PkaContext   *context);
gboolean    pka_context_is_authorized    (PkaContext   *context,
                                          PkaIOControl  ioctl);
PkaContext* pka_context_new              (void);
PkaContext* pka_context_ref              (PkaContext   *context);
void        pka_context_unref            (PkaContext   *context);
PkaContext* pka_context_default          (void);
guint       pka_context_get_id           (PkaContext   *context);

G_END_DECLS

#endif /* __PKA_CONTEXT_H__ */

