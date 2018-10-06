/**
 * Copyright (C) 2013-2017 Canonical, Ltd.
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

extern "C" {
#include "dispatcher.h"
}

#include <glib.h>
#include <glib-unix.h>
#include <memory>

/* Where it all begins */
int main (int argc, char * argv[])
{
    auto mainloop = g_main_loop_new(nullptr, false);

    g_unix_signal_add(SIGTERM, [](gpointer user_data) {
            g_main_loop_quit(static_cast<GMainLoop*>(user_data));
            return G_SOURCE_REMOVE;
        }, mainloop);

    auto tracker = overlay_tracker_new();

    /* Initialize Dispatcher */
    if (!dispatcher_init(mainloop, tracker)) {
        return -1;
    }

    /* Run Main */
    g_main_loop_run(mainloop);

    /* Clean up globals */
    dispatcher_shutdown();
    overlay_tracker_delete(tracker);
    g_main_loop_unref(mainloop);

    return 0;
}
