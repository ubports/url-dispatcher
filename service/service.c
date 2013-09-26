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
#include "dispatcher.h"

GMainLoop * mainloop = NULL;

/* Where it all begins */
int
main (int argc, char * argv[])
{
	mainloop = g_main_loop_new(NULL, FALSE);

	dispatcher_init(mainloop);

	/* Run Main */
	g_main_loop_run(mainloop);

	/* Clean up globals */
	dispatcher_shutdown();
	g_main_loop_unref(mainloop);

	return 0;
}
