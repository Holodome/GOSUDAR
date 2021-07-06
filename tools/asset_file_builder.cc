#include "assets.hh"

#include "game_enums.hh"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "wave.hh"

#include "lexer.hh"
#include "lexer.cc"

static bool asset_tag_id_lookup(const char *test, u32 *dst) {
    return DEBUG_enum_string_lookup(AssetTagID_strings, test, dst);
}

static bool asset_type_lookup(const char *test, u32 *dst) {
    return DEBUG_enum_string_lookup(AssetType_strings, test, dst);
}

static bool entity_kind_lookup(const char *test, u32 *dst) {
    return DEBUG_enum_string_lookup(EntityKind_strings, test, dst);
}

struct AssetFileBuilderAssetInfo {
    void *data;
    u64 data_size;
};

struct AssetBuilder {
    u32 tags_count;
    AssetTag tags[1024];
    u32 info_count;
    AssetInfo file_infos[1024];
    AssetFileBuilderAssetInfo infos[1024];
    AssetTypeInfo type_infos[ASSET_TYPE_SENTINEL];
    
    u32 current_asset_type;
    u32 current_info_idx;  
};

static void begin_asset_type(AssetBuilder *builder, u32 type) {
    // assert(!builder->current_asset_type);
    builder->current_asset_type = type;
    AssetTypeInfo *type_info = builder->type_infos + type;
    type_info->first_info_idx = builder->info_count;
}

// static void end_asset_type(AssetBuilder *builder) {
//     assert(builder->current_asset_type);
//     builder->current_asset_type = 0;
// }

static u32 add_asset_internal(AssetBuilder *builder) {
    assert(builder->current_asset_type);
    AssetTypeInfo *type_info = builder->type_infos + builder->current_asset_type;
    builder->current_info_idx = type_info->first_info_idx + type_info->asset_count++;
    ++builder->info_count;
    u32 result = builder->current_info_idx;
    builder->file_infos[result].first_tag_idx = builder->tags_count;
    return result;
}

static void add_texture_asset(AssetBuilder *builder, const char *filename) {
    u32 info_idx = add_asset_internal(builder);
    AssetInfo *info = builder->file_infos + info_idx;
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

static void add_font_asset(AssetBuilder *builder, const char *filename) {
    u32 info_idx = add_asset_internal(builder);
    AssetInfo *info = builder->file_infos + info_idx;
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

static void add_sound_asset(AssetBuilder *builder, const char *filename) {
    u32 info_idx = add_asset_internal(builder);
    AssetInfo *info = builder->file_infos + info_idx;
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

static void add_tag(AssetBuilder *builder, u32 tag_id, f32 value) {
    assert(builder->current_asset_type);
    AssetInfo *current_info = builder->file_infos + builder->current_info_idx;
    u32 tag_idx = current_info->first_tag_idx + current_info->tag_count++;
    AssetTag *tag = builder->tags + builder->tags_count++;
    tag->id = tag_id;
    tag->value = value;
}

#define peek lexer_peek
static void expect_token(Lexer *lexer, u32 expected) {
    if (peek(lexer)->token != expected) {
        printf("Unexpected token %u (expected %u)\n", peek(lexer)->token, expected);
        assert(false);
    }
}

static void parse_asset_file_description(AssetBuilder *builder, Lexer *lexer) {
// whatever..
    while (peek(lexer)->token != TOKEN_EOS) {
        // Parse asset type declaration
        if (peek(lexer)->token == '[') {
            eat_tok(lexer);
            expect_token(lexer, TOKEN_IDENT);
            u32 asset_type = ASSET_KIND_NONE;
            const char *asset_type_string = peek(lexer)->value_ident;
            if (!asset_type_lookup(asset_type_string, &asset_type)) {
                printf("Invalid asset type '%s'\n", asset_type_string);
            }
            assert(asset_type);
            begin_asset_type(builder, asset_type);
            eat_tok(lexer);
            expect_token(lexer, ']');
            eat_tok(lexer);
            
            printf("Begin asset type '%s'\n", asset_type_string);
        } else {
            //
            // Parse asset definition
            //
            expect_token(lexer, TOKEN_IDENT);
            u32 asset_kind = ASSET_KIND_NONE;
            const char *test = peek(lexer)->value_ident;
            if (strcmp(test, "texture") == 0) {
                asset_kind = ASSET_KIND_TEXTURE;
            } else if (strcmp(test, "font") == 0) {
                asset_kind = ASSET_KIND_FONT;
            } else if (strcmp(test, "sound") == 0) {
                asset_kind = ASSET_KIND_SOUND;
            } else {
                printf("Invalid asset kind '%s'\n", test);
            }
            assert(asset_kind);
            eat_tok(lexer);
            expect_token(lexer, TOKEN_STR);
            const char *filename = peek(lexer)->value_str;
            
            switch (asset_kind) {
                case ASSET_KIND_TEXTURE: {
                    add_texture_asset(builder, filename);
                    printf("Add texture asset '%s'\n", filename);
                } break;
                case ASSET_KIND_FONT: {
                    add_font_asset(builder, filename);
                    printf("Add font asset '%s'\n", filename);
                } break;
                case ASSET_KIND_SOUND: {
                    add_sound_asset(builder, filename);
                    printf("Add sound asset '%s'\n", filename);
                } break;
                INVALID_DEFAULT_CASE;
            }
            eat_tok(lexer);
            //
            // Parse tags
            //
            if (peek(lexer)->token == '{') {
                eat_tok(lexer);
                while(peek(lexer)->token != '}') {
                    u32 tag_id = 0;
                    f32 tag_value = 0;
                    expect_token(lexer, TOKEN_IDENT);
                    const char *tag_id_string = peek(lexer)->value_ident;
                    if (!asset_tag_id_lookup(tag_id_string, &tag_id)) {
                        printf("Invalid tag id '%s'\n", tag_id_string);
                    }
                    eat_tok(lexer);
                    const char *entity_kind_str = 0;
                    if (peek(lexer)->token == TOKEN_IDENT) {
                        const char *value_string = peek(lexer)->value_ident;
                        u32 value = 0;
                        if (!entity_kind_lookup(value_string, &value)) {
                            printf("Invalid tag value, entity kind '%s' is nonexistent\n", value_string);
                        }
                        entity_kind_str = value_string;
                        tag_value = value;
                    } else if (peek(lexer)->token == TOKEN_INT) {
                        tag_value = (f32)peek(lexer)->value_int;
                    } else if (peek(lexer)->token == TOKEN_REAL) {
                        tag_value = (f32)peek(lexer)->value_real;
                    } else {
                        assert(false);
                    }
                    eat_tok(lexer);
                    add_tag(builder, tag_id, tag_value);
                    printf("Add tag %u(%s) %.2f(%s)\n", tag_id, tag_id_string, tag_value, entity_kind_str ? entity_kind_str : "");
                }
                eat_tok(lexer);
            }
        }
    } 
}

int main(int argc, char **argv) {
    printf("Start\n");
    
    char *filename;
    if (argc != 2) {
        printf("Please add filename for asset info file\n");
        filename = "..\\assets\\assets.info";
        // return 1;
    } else {
        filename = argv[1];
    }
    
    printf("Loading asset info file '%s'\n", filename);
    
    FILE *in_file = fopen(filename, "rb");
    if (!in_file) {
        printf("Failed to locate asset info file '%s'\n", filename);
        return 1;
    }
    
#define LEXER_ARENA_SIZE MEGABYTES(16)
    Lexer lexer_;
    Lexer *lexer = &lexer_;
    arena_init(&lexer->arena, malloc(LEXER_ARENA_SIZE), LEXER_ARENA_SIZE);
    
    fseek(in_file, 0, SEEK_END);
    size_t in_file_size = ftell(in_file);
    fseek(in_file, 0, SEEK_SET);
    void *in_file_data = malloc(in_file_size + 1);
    fread(in_file_data, 1, in_file_size, in_file);
    ((char *)in_file_data)[in_file_size] = 0;
    
    lexer_init(lexer, in_file_data, in_file_size + 1);
    free(in_file_data);
    fclose(in_file);
    
    AssetBuilder *builder = (AssetBuilder *)malloc(sizeof(AssetBuilder));
    memset(builder, 0, sizeof(*builder));
    
    parse_asset_file_description(builder, lexer);
    free(lexer->arena.data);
    
#define OUT_FILENAME "assets.assets"
    printf("Writing to file '%s'\n", OUT_FILENAME);
    FILE *out = fopen(OUT_FILENAME, "wb");
    if (!out) {
        printf("Failed to file for writing '%s'. Check if it is in use\n", OUT_FILENAME);
        return 1;
    }
    
    AssetFileHeader header = {};
    header.magic_value = ASSET_FILE_MAGIC_VALUE;
    header.tags_count = builder->tags_count;
    header.tags_size = header.tags_count * sizeof(AssetTag);
    header.asset_infos_count = builder->info_count;
    header.asset_infos_size = header.asset_infos_count * sizeof(AssetInfo);
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
        AssetInfo *dst = builder->file_infos + i;
        dst->data_offset = data_offset;
        dst->data_size = src->data_size;
        fwrite(src->data, src->data_size, 1, out);
        data_offset += src->data_size;
    }
    fseek(out, tags_offset, SEEK_SET);
    fwrite(builder->tags, sizeof(AssetTag), builder->tags_count, out);
    fseek(out, asset_infos_offset, SEEK_SET);
    fwrite(builder->file_infos, sizeof(AssetInfo), builder->info_count, out);
    fseek(out, asset_type_infos_offset, SEEK_SET);
    fwrite(builder->type_infos, sizeof(AssetTypeInfo), ASSET_TYPE_SENTINEL, out);
    fclose(out);
    printf("Total assets file size: %llu (%llumb)\n", data_offset, data_offset >> 20);
    printf("Assets written: %u\n", builder->info_count);
    printf("End\n");
    return 0;
}