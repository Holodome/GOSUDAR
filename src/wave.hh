//
// File describing wave file format (WAV)
//
#if !defined(WAVE_HH)

#include "lib.hh"
#include "sound.hh"

#pragma pack(push, 1)
struct RIFFHeader {
    u32 chunk_id;
    u32 chunk_size;
    u32 format;
};  

struct RIFFChunk {
    u32 id;
    u32 size;
};

struct RIFFFMTChunk {
    u16 audio_format;
    u16 num_channels;
    u32 sample_rate;
    u32 byte_rate;
    u16 block_align;
    u16 bits_per_sample;  
};
#pragma pack(pop)

struct RIFFChunkIter {
    RIFFChunk *chunk;
    u8 *at;  
    u8 *stop;
};

inline RIFFChunkIter iterate_riff_chunks(void *at, void *stop) {
    RIFFChunkIter result;
    result.chunk = (RIFFChunk *)at;
    result.at = (u8 *)at;
    result.stop = (u8 *)stop;
    return result;
}

inline bool is_valid(RIFFChunkIter *iter) {
    return iter->at < iter->stop;
}

inline void advance(RIFFChunkIter *iter) {
    iter->at += sizeof(RIFFChunk) + iter->chunk->size;
    iter->chunk = (RIFFChunk *)iter->at;
}

#define PCM_FORMAT 1
#define RIFF_HEADER_CHUNK_ID  PACK_4U8_TO_U32('R', 'I', 'F', 'F')
#define RIFF_WAV_FORMAT       PACK_4U8_TO_U32('W', 'A', 'V', 'E')
#define RIFF_FMT_CHUNK_ID     PACK_4U8_TO_U32('f', 'm', 't', ' ')
#define RIFF_DATA_CHUNK_ID    PACK_4U8_TO_U32('d', 'a', 't', 'a')

#define WAVE_HH 1
#endif
