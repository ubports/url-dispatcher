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
#include "update-directory.h"
}

#include <glib.h>
#include <glib-unix.h>
#include <memory>

#include "config.h"

/* Where it all begins */
int main (int argc, char * argv[])
{
    auto mainloop = g_main_loop_new(nullptr, false);

    g_unix_signal_add(SIGTERM, [](gpointer user_data) {
            g_main_loop_quit(static_cast<GMainLoop*>(user_data));
            return G_SOURCE_REMOVE;
        }, mainloop);

    auto tracker = overlay_tracker_new();

    if (g_getenv("URL_DISPATCHER_DISABLE_DIRECTORY_UPDATES") == NULL) {
        gchar * home = g_strconcat(g_get_home_dir(), URLS_HOME_DIRECTORY, NULL);

        update_and_monitor_directory(URLS_SYSTEM_DIRECTORY);
        update_and_monitor_directory(home);
        g_free(home);
    } else {
        g_debug("Directory updates disabled with env");
    }

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
