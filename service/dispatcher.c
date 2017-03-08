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
#include "scope-checker.h"
#include "url-db.h"

/* Globals */
static OverlayTracker * tracker = NULL;
static ScopeChecker * checker = NULL;
static GCancellable * cancellable = NULL;
static ServiceIfaceComCanonicalURLDispatcher * skel = NULL;
static GRegex * applicationre = NULL;
static GRegex * appidre = NULL;
static GRegex * genericre = NULL;
static GRegex * intentre = NULL;
static sqlite3 * urldb = NULL;

/* Errors */
enum {
	ERROR_BAD_URL,
	ERROR_RESTRICTED_URL
};

G_DEFINE_QUARK(url_dispatcher, url_dispatcher_error)

/* Register our errors */
static void
register_dbus_errors ()
{
	g_dbus_error_register_error(url_dispatcher_error_quark(), ERROR_BAD_URL, "com.canonical.URLDispatcher.BadURL");
	g_dbus_error_register_error(url_dispatcher_error_quark(), ERROR_RESTRICTED_URL, "com.canonical.URLDispatcher.RestrictedURL");
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

	/* Popup the bad url dialog */
	overlay_tracker_badurl(tracker, pid, badurl);

	/* Report recoverable error */
	const gchar * additional[3] = {
		"BadURL",
		badurl,
		NULL
	};

	report_recoverable_problem("url-dispatcher-bad-url", pid, FALSE, additional);

	g_free(badurl);

	return;
}

/* Error based on the fact that we're using a restricted launch but the package
   doesn't match */
/* NOTE: Only sending back the data we were given. We don't want people to be
   able to parse the error as an info leak */
static gboolean
restricted_appid (GDBusMethodInvocation * invocation, const gchar * url, const gchar * package)
{
	g_dbus_method_invocation_return_error(invocation,
		url_dispatcher_error_quark(),
		ERROR_RESTRICTED_URL,
		"URL '%s' does not have a handler in package '%s'",
		url, package);

	return TRUE;
}

/* Say that we have a bad URL and report a recoverable error on the process that
   sent it to us. */
static gboolean
bad_url (GDBusMethodInvocation * invocation, const gchar * url)
{
	const gchar * sender = g_dbus_method_invocation_get_sender(invocation);
	GDBusConnection * conn = g_dbus_method_invocation_get_connection(invocation); /* transfer: none */

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

/* Print an error if we get one */
static void
send_open_cb (GObject * object, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;

	g_dbus_connection_call_finish(G_DBUS_CONNECTION(object), res, &error);

	if (error != NULL) {
		/* Mostly just to free the error, but printing for debugging */
		g_warning("Unable to send Open to dash: %s", error->message);
		g_error_free(error);
	}
}

/* Sends the URL to the dash, which isn't an app, but just on the bus generally. */
gboolean
send_to_dash (const gchar * url)
{
	if (url == NULL) {
		g_warning("Can not send nothing to the dash");
		return FALSE;
	}

	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	g_return_val_if_fail(bus != NULL, FALSE);

	/* Kinda sucks that we need to do this, should probably find it's way into
	   the libUAL API if it's needed outside */
	g_dbus_connection_emit_signal(bus,
		NULL, /* destination */
		"/", /* path */
		"com.canonical.UbuntuAppLaunch", /* interface */
		"UnityFocusRequest", /* signal */
		g_variant_new("(s)", "unity8-dash"),
		NULL);

	GVariantBuilder opendata;
	g_variant_builder_init(&opendata, G_VARIANT_TYPE_TUPLE);
	g_variant_builder_open(&opendata, G_VARIANT_TYPE_ARRAY);
	g_variant_builder_add_value(&opendata, g_variant_new_string(url));
	g_variant_builder_close(&opendata);
	g_variant_builder_add_value(&opendata, g_variant_new_array(G_VARIANT_TYPE("{sv}"), NULL, 0));

	/* Using the FD.o Application interface */
	g_dbus_connection_call(bus,
		"com.canonical.UnityDash",
		"/unity8_2ddash",
		"org.freedesktop.Application",
		"Open",
		g_variant_builder_end(&opendata),
		NULL,
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		send_open_cb, NULL);

	g_object_unref(bus);
	return TRUE;
}

/* Handles taking an application and an URL and sending them to Upstart */
gboolean
dispatcher_send_to_app (const gchar * app_id, const gchar * url)
{
	g_debug("Emitting 'application-start' for APP_ID='%s' and URLS='%s'", app_id, url);

	if (g_strcmp0(app_id, "unity8-dash") == 0) {
		return send_to_dash(url);
	}

	const gchar * urls[2] = {
		url,
		NULL
	};

	if (!ubuntu_app_launch_start_application(app_id, urls)) {
		g_warning("Unable to start application '%s' with URL '%s'", app_id, url);
	}

	return TRUE;
}

unsigned int _get_pid_from_dbus(GDBusConnection * conn, const gchar * sender)
{
    GError * error = NULL;
    unsigned int pid = 0;

    GVariant * callret = g_dbus_connection_call_sync(conn,
                                                     "org.freedesktop.DBus",
                                                     "/",
                                                     "org.freedesktop.DBus",
                                                     "GetConnectionUnixProcessID",
                                                     g_variant_new("(s)", sender),
                                                     G_VARIANT_TYPE("(u)"),
                                                     G_DBUS_CALL_FLAGS_NONE,
                                                     -1, /* timeout */
                                                     NULL, /* cancellable */
                                                     &error);

    if (error != NULL) {
        g_warning("Unable to get PID for '%s' from dbus: %s",
                  sender, error->message);
        g_clear_error(&error);
    } else {
        g_variant_get_child(callret, 0, "u", &pid);
        g_variant_unref(callret);
    }

    return pid;
}

/* Handles setting up the overlay with the URL */
gboolean
dispatcher_send_to_overlay (const gchar * app_id, const gchar * url, GDBusConnection * conn, const gchar * sender)
{
    unsigned int pid = _get_pid_from_dbus(conn, sender);
    if (pid == 0) {
        return FALSE;
    }

    /* If it is from a scope we need to overlay onto the
       dash instead */
    if (scope_checker_is_scope_pid(checker, pid)) {
        pid = _get_pid_from_dbus(conn, "com.canonical.Unity8Dash");
    }

    return overlay_tracker_add(tracker, app_id, pid, url);
}

/* Check to see if this is an overlay AppID */
gboolean
dispatcher_is_overlay (const gchar * appid)
{
	const gchar * systemdir = NULL;
	gboolean found = FALSE;
	gchar * desktopname = g_strdup_printf("%s.desktop", appid);

	/* First time, check the environment */
	if (G_UNLIKELY(systemdir == NULL)) {
		systemdir = g_getenv("URL_DISPATCHER_OVERLAY_DIR");
		if (systemdir == NULL) {
			systemdir = OVERLAY_SYSTEM_DIRECTORY;
		}
	}

	/* Check system dir */
	if (!found) {
		gchar * sysdir = g_build_filename(systemdir, desktopname, NULL);
		found = g_file_test(sysdir, G_FILE_TEST_EXISTS);
		g_free(sysdir);
	}

	/* Check user dir (clicks) */
	if (!found) {
		gchar * usrdir = g_build_filename(g_get_user_cache_dir(), "url-dispatcher", "url-overlays", desktopname, NULL);
		found = g_file_test(usrdir, G_FILE_TEST_EXISTS);
		g_free(usrdir);
	}

	g_free(desktopname);

	return found;
}

/* Whether we should restrict this appid based on the package name */
gboolean
dispatcher_appid_restrict (const gchar * appid, const gchar * package)
{
	if (package == NULL || package[0] == '\0') {
		return FALSE;
	}

	gchar * appackage = NULL;
	gboolean match = FALSE;

	if (ubuntu_app_launch_app_id_parse(appid, &appackage, NULL, NULL)) {
		/* Click application */
		match = (g_strcmp0(package, appackage) == 0);
	} else {
		/* Legacy application */
		match = (g_strcmp0(package, appid) == 0);
	}

	g_free(appackage);

	return !match;
}

/* Get a URL off of the bus */
static gboolean
dispatch_url_cb (GObject * skel, GDBusMethodInvocation * invocation, const gchar * url, const gchar * package, gpointer user_data)
{
	/* Nice debugging message depending on whether the @package variable
	   is valid from DBus */
	if (package == NULL || package[0] == '\0') {
		g_debug("Dispatching URL: %s", url);
	} else {
		g_debug("Dispatching Restricted URL: %s", url);
		g_debug("Package restriction: %s", package);
	}

	/* Check to ensure the URL is valid coming from DBus */
	if (url == NULL || url[0] == '\0') {
		return bad_url(invocation, url);
	}

	/* Actually do it */
	gchar * appid = NULL;
	const gchar * outurl = NULL;

	/* Discover the AppID */
	if (!dispatcher_url_to_appid(url, &appid, &outurl)) {
		return bad_url(invocation, url);
	}

	/* Check for the 'unconfined' app id which is causing problems */
	if (g_strcmp0(appid, "unconfined") == 0) {
		g_free(appid);
		return bad_url(invocation, url);
	}

	/* Check to see if we're allowed to use it */
	if (dispatcher_appid_restrict(appid, package)) {
		g_free(appid);
		return restricted_appid(invocation, url, package);
	}

	/* We're cleared to continue */
	gboolean sent = FALSE;
	if (!dispatcher_is_overlay(appid)) {
		sent = dispatcher_send_to_app(appid, outurl);
	} else {
		sent = dispatcher_send_to_overlay(
			appid,
			outurl,
			g_dbus_method_invocation_get_connection(invocation),
			g_dbus_method_invocation_get_sender(invocation));
	}
	g_free(appid);

	if (sent) {
		g_dbus_method_invocation_return_value(invocation, NULL);
	} else {
		bad_url(invocation, url);
	}

	return sent;
}

/* Test a URL to find it's AppID */
static gboolean
test_url_cb (GObject * skel, GDBusMethodInvocation * invocation, const gchar * const * urls, gpointer user_data)
{
	if (urls == NULL || urls[0] == NULL || urls[0][0] == '\0') {
		/* Right off the bat, let's deal with these */
		return bad_url(invocation, NULL);
	}

	GVariantBuilder builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

	int i;
	for (i = 0; urls[i] != NULL; i++) {
		const gchar * url = urls[i];

		g_debug("Testing URL: %s", url);

		if (url == NULL || url[0] == '\0') {
			g_variant_builder_clear(&builder);
			return bad_url(invocation, url);
		}

		gchar * appid = NULL;
		const gchar * outurl = NULL;

		if (dispatcher_url_to_appid(url, &appid, &outurl)) {
			GVariant * vappid = g_variant_new_take_string(appid);
			g_variant_builder_add_value(&builder, vappid);
		} else {
			g_variant_builder_clear(&builder);
			return bad_url(invocation, url);
		}
	}

	GVariant * varray = g_variant_builder_end(&builder);
	GVariant * tuple = g_variant_new_tuple(&varray, 1);
	g_dbus_method_invocation_return_value(invocation, tuple);

	return TRUE;
}

/* Determine the domain for an intent using the package variable */
static gchar *
intent_domain (const gchar * url)
{
	gchar * domain = NULL;
	GMatchInfo * intentmatch = NULL;

	if (g_regex_match(intentre, url, 0, &intentmatch)) {
		domain = g_match_info_fetch(intentmatch, 1);

		g_match_info_free(intentmatch);
	}

	return domain;
}

/* The core of the URL handling */
gboolean
dispatcher_url_to_appid (const gchar * url, gchar ** out_appid, const gchar ** out_url)
{
	g_return_val_if_fail(url != NULL, FALSE);
	g_return_val_if_fail(out_appid != NULL, FALSE);

	/* Special case the app id */
	GMatchInfo * appidmatch = NULL;
	if (g_regex_match(appidre, url, 0, &appidmatch)) {
		gchar * package = g_match_info_fetch(appidmatch, 1);
		gchar * app = g_match_info_fetch(appidmatch, 2);
		gchar * version = g_match_info_fetch(appidmatch, 3);
		gboolean retval = FALSE;

		*out_appid = ubuntu_app_launch_triplet_to_app_id(package, app, version);
		if (*out_appid != NULL) {
			/* Look at the current version of the app and ensure
			   we're not asking for an older version */
			gchar * testappid = ubuntu_app_launch_triplet_to_app_id(package, app, NULL);
			if (g_strcmp0(*out_appid, testappid) != 0) {
				retval = FALSE;
				g_clear_pointer(out_appid, g_free);
			} else {
				retval = TRUE;
			}

			g_free(testappid);
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

	/* Check the URL db */
	GMatchInfo * genericmatch = NULL;
	if (g_regex_match(genericre, url, 0, &genericmatch)) {
		gboolean found = FALSE;
		gchar * protocol = g_match_info_fetch(genericmatch, 1);
		gchar * domain = NULL;

		/* We special case the intent domain (further comment there) */
		if (g_strcmp0(protocol, "intent") == 0) {
			domain = intent_domain(url);
		} else {
			domain = g_match_info_fetch(genericmatch, 2);
		}

		*out_appid = url_db_find_url(urldb, protocol, domain);
		g_debug("Protocol '%s' for domain '%s' resulting in app id '%s'", protocol, domain, *out_appid);

		if (*out_appid != NULL) {
			found = TRUE;
			if (out_url != NULL) {
				*out_url = url;
			}
		}

		if (g_strcmp0(protocol, "scope") == 0) {
			/* Add a check for the scope if we can do that, since it is
			   a system URL that we can do more checking on */
			if (!scope_checker_is_scope(checker, domain)) {
				found = FALSE;
				g_clear_pointer(out_appid, g_free);
				if (out_url != NULL)
					g_clear_pointer(out_url, g_free);
			}
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
			g_error("Unable to connect to D-Bus");
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

/* Initialize all the globals */
gboolean
dispatcher_init (GMainLoop * mainloop, OverlayTracker * intracker, ScopeChecker * inchecker)
{
	tracker = intracker;
	checker = inchecker;
	cancellable = g_cancellable_new();

	urldb = url_db_create_database();
	g_return_val_if_fail(urldb != NULL, FALSE);

	applicationre = g_regex_new("^application:///([a-zA-Z0-9_\\.-]*)\\.desktop$", 0, 0, NULL);
	appidre = g_regex_new("^appid://([a-z0-9\\.-]*)/([a-zA-Z0-9-\\.]*)/([a-zA-Z0-9\\.-]*)$", 0, 0, NULL);
	genericre = g_regex_new("^([a-z][a-z0-9]*):(?://(?:.*@)?([a-zA-Z0-9\\.-_]*)(?::[0-9]*)?/?)?(.*)?$", 0, 0, NULL);
	intentre = g_regex_new("^intent://.*package=([a-zA-Z0-9\\.]*);.*$", 0, 0, NULL);

	g_bus_get(G_BUS_TYPE_SESSION, cancellable, bus_got, mainloop);

	skel = service_iface_com_canonical_urldispatcher_skeleton_new();
	g_signal_connect(skel, "handle-dispatch-url", G_CALLBACK(dispatch_url_cb), NULL);
	g_signal_connect(skel, "handle-test-url", G_CALLBACK(test_url_cb), NULL);

	return TRUE;
}

/* Clean up all the globals */
gboolean
dispatcher_shutdown ()
{
	g_cancellable_cancel(cancellable);

	g_object_unref(cancellable);
	g_object_unref(skel);
	g_regex_unref(applicationre);
	g_regex_unref(appidre);
	g_regex_unref(genericre);
	g_regex_unref(intentre);
	sqlite3_close(urldb);

	return TRUE;
}
