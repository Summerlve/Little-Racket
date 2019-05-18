#include "./interpreter.h"
#include "./load_racket_file.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>

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

}

Tokens *tokenizer(Raw_Code *raw_code)
{
    Tokens *tokens = malloc(sizeof(Tokens));
    tokens_new(tokens);

    void *aux_data;
    racket_file_map(raw_code, tokenizer_helper, aux_data);
    return tokens;
}