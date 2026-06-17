#define GL_GLEXT_PROTOTYPES
#define STB_TRUETYPE_IMPLEMENTATION
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <GL/gl.h>
#include "osd.h"
#include "font8x8.h"
#include "stb_truetype.h"
#include "shaders.h"

static GLuint font_tex = 0;
static GLuint osd_program = 0;
static GLuint osd_vao = 0, osd_vbo = 0;
static int use_truetype = 0;
static stbtt_bakedchar cdata[96];
static int tt_font_size = 24;
static float tt_ascent = 0;

static void gl_check_error(const char *where) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "GL error 0x%x after %s\n", err, where);
    }
}

static GLuint compile_shader(const char *src, GLenum kind) {
    GLuint shader = glCreateShader(kind);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    gl_check_error("OSD shader compile");
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fprintf(stderr, "OSD shader compile error: %s\n", log);
    }
    return shader;
}

static int try_load_truetype(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize <= 0) { fclose(f); return 0; }

    unsigned char *ttf_buf = malloc((size_t)fsize);
    if (!ttf_buf) { fclose(f); return 0; }
    if (fread(ttf_buf, 1, (size_t)fsize, f) != (size_t)fsize) {
        free(ttf_buf);
        fclose(f);
        return 0;
    }
    fclose(f);

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, ttf_buf, 0)) {
        free(ttf_buf);
        return 0;
    }

    unsigned char bitmap[512 * 512];
    int baked = stbtt_BakeFontBitmap(ttf_buf, 0, (float)tt_font_size, bitmap, 512, 512, 32, 96, cdata);
    if (baked <= 0) {
        free(ttf_buf);
        return 0;
    }

    tt_ascent = 0;
    for (int i = 0; i < 96; i++) {
        if (cdata[i].yoff < tt_ascent) tt_ascent = cdata[i].yoff;
    }
    tt_ascent = -tt_ascent;

    glGenTextures(1, &font_tex);
    gl_check_error("OSD truetype tex gen");
    glBindTexture(GL_TEXTURE_2D, font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 512, 512, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, bitmap);
    gl_check_error("OSD truetype tex upload");
    glBindTexture(GL_TEXTURE_2D, 0);

    free(ttf_buf);
    return 1;
}

static int name_matches(const char *filename, const char *name) {
    size_t nlen = strlen(name);
    for (size_t i = 0; i < nlen; i++) {
        char a = filename[i];
        char b = name[i];
        if (a == '\0') return 0;
        if (a >= 'A' && a <= 'Z') a += 32;
        if (b >= 'A' && b <= 'Z') b += 32;
        if (a != b) return 0;
    }
    return 1;
}

static int has_ext(const char *filename, const char *ext) {
    size_t flen = strlen(filename);
    size_t elen = strlen(ext);
    if (flen < elen) return 0;
    const char *fe = filename + flen - elen;
    for (size_t i = 0; i < elen; i++) {
        char a = fe[i];
        char b = ext[i];
        if (a >= 'A' && a <= 'Z') a += 32;
        if (b >= 'A' && b <= 'Z') b += 32;
        if (a != b) return 0;
    }
    return 1;
}

static int find_font(const char *name, char *out, size_t out_sz) {
    const char *base_dirs[] = {
        "/usr/share/fonts",
        "/usr/local/share/fonts",
        NULL
    };

    char stack[64][1024];
    int top = 0;

    for (int i = 0; base_dirs[i]; i++) {
        snprintf(stack[top], sizeof(stack[top]), "%s", base_dirs[i]);
        top++;
    }

    const char *home = getenv("HOME");
    if (home) {
        snprintf(stack[top], sizeof(stack[top]), "%s/.fonts", home);
        top++;
        snprintf(stack[top], sizeof(stack[top]), "%s/.local/share/fonts", home);
        top++;
    }

    while (top > 0) {
        top--;
        char *dir = stack[top];

        DIR *d = opendir(dir);
        if (!d) continue;

        struct dirent *entry;
        while ((entry = readdir(d))) {
            if (entry->d_name[0] == '.') continue;

            char full[1024];
            int n = snprintf(full, sizeof(full), "%s/%s", dir, entry->d_name);
            if (n < 0 || (size_t)n >= sizeof(full)) continue;

            struct stat st;
            if (stat(full, &st) != 0) continue;

            if (S_ISDIR(st.st_mode)) {
                if (top < 64) {
                    snprintf(stack[top], sizeof(stack[top]), "%s", full);
                    top++;
                }
            } else if (S_ISREG(st.st_mode)) {
                if (!has_ext(entry->d_name, ".ttf") && !has_ext(entry->d_name, ".otf"))
                    continue;
                if (name_matches(entry->d_name, name)) {
                    snprintf(out, out_sz, "%s", full);
                    closedir(d);
                    return 1;
                }
            }
        }
        closedir(d);
    }

    return 0;
}

static int try_init_truetype(const char *font_path) {
    if (font_path && font_path[0]) {
        if (strchr(font_path, '/')) {
            if (try_load_truetype(font_path)) {
                printf("OSD: using truetype font (%s)\n", font_path);
                return 1;
            }
        } else {
            char found[1024];
            if (find_font(font_path, found, sizeof(found))) {
                if (try_load_truetype(found)) {
                    printf("OSD: using truetype font (%s)\n", found);
                    return 1;
                }
            }
        }
    }

    const char *paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/Ubuntu-R.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        if (try_load_truetype(paths[i])) {
            printf("OSD: using truetype font (%s)\n", paths[i]);
            return 1;
        }
    }
    return 0;
}

int osd_init(const char *font_path) {
    GLuint vs = compile_shader(OSD_VERT_SRC, GL_VERTEX_SHADER);
    GLuint fs = compile_shader(OSD_FRAG_SRC, GL_FRAGMENT_SHADER);
    osd_program = glCreateProgram();
    glAttachShader(osd_program, vs);
    glAttachShader(osd_program, fs);
    glLinkProgram(osd_program);
    gl_check_error("OSD program link");

    GLint ok;
    glGetProgramiv(osd_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(osd_program, sizeof(log), NULL, log);
        fprintf(stderr, "OSD program link error: %s\n", log);
        return 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);

    if (try_init_truetype(font_path)) {
        use_truetype = 1;
    } else {
        use_truetype = 0;
        fprintf(stderr, "OSD: truetype font not found, using built-in 8x8 font\n");

        unsigned char tex[1024 * 8];
        memset(tex, 0, sizeof(tex));
        for (int c = 0; c < 128; c++) {
            for (int r = 0; r < 8; r++) {
                unsigned char byte = font8x8[c][r];
                for (int col = 0; col < 8; col++) {
                    tex[r * 1024 + c * 8 + col] = (byte >> (7 - col)) & 1 ? 255 : 0;
                }
            }
        }

        glGenTextures(1, &font_tex);
        gl_check_error("OSD bitmap tex gen");
        glBindTexture(GL_TEXTURE_2D, font_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 1024, 8, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, tex);
        gl_check_error("OSD bitmap tex upload");
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glGenVertexArrays(1, &osd_vao);
    glGenBuffers(1, &osd_vbo);

    return 1;
}

#define OSD_MAX_CHARS 256

void osd_render(const char *shader_name, float zoom, float fps, Vec2f window_size) {
    if (!osd_program) return;

    char lines[3][64];
    int nlines = 0;
    if (shader_name) {
        snprintf(lines[nlines], sizeof(lines[nlines]), "Shader: %s", shader_name);
        nlines++;
    }
    snprintf(lines[nlines], sizeof(lines[nlines]), "Zoom: %.0f%%", zoom);
    nlines++;
    snprintf(lines[nlines], sizeof(lines[nlines]), "FPS: %.0f", fps);
    nlines++;

    int total_chars = 0;
    for (int i = 0; i < nlines; i++) {
        total_chars += (int)strlen(lines[i]);
    }
    if (total_chars == 0 || total_chars > OSD_MAX_CHARS) return;

    int nverts = total_chars * 6;
    float verts[nverts * 4];
    float margin = 12.0f;
    int vi = 0;

    if (use_truetype) {
        float line_h = (float)tt_font_size * 1.3f;
        for (int line = 0; line < nlines; line++) {
            const char *str = lines[line];
            float x = margin;
            float y = margin + tt_ascent + line * line_h;
            for (int ci = 0; ci < (int)strlen(str); ci++) {
                unsigned char c = (unsigned char)str[ci];
                if (c < 32 || c >= 128) continue;

                stbtt_aligned_quad q;
                stbtt_GetBakedQuad(cdata, 512, 512, c - 32, &x, &y, &q, 1);

                verts[vi++] = q.x0; verts[vi++] = q.y0; verts[vi++] = q.s0; verts[vi++] = q.t0;
                verts[vi++] = q.x1; verts[vi++] = q.y0; verts[vi++] = q.s1; verts[vi++] = q.t0;
                verts[vi++] = q.x0; verts[vi++] = q.y1; verts[vi++] = q.s0; verts[vi++] = q.t1;
                verts[vi++] = q.x1; verts[vi++] = q.y0; verts[vi++] = q.s1; verts[vi++] = q.t0;
                verts[vi++] = q.x1; verts[vi++] = q.y1; verts[vi++] = q.s1; verts[vi++] = q.t1;
                verts[vi++] = q.x0; verts[vi++] = q.y1; verts[vi++] = q.s0; verts[vi++] = q.t1;
            }
        }
    } else {
        float scale = 2.5f;
        float char_w = 8.0f * scale;
        float char_h = 8.0f * scale;
        float line_h = char_h * 1.5f;

        for (int line = 0; line < nlines; line++) {
            const char *str = lines[line];
            float x = margin;
            float y = margin + line * line_h;
            for (int ci = 0; ci < (int)strlen(str); ci++) {
                unsigned char c = (unsigned char)str[ci];
                if (c < 32 || c >= 128) c = 32;

                float l = x, r = x + char_w;
                float t = y, b = y + char_h;
                float u0 = c / 128.0f, u1 = (c + 1) / 128.0f;

                verts[vi++] = l; verts[vi++] = t; verts[vi++] = u0; verts[vi++] = 0.0f;
                verts[vi++] = r; verts[vi++] = t; verts[vi++] = u1; verts[vi++] = 0.0f;
                verts[vi++] = l; verts[vi++] = b; verts[vi++] = u0; verts[vi++] = 1.0f;
                verts[vi++] = r; verts[vi++] = t; verts[vi++] = u1; verts[vi++] = 0.0f;
                verts[vi++] = r; verts[vi++] = b; verts[vi++] = u1; verts[vi++] = 1.0f;
                verts[vi++] = l; verts[vi++] = b; verts[vi++] = u0; verts[vi++] = 1.0f;

                x += char_w;
            }
        }
    }

    glUseProgram(osd_program);
    glUniform2f(glGetUniformLocation(osd_program, "windowSize"), window_size.x, window_size.y);
    glUniform3f(glGetUniformLocation(osd_program, "textColor"), 1.0f, 1.0f, 1.0f);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, font_tex);
    glUniform1i(glGetUniformLocation(osd_program, "fontTex"), 1);

    glBindVertexArray(osd_vao);
    glBindBuffer(GL_ARRAY_BUFFER, osd_vbo);
    glBufferData(GL_ARRAY_BUFFER, nverts * 4 * sizeof(float), verts, GL_DYNAMIC_DRAW);
    gl_check_error("OSD vbo upload");

    GLint pos_attr = glGetAttribLocation(osd_program, "position");
    GLint tc_attr = glGetAttribLocation(osd_program, "texcoord");

    glEnableVertexAttribArray(pos_attr);
    glEnableVertexAttribArray(tc_attr);
    glVertexAttribPointer(pos_attr, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribPointer(tc_attr, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glDrawArrays(GL_TRIANGLES, 0, nverts);

    glDisableVertexAttribArray(pos_attr);
    glDisableVertexAttribArray(tc_attr);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(0);
}

void osd_cleanup(void) {
    glDeleteTextures(1, &font_tex);
    glDeleteProgram(osd_program);
    glDeleteVertexArrays(1, &osd_vao);
    glDeleteBuffers(1, &osd_vbo);
    font_tex = 0;
    osd_program = 0;
    osd_vao = 0;
    osd_vbo = 0;
}
