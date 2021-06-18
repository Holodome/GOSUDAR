#include "framework/lexer.hh"


const Str &Token::get_str() const {
    assert(this->kind == TokenKind::String);
    return this->string;    
}

const Str &Token::get_ident() const {
    assert(this->kind == TokenKind::Identifier);
    return this->ident;    
}

i64 Token::get_int() const {
    assert(this->kind == TokenKind::Integer);
    return this->integer;    
}

f64 Token::get_real() const {
    assert(this->kind == TokenKind::Real);
    return this->real;    
}

bool Token::is_kind(TokenKind kind) const {
    return this->kind == kind;
}

bool Token::is_kind(int ascii) const {
    assert(0 <= ascii && ascii <= 0xFF);
    return (u8)this->kind == ascii;
}

void Lexer::init(const void *buffer, size_t buffer_size) {
    this->file_data = (u8 *)Mem::alloc(buffer_size);
    memcpy(this->file_data, buffer, buffer_size);
    this->file_size = buffer_size;
    
    this->cursor = this->file_data;
    this->current_symb = *this->cursor;
    this->current_line_number = 0;
    this->current_char_number = 0;
    // this->advance_character();
}

void Lexer::cleanup() {
    Mem::free(this->file_data);
    for (Token *tok = this->first_token; tok; ) {
        Token *next = tok->next;
        delete tok;
        tok = next;
    }
}
    
const Token *Lexer::peek_tok() {
    if (this->active_token) {
        return this->active_token;
    }    
    
    Token *token = new Token;
    if (!this->first_token) {
        this->first_token = token;
    }
    if (this->last_token) {
        this->last_token->next = token;
    }
    this->active_token = token;
    this->last_token = token;
    
    for (;;) {
        if (!this->current_symb) {
            token->kind = TokenKind::EOS;
            break;
        }
        
        if (this->current_symb == '\n') {
            ++this->current_line_number;
            this->current_char_number = 0;
            this->advance_character();
            continue;
        } else if (isspace(this->current_symb)) {
            this->advance_character();
            continue;
        } else if (this->current_symb == '#') {
            while (this->current_symb != '\n') {
                this->advance_character();
            }
            continue;
        }
        
        if (isdigit(this->current_symb)) {
            const u8 *ident_start = this->cursor;
            bool is_real = false;
            do {
                this->advance_character();
                if (this->current_symb == '.') {
                    is_real = true;
                }
            } while (isdigit(this->current_symb) || this->current_symb == '.');
            // @SPEED
            Str tok = Str((const char *)ident_start, this->cursor - ident_start); 
            if (is_real) {
                token->kind = TokenKind::Real;
                token->real = atof(tok.c_str());
            } else {
                token->kind = TokenKind::Integer;
                token->integer = atoi(tok.c_str());
            }
            break;
        } else if (isalpha(this->current_symb) && this->current_symb < 0xFF) {
            const u8 *ident_start = this->cursor;
            do {
                this->advance_character();
            } while (isalpha(this->current_symb) || isdigit(this->current_symb) || this->current_symb == '_');
            token->ident = Str((const char *)ident_start, cursor - ident_start);
            token->kind = TokenKind::Identifier;
            break;
        } else if (this->current_symb == '\"') {
            this->advance_character();
            const u8 *start = this->cursor;
            do {
                this->advance_character();
            } while (this->current_symb != '\"');
            token->kind = TokenKind::String;
            token->string = Str((const char *)start, this->cursor - start);
            this->advance_character();
            break;
        } else if (ispunct(this->current_symb)) {
            token->kind = (TokenKind)((u8)this->current_symb);
            this->advance_character();
            break;
        } else {
            assert(false);
        }
    }
    return token;
}

void Lexer::eat_tok() {
    assert(this->active_token);
    this->active_token = 0;
}
    
void Lexer::advance_character() {
    ++this->current_char_number;
    ++this->cursor;
	this->current_symb = *this->cursor;
}

const Token *Lexer::peek_next_tok() {
    this->eat_tok();
    return this->peek_tok();
}