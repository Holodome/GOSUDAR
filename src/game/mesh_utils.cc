
static Mesh *make_cube() {
    Vec3 p0 = Vec3(0);
    Vec3 p1 = Vec3(1);
    Vec3 p[] = {
        Vec3(p1.x, p0.y, p0.z), Vec3(p1.x, p0.y, p1.z), 
        Vec3(p0.x, p0.y, p1.z), Vec3(p0.x, p0.y, p0.z),
        Vec3(p1.x, p1.y, p0.z), Vec3(p1.x, p1.y, p1.z), 
        Vec3(p0.x, p1.y, p1.z), Vec3(p0.x, p1.y, p0.z),
        Vec3(p1.x, p1.y, p1.z), Vec3(p1.x, p0.y, p1.z),
        Vec3(p1.x, p1.y, p1.z), Vec3(p0.x, p1.y, p1.z),
        Vec3(p0.x, p0.y, p1.z), Vec3(p0.x, p0.y, p1.z),
        Vec3(p0.x, p1.y, p1.z), Vec3(p0.x, p1.y, p0.z),
        Vec3(p0.x, p0.y, p0.z), Vec3(p0.x, p1.y, p0.z),
        Vec3(p1.x, p0.y, p0.z), Vec3(p1.x, p1.y, p0.z),
        Vec3(p1.x, p0.y, p0.z), Vec3(p1.x, p0.y, p1.z),
        Vec3(p0.x, p0.y, p0.z), Vec3(p1.x, p1.y, p0.z),
    };
    Vec3 n[] = {
        Vec3(0.000000, 0.000000, -0.999756), Vec3(0.000000, -0.999756, 0.000000), 
        Vec3(0.000000, -0.999756, 0.000000), Vec3(0.000000, -0.999756, 0.000000),
        Vec3(0.999756, 0.000000, 0.000000), Vec3(0.000000, 0.999756, 0.000000),
        Vec3(0.000000, 0.999756, 0.000000), Vec3(0.000000, 0.999756, 0.000000), 
        Vec3(0.999756, 0.000000, 0.000000), Vec3(0.999756, 0.000000, 0.000000), 
        Vec3(-0.000000, 0.000000, 0.999756), Vec3(-0.000000, 0.000000, 0.999756), 
        Vec3(-0.000000, 0.000000, 0.999756), Vec3(-0.999756, -0.000000, -0.000000), 
        Vec3(-0.999756, -0.000000, -0.000000), Vec3(-0.999756, -0.000000, -0.000000), 
        Vec3(0.000000, 0.000000, -0.999756), Vec3(0.000000, 0.000000, -0.999756), 
        Vec3(0.000000, -0.999756, 0.000000), Vec3(0.000000, 0.999756, 0.000000),
        Vec3(0.999756, 0.000000, 0.000000), Vec3(-0.000000, 0.000000, 0.999756),
        Vec3(-0.999756, -0.000000, -0.000000), Vec3(0.000000, 0.000000, -0.999756),
    };
    Vec2 uv[] = {
        Vec2(0.000000, 0.333333), Vec2(0.000000, 1.000000), Vec2(0.000000, 1.000000), 
        Vec2(0.000000, 0.666667), Vec2(0.000000, 0.000000), Vec2(0.000000, 0.666667), 
        Vec2(0.000000, 0.666667), Vec2(0.000000, 1.000000), Vec2(0.000000, 0.000000), 
        Vec2(0.000000, 0.333333), Vec2(0.000000, 0.333333), Vec2(0.000000, 0.666667), 
        Vec2(0.000000, 0.666667), Vec2(0.000000, 0.333333), Vec2(0.000000, 0.000000), 
        Vec2(0.000000, 0.000000), Vec2(0.000000, 0.333333), Vec2(0.000000, 0.000000), 
        Vec2(0.000000, 0.666667), Vec2(0.000000, 1.000000), Vec2(0.000000, 0.333333), 
        Vec2(0.000000, 0.333333), Vec2(0.000000, 0.333333), Vec2(0.000000, 0.000000),
    };
    u32 vi[] = {
        1, 2, 3, 7, 6, 5, 4, 8, 9, 10, 11, 12, 13, 14, 15, 0, 16,
        17, 18, 1, 3, 19, 7, 5, 20, 4, 9, 21, 10, 12, 22, 13, 15, 23, 0, 17,  
    };
    
    Vertex vertices[ARRAY_SIZE(p)];
    for (size_t i = 0; i < ARRAY_SIZE(p); ++i) {
        vertices[i].p = p[i];
        vertices[i].uv = uv[i];
        vertices[i].n = n[i];
        vertices[i].c = Vec4(1);
    } 
    
    return new Mesh(vertices, ARRAY_SIZE(p), vi, ARRAY_SIZE(vi));
}

static Mesh *make_rect() {
    Vec3 p[] = {
        Vec3(0, 0, 0),
        Vec3(0, 1, 0),
        Vec3(1, 0, 0),
        Vec3(1, 1, 0),
    };
    u32 vi[] = {
        0, 1, 2, 3, 2, 1
    };
    Vertex vertices[ARRAY_SIZE(p)];
    for (size_t i = 0; i < ARRAY_SIZE(p); ++i) {
        vertices[i].p = p[i];
        vertices[i].uv = p[i].xy;
        vertices[i].n = Vec3(0, 0, 1);
        vertices[i].c = Vec4(1);
    }
    return new Mesh(vertices, ARRAY_SIZE(p), vi, ARRAY_SIZE(vi));
}

static Mesh *make_map(Vec2i size, f32 *height_map) {
    u32 index_count = (size.x - 1) * (size.y - 1) * 6;
#if 0
    TempArray<u32> indices(index_count);
    TempArray<Vertex> vertices((size_t)size.product());
    u32 cursor = 0;
    for (u32 i = 0; i < size.y; ++i) {
        for (u32 j = 0; j < size.x; ++j) {
            vertices[cursor].p = Vec3(i, height_map[i * size.x + j], j);
            vertices[cursor].c = bilerp(Colors::red, Colors::green, Colors::blue, Colors::white, 
                                        (f32)j / (f32)size.x, (f32)i / (f32)size.y);
            vertices[cursor].n = Vec3(0, 1, 0);
            vertices[cursor].uv = Vec2(0, 0);
            ++cursor;
        }
    }
    
    cursor = 0;
    for (u32 i = 0; i < size.y - 1; ++i) {
        for (u32 j = 0; j < size.x - 1; ++j) {
            u32 v00 = (i + 0) * size.x + (j + 0);
            u32 v01 = (i + 0) * size.x + (j + 1);
            u32 v10 = (i + 1) * size.x + (j + 0);
            u32 v11 = (i + 1) * size.x + (j + 1);
            
            indices[cursor++] = v00;
            indices[cursor++] = v01;
            indices[cursor++] = v11;
            indices[cursor++] = v11;
            indices[cursor++] = v10;
            indices[cursor++] = v00;
        }
    }
    
    return new Mesh(vertices.data, vertices.len, indices.data, indices.len);
#else 
    TempArray<u32> indices(index_count);
    TempArray<Vertex> vertices(index_count);
    TempArray<Vertex> actual_vertices((size_t)size.product());
    for (u32 i = 0; i < size.y; ++i) {
        for (u32 j = 0; j < size.x; ++j) {
            actual_vertices[i * size.x + j].p = Vec3(i, height_map[i * size.x + j], j);
            actual_vertices[i * size.x + j].c = bilerp(Colors::red, Colors::green, Colors::blue, Colors::white, 
                                        (f32)j / (f32)size.x, (f32)i / (f32)size.y);
            actual_vertices[i * size.x + j].n = Vec3(0, 1, 0);
            actual_vertices[i * size.x + j].uv = Vec2(0, 0);
        }
    }
    u32 cursor = 0;
    for (u32 i = 0; i < size.y - 1; ++i) {
        for (u32 j = 0; j < size.x - 1; ++j) {
            u32 v00 = (i + 0) * size.x + (j + 0);
            u32 v01 = (i + 0) * size.x + (j + 1);
            u32 v10 = (i + 1) * size.x + (j + 0);
            u32 v11 = (i + 1) * size.x + (j + 1);
            
            vertices[cursor] = actual_vertices[v00];
            vertices[cursor].n = triangle_normal(actual_vertices[v00].p, actual_vertices[v01].p, actual_vertices[v11].p);
            vertices[cursor].c = (actual_vertices[v00].c, actual_vertices[v01].c, actual_vertices[v11].c) / 3.0f;
            indices[cursor] = cursor;
            ++cursor;
            vertices[cursor] = actual_vertices[v01];
            indices[cursor] = cursor;
            ++cursor;
            vertices[cursor] = actual_vertices[v11];
            indices[cursor] = cursor;
            ++cursor;
            vertices[cursor] = actual_vertices[v11];
            vertices[cursor].n = triangle_normal(actual_vertices[v11].p, actual_vertices[v10].p, actual_vertices[v00].p);
            vertices[cursor].c = (actual_vertices[v11].c + actual_vertices[v10].c + actual_vertices[v00].c) / 3.0f;
            indices[cursor] = cursor;
            ++cursor;
            vertices[cursor] = actual_vertices[v10];
            indices[cursor] = cursor;
            ++cursor;
            vertices[cursor] = actual_vertices[v00];
            indices[cursor] = cursor;
            ++cursor;
        }
    }
    
    return new Mesh(vertices.data, vertices.len, indices.data, indices.len);
#endif 
}