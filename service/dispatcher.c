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

#include <gio/gio.h>
#include <json-glib/json-glib.h>
#include <ubuntu-app-launch.h>
#include "dispatcher.h"
#include "service-iface.h"
#include "recoverable-problem.h"
#include "url-db.h"

/* Globals */
static GCancellable * cancellable = NULL;
static ServiceIfaceComCanonicalURLDispatcher * skel = NULL;
static GRegex * applicationre = NULL;
static GRegex * appidre = NULL;
static GRegex * genericre = NULL;
static GRegex * musicfilere = NULL; /* FIXME */
static GRegex * videofilere = NULL; /* FIXME */
static sqlite3 * urldb = NULL;

/* Errors */
enum {
	ERROR_BAD_URL
};

G_DEFINE_QUARK(url_dispatcher, url_dispatcher_error);

/* Register our errors */
static void
register_dbus_errors (void)
{
	g_dbus_error_register_error(url_dispatcher_error_quark(), ERROR_BAD_URL, "com.canonical.URLDispatcher.BadURL");
	return;
}

/* We should have the PID now so we can make sure to file the
   problem on the right package. */
static void
recoverable_problem_file (GObject * obj, GAsyncResult * res, gpointer user_data)
{
	gchar * badurl = (gchar *)user_data;
	GVariant * pid_tuple = NULL;
	GError * error = NULL;

	pid_tuple = g_dbus_connection_call_finish(G_DBUS_CONNECTION(obj), res, &error);
	if (error != NULL) {
		g_warning("Unable to get PID for calling program with URL '%s': %s", badurl, error->message);
		g_free(badurl);
		g_error_free(error);
		return;
	}

	guint32 pid = 0;
	g_variant_get(pid_tuple, "(u)", &pid);
	g_variant_unref(pid_tuple);

	gchar * signature = g_strdup_printf("url-dispatcher;bad-url;%s", badurl);
	const gchar * additional[3] = {
		"BadURL",
		badurl,
		NULL
	};

	/* Allow disabling for testing, we don't want to report bugs on
	   our tests ;-) */
	if (g_getenv("URL_DISPATCHER_DISABLE_RECOVERABLE_ERROR") == NULL) {
		report_recoverable_problem(signature, pid, FALSE, additional);
	}

	g_free(signature);
	g_free(badurl);

	return;
}

/* Say that we have a bad URL and report a recoverable error on the process that
   sent it to us. */
static gboolean
bad_url (GDBusMethodInvocation * invocation, const gchar * url)
{
	const gchar * sender = g_dbus_method_invocation_get_sender(invocation);
	GDBusConnection * conn = g_dbus_method_invocation_get_connection(invocation);

	g_dbus_connection_call(conn,
		"org.freedesktop.DBus",
		"/",
		"org.freedesktop.DBus",
		"GetConnectionUnixProcessID",
		g_variant_new("(s)", sender),
		G_VARIANT_TYPE("(u)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1, /* timeout */
		NULL, /* cancellable */
		recoverable_problem_file,
		g_strdup(url));

	g_dbus_method_invocation_return_error(invocation,
		url_dispatcher_error_quark(),
		ERROR_BAD_URL,
		"URL '%s' is not handleable by the URL Dispatcher",
		url);

	return TRUE;
}

/* Handles taking an application and an URL and sending them to Upstart */
static void
pass_url_to_app (const gchar * app_id, const gchar * url)
{
	g_debug("Emitting 'application-start' for APP_ID='%s' and URLS='%s'", app_id, url);

	const gchar * urls[2] = {
		url,
		NULL
	};

	if (!ubuntu_app_launch_start_application(app_id, urls)) {
		g_warning("Unable to start application '%s' with URL '%s'", app_id, url);
	}

	return;
}

/* Get a URL off of the bus */
static gboolean
dispatch_url_cb (GObject * skel, GDBusMethodInvocation * invocation, const gchar * url, gpointer user_data)
{
	g_debug("Dispatching URL: %s", url);

	if (url == NULL || url[0] == '\0') {
		return bad_url(invocation, url);
	}

	gchar * appid = NULL;
	const gchar * outurl = NULL;

	if (dispatch_url(url, &appid, &outurl)) {
		pass_url_to_app(appid, outurl);
		g_free(appid);

		g_dbus_method_invocation_return_value(invocation, NULL);
	} else {
		bad_url(invocation, url);
	}

	return TRUE;
}

/* The core of the URL handling */
gboolean
dispatch_url (const gchar * url, gchar ** out_appid, const gchar ** out_url)
{
	/* Special case the app id */
	GMatchInfo * appidmatch = NULL;
	if (g_regex_match(appidre, url, 0, &appidmatch)) {
		gchar * package = g_match_info_fetch(appidmatch, 1);
		gchar * app = g_match_info_fetch(appidmatch, 2);
		gchar * version = g_match_info_fetch(appidmatch, 3);
		gboolean retval = FALSE;

		*out_appid = ubuntu_app_launch_triplet_to_app_id(package, app, version);
		if (*out_appid != NULL) {
			retval = TRUE;
		}

		g_free(package);
		g_free(app);
		g_free(version);
		g_match_info_free(appidmatch);

		return retval;
	}

	/* Special case the application URL */
	GMatchInfo * appmatch = NULL;
	if (g_regex_match(applicationre, url, 0, &appmatch)) {
		*out_appid = g_match_info_fetch(appmatch, 1);

		g_match_info_free(appmatch);

		return TRUE;
	}
	g_match_info_free(appmatch);

	/* start FIXME: These are needed work arounds until everything migrates away
	   from them.  Ewww */
	GMatchInfo * musicmatch = NULL;
	if (g_regex_match(musicfilere, url, 0, &musicmatch)) {
		gboolean retval = FALSE;

		*out_appid = ubuntu_app_launch_triplet_to_app_id("com.ubuntu.music", "music", NULL);
		if (*out_appid != NULL) {
			*out_url = url;
			retval = TRUE;
		}

		g_match_info_free(musicmatch);
		return retval;
	}
	g_match_info_free(musicmatch);

	GMatchInfo * videomatch = NULL;
	if (g_regex_match(videofilere, url, 0, &videomatch)) {
		*out_appid = g_strdup("mediaplayer-app");
		*out_url = url;

		g_match_info_free(videomatch);
		return TRUE;
	}
	g_match_info_free(videomatch);
	/* end FIXME: Making the ugly stop */

	/* Check the URL db */
	GMatchInfo * genericmatch = NULL;
	if (g_regex_match(genericre, url, 0, &genericmatch)) {
		gboolean found = FALSE;
		gchar * protocol = g_match_info_fetch(genericmatch, 1);
		gchar * domain = g_match_info_fetch(genericmatch, 2);

		*out_appid = url_db_find_url(urldb, protocol, domain);

		if (*out_appid != NULL) {
			found = TRUE;
			*out_url = url;
		}

		g_free(protocol);
		g_free(domain);

		g_match_info_free(genericmatch);

		return found;
	}
	g_match_info_free(genericmatch);

	return FALSE;
}

/* We're goin' down cap'n */
static void
name_lost (GDBusConnection * con, const gchar * name, gpointer user_data)
{
	GMainLoop * mainloop = (GMainLoop *)user_data;
	g_warning("Unable to get name '%s'", name);
	g_main_loop_quit(mainloop);
	return;
}

/* Callback when we're connected to dbus */
static void
bus_got (GObject * obj, GAsyncResult * res, gpointer user_data)
{
	GMainLoop * mainloop = (GMainLoop *)user_data;
	GDBusConnection * bus = NULL;
	GError * error = NULL;

	bus = g_bus_get_finish(res, &error);

	if (error != NULL) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_error("Unable to connect to D-Bus: %s", error->message);
			g_main_loop_quit(mainloop);
		}
		g_error_free(error);
		return;
	}

	register_dbus_errors();

	g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(skel), bus, "/com/canonical/URLDispatcher", &error);
	if (error != NULL) {
		g_error("Unable to export interface skeleton: %s", error->message);
		g_main_loop_quit(mainloop);
		return;
	}

	g_bus_own_name_on_connection(bus,
		"com.canonical.URLDispatcher",
		G_BUS_NAME_OWNER_FLAGS_NONE, /* flags */
		NULL, /* name acquired */
		name_lost,
		user_data, NULL); /* user data */

	g_object_unref(bus);

	return;
}

#define USERNAME_REGEX  "[a-zA-Z0-9_\\-]*"

/* Initialize all the globals */
gboolean
dispatcher_init (GMainLoop * mainloop)
{
	cancellable = g_cancellable_new();

	urldb = url_db_create_database();

	applicationre = g_regex_new("^application:///([a-zA-Z0-9_\\.-]*)\\.desktop$", 0, 0, NULL);
	appidre = g_regex_new("^appid://([a-z0-9\\.-]*)/([a-zA-Z0-9-]*)/([a-zA-Z0-9\\.-]*)$", 0, 0, NULL);
	genericre = g_regex_new("^(.*)://([a-z0-9\\.-]*)?/?(.*)?$", 0, 0, NULL);

	/* FIXME: Legacy */
	musicfilere = g_regex_new("^file:///home/" USERNAME_REGEX "/Music/", 0, 0, NULL);
	videofilere = g_regex_new("^file:///home/" USERNAME_REGEX "/Videos/", 0, 0, NULL);

	g_bus_get(G_BUS_TYPE_SESSION, cancellable, bus_got, mainloop);

	skel = service_iface_com_canonical_urldispatcher_skeleton_new();
	g_signal_connect(skel, "handle-dispatch-url", G_CALLBACK(dispatch_url_cb), NULL);

	return TRUE;
}

/* Clean up all the globals */
gboolean
dispatcher_shutdown (void)
{
	g_cancellable_cancel(cancellable);

	g_object_unref(cancellable);
	g_object_unref(skel);
	g_regex_unref(applicationre);
	g_regex_unref(appidre);
	g_regex_unref(genericre);
	g_regex_unref(musicfilere); /* FIXME */
	g_regex_unref(videofilere); /* FIXME */
	sqlite3_close(urldb);

	return TRUE;
}
