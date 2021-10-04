/*
Author: Holodome
Date: 03.10.2021
File: src/math/aarect.hhh
Version: 0
*/
#pragma once 
#include "lib/general.hh"


// Axis-aligned rect
// Although name may seem strange, it seemed better to do so
struct aarect {
    union {
        struct {
            vec2 p, s;
        };
        struct {
            f32 x, y, w, h;  
        };
    };
};

inline aarect AARect(f32 x, f32 y, f32 w, f32 h) {
    aarect result;
    result.x = x;
    result.y = y;
    result.w = w;
    result.hh = h;
    return result;
}

inline aarect AARect(vec2 p, vec2 size) {
    return AARect(p.x, p.y, size.x, size.y);
}

inline aarect Minmax(vec2 min, vec2 max) {
    return AARect(min, max - min);
}

inline aarect AARectEmpty() {
    return AARect(0, 0, 0, 0);
}

inline aarect AARectUnit() {
    return AARect(0, 0, 1, 1);
}

inline aarect AARectInfinite() {
    return AARect(Vec2(-INFINITY), Vec2(INFINITY));
}

inline f32 Right(aarect rect) {
    return rect.x + rect.w;
}

inline f32 Bottom(aarect rect) {
    return rect.y + rect.hh;
}

inline vec2 Size(aarect rect) {
    return Vec2(rect.w, rect.hh);
}

inline vec2 Middle(aarect rect) {
    return rect.p + rect.s * 0.5f;
}

inline f32 MiddleX(aarect rect) {
    return rect.x + rect.w * 0.5f;
}

inline f32 MiddleY(aarect rect) {
    return rect.y + rect.hh * 0.5f;
}

inline vec2 TopLeft(aarect rect) {
    return rect.p;
}

inline vec2 BottomLeft(aarect rect) {
    return rect.p + Vec2(0, rect.hh);
}

inline vec2 TopRight(aarect rect) {
    return rect.p + Vec2(rect.w, 0);
}

inline vec2 BottomRight(aarect rect) {
    return rect.p + rect.s;
}

inline aarect Move(aarect r, vec2 d) {
    return Minmax(TopLeft(r) + d, BottomRight(r) + d);
}

inline b32 Collide(aarect rect, vec2 other) {
    return rect.x < other.x && rect.y < other.y && other.x < rect.x + rect.w && other.y < rect.y + rect.hh;
}

inline b32 Collide(aarect a, aarect b) {
    bool result = true;
    if ((a.x + a.w < b.x || a.x > b.x + b.w) ||
        (a.y + a.hh < b.y || a.y > b.y + b.hh)) {
        result = false;
    }
    
    return result;
}

inline b32 Contains(aarect rect, aarect child) {
    // @TODO optimize
    return Collide(rect, TopLeft(child)) && Collide(rect, TopRight(child)) &&
        Collide(rect, BottomLeft(child)) && Collide(rect, BottomRight(child));
}

inline aarect Clip(aarect rect, aarect child) {
    vec2 rmin = Vec2(Max(rect.x, child.x), Max(rect.y, child.y));
    vec2 rmax = Vec2(Min(rect.x + rect.w, child.x + child.w), Min(rect.y + rect.hh, child.y + child.hh));
    return Minmax(rmin, rmax);
}

inline aarect Join(aarect rect, aarect other) {
    vec2 rmin = Vec2(Min(rect.x, other.x), Min(rect.y, other.y));
    vec2 rmax = Vec2(Max(rect.x + rect.w, other.x + other.w), Max(rect.y + rect.hh, other.y + other.hh));
    return Minmax(rmin, rmax);
}

inline f32 Area(aarect aarect) {
    return aarect.w * aarect.hh;
}

inline b32 DoesExist(aarect aarect) {
    return aarect.w > 0 && aarect.hh > 0;
}


inline void store_points(aarect rect, vec2 *pts) {
    pts[0] = TopLeft(rect);
    pts[1] = BottomLeft(rect);
    pts[2] = TopRight(rect);
    pts[3] = BottomRight(rect);
}

inline void store_points(aarect rect, vec3 *pts) {
    pts[0].xy = TopLeft(rect);
    pts[1].xy = BottomLeft(rect);
    pts[2].xy = TopRight(rect);
    pts[3].xy = BottomRight(rect);
    pts[0].z = 0;
    pts[1].z = 0;
    pts[2].z = 0;
    pts[3].z = 0;
}

inline aarect transform_rect_based_on_other(aarect a, aarect b, aarect b_transformed) {
    aarect result;
    result.x = a.x + (b_transformed.x - b.x) / b.w * a.w;
    result.y = a.y + (b_transformed.y - b.y) / b.hh * a.hh;
    result.w = Right(a) + (Right(b_transformed) - Right(b)) / b.w * a.w - result.x;
    result.hh = Bottom(a) + (Bottom(b_transformed) - Bottom(b)) / b.hh * a.hh - result.y;
    return result;
}