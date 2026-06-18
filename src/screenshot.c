#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <zlib.h>
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

static void png_write_chunk(FILE *f, const char *type,
                             const unsigned char *data, unsigned len,
                             unsigned long *crc)
{
    unsigned char buf[4];
    buf[0] = (unsigned char)(len >> 24);
    buf[1] = (unsigned char)(len >> 16);
    buf[2] = (unsigned char)(len >> 8);
    buf[3] = (unsigned char)len;
    fwrite(buf, 1, 4, f);
    fwrite(type, 1, 4, f);
    *crc = crc32(*crc, (const unsigned char *)type, 4);
    if (data && len) {
        fwrite(data, 1, len, f);
        *crc = crc32(*crc, data, len);
    }
    buf[0] = (unsigned char)(*crc >> 24);
    buf[1] = (unsigned char)(*crc >> 16);
    buf[2] = (unsigned char)(*crc >> 8);
    buf[3] = (unsigned char)*crc;
    fwrite(buf, 1, 4, f);
}

void save_to_png(XImage *image, const char *file_path) {
    FILE *f = fopen(file_path, "wb");
    if (!f) return;

    unsigned char sig[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    fwrite(sig, 1, 8, f);

    unsigned w = image->width;
    unsigned h = image->height;

    unsigned char ihdr[13] = {
        (unsigned char)(w >> 24), (unsigned char)(w >> 16),
        (unsigned char)(w >> 8),  (unsigned char)w,
        (unsigned char)(h >> 24), (unsigned char)(h >> 16),
        (unsigned char)(h >> 8),  (unsigned char)h,
        8, 2, 0, 0, 0
    };
    unsigned long crc = crc32(0, Z_NULL, 0);
    png_write_chunk(f, "IHDR", ihdr, 13, &crc);

    unsigned long row_bytes = (unsigned long)w * 3;
    unsigned long raw_size = (row_bytes + 1) * h;
    unsigned char *raw = malloc(raw_size);
    if (!raw) {
        fclose(f);
        return;
    }
    for (unsigned y = 0; y < h; y++) {
        raw[y * (row_bytes + 1)] = 0;
        for (unsigned x = 0; x < w; x++) {
            unsigned long src_i = ((unsigned long)y * w + x) * 4;
            unsigned long dst_i = y * (row_bytes + 1) + 1 + (unsigned long)x * 3;
            raw[dst_i]     = (unsigned char)image->data[src_i + 2];
            raw[dst_i + 1] = (unsigned char)image->data[src_i + 1];
            raw[dst_i + 2] = (unsigned char)image->data[src_i + 0];
        }
    }

    uLongf compressed_size = compressBound(raw_size);
    unsigned char *compressed = malloc(compressed_size);
    if (!compressed) {
        free(raw);
        fclose(f);
        return;
    }
    compress(compressed, &compressed_size, raw, raw_size);

    crc = crc32(0, Z_NULL, 0);
    png_write_chunk(f, "IDAT", compressed, (unsigned)compressed_size, &crc);

    crc = crc32(0, Z_NULL, 0);
    png_write_chunk(f, "IEND", NULL, 0, &crc);

    free(compressed);
    free(raw);
    fclose(f);
}
