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

#include "url-dispatcher.h"
#include <gio/gio.h>

typedef struct _dispatch_data_t dispatch_data_t;
struct _dispatch_data_t {
	URLDispatchCallback cb;
	gpointer user_data;
	gchar * url;
	gchar * package;
};

static void
url_dispatched (GObject * obj, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;
	dispatch_data_t * dispatch_data = (dispatch_data_t *)user_data;

	g_dbus_connection_call_finish(
		G_DBUS_CONNECTION(obj),
		res,
		&error);

	if (error != NULL) {
		g_warning("Unable to dispatch url '%s':%s", dispatch_data->url, error->message);
		g_error_free(error);

		if (dispatch_data->cb != NULL) {
			dispatch_data->cb(dispatch_data->url, FALSE, dispatch_data->user_data);
		}
	} else {
		if (dispatch_data->cb != NULL) {
			dispatch_data->cb(dispatch_data->url, TRUE, dispatch_data->user_data);
		}
	}

	g_free(dispatch_data->url);
	g_free(dispatch_data->package);
	g_free(dispatch_data);

	return;
}

void
url_dispatch_send (const gchar * url, URLDispatchCallback cb, gpointer user_data)
{
	url_dispatch_send_restricted(url, NULL, cb, user_data);
}

void
url_dispatch_send_restricted (const gchar * url, const gchar * package, URLDispatchCallback cb, gpointer user_data)
{
	GError * error = NULL;
	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

	if (error != NULL) {
		g_warning("Unable to get session bus: %s", error->message);
		g_error_free(error);
		return;
	}

	dispatch_data_t * dispatch_data = NULL;
	
	if (cb != NULL) {
		dispatch_data = g_new0(dispatch_data_t, 1);

		dispatch_data->cb = cb;
		dispatch_data->user_data = user_data;
		dispatch_data->url = g_strdup(url);
		dispatch_data->package = g_strdup(package);
	}

	g_dbus_connection_call(bus,
	                       "com.canonical.URLDispatcher",
	                       "/com/canonical/URLDispatcher",
	                       "com.canonical.URLDispatcher",
	                       "DispatchURL",
	                       g_variant_new("(ss)", url, package ? package : ""),
	                       NULL,
	                       G_DBUS_CALL_FLAGS_NO_AUTO_START,
	                       -1, /* timeout */
	                       NULL, /* cancelable */
	                       cb != NULL ? url_dispatched : NULL,
	                       dispatch_data);

	if (cb == NULL) {
		g_dbus_connection_flush_sync(bus, NULL, NULL);
	}

	g_object_unref(bus);

	return;
}

gchar **
url_dispatch_url_appid (const gchar ** urls)
{
	GError * error = NULL;
	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

	if (error != NULL) {
		g_warning("Unable to get session bus: %s", error->message);
		g_error_free(error);
		return NULL;
	}

	GVariant * vurls = g_variant_new_strv(urls, -1);
	GVariant * vparam = g_variant_new_tuple(&vurls, 1);

	GVariant * retval = NULL;
	retval = g_dbus_connection_call_sync(bus,
	                                     "com.canonical.URLDispatcher",
	                                     "/com/canonical/URLDispatcher",
	                                     "com.canonical.URLDispatcher",
	                                     "TestURL",
	                                     vparam,
	                                     G_VARIANT_TYPE("(as)"),
	                                     G_DBUS_CALL_FLAGS_NO_AUTO_START,
	                                     -1, /* timeout */
	                                     NULL, /* cancelable */
	                                     &error);

	if (error != NULL) {
		g_warning("Unable to test URL with URL Dispatcher: %s", error->message);
		g_error_free(error);
		g_object_unref(bus);
		return NULL;
	}

	GVariant * varstr = g_variant_get_child_value(retval, 0);
	gchar ** appids = g_variant_dup_strv(varstr, NULL);
	g_variant_unref(varstr);
	g_variant_unref(retval);

	g_object_unref(bus);

	return appids;
}
