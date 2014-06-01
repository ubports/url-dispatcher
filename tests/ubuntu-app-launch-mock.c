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

#include <ubuntu-app-launch.h>
#include "ubuntu-app-launch-mock.h"

static gchar * last_appid = NULL;

gboolean
ubuntu_app_launch_start_application (const gchar * appid, const gchar * const * uris)
{
	ubuntu_app_launch_mock_clear_last_app_id();
	last_appid = g_strdup(appid);
	return TRUE;
}

void
ubuntu_app_launch_mock_clear_last_app_id (void)
{
	g_free(last_appid);
	last_appid = NULL;
	return;
}

gchar *
ubuntu_app_launch_mock_get_last_app_id (void)
{
	return last_appid;
}
