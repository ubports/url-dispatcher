#include <gio/gio.h>

/* Globals */
GMainLoop * mainloop = NULL;

int
main (int argc, char * argv[])
{
	mainloop = g_main_loop_new(NULL, FALSE);


	/* Run Main */
	g_main_loop_run(mainloop);

	/* Clean up globals */
	g_main_loop_unref(mainloop);

	return 0;
}
