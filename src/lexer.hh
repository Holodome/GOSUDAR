#if !defined(LEXER_HH)

#include "lib.hh"

enum {
    TOKEN_NONE,
    // ASCII tokens here...
    TOKEN_EOS  = 0x100, 
    TOKEN_IDENT,
    TOKEN_INT,
    TOKEN_REAL,
    TOKEN_STR,
};

struct Token {
    u32 token;
    union {
        const char *value_ident;
        i64         value_int;
        f64         value_real;
        const char *value_str;
    }; 
};  

struct Lexer {
    MemoryArena arena;
    
    const u8 *file_data;
    uptr file_data_size;
    
    const u8 *cursor;
    u32 current_symb;
    u32 current_line_number;
    u32 current_char_number;
    
    Token *active_token;
};  

// Lexer should have memory arena initialized prior to this call
// Buffer is zero-terminated 
// @TODO remove need to zero terminate
void lexer_init(Lexer *lexer, const void *buffer, uptr buffer_size);
Token *lexer_peek(Lexer *lexer);
void eat_tok(Lexer *lexer);

#define LEXER_HH 1
#endif
