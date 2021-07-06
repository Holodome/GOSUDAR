#if !defined(ASSET_FILE_HH)

#include "lib.hh"

#define ASSET_FILE_MAGIC_VALUE PACK_4U8_TO_U32('G', 'O', 'S', 'U')

#pragma pack(push, 1)
struct AssetInfo {
    u32 kind;
    u32 first_tag_idx;
    u32 tag_count;
    // Where data is stored?
    u64 data_offset;
    u64 data_size;
    //
    // Per-kind info values
    //
    // Texture
    u32 width, height; 
    // Data is: u8[width * height * 4]
    // Font
    u32 size;
    u32 atlas_width;
    u32 atlas_height;
    u32 first_codepoint;
    u32 codepoint_count;
    // Data is: FontGlyph [codepoint_count], u8[atlas_width * atlas_height * 4]
    // Sound
    u32 channels;
    u32 sample_rate;
    u32 sample_count;
    // Data is: i16[sample_count]
};  

struct AssetFileHeader {
    u32 magic_value;
    
    u32 tags_count;
    u64 tags_offset;
    u64 tags_size; // tags_count * sizeof(AssetFileTag)
    
    u32 asset_infos_count;
    u64 asset_infos_offset;
    u64 asset_infos_size; // asset_infos_count * sizeof(AssetInfo)
    
    u32 asset_type_infos_count;
    u64 asset_type_infos_offset;
    u64 asset_type_infos_size; // asset_type_infos_count * sizeof(AssetTypeInfo)
    
    // AssetTag           [tags_count]
    // AssetInfo [asset_infos_count]
    // AssetTypeInfo      [asset_type_infos_count]
    // Data block - offsets in asset infos are used
};
#pragma pack(pop)

#define ASSET_FILE_HH 1
#endif
