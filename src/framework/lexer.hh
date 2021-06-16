#if !defined(LEXER_HH)

#include "lib/lib.hh"

enum struct TokenKind {
    None = 0x0,
    EOS = 0x100,
    Identifier,
    Integer,
    Real,
    String
};

struct Token {
    TokenKind kind = TokenKind::None;
    
    u32 utf32; 
    Str ident;
    i64 integer;
    f64 real;
    Str string;
    
    Token *next = 0;
};

struct Lexer {
    u8 *file_data = 0;
    size_t file_size = 0;
    
    const u8 *cursor = 0;
    u32 current_symb = 0;
    u32 current_line_number = 0;
    u32 current_char_number = 0;
    
    Token *first_token = 0;
    Token *last_token = 0;
    Token *active_token = 0;
    
    void init(const void *buffer, size_t buffer_size);
    void cleanup();
    
    Token *peek_tok();
    void eat_tok();
    
    void advance_character();
};

#define LEXER_HH 1
#endif
