/**
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <glib.h>

#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__ 1

G_BEGIN_DECLS

gboolean dispatcher_init (GMainLoop * mainloop);
gboolean dispatcher_shutdown (void);
gboolean dispatch_url (const gchar * url);

G_END_DECLS

#endif /* __DISPATCHER_H__ */
