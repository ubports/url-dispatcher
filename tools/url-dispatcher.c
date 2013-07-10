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

static gboolean global_success = FALSE;

static void
callback (const gchar * url, gboolean success, gpointer user_data)
{
	global_success = success;
	g_main_loop_quit((GMainLoop *)user_data);
	return;
}

int
main (int argc, char * argv[])
{
	if (argc != 2) {
		g_printerr("Usage: %s <url>\n", argv[0]);
		return 1;
	}

	GMainLoop * mainloop = g_main_loop_new(NULL, FALSE);

	url_dispatch_send(argv[1], callback, mainloop);

	g_main_loop_run(mainloop);

	if (global_success) {
		return 0;
	} else {
		return 1;
	}
}
