/*
 * Copyright © 2015 Canonical Ltd.
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
#include <ubuntu-app-launch.h>

int
main (int argc, char * argv[])
{
	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	g_return_val_if_fail(bus != NULL, -1);

	gchar * exec = g_strdup_printf("qmlscene %s %s", QML_BAD_URL, g_getenv("APP_URIS"));

	gboolean sended = ubuntu_app_launch_helper_set_exec(exec, NULL);
	g_free(exec);

	/* Ensuring the messages get on the bus before we quit */
	g_dbus_connection_flush_sync(bus, NULL, NULL);
	g_clear_object(&bus);

	if (sended)
		return 0;
	else
		return -1;
}
