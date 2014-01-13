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
#include <json-glib/json-glib.h>
#include <upstart-app-launch.h>
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
static gchar * click_exec = NULL;
static sqlite3 * urldb = NULL;

#define CURRENT "current-user-version"

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

	if (!upstart_app_launch_start_application(app_id, urls)) {
		g_warning("Unable to start application '%s' with URL '%s'", app_id, url);
	}

	return;
}

typedef enum _app_name_t app_name_t;
enum _app_name_t {
	APP_NAME_PRECISE,
	APP_NAME_ONLY,
	APP_NAME_FIRST,
	APP_NAME_LAST
};

typedef enum _version_search_t version_search_t;
enum _version_search_t {
	VERSION_SEARCH_PRECISE,
	VERSION_SEARCH_CURRENT
};

/* Check to see if the app name is one of our keywords */
static app_name_t
app_name_type (const gchar * appname)
{
	if (g_strcmp0(appname, "only-listed-app") == 0) {
		return APP_NAME_ONLY;
	}

	if (g_strcmp0(appname, "first-listed-app") == 0) {
		return APP_NAME_FIRST;
	}

	if (g_strcmp0(appname, "last-listed-app") == 0) {
		return APP_NAME_LAST;
	}

	return APP_NAME_PRECISE;
}

/* Check to see if the version is special */
static version_search_t
version_search_type (const gchar * version)
{
	if (g_strcmp0(version, CURRENT) == 0) {
		return VERSION_SEARCH_CURRENT;
	}

	return VERSION_SEARCH_PRECISE;
}

/* Try and get a manifest file and do a couple sanity checks on it */
JsonParser *
get_manifest_file (const gchar * pkg)
{
	/* Get the directory from click */
	GError * error = NULL;
	gchar * cmdline = g_strdup_printf("%s info \"%s\"", 
		click_exec == NULL ? "click" : click_exec,
		pkg);

	gchar * output = NULL;
	g_spawn_command_line_sync(cmdline, &output, NULL, NULL, &error);
	g_free(cmdline);

	if (error != NULL) {
		g_warning("Unable to get manifest for '%s' package: %s", pkg, error->message);
		g_error_free(error);
		g_free(output);
		return NULL;
	}

	/* Let's look at that manifest file */
	JsonParser * parser = json_parser_new();
	json_parser_load_from_data(parser, output, -1, &error);
	g_free(output);

	if (error != NULL) {
		g_warning("Unable to load manifest for '%s': %s", pkg, error->message);
		g_error_free(error);
		g_object_unref(parser);
		return NULL;
	}

	JsonNode * root = json_parser_get_root(parser);
	if (json_node_get_node_type(root) != JSON_NODE_OBJECT) {
		g_warning("Manifest file for package '%s' does not have an object as its root node", pkg);
		g_object_unref(parser);
		return NULL;
	}

	JsonObject * rootobj = json_node_get_object(root);

	if (!json_object_has_member(rootobj, "version")) {
		g_warning("Manifest file for package '%s' does not have a version", pkg);
		g_object_unref(parser);
		return NULL;
	}

	if (!json_object_has_member(rootobj, "hooks")) {
		g_warning("Manifest file for package '%s' does not have a hooks section", pkg);
		g_object_unref(parser);
		return NULL;
	}

	return parser;
}

/* Figure out the app name using the manifest */
const gchar *
manifest_app_name (JsonParser * manifest, app_name_t app_type, const gchar * original_app)
{
	if (app_type == APP_NAME_PRECISE) {
		return original_app;
	}

	JsonNode * root_node = json_parser_get_root(manifest);
	JsonObject * root_obj = json_node_get_object(root_node);
	JsonObject * hooks = json_object_get_object_member(root_obj, "hooks");

	if (hooks == NULL) {
		return NULL;
	}

	GList * apps = json_object_get_members(hooks);
	if (apps == NULL) {
		return NULL;
	}

	const gchar * retapp = NULL;

	switch (app_type) {
	case APP_NAME_ONLY:
		if (g_list_length(apps) == 1) {
			retapp = (const gchar *)apps->data;
		}
		break;
	case APP_NAME_FIRST:
		retapp = (const gchar *)apps->data;
		break;
	case APP_NAME_LAST:
		retapp = (const gchar *)(g_list_last(apps)->data);
		break;
	default:
		break;
	}

	g_list_free(apps);

	return retapp;
}

/* Figure out the app name using the manifest */
const gchar *
manifest_version (JsonParser * manifest, version_search_t version_type, const gchar * original_ver)
{
	if (version_type == VERSION_SEARCH_PRECISE) {
		return original_ver;
	}

	if (version_type == VERSION_SEARCH_CURRENT) {
		JsonNode * node = json_parser_get_root(manifest);
		JsonObject * obj = json_node_get_object(node);
		return json_object_get_string_member(obj, "version");
	}

	return NULL;
}

/* Works with a fuzzy set of parameters to determine the right app to
   call and then calls pass_url_to_app() with the full AppID */
static gboolean
app_id_discover (const gchar * pkg, const gchar * app, const gchar * version, const gchar * url)
{
	version_search_t version_search = version_search_type(version);
	app_name_t app_name = app_name_type(app);

	if (version_search == VERSION_SEARCH_PRECISE && app_name == APP_NAME_PRECISE) {
		/* This the non-search case, just put it together and pass along */
		gchar * appid = g_strdup_printf("%s_%s_%s", pkg, app, version);

		pass_url_to_app(appid, url);

		g_free(appid);
		return TRUE;
	}

	/* Get the manifest and turn that into our needed values */
	JsonParser * parser = get_manifest_file(pkg);
	if (parser == NULL) {
		return FALSE;
	}

	const gchar * final_app = manifest_app_name(parser, app_name, app);
	const gchar * final_ver = manifest_version(parser, version_search, version);

	if (final_app == NULL || final_ver == NULL) {
		g_object_unref(parser);
		return FALSE;
	}

	gchar * appid = g_strdup_printf("%s_%s_%s", pkg, final_app, final_ver);
	g_object_unref(parser);

	pass_url_to_app(appid, url);

	g_free(appid);
	return TRUE;
}

/* Get a URL off of the bus */
static gboolean
dispatch_url_cb (GObject * skel, GDBusMethodInvocation * invocation, const gchar * url, gpointer user_data)
{
	g_debug("Dispatching URL: %s", url);

	if (url == NULL || url[0] == '\0') {
		return bad_url(invocation, url);
	}

	if (dispatch_url(url)) {
		g_dbus_method_invocation_return_value(invocation, NULL);
	} else {
		bad_url(invocation, url);
	}

	return TRUE;
}

/* The core of the URL handling */
gboolean
dispatch_url (const gchar * url)
{
	/* Special case the app id */
	GMatchInfo * appidmatch = NULL;
	if (g_regex_match(appidre, url, 0, &appidmatch)) {
		gchar * package = g_match_info_fetch(appidmatch, 1);
		gchar * app = g_match_info_fetch(appidmatch, 2);
		gchar * version = g_match_info_fetch(appidmatch, 3);
		gboolean retval = TRUE;

		retval = app_id_discover(package, app, version, NULL);

		g_free(package);
		g_free(app);
		g_free(version);
		g_match_info_free(appidmatch);

		return retval;
	}

	/* Special case the application URL */
	GMatchInfo * appmatch = NULL;
	if (g_regex_match(applicationre, url, 0, &appmatch)) {
		gchar * appid = g_match_info_fetch(appmatch, 1);
		pass_url_to_app(appid, NULL);

		g_free(appid);
		g_match_info_free(appmatch);

		return TRUE;
	}
	g_match_info_free(appmatch);

	/* Check the URL db */
	GMatchInfo * genericmatch = NULL;
	if (g_regex_match(genericre, url, 0, &genericmatch)) {
		gboolean found = FALSE;
		gchar * protocol = g_match_info_fetch(genericmatch, 1);
		gchar * domain = g_match_info_fetch(genericmatch, 2);

		gchar * appid = url_db_find_url(urldb, protocol, domain);

		if (appid != NULL) {
			found = TRUE;
			pass_url_to_app(appid, url);
			g_free(appid);
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

/* Initialize all the globals */
gboolean
dispatcher_init (GMainLoop * mainloop)
{
	cancellable = g_cancellable_new();

	urldb = url_db_create_database();

	applicationre = g_regex_new("^application:///([a-zA-Z0-9_\\.-]*)\\.desktop$", 0, 0, NULL);
	appidre = g_regex_new("^appid://([a-z0-9\\.-]*)/([a-zA-Z0-9-]*)/([a-zA-Z0-9\\.-]*)$", 0, 0, NULL);
	genericre = g_regex_new("^(.*)://([a-z0-9\\.-]*)(/.*)$", 0, 0, NULL);

	if (g_getenv("URL_DISPATCHER_CLICK_EXEC") != NULL) {
		click_exec = g_strdup(g_getenv("URL_DISPATCHER_CLICK_EXEC"));
	}

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
	g_free(click_exec);
	sqlite3_close(urldb);

	return TRUE;
}
