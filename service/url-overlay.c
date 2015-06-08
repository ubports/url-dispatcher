/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Ted Gould <ted.gould@canonical.com>
 */

#include <glib.h>
#include <click.h>
#include <ubuntu-app-launch.h>

#include "recoverable-problem.h"

gchar *
build_exec (const gchar * appid, const gchar * directory)
{
	gchar * appid_desktop = g_strdup_printf("%s.desktop", appid);
	gchar * desktopfilepath = g_build_filename(directory, appid_desktop, NULL);
	g_free(appid_desktop);

	if (!g_file_test(desktopfilepath, G_FILE_TEST_EXISTS)) {
		g_free(desktopfilepath);
		return NULL;
	}

	GError * error = NULL;
	GKeyFile * keyfile = g_key_file_new();
	g_key_file_load_from_file(keyfile, desktopfilepath, G_KEY_FILE_NONE, &error);

	if (error != NULL) {
		g_error("Unable to read url-overlay desktop file '%s': %s", desktopfilepath, error->message);
		g_free(desktopfilepath);
		g_key_file_free(keyfile);
		g_error_free(error);
		return NULL;
	}

	g_free(desktopfilepath);

	if (!g_key_file_has_key(keyfile, "Desktop Entry", "Exec", NULL)) {
		g_error("Desktop file for '%s' in '%s' does not have 'Exec' key", appid, directory);
		g_key_file_free(keyfile);
		return NULL;
	}

	gchar * exec = g_key_file_get_string(keyfile, "Desktop Entry", "Exec", NULL);
	g_key_file_free(keyfile);

	return exec;
}

gchar *
build_dir (const gchar * appid)
{
	GError * error = NULL;
	gchar * package = NULL;

	/* 'Parse' the App ID */
	if (!ubuntu_app_launch_app_id_parse(appid, &package, NULL, NULL)) {
		g_warning("Unable to parse App ID: '%s'", appid);
		return NULL;
	}

	/* Check click to find out where the files are */
	ClickDB * db = click_db_new();

	/* If TEST_CLICK_DB is unset, this reads the system database. */
	click_db_read(db, g_getenv("TEST_CLICK_DB"), &error);
	if (error != NULL) {
		g_warning("Unable to read Click database: %s", error->message);
		g_error_free(error);
		g_free(package);
		return NULL;
	}

	/* If TEST_CLICK_USER is unset, this uses the current user name. */
	ClickUser * user = click_user_new_for_user(db, g_getenv("TEST_CLICK_USER"), &error);
	if (error != NULL) {
		g_warning("Unable to read Click database: %s", error->message);
		g_error_free(error);
		g_free(package);
		g_object_unref(db);
		return NULL;
	}

	gchar * pkgdir = click_user_get_path(user, package, &error);

	g_object_unref(user);
	g_object_unref(db);
	g_free(package);

	if (error != NULL) {
		g_warning("Unable to get the Click package directory for %s: %s", package, error->message);
		g_error_free(error);
		return NULL;
	}

	return pkgdir;
}

int
main (int argc, char * argv[])
{
	/* Build up our exec */
	const gchar * appid = g_getenv("APP_ID");
	if (appid == NULL) {
		report_recoverable_problem("url-dispatcher-url-overlay-no-appid", 0, TRUE, NULL);
		return -1;
	}

	gchar * exec = NULL;

	/* Allow for environment override */
	const gchar * envdir = g_getenv("URL_DISPATCHER_OVERLAY_DIR");
	if (G_UNLIKELY(envdir != NULL)) { /* Mostly for testing */
		exec = build_exec(appid, envdir);
	}

	/* Try the system directory */
	if (exec == NULL) {
		exec = build_exec(appid, OVERLAY_SYSTEM_DIRECTORY);
	}

	/* If not there look to the user directory (click) */
	if (exec == NULL) {
		gchar * userdir = g_build_filename(g_get_user_cache_dir(), "url-dispatcher", "url-overlays", NULL);
		exec = build_exec(appid, userdir);
		g_free(userdir);
	}

	if (exec == NULL) {
		const gchar * props[3] = {
			"AppID",
			appid,
			NULL
		};

		report_recoverable_problem("url-dispatcher-url-overlay-bad-appid", 0, TRUE, props);
		return -1;
	}

	gchar * dir = build_dir(appid);
	/* NOTE: Dir will be NULL for system apps */

	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	g_return_val_if_fail(bus != NULL, -1);

	gboolean sended = ubuntu_app_launch_helper_set_exec(exec, dir);
	g_free(exec);
	g_free(dir);

	/* Ensuring the messages get on the bus before we quit */
	g_dbus_connection_flush_sync(bus, NULL, NULL);
	g_clear_object(&bus);

	if (sended) {
		return 0;
	} else {
		g_critical("Unable to send exec to Upstart");
		return -1;
	}
}
