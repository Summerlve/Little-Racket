#include "./interpreter.h"
#include "./load_racket_file.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>

// tokenizer part
/* define ascii character here*/
#define WHITE_SPACE 0x20 // ' '
#define POUND 0x23 // '#'
#define LEFT_PAREN 0x28 // '('
#define RIGHT_PAREN 0x29 // ')'
#define LEFT_SQUARE_BRACKET 0x5b // '['
#define RIGHT_SQUARE_BRACKET 0x5d // ']'
#define APOSTROPHE 0x27 // '\''
#define DOT 0x2e // '.'
#define SEMICOLON 0x3b // ';'

static void tokens_new(Tokens *tokens)
{
    tokens->allocated_length = 4; // init 4 space to store token.
    tokens->logical_length = 0;
    tokens->contents = (Token *)malloc(tokens->allocated_length * sizeof(Token));
}

int free_tokens(Tokens *tokens)
{
    return 0;
}

static Token *tokens_nth(Tokens *tokens, int index)
{
    // explicit cast to (Token *) avoid gcc warning
    return (Token *)((char *)tokens->contents + index * sizeof(Token));
}

static int add_token(Tokens *tokens, Token *token)
{
    // expand the Tokens::contents
    if (tokens->logical_length == tokens->allocated_length)
    {
        tokens->contents = realloc(tokens->contents, tokens->allocated_length * 2 * sizeof(Token));
        if (tokens->contents == NULL)
        {
            printf("errorno is: %d\n", errno);
            perror("Tokens:contents expand failed");
            return 1;
        }
        tokens->allocated_length *= 2;
    }

    memcpy(&(tokens->contents[tokens->logical_length]), token, sizeof(Token));
    tokens->logical_length ++;
    
    return 0;
}

static void tokenizer_helper(const char *line, void *aux_data)
{
    Tokens *tokens = (Tokens *)aux_data;
    int line_length = strlen(line);    
    int cursor = 0;

    for(int i = 0; i < line_length; i++)
    {
        // handle whitespace.
        if (line[i] == WHITE_SPACE)
        {
            continue;
        }

        // handle language.
        // supports only: #lang racket
        if (line[i] == POUND)
        {
            const char lang_sign[] = "lang";
            const char racket_sign[] = "racket";
            int cmp = -1;

            cursor = i + 1;
            cmp = strncmp(&line[cursor], lang_sign, strlen(lang_sign));
            if (cmp != 0)
            {
                fprintf(stderr, "please use #lang to determine which language are used, supports only: #lang racket\n");
                exit(EXIT_FAILURE);
            }

            cursor = i + 5;
            if (line[cursor] != WHITE_SPACE)
            {
                fprintf(stderr, "please use #lang to determine which language are used, supports only: #lang racket\n");
                exit(EXIT_FAILURE);
            }

            cursor = i + 6;
            cmp = strcmp(&line[cursor], racket_sign);
            if (cmp != 0)
            {
                fprintf(stderr, "please dont use #lang %s, supports only: #lang racket\n", &line[cursor]);
                exit(EXIT_FAILURE);
            }

            Token *token = (Token *)malloc(sizeof(Token));
            token->type = LANGUAGE;
            token->value = (char *)malloc(strlen(&line[cursor])+ 1);
            strcpy(token->value, &line[cursor]);
            add_token(tokens, token);
            continue;
        }

        // handle comment
        // supports only: ; single line comment
        if (line[i] == SEMICOLON)
        {
            Token *token = (Token *)malloc(sizeof(Token));
            token->type = COMMENT; 
            token->value = (char *)malloc(strlen(&line[i + 1]) + 1);
            strcpy(token->value, &line[i + 1]);
            add_token(tokens, token);
            continue;
        }

        // handle identifier

        // handle paren --- maybe some problem? ---
        if (line[i] == LEFT_PAREN)
        {
            Token *token = (Token *)malloc(sizeof(Token));
            token->type = PAREN;
            token->value = (char *)malloc(sizeof(char));
            strncpy(token->value, &line[i], sizeof(char));
            add_token(tokens, token); 
            continue;
        }

        if (line[i] == RIGHT_PAREN)
        {
            Token *token = (Token *)malloc(sizeof(Token));
            token->type = PAREN;
            token->value = (char *)malloc(sizeof(char));
            strncpy(token->value, &line[i], sizeof(char));
            add_token(tokens, token); 
            continue;
        }

        // handle square_bracket
        if (line[i] == LEFT_SQUARE_BRACKET)
        {
            Token *token = (Token *)malloc(sizeof(Token));
            token->type = SQUARE_BRACKET;
            token->value = (char *)malloc(sizeof(char));
            strncpy(token->value, &line[i], sizeof(char));
            add_token(tokens, token); 
            continue;
        }

        if (line[i] == RIGHT_SQUARE_BRACKET)
        {
            Token *token = (Token *)malloc(sizeof(Token));
            token->type = SQUARE_BRACKET;
            token->value = (char *)malloc(sizeof(char));
            strncpy(token->value, &line[i], sizeof(char));
            add_token(tokens, token); 
            continue;
        }

        // handle number

        // handle string

        // handle character

        // handle apostrophe

        // handle dot

        // handle ... 
    }
}

Tokens *tokenizer(Raw_Code *raw_code)
{
    Tokens *tokens = malloc(sizeof(Tokens));
    tokens_new(tokens);
    racket_file_map(raw_code, tokenizer_helper, tokens);
    
    return tokens;
}

void tokens_map(Tokens *tokens, TokensMapFunction map, void *aux_data)
{
    int length = tokens->logical_length;

    for (int i = 0; i < length; i++)
    {
        const Token *token = tokens_nth(tokens, i);
        map(token, aux_data);
    }
}