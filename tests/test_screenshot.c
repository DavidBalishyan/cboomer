#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <unistd.h>
#include <zlib.h>
#include "test.h"
#include "screenshot.h"

#define IMG_W 3
#define IMG_H 2
#define IMG_PIXELS (IMG_W * IMG_H)

/* 4 bytes per pixel, BGRX layout as captured from X11 */
static char image_data[IMG_PIXELS * 4];

/* Never call XDestroyImage on this: it is a macro dereferencing
 * image->f.destroy_image, which stays NULL here. Only width, height
 * and data are read by the save functions. */
static XImage make_test_image(void) {
    XImage img;
    memset(&img, 0, sizeof(img));
    img.width = IMG_W;
    img.height = IMG_H;
    img.data = image_data;
    for (int i = 0; i < IMG_PIXELS; i++) {
        image_data[i * 4 + 0] = (char)(i * 10);      /* B */
        image_data[i * 4 + 1] = (char)(i * 10 + 1);  /* G */
        image_data[i * 4 + 2] = (char)(i * 10 + 2);  /* R */
        image_data[i * 4 + 3] = (char)0xFF;          /* X */
    }
    return img;
}

static void expected_rgb(unsigned char *out) {
    for (int i = 0; i < IMG_PIXELS; i++) {
        out[i * 3 + 0] = (unsigned char)(i * 10 + 2);  /* R */
        out[i * 3 + 1] = (unsigned char)(i * 10 + 1);  /* G */
        out[i * 3 + 2] = (unsigned char)(i * 10);      /* B */
    }
}

static char out_path[64];

static const char *tmp_out_path(void) {
    strcpy(out_path, "/tmp/cboomer_test_XXXXXX");
    int fd = mkstemp(out_path);
    if (fd == -1) {
        perror("mkstemp");
        exit(1);
    }
    close(fd);
    return out_path;
}

static long read_file(const char *path, unsigned char *buf, long cap) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    long n = (long)fread(buf, 1, (size_t)cap, f);
    fclose(f);
    return n;
}

static unsigned long read_be32(const unsigned char *p) {
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) |
           ((unsigned long)p[2] << 8) | (unsigned long)p[3];
}

static void test_save_to_ppm(void) {
    XImage img = make_test_image();
    save_to_ppm(&img, tmp_out_path());

    unsigned char buf[256];
    long n = read_file(out_path, buf, sizeof(buf));
    unlink(out_path);

    const char header[] = "P6\n3 2\n255\n";
    long header_len = (long)strlen(header);
    ASSERT_EQ_INT(n, header_len + IMG_PIXELS * 3);
    ASSERT_MEM_EQ(buf, header, header_len);

    unsigned char rgb[IMG_PIXELS * 3];
    expected_rgb(rgb);
    ASSERT_MEM_EQ(buf + header_len, rgb, sizeof(rgb));
}

static void test_save_to_png(void) {
    XImage img = make_test_image();
    save_to_png(&img, tmp_out_path());

    unsigned char buf[1024];
    long n = read_file(out_path, buf, sizeof(buf));
    unlink(out_path);
    ASSERT_TRUE(n > 8);

    const unsigned char sig[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    ASSERT_MEM_EQ(buf, sig, 8);

    /* IHDR */
    const unsigned char *p = buf + 8;
    ASSERT_EQ_INT(read_be32(p), 13);
    ASSERT_MEM_EQ(p + 4, "IHDR", 4);
    const unsigned char *ihdr = p + 8;
    ASSERT_EQ_INT(read_be32(ihdr), IMG_W);
    ASSERT_EQ_INT(read_be32(ihdr + 4), IMG_H);
    ASSERT_EQ_INT(ihdr[8], 8);   /* bit depth */
    ASSERT_EQ_INT(ihdr[9], 2);   /* color type: truecolor */
    ASSERT_EQ_INT(read_be32(p + 8 + 13), crc32(crc32(0, Z_NULL, 0), p + 4, 4 + 13));

    /* IDAT: decompress and check filter bytes + pixel data */
    p += 8 + 13 + 4;
    unsigned long idat_len = read_be32(p);
    ASSERT_MEM_EQ(p + 4, "IDAT", 4);
    ASSERT_TRUE(p + 8 + idat_len + 4 <= buf + n);
    ASSERT_EQ_INT(read_be32(p + 8 + idat_len),
                  crc32(crc32(0, Z_NULL, 0), p + 4, 4 + (unsigned)idat_len));

    unsigned char raw[IMG_H * (1 + IMG_W * 3)];
    uLongf raw_len = sizeof(raw);
    ASSERT_EQ_INT(uncompress(raw, &raw_len, p + 8, idat_len), Z_OK);
    ASSERT_EQ_INT(raw_len, sizeof(raw));

    unsigned char rgb[IMG_PIXELS * 3];
    expected_rgb(rgb);
    for (int y = 0; y < IMG_H; y++) {
        const unsigned char *row = raw + y * (1 + IMG_W * 3);
        ASSERT_EQ_INT(row[0], 0);  /* filter: none */
        ASSERT_MEM_EQ(row + 1, rgb + y * IMG_W * 3, IMG_W * 3);
    }

    /* IEND */
    p += 8 + idat_len + 4;
    ASSERT_EQ_INT(read_be32(p), 0);
    ASSERT_MEM_EQ(p + 4, "IEND", 4);
    ASSERT_EQ_INT(read_be32(p + 8), crc32(crc32(0, Z_NULL, 0), p + 4, 4));
    ASSERT_EQ_INT(p + 12 - buf, n);
}

void run_screenshot_tests(void) {
    RUN_TEST(test_save_to_ppm);
    RUN_TEST(test_save_to_png);
}
