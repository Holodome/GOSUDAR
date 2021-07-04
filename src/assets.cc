#include "assets.hh"

#include "os.hh"
#include "mips.hh"

#include "wave.hh"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "entity_kinds.hh"

AssetID get_closest_asset_match(Assets *assets, AssetType type, AssetTagList *weights, AssetTagList *matches) {
    TIMED_FUNCTION();
    assert(type && type <= ASSET_TYPE_SENTINEL);
    AssetID result = INVALID_ASSET_ID;
    
    f32 best_diff = INFINITY;
    u32 best_idx = 0;
    
    AssetTypeInfo *info = assets->type_infos + type;
    for (size_t info_idx = info->first_info_idx, i = 0;
         i < info->asset_count;
         ++info_idx, ++i) {
        AssetInfo *info = assets->asset_infos + info_idx;
        f32 total_weigted_diff = 0;
        for (size_t tag_idx = info->first_tag_idx, j = 0;
             j < info->tag_count;
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

AssetID get_first_of_type(Assets *assets, AssetType type) {
    assert(type && type <= ASSET_TYPE_SENTINEL);
    AssetID result = INVALID_ASSET_ID;
    AssetTypeInfo *info = assets->type_infos + type;
    assert(info->asset_count);
    result.value = info->first_info_idx;
    return result;
}

static void *generate_mipmaps_from_texture_data(Assets *assets, void *pixels, u32 w, u32 h) {
    void *mips_data = arena_alloc(&assets->arena, get_total_size_for_mips(w, h));
    memcpy(mips_data, pixels, w * h * 4);
    generate_sequential_mips(w, h, mips_data);
    return mips_data;
}

Texture *assets_get_texture(Assets *assets, AssetID id) {
    Texture *result = 0;
    AssetInfo *asset = assets->asset_infos + id.value;
    // @TODO this is kinda stupid whatever
    if (asset->kind == ASSET_KIND_FONT) {
        AssetFont *font = assets_get_font(assets, id);
        result = &font->texture;
    } else {
        if (asset->state == ASSET_STATE_LOADED) {
            result = asset->texture;
        } else {
            result = alloc_struct(&assets->arena, Texture);
            TempMemory load_temp = begin_temp_memory(&assets->arena);
            FileHandle file = open_file(asset->filename);
            assert(file_handle_valid(file));
            size_t file_size = get_file_size(file);
            void *file_contents = arena_alloc(&assets->arena, file_size);
            read_file(file, 0, file_size, file_contents);
            close_file(file);
            
            int w, h;
            void *pixels = stbi_load_from_memory((const u8 *)file_contents, file_size,
                &w, &h, 0, 4);
            void *mipmaps = generate_mipmaps_from_texture_data(assets, pixels, w, h);
            free(pixels);
            
            Texture texture = renderer_create_texture_mipmaps(assets->renderer, mipmaps, Vec2i(w, h));
            *result = texture;
            
            end_temp_memory(load_temp);
            asset->texture = result;
            asset->state = ASSET_STATE_LOADED;
        }
    }
    return result;
}

AssetFont *assets_get_font(Assets *assets, AssetID id) {
    AssetFont *result = 0;
    
    AssetInfo *asset = assets->asset_infos + id.value;
    assert(asset->kind == ASSET_KIND_FONT);    
    if (asset->state == ASSET_STATE_LOADED) {
        result = asset->font;
    } else {
    #define FONT_HEIGHT 16    
#define FONT_ATLAS_WIDTH  512
#define FONT_ATLAS_HEIGHT 512
#define FONT_FIRST_CODEPOINT 32
#define FONT_CODEPOINT_COUNT 95
        result = alloc_struct(&assets->arena, AssetFont);
        result->glyphs = alloc_arr(&assets->arena, FONT_CODEPOINT_COUNT, FontGlyph);
        
        TempMemory load_temp = begin_temp_memory(&assets->arena);
        FileHandle file = open_file(asset->filename);
        assert(file_handle_valid(file));
        size_t file_size = get_file_size(file);
        void *file_contents = arena_alloc(&assets->arena, file_size);
        read_file(file, 0, file_size, file_contents);
        close_file(file);

        stbtt_packedchar *glyphs = alloc_arr(&assets->arena, FONT_CODEPOINT_COUNT, stbtt_packedchar);
        u8 *font_atlas_single_channel = alloc_arr(&assets->arena, FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT, u8);
        stbtt_pack_context pack_context = {};
        stbtt_PackBegin(&pack_context, font_atlas_single_channel, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 0, 1, 0);
        stbtt_PackSetOversampling(&pack_context, 2, 2);
        stbtt_PackFontRange(&pack_context, (u8 *)file_contents, 0, FONT_HEIGHT, FONT_FIRST_CODEPOINT, FONT_CODEPOINT_COUNT,
            glyphs);
        stbtt_PackEnd(&pack_context);
        
        u8 *font_atlas = alloc_arr(&assets->arena, FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT * 4, u8);
        memset(font_atlas, 0xFF, FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT * 4);
        for (size_t i = 0; i < FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT; ++i) {
            font_atlas[i * 4 + 3] = font_atlas_single_channel[i];
        }
        void *mipmaps = generate_mipmaps_from_texture_data(assets, font_atlas, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT);

        Texture texture = renderer_create_texture_mipmaps(assets->renderer, mipmaps, Vec2i(FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT));
        result->texture = texture;
        result->first_codepoint = FONT_FIRST_CODEPOINT;
        result->codepoint_count = FONT_CODEPOINT_COUNT;
        result->height = FONT_HEIGHT;
        for (u32 i = 0; i < FONT_CODEPOINT_COUNT; ++i) {
            result->glyphs[i].utf32 = FONT_FIRST_CODEPOINT + i;
            result->glyphs[i].min_x = glyphs[i].x0;
            result->glyphs[i].min_y = glyphs[i].y0;
            result->glyphs[i].max_x = glyphs[i].x1;
            result->glyphs[i].max_y = glyphs[i].y1;
            result->glyphs[i].offset1_x = glyphs[i].xoff;
            result->glyphs[i].offset1_y = glyphs[i].yoff;
            result->glyphs[i].offset2_x = glyphs[i].xoff2;
            result->glyphs[i].offset2_y = glyphs[i].yoff2;
            result->glyphs[i].x_advance = glyphs[i].xadvance;
        }
        
        end_temp_memory(load_temp);
        asset->font = result;
        asset->state = ASSET_STATE_LOADED;
    }
    return result;
}

AssetSound *assets_get_sound(Assets *assets, AssetID id) {
    AssetSound *result = 0;
    AssetInfo *asset = assets->asset_infos + id.value;
    assert(asset->kind == ASSET_KIND_SOUND);
    if (asset->state == ASSET_STATE_LOADED) {
        result = asset->sound;
    } else {
        FileHandle file = open_file(asset->filename);
        assert(file_handle_valid(file));
        // For now loading is done not in the most optimal way.
        // First we parse the header to get all information needed,
        // and then load sample data
        // This is kinda janky because due to RIFF format specification we can't really know
        // where info headers end and data start, so we load first kilobyte of data hoping that
        // everything weill be fine. And it is, but nevertheless it would be better not to use some magic number for loading
        // This is all will not be a problem once we switch to having asset files, where we could store this data and
        // don't need additional lookups
        u32 nchannels = 0;
        u32 sample_rate = 0;
        u32 sample_count = 0;
        u32 bits_per_sample = 0;
        u64 sample_data_offset = 0;
        u64 sample_data_size = 0;
        // Load file info
        {
            TempMemory load_temp = begin_temp_memory(&assets->arena);
#define WAV_HEADERS_REGION_SIZE 1024
            void *file_contents = arena_alloc(&assets->arena, WAV_HEADERS_REGION_SIZE);
            read_file(file, 0, WAV_HEADERS_REGION_SIZE, file_contents);
            
            RIFFHeader *header = (RIFFHeader *)file_contents;
            assert(header->chunk_id == RIFF_HEADER_CHUNK_ID);
            assert(header->format == RIFF_WAV_FORMAT);
            for (RIFFChunkIter iter = iterate_riff_chunks(header + 1, (u8 *)file_contents + WAV_HEADERS_REGION_SIZE);
                is_valid(&iter);
                advance(&iter)) {
                switch (iter.chunk->id) {
                    case RIFF_FMT_CHUNK_ID: {
                        RIFFFMTChunk *fmt = (RIFFFMTChunk *)(iter.chunk + 1);
                        assert(fmt->audio_format == PCM_FORMAT);
                        assert(fmt->num_channels == 2 || fmt->num_channels == 1);
                        nchannels = fmt->num_channels;
                        sample_rate = fmt->sample_rate;
                        assert(fmt->bits_per_sample == 16);
                        bits_per_sample = fmt->bits_per_sample;
                    } break;
                    case RIFF_DATA_CHUNK_ID: {
                        sample_count = 8 * iter.chunk->size / (nchannels * bits_per_sample);     
                        sample_data_size = iter.chunk->size;       
                        sample_data_offset = (u8 *)(i16 *)(iter.chunk + 1) - (u8 *)file_contents;
                    } break;
                }        
            }
            end_temp_memory(load_temp);
        }
        assert(nchannels && sample_count);
        result = alloc_struct(&assets->arena, AssetSound);
        result->channels = nchannels;
        result->sample_rate = sample_rate;
        result->sample_count = sample_count * nchannels;
        // Note that resulting data size and source data size is the same, we just have to reoreder source data to use
        result->samples = (i16 *)arena_alloc(&assets->arena, sample_data_size);
        {
            TempMemory load_temp = begin_temp_memory(&assets->arena);
            i16 *source_samples = (i16 *)arena_alloc(&assets->arena, sample_data_size);
            read_file(file, sample_data_offset, sample_data_size, source_samples);
            memcpy(result->samples, source_samples, sample_data_size);
            end_temp_memory(load_temp);
        }
        close_file(file);
        asset->sound = result;
        asset->state = ASSET_STATE_LOADED;
    }
    return result;
}

Vec2 get_text_size(AssetFont *font, const char *text) {
    size_t count = strlen(text);
    Vec2 result = {};
    for (u32 i = 0; i < count; ++i) {
        u8 codepoint = text[i];
        if (codepoint >= font->first_codepoint) {
            FontGlyph *glyph = &font->glyphs[codepoint - font->first_codepoint];
            // FontGlyph *glyph = &glyphs[first_codepoint];
            result.x += glyph->x_advance;
        }
    }
    result.y = font->height;
    return result;
}

struct AssetBuilder {
    Assets *assets;
    u32 current_asset_type;
    u32 current_info_idx;  
};

void begin_asset_type(AssetBuilder *builder, u32 type) {
    assert(!builder->current_asset_type);
    builder->current_asset_type = type;
    AssetTypeInfo *type_info = builder->assets->type_infos + type;
    type_info->first_info_idx = builder->assets->asset_info_count;
}

void end_asset_type(AssetBuilder *builder) {
    assert(builder->current_asset_type);
    builder->current_asset_type = 0;
}

AssetInfo *add_asset_internal(AssetBuilder *builder) {
    assert(builder->current_asset_type);
    AssetTypeInfo *type_info = builder->assets->type_infos + builder->current_asset_type;
    builder->current_info_idx = type_info->first_info_idx + type_info->asset_count++;
    ++builder->assets->asset_info_count;
    AssetInfo *info = builder->assets->asset_infos + builder->current_info_idx;
    info->first_tag_idx = builder->assets->tags_count;
    return info;
}

void add_texture_asset(AssetBuilder *builder, const char *filename) {
    AssetInfo *info = add_asset_internal(builder);
    info->filename = filename;
    info->kind = ASSET_KIND_TEXTURE;
}

void add_font_asset(AssetBuilder *builder, const char *filename) {
    assert(builder->current_asset_type);
    AssetInfo *info = add_asset_internal(builder);
    info->filename = filename;
    info->kind = ASSET_KIND_FONT;
}

void add_sound_asset(AssetBuilder *builder, const char *filename) {
    assert(builder->current_asset_type);
    AssetInfo *info = add_asset_internal(builder);
    info->filename = filename;
    info->kind = ASSET_KIND_SOUND;
}

void add_tag(AssetBuilder *builder, u32 tag_id, f32 value) {
    assert(builder->current_asset_type);
    AssetInfo *current_info = builder->assets->asset_infos + builder->current_info_idx;
    u32 tag_idx = current_info->first_tag_idx + current_info->tag_count++;
    AssetTag *tag = builder->assets->tags + builder->assets->tags_count++;
    tag->id = tag_id;
    tag->value = value;
}

Assets *assets_init(Renderer *renderer) {
    Assets *assets = bootstrap_alloc_struct(Assets, arena, MEGABYTES(512));
    assets->renderer = renderer;
#define MAX_ASSET_INFOS 1024
    assets->max_asset_infos = MAX_ASSET_INFOS;
    assets->asset_infos = alloc_arr(&assets->arena, MAX_ASSET_INFOS, AssetInfo);
#define MAX_ASSET_TAGS 4096
    assets->max_tags_count = MAX_ASSET_TAGS;
    assets->tags = alloc_arr(&assets->arena, MAX_ASSET_TAGS, AssetTag);
    
    AssetBuilder builder = {};
    builder.assets = assets;
    begin_asset_type(&builder, ASSET_TYPE_PLAYER);
    add_texture_asset(&builder, "dude.png");
    end_asset_type(&builder);
    
    begin_asset_type(&builder, ASSET_TYPE_WORLD_OBJECT);
    add_texture_asset(&builder, "tree.png");
    add_tag(&builder, ASSET_TAG_WORLD_OBJECT_KIND, WORLD_OBJECT_KIND_TREE_FOREST);
    add_texture_asset(&builder, "cactus.png");
    add_tag(&builder, ASSET_TAG_WORLD_OBJECT_KIND, WORLD_OBJECT_KIND_TREE_DESERT);
    add_texture_asset(&builder, "jungle.png");
    add_tag(&builder, ASSET_TAG_WORLD_OBJECT_KIND, WORLD_OBJECT_KIND_TREE_JUNGLE);
    add_texture_asset(&builder, "building.png");
    add_tag(&builder, ASSET_TAG_WORLD_OBJECT_KIND, WORLD_OBJECT_KIND_BUILDING2);
    add_texture_asset(&builder, "building1.png");
    add_tag(&builder, ASSET_TAG_WORLD_OBJECT_KIND, WORLD_OBJECT_KIND_BUILDING1);
    add_tag(&builder, ASSET_TAG_BUILDING_IS_BUILT, 1.0f);
    add_texture_asset(&builder, "building1.png");
    add_tag(&builder, ASSET_TAG_WORLD_OBJECT_KIND, WORLD_OBJECT_KIND_BUILDING2);
    add_texture_asset(&builder, "gold.png");
    add_tag(&builder, ASSET_TAG_WORLD_OBJECT_KIND, WORLD_OBJECT_KIND_GOLD_DEPOSIT);
    end_asset_type(&builder);
    
    begin_asset_type(&builder, ASSET_TYPE_FONT);
    add_font_asset(&builder, "c:/windows/fonts/consola.ttf");
    end_asset_type(&builder);
    
    begin_asset_type(&builder, ASSET_TYPE_GRASS);
    add_texture_asset(&builder, "grass.png");
    end_asset_type(&builder);
    
    begin_asset_type(&builder, ASSET_TYPE_ADDITIONAL);
    add_texture_asset(&builder, "select.png");
    end_asset_type(&builder);
    
    begin_asset_type(&builder, ASSET_TYPE_SOUND);
    add_sound_asset(&builder, "music.wav");
    end_asset_type(&builder);
    
    return assets;
}