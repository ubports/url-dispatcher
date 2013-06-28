
#include "url-dispatcher.h"

static gboolean global_succcess = FALSE;

static void
callback (const gchar * url, gboolean success, gpointer user_data)
{
	global_success = success;
	g_main_loop_quit((GMainLoop *)user_data);
	return;
}

int
main (int argc, char * argv[])
{
	if (argc != 2) {
		g_printerr("Usage: %s <url>\n", argv[0]);
		return 1;
	}

	GMainLoop * mainloop = g_main_loop_new(NULL, FALSE);

	url_dispatch_send(argv[1], callback, mainloop);

	g_main_loop_run(mainloop);

	if (global_success) {
		return 0;
	} else {
		return 1;
	}
}
