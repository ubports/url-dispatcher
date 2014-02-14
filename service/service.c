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
#include <glib-unix.h>
#include "dispatcher.h"

GMainLoop * mainloop = NULL;

static gboolean
sig_term (gpointer user_data)
{
	g_debug("SIGTERM");
	g_main_loop_quit((GMainLoop *)user_data);
	return G_SOURCE_CONTINUE;
}

/* Where it all begins */
int
main (int argc, char * argv[])
{
	mainloop = g_main_loop_new(NULL, FALSE);

	guint term_source = g_unix_signal_add(SIGTERM, sig_term, mainloop);

	dispatcher_init(mainloop);

	/* Run Main */
	g_main_loop_run(mainloop);

	/* Clean up globals */
	dispatcher_shutdown();
	g_source_remove(term_source);
	g_main_loop_unref(mainloop);

	return 0;
}
