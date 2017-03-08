/**
 * Copyright © 2015 Canonical, Ltd.
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
#include <sys/apparmor.h>

const char * aa_mock_gettask_profile = NULL;

int aa_gettaskcon (pid_t pid, char ** profile, char ** mode)
{
	g_debug("Gettask Con '%s'", aa_mock_gettask_profile);

	if (aa_mock_gettask_profile == NULL) {
		return 1;
	}

	if (profile != NULL) {
		*profile = g_strdup(aa_mock_gettask_profile);
	}

	return 0;
}

/* override the unity8 pid for tests */
gboolean g_spawn_command_line_sync (const gchar * command_line,
                                    gchar ** standard_output,
                                    gchar ** standard_error,
                                    gint * exit_status,
                                    GError ** error)
{
    if (g_strcmp0(command_line, "pidof unity8-dash") == 0) {
        /* We only use standard_output arg in the real code for this. */
        *standard_output = g_strdup("1234");
        return TRUE;
    } else {
        gchar ** args;
        g_shell_parse_argv(command_line, NULL, &args, NULL);
        gboolean result = g_spawn_sync(NULL, args, NULL, 0, NULL, NULL,
                                       standard_output, standard_error,
                                       exit_status, error);
        g_strfreev(args);
        return result;
    }
}
