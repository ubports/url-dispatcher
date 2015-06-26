
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
