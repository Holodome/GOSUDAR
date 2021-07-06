#include "lexer.hh"

void lexer_init(Lexer *lexer, const void *buffer, size_t buffer_size) {
    lexer->file_data = (u8 *)alloc_copy(&lexer->arena, buffer, buffer_size);
    lexer->file_data_size = buffer_size;
    lexer->cursor = lexer->file_data;
    lexer->current_symb = *lexer->cursor;
    lexer->current_line_number = 0;
    lexer->current_char_number = 0;
    lexer->active_token = 0;
}

static void advance_character(Lexer *lexer) {
    ++lexer->current_char_number;
    ++lexer->cursor;
    lexer->current_symb = *lexer->cursor;
}

Token *lexer_peek(Lexer *lexer) {
    if (lexer->active_token) {
        return lexer->active_token;
    }
    
    Token *token = alloc_struct(&lexer->arena, Token);
    lexer->active_token = token;
    
    for (;;) {
        if (!lexer->current_symb) {
            token->token = TOKEN_EOS;
            break;
        }
        
        if (lexer->current_symb == '\n') {
            ++lexer->current_line_number;
            lexer->current_char_number = 0;
            advance_character(lexer);
            continue;
        } else if (isspace(lexer->current_symb)) {
            advance_character(lexer);
            continue;
        } else if (lexer->current_symb == '#') {
            while (lexer->current_symb != '\n') {
                advance_character(lexer);
            }
            continue;
        }
        
        if (isdigit(lexer->current_symb)) {
            const u8 *ident_start = lexer->cursor;
            bool is_real = false;
            do {
                advance_character(lexer);
                if (lexer->current_symb == '.') {
                    is_real = true;
                }
            } while (isdigit(lexer->current_symb) || lexer->current_symb == '.');
            // @SPEED
            TempMemory temp = begin_temp_memory(&lexer->arena);
            u32 len = lexer->cursor - ident_start;
            char *number_str = (char *)alloc_copy(&lexer->arena, ident_start, len + 1);
            number_str[len] = 0;
            if (is_real) {
                token->token = TOKEN_REAL;
                token->value_real = atof(number_str);
            } else {
                token->token = TOKEN_INT;
                token->value_int = atoi(number_str);
            }
            end_temp_memory(temp);
            break;
        } else if (isalpha(lexer->current_symb) && lexer->current_symb < 0xFF) {
            const u8 *ident_start = lexer->cursor;
            do {
                advance_character(lexer);
            } while (isalpha(lexer->current_symb) || isdigit(lexer->current_symb) || lexer->current_symb == '_');
            u32 len = lexer->cursor - ident_start;
            char *string = (char *)alloc_copy(&lexer->arena, ident_start, len + 1);
            string[len] = 0;
            token->token = TOKEN_IDENT;
            token->value_ident = string;
            break;
        } else if (lexer->current_symb == '\"') {
            advance_character(lexer);
            const u8 *start = lexer->cursor;
            do {
                advance_character(lexer);
            } while (lexer->current_symb != '\"');
            u32 len = lexer->cursor - start;
            char *string = (char *)alloc_copy(&lexer->arena, start, len + 1);
            string[len] = 0;
            token->token = TOKEN_STR;
            token->value_str = string;
            advance_character(lexer);
            break;
        } else if (ispunct(lexer->current_symb)) {
            token->token = (u8)lexer->current_symb;
            advance_character(lexer);
            break;
        } else {
            printf("Undexpected symbol %u.\n", lexer->current_symb);
            break;
        }
    }
    return token;
}

void eat_tok(Lexer *lexer) {
    if (lexer->active_token) {
        lexer->active_token = 0;
    }
}


