#include "./interpreter.h"
#include "./load_racket_file.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>

// tokenizer part
/* define ascii character here*/
#define WHITE_SPACE 0x20

static void tokens_new(Tokens *tokens)
{
    tokens->allocated_length = 4; // init 4 space to store token.
    tokens->logical_length = 0;
    tokens->content = (Token *)malloc(tokens->allocated_length * sizeof(Token));
}

static int tokens_free(Tokens *tokens)
{
    return 0;
}

static Token *tokens_nth(Tokens *tokens, int index)
{
    // explicit cast to (Token *) avoid gcc warning
    return (Token *)((char *)tokens->content + index * sizeof(Token));
}

static int add_token(Tokens *tokens, Token *token)
{
    // expand the Tokens::content
    if (tokens->logical_length == tokens->allocated_length)
    {
        tokens->content = realloc(tokens->content, tokens->allocated_length * 2 * sizeof(Token));
        if (tokens->content == NULL)
        {
            printf("errorno is: %d\n", errno);
            perror("Tokens:content expand failure");
            return 1;
        }
        tokens->allocated_length *= 2;
    }

    memcpy(&(tokens->content[tokens->logical_length]), token, sizeof(Token));
    tokens->logical_length ++;
    
    return 0;
}

static void tokenizer_helper(const char *line, void *aux_data)
{
    Tokens *tokens = (Tokens *)aux_data;
    int line_length = strlen(line);    
    int cursor = 0;

    for (int i = 0; i < line_length; i++)
    {
        cursor = i;

        // handle whitespace.
        if (line[i] == WHITE_SPACE)
        {
            continue;
        }

        // handle language.
        // #lang racket
        if (line[i] == '#')
        {
            const char lang_sign[] = "lang";
            const char racket_sign[] = "racket";
            int cmp = -1;

            cmp = strncmp(&line[i + 1], lang_sign, 4);
            if (cmp != 0)
            {
                fprintf(stderr, "please use #lang to determine which language are used, supports only: #lang racket\n");
                exit(EXIT_FAILURE);
            }

            
        }
    }
}

Tokens *tokenizer(Raw_Code *raw_code)
{
    Tokens *tokens = malloc(sizeof(Tokens));
    tokens_new(tokens);

    racket_file_map(raw_code, tokenizer_helper, tokens);
    return tokens;
}