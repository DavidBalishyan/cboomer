#ifndef SCREENSHOT_H_
#define SCREENSHOT_H_

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef MITSHM
#include <X11/extensions/XShm.h>
#endif

typedef struct {
    XImage *image;
#ifdef MITSHM
    XShmSegmentInfo *shminfo;
#endif
} Screenshot;

Screenshot new_screenshot(Display *display, Window window);
void destroy_screenshot(Screenshot screenshot, Display *display);
void refresh_screenshot(Screenshot *screenshot, Display *display, Window window);
void save_to_ppm(XImage *image, const char *file_path);
void save_to_png(XImage *image, const char *file_path);

#endif // SCREENSHOT_H_
