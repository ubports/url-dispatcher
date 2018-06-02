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

#include <gio/gio.h>
#include <glib.h>
#include <ubuntu-app-launch.h>

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
	gchar * package = NULL;

	/* 'Parse' the App ID */
	if (!ubuntu_app_launch_app_id_parse(appid, &package, NULL, NULL)) {
		g_warning("Unable to parse App ID: '%s'", appid);
		return NULL;
	}

	return NULL;
}

int
main (int argc, char * argv[])
{
	/* Build up our exec */
	const gchar * appid = g_getenv("APP_ID");
	if (appid == NULL) {
        g_critical("APP_ID not set for url-overlay.");
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

	if (exec == NULL) {
        g_warning("Invalid appid '%s' for url overlay.", appid);
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
