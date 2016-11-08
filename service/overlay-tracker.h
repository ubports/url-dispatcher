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

#pragma once
#include <glib.h>

typedef struct _OverlayTracker OverlayTracker;

OverlayTracker * overlay_tracker_new ();
void overlay_tracker_delete (OverlayTracker * tracker);
gboolean overlay_tracker_add (OverlayTracker * tracker, const char * appid, unsigned long pid, const char * url);
gboolean overlay_tracker_badurl (OverlayTracker * tracker, unsigned long pid, const char * url);

