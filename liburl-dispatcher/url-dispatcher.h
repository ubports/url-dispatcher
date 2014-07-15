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

/**
 * url_dispatch_send:
 * @url: URL to send to the dispatcher
 * @cb: Function to call with the result of the URL processing
 * @user_data: data pointer for @cb
 * 
 * Sends a URL to the dispatcher for processing. Most of the time,
 * things will work out and that URL will result in an application
 * being opened with the URL as requested. In some cases the URL
 * may not have a valid handler and an error will be returned. In
 * that case a bug will be filled on this package.
 */
void       url_dispatch_send            (const gchar *         url,
                                         URLDispatchCallback   cb,
                                         gpointer              user_data);

/**
 * url_dispatch_send_restricted:
 * @url: URL to send to the dispatcher
 * @cb: Function to call with the result of the URL processing
 * @user_data: data pointer for @cb
 * 
 * Very much like url_dispatch_send() except that it also says which
 * package is allowed to have the URL. This should be checked with the
 * url_dispatch_url_appid() function ahead of time, but is used to avoid
 * races that can occur between the test and the processing.
 */
void       url_dispatch_send_restricted (const gchar *         url,
                                         const gchar *         package,
                                         URLDispatchCallback   cb,
                                         gpointer              user_data);

/**
 * url_dispatch_url_appid:
 * @urls: URLs to check the AppIDs for
 *
 * Takes a list of URLs and return which AppIDs will be launched to
 * handle those URLs.
 *
 * NOTE: This function is *not* available for confined applications and
 * only for trusted helpers. It could result in discovery of which 
 * applications are installed on the system if exposed.
 */
gchar **   url_dispatch_url_appid       (const gchar **        urls);

G_END_DECLS

#pragma GCC visibility pop

#endif /* __URL_DISPATCH_H__ */
