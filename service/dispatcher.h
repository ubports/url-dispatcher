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
 * Author: Ted Gould <ted@canonical.com>
 *
 */

#ifndef DISPATCHER_H
#define DISPATCHER_H 1

#include <glib.h>
#include "overlay-tracker.h"

G_BEGIN_DECLS

gboolean dispatcher_init (GMainLoop * mainloop, OverlayTracker * tracker);
gboolean dispatcher_shutdown (void);
gboolean dispatcher_url_to_appid (const gchar * url, gchar ** out_appid, const gchar ** out_url);
gboolean dispatcher_appid_restrict (const gchar * appid, const gchar * package);
gboolean dispatcher_send_to_app (const gchar * appid, const gchar * url);

G_END_DECLS

#endif /* DISPATCHER_H */
