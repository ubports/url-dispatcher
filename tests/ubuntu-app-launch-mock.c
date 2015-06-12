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
ubuntu_app_launch_mock_clear_last_app_id ()
{
	g_free(last_appid);
	last_appid = NULL;
	return;
}

gchar *
ubuntu_app_launch_mock_get_last_app_id ()
{
	return last_appid;
}

UbuntuAppLaunchHelperObserver ubuntu_app_launch_mock_observer_helper_stop_func = NULL;
gchar * ubuntu_app_launch_mock_observer_helper_stop_type = NULL;
void * ubuntu_app_launch_mock_observer_helper_stop_user_data = NULL;

gboolean
ubuntu_app_launch_observer_add_helper_stop (UbuntuAppLaunchHelperObserver func, const gchar * type, gpointer user_data)
{
	ubuntu_app_launch_mock_observer_helper_stop_func = func;
	ubuntu_app_launch_mock_observer_helper_stop_type = g_strdup(type);
	ubuntu_app_launch_mock_observer_helper_stop_user_data = user_data;

	return TRUE;
}

gboolean
ubuntu_app_launch_observer_delete_helper_stop (UbuntuAppLaunchHelperObserver func, const gchar * type, gpointer user_data)
{
	gboolean same = ubuntu_app_launch_mock_observer_helper_stop_func == func &&
		g_strcmp0(ubuntu_app_launch_mock_observer_helper_stop_type, type) == 0 &&
		ubuntu_app_launch_mock_observer_helper_stop_user_data == user_data;
	
	ubuntu_app_launch_mock_observer_helper_stop_func = NULL;
	g_clear_pointer(&ubuntu_app_launch_mock_observer_helper_stop_type, g_free);
	ubuntu_app_launch_mock_observer_helper_stop_user_data = NULL;

	return same;
}

gchar * ubuntu_app_launch_mock_last_start_session_helper = NULL;
MirPromptSession * ubuntu_app_launch_mock_last_start_session_session = NULL;
gchar * ubuntu_app_launch_mock_last_start_session_appid = NULL;
gchar ** ubuntu_app_launch_mock_last_start_session_uris = NULL;

gchar *
ubuntu_app_launch_start_session_helper (const gchar * type, MirPromptSession * session, const gchar * appid, const gchar * const * uris)
{
	g_clear_pointer(&ubuntu_app_launch_mock_last_start_session_helper, g_free);
	g_clear_pointer(&ubuntu_app_launch_mock_last_start_session_appid, g_free);
	g_clear_pointer(&ubuntu_app_launch_mock_last_start_session_uris, g_strfreev);

	ubuntu_app_launch_mock_last_start_session_helper = g_strdup(type);
	ubuntu_app_launch_mock_last_start_session_session = session;
	ubuntu_app_launch_mock_last_start_session_appid = g_strdup(appid);
	ubuntu_app_launch_mock_last_start_session_uris = g_strdupv((gchar **)uris);

	return g_strdup("instance");
}

gchar * ubuntu_app_launch_mock_last_stop_helper = NULL;
gchar * ubuntu_app_launch_mock_last_stop_appid = NULL;
gchar * ubuntu_app_launch_mock_last_stop_instance = NULL;

gboolean
ubuntu_app_launch_stop_multiple_helper (const gchar * helper_type, const gchar * appid, const gchar * instance) {
	g_clear_pointer(&ubuntu_app_launch_mock_last_stop_helper, g_free);
	g_clear_pointer(&ubuntu_app_launch_mock_last_stop_appid, g_free);
	g_clear_pointer(&ubuntu_app_launch_mock_last_stop_instance, g_free);

	ubuntu_app_launch_mock_last_stop_helper = g_strdup(helper_type);
	ubuntu_app_launch_mock_last_stop_appid = g_strdup(appid);
	ubuntu_app_launch_mock_last_stop_instance = g_strdup(instance);

	return TRUE;
}
