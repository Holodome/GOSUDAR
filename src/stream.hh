#if !defined(STREAM_HH)

#include "lib.hh"

struct StreamChunk {
    u32 contents_size;
    void *contents;
    
    StreamChunk *next;
};

struct Stream {
    u32 contents_size;
    void *contents;
    
    u32 bit_count;
    u32 bit_buf;
    
    bool is_underflowed;
    StreamChunk *first;
    StreamChunk *last;
};

extern Stream *error_stream;
extern Stream *info_stream;
void outv(Stream *stream, const char *format, va_list args);
void outf(Stream *stream, const char *format, ...);

Stream on_demand_memory_stream() {
    Stream result = {};
    return result;
}
void refill_in_necessary(Stream *stream) {
    if (stream->contents_size == 0 && stream->first) {
        StreamChunk *current = stream->first;
        stream->contents_size = current->contents_size;
        stream->contents = current->contents;
        stream->first = current->next;
    }
}
#define consume_type(_stream, _type) (_type *)consume_size(_stream, sizeof(_type))
void *consume_size(Stream *stream, u32 size) {
    void *result = 0;
    refill_in_necessary(stream);
    if (stream->contents_size >= size) {
        result = stream->contents;
        stream->contents = (u8 *)stream->contents + size;
        stream->contents_size -= size;
    } else {
        outf(error_stream, "Stream underflow");
        stream->contents_size = 0;
        stream->is_underflowed = true;
    }
    assert(!stream->is_underflowed);
    return result;
}
u32 peek_bits(Stream *stream, u32 bit_count) {
    assert(bit_count <= 32);
    u32 result = 0;
    while (stream->bit_count < bit_count && !stream->is_underflowed) {
        u32 byte = *consume_type(stream, u8);
        stream->bit_buf |= (byte << stream->bit_count);
        stream->bit_count += 8;
    }
    result = stream->bit_buf & ((1 << bit_count) - 1);
    return result;
}
void discard_bits(Stream *stream, u32 bit_count) {
    stream->bit_count -= bit_count;
    stream->bit_buf >>= bit_count;
}
u32 consume_bits(Stream *stream, u32 bit_count) {
    u32 result = peek_bits(stream, bit_count);
    discard_bits(stream, bit_count);
    return result;
}
void flush_byte(Stream *stream) {
    u32 flush_count = (stream->bit_count % 8);
    consume_bits(stream, flush_count);
}
#define STREAM_HH 1
#endif
