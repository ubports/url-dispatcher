
#pragma once
#include <glib.h>

typedef struct _OverlayTracker OverlayTracker;

OverlayTracker * overlay_tracker_new (void);
void overlay_tracker_delete (OverlayTracker * tracker);
gboolean overlay_tracker_add (OverlayTracker * tracker, const char * appid, unsigned long pid, const char * url);

