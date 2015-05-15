
typedef struct _OverlayTracker OverlayTracker;

OverlayTracker * overlay_tracker_new (void);
void overlay_tracker_delete (OverlayTracker * tracker);
void overlay_tracker_add (OverlayTracker * tracker, const char * appid, unsigned long pid);

