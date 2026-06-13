#ifndef LA_H_
#define LA_H_

typedef struct {
    float x;
    float y;
} Vec2f;

Vec2f vec2(float x, float y);
Vec2f vec2_mul_f(Vec2f a, float s);
Vec2f vec2_div_f(Vec2f a, float s);
Vec2f vec2_mul(Vec2f a, Vec2f b);
Vec2f vec2_div(Vec2f a, Vec2f b);
Vec2f vec2_sub(Vec2f a, Vec2f b);
Vec2f vec2_add(Vec2f a, Vec2f b);
void vec2_add_eq(Vec2f *a, Vec2f b);
void vec2_sub_eq(Vec2f *a, Vec2f b);
float vec2_length(Vec2f a);
Vec2f vec2_normalize(Vec2f a);

#endif
