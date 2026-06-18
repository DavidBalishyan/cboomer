#ifndef OSD_H_
#define OSD_H_

#include <X11/Xlib.h>
#include "la.h"
#include "config.h"

int osd_init(const char *font_path);
void osd_render(const char *shader_name, float zoom, float fps, Vec2f window_size);
void osd_cleanup(void);

#endif // OSD_H_
