#include "assets.hh"

#include "os.hh"
#include "mips.hh"

#include "wave.hh"

AssetID assets_get_closest_match(Assets *assets, AssetType type, AssetTagList *weights, AssetTagList *matches) {
    TIMED_FUNCTION();
    assert(type && type <= ASSET_TYPE_SENTINEL);
    AssetID result = INVALID_ASSET_ID;
    
    f32 best_diff = INFINITY;
    u32 best_idx = 0;
    
    AssetTypeInfo *info = assets->type_infos + type;
    for (size_t info_idx = info->first_info_idx, i = 0;
         i < info->asset_count;
         ++info_idx, ++i) {
        Asset *info = assets->asset_infos + info_idx;
        f32 total_weigted_diff = 0;
        for (size_t tag_idx = info->file_info.first_tag_idx, j = 0;
             j < info->file_info.tag_count;
             ++tag_idx, ++j) {
            AssetTag *tag = assets->tags + tag_idx;
            f32 diff = matches->tags[tag->id] - tag->value;
            f32 weighted_diff = weights->tags[tag->id] * fabsf(diff);
            total_weigted_diff += weighted_diff;
        }                
        
        if (total_weigted_diff < best_diff) {
            best_diff = total_weigted_diff;
            best_idx = info_idx;
        }
    }
    
    result.value = best_idx;
    return result;
}

AssetID assets_get_first_of_type(Assets *assets, AssetType type) {
    assert(type && type <= ASSET_TYPE_SENTINEL);
    AssetID result = INVALID_ASSET_ID;
    AssetTypeInfo *info = assets->type_infos + type;
    assert(info->asset_count);
    result.value = info->first_info_idx;
    return result;
}

static void *generate_mipmaps_from_texture_data(MemoryArena *arena, void *pixels, u32 w, u32 h) {
    void *mips_data = alloc(arena, get_total_size_for_mips(w, h));
    memcpy(mips_data, pixels, w * h * 4);
    generate_sequential_mips(w, h, mips_data);
    return mips_data;
}

AssetInfo *assets_get_info(Assets *assets, AssetID id) {
    Asset *asset = assets->asset_infos + id.value;
    return &asset->file_info;
}

Texture *assets_get_texture(Assets *assets, AssetID id) {
    Texture *result = 0;
    Asset *asset = assets->asset_infos + id.value;
    // @TODO this is kinda stupid whatever
    if (asset->file_info.kind == ASSET_KIND_FONT) {
        AssetFont *font = assets_get_font(assets, id);
        result = &font->texture;
    } else {
        while (asset->state != ASSET_STATE_LOADED) {
            void *pixels = alloc(assets->frame_arena, asset->file_info.data_size);
            read_file(assets->asset_file, asset->file_info.data_offset, asset->file_info.data_size, pixels);
            void *mipmaps = generate_mipmaps_from_texture_data(assets->frame_arena, pixels, asset->file_info.width, asset->file_info.height);
            asset->texture = renderer_create_texture_mipmaps(assets->renderer, mipmaps, asset->file_info.width, asset->file_info.height);
            asset->state = ASSET_STATE_LOADED;
        }
        result = &asset->texture;
    }
    return result;
}

AssetFont *assets_get_font(Assets *assets, AssetID id) {
    AssetFont *result = 0;
    
    Asset *asset = assets->asset_infos + id.value;
    assert(asset->file_info.kind == ASSET_KIND_FONT);    
    while (asset->state != ASSET_STATE_LOADED) {
        void *data = alloc(assets->frame_arena, asset->file_info.data_size);
        read_file(assets->asset_file, asset->file_info.data_offset, asset->file_info.data_size, data);
        
        // This check is due to our stupid method to purge textures..
        // we should already separate fonts and atlases
        if (!asset->font.glyphs) {
            asset->font.glyphs = alloc_arr(&assets->arena, asset->file_info.codepoint_count, FontGlyph);
            memcpy(asset->font.glyphs, data, asset->file_info.codepoint_count * sizeof(FontGlyph));
        }
        void *pixels = (u8 *)data + asset->file_info.codepoint_count * sizeof(FontGlyph);
        void *mipmaps = generate_mipmaps_from_texture_data(assets->frame_arena, pixels, asset->file_info.atlas_width, asset->file_info.atlas_height);
        asset->font.texture = renderer_create_texture_mipmaps(assets->renderer, mipmaps, 
            asset->file_info.atlas_width, asset->file_info.atlas_height);
        asset->state = ASSET_STATE_LOADED;
    }
    result = &asset->font;
    return result;
}

AssetSound *assets_get_sound(Assets *assets, AssetID id) {
    AssetSound *result = 0;
    Asset *asset = assets->asset_infos + id.value;
    assert(asset->file_info.kind == ASSET_KIND_SOUND);
    while (asset->state != ASSET_STATE_LOADED) {
        result->samples = (i16 *)alloc(&assets->arena, asset->file_info.data_size);
        read_file(assets->asset_file, asset->file_info.data_offset, asset->file_info.data_size, result->samples);
        asset->state = ASSET_STATE_LOADED;
    }
    result = &asset->sound;
    return result;
}

Vec2 DEBUG_get_text_size(Assets *assets, AssetID id, const char *text) {
    AssetInfo *info = assets_get_info(assets, id);
    AssetFont *font = assets_get_font(assets, id);
    assert(info->kind == ASSET_KIND_FONT);
    size_t count = strlen(text);
    Vec2 result = {};
    for (u32 i = 0; i < count; ++i) {
        u8 codepoint = text[i];
        if (codepoint >= info->first_codepoint) {
            FontGlyph *glyph = &font->glyphs[codepoint - info->first_codepoint];
            result.x += glyph->x_advance;
        }
    }
    result.y = info->size;
    return result;
}

void assets_purge_textures(Assets *assets) {
    // WE DO SOMETHING VERY STUPID HERE!!!
    // THIS IS JUST TO TEST HOW CHANGING RENDERER SETTINGS INTERACTS WITH ASSESTS
    for (size_t i = 0; i < assets->asset_info_count; ++i) {
        Asset *asset = assets->asset_infos + i;
        if (asset->file_info.kind == ASSET_KIND_TEXTURE && asset->state == ASSET_STATE_LOADED) {
            asset->state = ASSET_STATE_UNLOADED;
        } else if (asset->file_info.kind == ASSET_KIND_FONT && asset->state == ASSET_STATE_LOADED) {
            asset->state = ASSET_STATE_UNLOADED;
        }
    }
}

Assets *assets_init(Renderer *renderer, MemoryArena *frame_arena) {
    Assets *assets = bootstrap_alloc_struct(Assets, arena, MEGABYTES(512));
    assets->renderer = renderer;
    assets->frame_arena = frame_arena;
    
    FileHandle file = open_file("assets.assets");
    assets->asset_file = file;
    assert(file_handle_valid(file));
    size_t file_size = get_file_size(file);
    AssetFileHeader header;
    read_file(file, 0, sizeof(AssetFileHeader), &header);
    assert(header.magic_value == ASSET_FILE_MAGIC_VALUE);
    
    assets->tags_count = header.tags_count;
    assets->tags = alloc_arr(&assets->arena, assets->tags_count, AssetTag);
    assets->asset_info_count = header.asset_infos_count;
    assets->asset_infos = alloc_arr(&assets->arena, assets->asset_info_count, Asset);
    assets->type_info_count = header.asset_type_infos_count;
    assets->type_infos = alloc_arr(&assets->arena, assets->type_info_count, AssetTypeInfo);
    
    read_file(file, header.tags_offset, header.tags_size, assets->tags);
    read_file(file, header.asset_type_infos_offset, header.asset_type_infos_size, assets->type_infos);
    TempMemory info_temp = begin_temp_memory(&assets->arena);
    AssetInfo *src_infos = alloc_arr(&assets->arena, header.asset_infos_count, AssetInfo);
    read_file(file, header.asset_infos_offset, header.asset_infos_size, src_infos);
    for (size_t i = 0; i < header.asset_infos_count; ++i) {
        AssetInfo *src = src_infos + i;
        AssetInfo *dst = &assets->asset_infos[i].file_info;
        *dst = *src;
    }
    end_temp_memory(info_temp);
    
    return assets;
}