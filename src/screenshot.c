#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "screenshot.h"

Screenshot new_screenshot(Display *display, Window window) {
    Screenshot result;
    XWindowAttributes attributes;
    if (!XGetWindowAttributes(display, window, &attributes)) {
        attributes.width = 0;
        attributes.height = 0;
    }

#ifdef MITSHM
    result.shminfo = malloc(sizeof(XShmSegmentInfo));
    int screen = DefaultScreen(display);
    result.image = XShmCreateImage(
        display,
        DefaultVisual(display, screen),
        DefaultDepthOfScreen(ScreenOfDisplay(display, screen)),
        ZPixmap,
        NULL,
        result.shminfo,
        attributes.width,
        attributes.height);

    if (result.image) {
        result.shminfo->shmid = shmget(
            IPC_PRIVATE,
            result.image->bytes_per_line * result.image->height,
            IPC_CREAT | 0777);

        if (result.shminfo->shmid != -1) {
            result.shminfo->shmaddr = shmat(result.shminfo->shmid, 0, 0);
            if (result.shminfo->shmaddr != (void*)-1) {
                result.image->data = result.shminfo->shmaddr;
                result.shminfo->readOnly = 0;

                XShmAttach(display, result.shminfo);
                XShmGetImage(display, window, result.image, 0, 0, AllPlanes);
                return result;
            }
            shmctl(result.shminfo->shmid, IPC_RMID, 0);
        }
        XDestroyImage(result.image);
    }
    free(result.shminfo);
    result.shminfo = NULL;
#endif
    if (attributes.width == 0 || attributes.height == 0) {
        result.image = NULL;
    } else {
        result.image = XGetImage(
            display, window,
            0, 0,
            attributes.width, attributes.height,
            AllPlanes,
            ZPixmap);
    }

    return result;
}

void destroy_screenshot(Screenshot screenshot, Display *display) {
    if (!screenshot.image) return;
#ifdef MITSHM
    if (screenshot.shminfo) {
        XSync(display, 0);
        XShmDetach(display, screenshot.shminfo);
        XDestroyImage(screenshot.image);
        shmdt(screenshot.shminfo->shmaddr);
        shmctl(screenshot.shminfo->shmid, IPC_RMID, 0);
        free(screenshot.shminfo);
        return;
    }
#endif
    (void)display;
    XDestroyImage(screenshot.image);
}

void refresh_screenshot(Screenshot *screenshot, Display *display, Window window) {
    XWindowAttributes attributes;
    if (!XGetWindowAttributes(display, window, &attributes)) return;

#ifdef MITSHM
    if (XShmGetImage(display, window, screenshot->image, 0, 0, AllPlanes) == 0 ||
        attributes.width != screenshot->image->width ||
        attributes.height != screenshot->image->height) {
        destroy_screenshot(*screenshot, display);
        *screenshot = new_screenshot(display, window);
    }
#else
    if (attributes.width == screenshot->image->width && attributes.height == screenshot->image->height) {
        XImage *refreshedImage = XGetSubImage(
            display, window,
            0, 0,
            screenshot->image->width, screenshot->image->height,
            AllPlanes,
            ZPixmap,
            screenshot->image,
            0, 0);
        if (refreshedImage) return;
    }
    {
        XImage *newImage = XGetImage(
            display, window,
            0, 0,
            attributes.width, attributes.height,
            AllPlanes,
            ZPixmap);
        if (newImage) {
            XDestroyImage(screenshot->image);
            screenshot->image = newImage;
        }
    }
#endif
}

void save_to_ppm(XImage *image, const char *file_path) {
    FILE *f = fopen(file_path, "wb");
    if (!f) return;

    unsigned long total_pixels = (unsigned long)image->width * image->height;
    fprintf(f, "P6\n%u %u\n255\n", image->width, image->height);
    for (unsigned long i = 0; i < total_pixels; i++) {
        fputc(image->data[i * 4 + 2], f);
        fputc(image->data[i * 4 + 1], f);
        fputc(image->data[i * 4 + 0], f);
    }
    fclose(f);
}
