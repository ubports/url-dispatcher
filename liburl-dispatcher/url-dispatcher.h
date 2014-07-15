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

#ifndef __URL_DISPATCH_H__
#define __URL_DISPATCH_H__ 1

#pragma GCC visibility push(default)

G_BEGIN_DECLS

typedef void (*URLDispatchCallback) (const gchar * url, gboolean success, gpointer user_data);

void       url_dispatch_send            (const gchar *         url,
                                         URLDispatchCallback   cb,
                                         gpointer              user_data);

void       url_dispatch_send_restricted (const gchar *         url,
                                         const gchar *         package,
                                         URLDispatchCallback   cb,
                                         gpointer              user_data);

gchar **   url_dispatch_url_appid       (const gchar **        urls);

G_END_DECLS

#pragma GCC visibility pop

#endif /* __URL_DISPATCH_H__ */
