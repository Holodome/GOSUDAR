#include "assets.hh"

#include "entity_kinds.hh"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "wave.hh"

struct AssetFileBuilderAssetInfo {
    void *data;
    u64 data_size;
};

struct AssetBuilder {
    u32 tags_count;
    AssetTag tags[1024];
    u32 info_count;
    AssetFileAssetInfo file_infos[1024];
    AssetFileBuilderAssetInfo infos[1024];
    AssetTypeInfo type_infos[ASSET_TYPE_SENTINEL];
    
    u32 current_asset_type;
    u32 current_info_idx;  
};

void begin_asset_type(AssetBuilder *builder, u32 type) {
    assert(!builder->current_asset_type);
    builder->current_asset_type = type;
    AssetTypeInfo *type_info = builder->type_infos + type;
    type_info->first_info_idx = builder->info_count;
}

void end_asset_type(AssetBuilder *builder) {
    assert(builder->current_asset_type);
    builder->current_asset_type = 0;
}

u32 add_asset_internal(AssetBuilder *builder) {
    assert(builder->current_asset_type);
    AssetTypeInfo *type_info = builder->type_infos + builder->current_asset_type;
    builder->current_info_idx = type_info->first_info_idx + type_info->asset_count++;
    ++builder->info_count;
    u32 result = builder->current_info_idx;
    builder->file_infos[result].first_tag_idx = builder->tags_count;
    return result;
}

void add_texture_asset(AssetBuilder *builder, const char *filename) {
    u32 info_idx = add_asset_internal(builder);
    AssetFileAssetInfo *info = builder->file_infos + info_idx;
    AssetFileBuilderAssetInfo *builder_data = builder->infos + info_idx;
    info->kind = ASSET_KIND_TEXTURE;
    
    int w, h;
    void *pixels = stbi_load(filename, &w, &h, 0, 4);
    assert(pixels);
    info->width = w;
    info->height = h;
    builder_data->data = pixels;
    builder_data->data_size = w * h * 4;
}

void add_font_asset(AssetBuilder *builder, const char *filename) {
    u32 info_idx = add_asset_internal(builder);
    AssetFileAssetInfo *info = builder->file_infos + info_idx;
    AssetFileBuilderAssetInfo *builder_data = builder->infos + info_idx;
    info->kind = ASSET_KIND_FONT;
    
#define FONT_HEIGHT 16    
#define FONT_ATLAS_WIDTH  512
#define FONT_ATLAS_HEIGHT 512
#define FONT_FIRST_CODEPOINT 32
#define FONT_CODEPOINT_COUNT 95
    info->size = FONT_HEIGHT;
    info->atlas_width = FONT_ATLAS_WIDTH;
    info->atlas_height = FONT_ATLAS_HEIGHT;
    info->first_codepoint = FONT_FIRST_CODEPOINT;
    info->codepoint_count = FONT_CODEPOINT_COUNT;
    u64 data_size = FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT * 4 + sizeof(FontGlyph) * FONT_CODEPOINT_COUNT;
    void *data = malloc(data_size);
    builder_data->data = data;
    builder_data->data_size = data_size;
    
    FILE *file = fopen(filename, "rb");
    assert(file);
    fseek(file, 0, SEEK_END);    
    u64 file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    void *file_contents = malloc(file_size);
    fread(file_contents, 1, file_size, file);
    fclose(file);

    stbtt_packedchar *glyphs = (stbtt_packedchar *)calloc(FONT_CODEPOINT_COUNT, sizeof(stbtt_packedchar));
    u8 *font_atlas_single_channel = (u8 *)calloc(FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT, sizeof(u8));
    stbtt_pack_context pack_context = {};
    stbtt_PackBegin(&pack_context, font_atlas_single_channel, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 0, 1, 0);
    stbtt_PackSetOversampling(&pack_context, 2, 2);
    stbtt_PackFontRange(&pack_context, (u8 *)file_contents, 0, FONT_HEIGHT, FONT_FIRST_CODEPOINT, FONT_CODEPOINT_COUNT,
        glyphs);
    stbtt_PackEnd(&pack_context);
    free(file_contents);
    
    u8 *font_atlas = (u8 *)data + sizeof(FontGlyph) * FONT_CODEPOINT_COUNT;
    memset(font_atlas, 0xFF, FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT * 4);
    for (size_t i = 0; i < FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT; ++i) {
        font_atlas[i * 4 + 3] = font_atlas_single_channel[i];
    }
    
    FontGlyph *dst_glyphs = (FontGlyph *)data;
    for (u32 i = 0; i < FONT_CODEPOINT_COUNT; ++i) {
        dst_glyphs[i].utf32 = FONT_FIRST_CODEPOINT + i;
        dst_glyphs[i].min_x = glyphs[i].x0;
        dst_glyphs[i].min_y = glyphs[i].y0;
        dst_glyphs[i].max_x = glyphs[i].x1;
        dst_glyphs[i].max_y = glyphs[i].y1;
        dst_glyphs[i].offset1_x = glyphs[i].xoff;
        dst_glyphs[i].offset1_y = glyphs[i].yoff;
        dst_glyphs[i].offset2_x = glyphs[i].xoff2;
        dst_glyphs[i].offset2_y = glyphs[i].yoff2;
        dst_glyphs[i].x_advance = glyphs[i].xadvance;
    }
}

void add_sound_asset(AssetBuilder *builder, const char *filename) {
    u32 info_idx = add_asset_internal(builder);
    AssetFileAssetInfo *info = builder->file_infos + info_idx;
    AssetFileBuilderAssetInfo *builder_data = builder->infos + info_idx;
    info->kind = ASSET_KIND_SOUND;
    
    FILE *file = fopen(filename, "rb");
    assert(file);
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
#define WAV_HEADERS_REGION_SIZE 1024
        void *file_contents = malloc(WAV_HEADERS_REGION_SIZE);
        fread(file_contents, WAV_HEADERS_REGION_SIZE, 1, file);
        
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
        free(file_contents);
    }
    assert(nchannels && sample_count);
    
    info->channels = nchannels;
    info->sample_rate = sample_rate;
    info->sample_count = sample_count * nchannels;
    // Note that resulting data size and source data size is the same, we just have to reoreder source data to use
    void *data = malloc(sample_data_size);
    fseek(file, sample_data_offset, SEEK_SET);
    fread(data, 1, sample_data_size, file);
    builder_data->data = data;
    builder_data->data_size = sample_data_size;
    
    fclose(file);
}

void add_tag(AssetBuilder *builder, u32 tag_id, f32 value) {
    assert(builder->current_asset_type);
    AssetFileAssetInfo *current_info = builder->file_infos + builder->current_info_idx;
    u32 tag_idx = current_info->first_tag_idx + current_info->tag_count++;
    AssetTag *tag = builder->tags + builder->tags_count++;
    tag->id = tag_id;
    tag->value = value;
}

int main() {
    printf("Start\n");
    
    AssetBuilder *builder = (AssetBuilder *)malloc(sizeof(AssetBuilder));
    memset(builder, 0, sizeof(*builder));
    
    begin_asset_type(builder, ASSET_TYPE_PLAYER);
    add_texture_asset(builder, "dude.png");
    end_asset_type(builder);
    
    begin_asset_type(builder, ASSET_TYPE_WORLD_OBJECT);
    add_texture_asset(builder, "tree.png");
    add_tag(builder, ASSET_TAG_WORLD_OBJECT_KIND, WORLD_OBJECT_KIND_TREE_FOREST);
    add_texture_asset(builder, "cactus.png");
    add_tag(builder, ASSET_TAG_WORLD_OBJECT_KIND, WORLD_OBJECT_KIND_TREE_DESERT);
    add_texture_asset(builder, "jungle.png");
    add_tag(builder, ASSET_TAG_WORLD_OBJECT_KIND, WORLD_OBJECT_KIND_TREE_JUNGLE);
    add_texture_asset(builder, "building.png");
    add_tag(builder, ASSET_TAG_WORLD_OBJECT_KIND, WORLD_OBJECT_KIND_BUILDING1);
    add_tag(builder, ASSET_TAG_BUILDING_IS_BUILT, 0.0f);
    add_texture_asset(builder, "building1.png");
    add_tag(builder, ASSET_TAG_WORLD_OBJECT_KIND, WORLD_OBJECT_KIND_BUILDING1);
    add_tag(builder, ASSET_TAG_BUILDING_IS_BUILT, 1.0f);
    add_texture_asset(builder, "building1.png");
    add_tag(builder, ASSET_TAG_WORLD_OBJECT_KIND, WORLD_OBJECT_KIND_BUILDING2);
    add_texture_asset(builder, "gold.png");
    add_tag(builder, ASSET_TAG_WORLD_OBJECT_KIND, WORLD_OBJECT_KIND_GOLD_DEPOSIT);
    end_asset_type(builder);
    
    begin_asset_type(builder, ASSET_TYPE_FONT);
    add_font_asset(builder, "c:/windows/fonts/consola.ttf");
    end_asset_type(builder);
    
    begin_asset_type(builder, ASSET_TYPE_GRASS);
    add_texture_asset(builder, "grass.png");
    end_asset_type(builder);
    
    begin_asset_type(builder, ASSET_TYPE_ADDITIONAL);
    add_texture_asset(builder, "select.png");
    end_asset_type(builder);
    
    begin_asset_type(builder, ASSET_TYPE_SOUND);
    add_sound_asset(builder, "music.wav");
    end_asset_type(builder);
    
#define OUT_FILENAME "assets.assets"
    FILE *out = fopen(OUT_FILENAME, "wb");
    assert(out);
    
    AssetFileHeader header = {};
    header.magic_value = ASSET_FILE_MAGIC_VALUE;
    header.tags_count = builder->tags_count;
    header.tags_size = header.tags_count * sizeof(AssetTag);
    header.asset_infos_count = builder->info_count;
    header.asset_infos_size = header.asset_infos_count * sizeof(AssetFileAssetInfo);
    header.asset_type_infos_count = ASSET_TYPE_SENTINEL;
    header.asset_type_infos_size = header.asset_type_infos_count * sizeof(AssetTypeInfo);
    
    u64 tags_offset = sizeof(header);
    u64 asset_infos_offset = tags_offset + header.tags_size;
    u64 asset_type_infos_offset = asset_infos_offset + header.asset_infos_size;
    u64 data_base_offset = asset_type_infos_offset + header.asset_type_infos_size;
    header.tags_offset = tags_offset;
    header.asset_infos_offset = asset_infos_offset;
    header.asset_type_infos_offset = asset_type_infos_offset;
    fwrite(&header, sizeof(header), 1, out);
    // Write data first, set data offsets
    
    fseek(out, data_base_offset, SEEK_SET);
    u64 data_offset = data_base_offset;
    for (size_t i = 0; i < builder->info_count; ++i) {
        AssetFileBuilderAssetInfo *src = builder->infos + i;
        AssetFileAssetInfo *dst = builder->file_infos + i;
        dst->data_offset = data_offset;
        dst->data_size = src->data_size;
        fwrite(src->data, src->data_size, 1, out);
        data_offset += src->data_size;
    }
    fseek(out, tags_offset, SEEK_SET);
    fwrite(builder->tags, sizeof(AssetTag), builder->tags_count, out);
    fseek(out, asset_infos_offset, SEEK_SET);
    fwrite(builder->file_infos, sizeof(AssetFileAssetInfo), builder->info_count, out);
    fseek(out, asset_type_infos_offset, SEEK_SET);
    fwrite(builder->type_infos, sizeof(AssetTypeInfo), ASSET_TYPE_SENTINEL, out);
    fclose(out);
    printf("Total assets file size: %llu\n", data_offset);
    printf("Assets written: %u\n", builder->info_count);
    printf("End\n");
    return 0;
}