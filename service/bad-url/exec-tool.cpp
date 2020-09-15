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

#include <ubuntu-app-launch/helper.h>
#include <glib.h>
#include <stdexcept>

int
main (int argc, char * argv[])
{
    try {
        ubuntu::app_launch::Helper::setExec({"qmlscene", QML_BAD_URL});
        return EXIT_SUCCESS;
    } catch (std::runtime_error &e) {
        g_warning("Unable to set helper: %s", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        g_warning("Unknown failure setting exec line");
        return EXIT_FAILURE;
    }
}
