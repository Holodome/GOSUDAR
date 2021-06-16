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
private:
    Token *next = 0;
    TokenKind kind = TokenKind::None;
    Str ident;
    i64 integer;
    f64 real;
    Str string;
    friend struct Lexer;
public:
    const Str &get_str() const;
    const Str &get_ident() const;
    i64 get_int() const;
    f64 get_real() const;
    
    bool is_kind(TokenKind kind) const;
    bool is_kind(int ascii) const;
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
    
    const Token *peek_tok();
    void eat_tok();
    const Token *peek_next_tok();
    
    void advance_character();
};

#define LEXER_HH 1
#endif
