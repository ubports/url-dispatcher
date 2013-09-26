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

#include <gio/gio.h>
#include <upstart-app-launch.h>
#include "service-iface.h"
#include "recoverable-problem.h"

/* Globals */
static GMainLoop * mainloop = NULL;
static GCancellable * cancellable = NULL;
static ServiceIfaceComCanonicalURLDispatcher * skel = NULL;
static GRegex * applicationre = NULL;

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
	gchar * additional[3] = {
		"BadURL",
		badurl,
		NULL
	};

	report_recoverable_problem(signature, pid, FALSE, additional);

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

	if (!upstart_app_launch_start_application(app_id, urls)) {
		g_warning("Unable to start application '%s' with URL '%s'", app_id, url);
	}

	return;
}

/* URL handlers need to be identified */
typedef struct _url_type_t url_type_t;
struct _url_type_t {
	gchar * regex_patern;
	GRegex * regex_object;
	gchar * app_id;
};

#define USERNAME_REGEX  "[a-zA-Z0-9_\\-]*"

/* TODO: Make these come from registrations, but this works for now */
url_type_t url_types[] = {
	/* Address Book */
	{
		.regex_patern = "^addressbook:///",
		.regex_object = NULL,
		.app_id = "address-book-app"
	},
	/* Messages */
	{
		.regex_patern = "^message:///",
		.regex_object = NULL,
		.app_id = "messaging-app"
	},
	/* Music */
	{
		.regex_patern = "^music:///",
		.regex_object = NULL,
		.app_id = "music-app"
	},
	{
		/* TODO: This is temporary for 13.10, we expect to be smarter in the future */
		.regex_patern = "^file:///home/" USERNAME_REGEX "/Music/",
		.regex_object = NULL,
		.app_id = "music-app"
	},
	/* Phone Numbers */
	{
		.regex_patern = "^tel:///[\\d\\.+x,\\(\\)-]*$",
		.regex_object = NULL,
		.app_id = "dialer-app"
	},
	/* Settings */
	{
		.regex_patern = "^settings:///system/",
		.regex_object = NULL,
		.app_id = "ubuntu-system-settings"
	},
	/* Video */
	{
		.regex_patern = "^video:///",
		.regex_object = NULL,
		.app_id = "mediaplayer-app"
	},
	{
		/* TODO: This is temporary for 13.10, we expect to be smarter in the future */
		.regex_patern = "^file:///home/" USERNAME_REGEX "/Videos/",
		.regex_object = NULL,
		.app_id = "mediaplayer-app"
	},
	/* Web Stuff */
	{
		.regex_patern = "^http://",
		.regex_object = NULL,
		.app_id = "webbrowser-app"
	},
	{
		.regex_patern = "^https://",
		.regex_object = NULL,
		.app_id = "webbrowser-app"
	}
};

/* Get a URL off of the bus */
static gboolean
dispatch_url (GObject * skel, GDBusMethodInvocation * invocation, const gchar * url, gpointer user_data)
{
	g_debug("Dispatching URL: %s", url);

	if (url == NULL || url[0] == '\0') {
		return bad_url(invocation, url);
	}

	/* Special case the application URL */
	GMatchInfo * appmatch = NULL;
	if (g_regex_match(applicationre, url, 0, &appmatch)) {
		gchar * appid = g_match_info_fetch(appmatch, 1);
		pass_url_to_app(appid, NULL);

		g_free(appid);
		g_match_info_free(appmatch);

		g_dbus_method_invocation_return_value(invocation, NULL);
		return TRUE;
	}
	g_match_info_free(appmatch);

	int i;
	for (i = 0; i < G_N_ELEMENTS(url_types); i++) {
		if (url_types[i].regex_object == NULL) {
			url_types[i].regex_object = g_regex_new(url_types[i].regex_patern, 0, 0, NULL);
		}

		if (g_regex_match(url_types[i].regex_object, url, 0, NULL)) {
			pass_url_to_app(url_types[i].app_id, url);

			g_dbus_method_invocation_return_value(invocation, NULL);
			return TRUE;
		}
	}

	return bad_url(invocation, url);
}

/* We're goin' down cap'n */
static void
name_lost (GDBusConnection * con, const gchar * name, gpointer user_data)
{
	g_error("Unable to get name '%s'", name);
	g_main_loop_quit(mainloop);
	return;
}

/* Callback when we're connected to dbus */
static void
bus_got (GObject * obj, GAsyncResult * res, gpointer user_data)
{
	GDBusConnection * bus = NULL;
	GError * error = NULL;

	bus = g_bus_get_finish(res, &error);

	if (error != NULL) {
		g_error("Unable to connect to D-Bus: %s", error->message);
		g_main_loop_quit(mainloop);
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
		NULL, NULL); /* user data */

	g_object_unref(bus);

	return;
}

/* Where it all begins */
int
main (int argc, char * argv[])
{
	mainloop = g_main_loop_new(NULL, FALSE);
	cancellable = g_cancellable_new();
	applicationre = g_regex_new("^application:///([a-zA-Z0-9_-]*)\\.desktop$", 0, 0, NULL);

	g_bus_get(G_BUS_TYPE_SESSION, cancellable, bus_got, NULL);

	skel = service_iface_com_canonical_urldispatcher_skeleton_new();
	g_signal_connect(skel, "handle-dispatch-url", G_CALLBACK(dispatch_url), NULL);

	/* Run Main */
	g_main_loop_run(mainloop);

	/* Clean up globals */
	g_main_loop_unref(mainloop);
	g_object_unref(cancellable);
	g_object_unref(skel);
	g_regex_unref(applicationre);

	int i;
	for (i = 0; i < G_N_ELEMENTS(url_types); i++) {
		g_clear_pointer(&url_types[i].regex_object, g_regex_unref);
	}

	return 0;
}
