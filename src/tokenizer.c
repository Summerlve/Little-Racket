#include "../include/tokenizer.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <stddef.h>
#include <ctype.h>

// tokenizer helper function
static void tokenizer_helper(const unsigned char *line, void *aux_data);

// number type
Number_Type *number_type_new(void)
{
    Number_Type *number = (Number_Type *)malloc(sizeof(Number_Type));
    number->allocated_length = 4;
    number->logical_length = 0;
    number->contents = (unsigned char *)malloc(number->allocated_length * sizeof(unsigned char));

    if (number->contents == NULL)
    {
        perror("Number_Type::contents malloc failed");
        exit(EXIT_FAILURE);
    }

    return number;
}

int number_type_free(Number_Type *number)
{
    if (number == NULL) return 1;
    // free contents
    free(number->contents);
    // free number itself
    free(number);
    return 0;
}

int number_type_append(Number_Type *number, const unsigned char ch)
{
    // the ch must be a digit or '.' or '-' when a negative number
    if (isdigit(ch) == 0 && ch != DOT && ch != BAR)
    {
        fprintf(stderr, "number_type_append(): the ch must be a digit number or '.' or '-'.\n");
        exit(EXIT_FAILURE);
    }

    // expand the Number_Type::contents
    if (number->logical_length == number->allocated_length - 1)
    {
        number->allocated_length *= 2;
        number->contents = realloc(number->contents, number->allocated_length * sizeof(unsigned char));
        if (number->contents == NULL)
        {
            printf("errorno is: %d\n", errno);
            perror("Number_Type:contents expand failed");
            return 1;
        }
    }

    memcpy(&(number->contents[number->logical_length]), &ch, sizeof(unsigned char));
    number->logical_length ++;
    number->contents[number->logical_length] = '\0';

    return 0;
}

// remember free the unsigned char * return from here
unsigned char *get_number_type_contents(Number_Type *number)
{
    unsigned char *contents = (unsigned char *)malloc(number->logical_length + sizeof(unsigned char));
    strcpy((char *)contents, (char *)(number->contents));
    number_type_free(number);
    return contents;
}

// value must be a null-terminated string
Token *token_new(Token_Type type, const unsigned char *value)
{
    Token *token = (Token *)malloc(sizeof(Token));
    token->type = type;
    token->value = (unsigned char *)malloc(strlen((char *)value) + 1);
    strcpy((char *)(token->value), (const char *)value);
    return token;
}

int token_free(Token *token)
{
    if (token == NULL) return 1;
    free(token->value);
    free(token);
    return 0;
}

Tokens *tokens_new()
{
    Tokens *tokens = (Tokens *)malloc(sizeof(Tokens));
    tokens->allocated_length = 4; // init 4 space to store token
    tokens->logical_length = 0;
    tokens->contents = (Token **)malloc(tokens->allocated_length * sizeof(Token *));
    return tokens;
}

int tokens_free(Tokens *tokens)
{
    if (tokens == NULL) return 1;
    size_t length = tokens->logical_length;
    for (size_t i = 0; i < length; i++)
    {
        Token *token = tokens_nth(tokens, i);
        token_free(token);
    }
    free(tokens->contents);
    free(tokens);
    return 0;
}

int add_token(Tokens *tokens, Token *token)
{
    // expand the Tokens::contents
    if (tokens->logical_length == tokens->allocated_length)
    {
        tokens->allocated_length *= 2;
        tokens->contents = realloc(tokens->contents, tokens->allocated_length * sizeof(Token *));
        if (tokens->contents == NULL)
        {
            printf("errorno is: %d\n", errno);
            perror("Tokens:contents expand failed");
            return 1;
        }
    }

    memcpy(&(tokens->contents[tokens->logical_length]), &token, sizeof(Token *));
    tokens->logical_length ++;
    
    return 0;
}

size_t tokens_length(Tokens *tokens)
{
    return tokens->logical_length;
}

Token *tokens_nth(Tokens *tokens, size_t index)
{
    return tokens->contents[index];
}

void tokens_map(Tokens *tokens, TokensMapFunction map, void *aux_data)
{
    size_t length = tokens->logical_length;

    for (size_t i = 0; i < length; i++)
    {
        const Token *token = tokens_nth(tokens, i);
        map(token, aux_data);
    }
}

Tokens *tokenizer(Raw_Code *raw_code)
{
    Tokens *tokens = tokens_new();
    racket_file_map(raw_code, tokenizer_helper, tokens);
    return tokens;
}

static void tokenizer_helper(const unsigned char *line, void *aux_data)
{
    Tokens *tokens = (Tokens *)aux_data;
    size_t line_length = strlen((const char *)line);    
    size_t cursor = 0;

    for(size_t i = 0; i < line_length; i++)
    {
        // handle whitespace
        if (line[i] == WHITE_SPACE)
        {
            continue;
        }

        // handle language, character, boolean
        // language: supports only: #lang racket
        if (line[i] == POUND)
        {
            cursor = i + 1;

            // handle character such as: '#\a'
            if (line[cursor] == BACK_SLASH)
            {
                // check if it is not single char #\aa ...
                // !bug here #\11 will not be resolved correctly 
                if (isalpha(line[cursor + 2]) != 0)
                {
                    fprintf(stderr, "Character must be a single char, can not be: %s\n", &line[i]);
                    exit(EXIT_FAILURE);
                }

                const unsigned char *char_value = &line[cursor + 1];
                unsigned char *tmp = (unsigned char *)malloc(sizeof(unsigned char) * 2);
                memcpy(tmp, char_value, sizeof(unsigned char));
                tmp[1] = '\0';
                Token *token = token_new(CHARACTER, tmp);
                free(tmp);
                add_token(tokens, token);
                
                i = cursor + 1;
                continue;
            }

            // #t #f
            if (line[cursor] == 't' || line[cursor] == 'f')
            {
                const unsigned char *char_value = &line[cursor];
                unsigned char *tmp = (unsigned char *)malloc(sizeof(unsigned char) * 2);
                memcpy(tmp, char_value, sizeof(unsigned char));
                tmp[1] = '\0';
                Token *token = token_new(BOOLEAN, tmp);
                free(tmp);
                add_token(tokens, token);
                
                i = cursor;
                continue;
            }

            // #lang
            int cmp = -1;

            cmp = strncmp((const char *)(&line[cursor]), LANGUAGE_SIGN, strlen(LANGUAGE_SIGN));
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
            cmp = strcmp((const char *)(&line[cursor]), RACKET_SIGN);
            if (cmp != 0)
            {
                fprintf(stderr, "please dont use #lang %s, supports only: #lang racket\n", &line[cursor]);
                exit(EXIT_FAILURE);
            }

            Token *token = token_new(LANGUAGE, &line[cursor]); 
            add_token(tokens, token);
            return; // go to the next line
        }

        // handle comment
        // supports only: ; single line comment
        if (line[i] == SEMICOLON)
        {
            Token *token = token_new(COMMENT, &line[i + 1]);
            add_token(tokens, token);
            return; // go to the next line
        }

        // handle paren
        if (line[i] == LEFT_PAREN)
        {
            Token *token = token_new(PUNCTUATION, (const unsigned char *)"("); 
            add_token(tokens, token); 
            continue;
        }

        if (line[i] == RIGHT_PAREN)
        {
            Token *token = token_new(PUNCTUATION, (const unsigned char *)")");
            add_token(tokens, token); 
            continue;
        }

        // handle square_bracket
        if (line[i] == LEFT_SQUARE_BRACKET)
        {
            Token *token = token_new(PUNCTUATION, (const unsigned char *)"[");
            add_token(tokens, token); 
            continue;
        }

        if (line[i] == RIGHT_SQUARE_BRACKET)
        {
            Token *token = token_new(PUNCTUATION, (const unsigned char *)"]");
            add_token(tokens, token); 
            continue;
        }

        // handle number or negative nubmer
        if (isdigit(line[i]) != 0 ||
            (line[i] == BAR && isdigit(line[i + 1]) != 0))
        {
            cursor = i + 1;
            int dot_count = 0;
            Number_Type *number = number_type_new();
            number_type_append(number, line[i]);

            while (cursor < line_length)
            {

                if (isdigit(line[cursor]) != 0)
                {
                    number_type_append(number, line[cursor]);
                    cursor++;
                }
                else if (line[cursor] == DOT)
                {
                    number_type_append(number, line[cursor]);
                    dot_count++;
                    cursor++;
                    if (dot_count > 1)
                    {
                        fprintf(stderr, "A number can not be: %s\n", number->contents);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    break;
                }
            }

            unsigned char *contents = get_number_type_contents(number);
            Token *token = token_new(NUMBER, contents);
            free(contents);
            add_token(tokens, token);

            i = cursor - 1;
            continue;
        }

        // handle string
        if (line[i] == DOUBLE_QUOTE)
        {
            size_t count = 0;
            cursor = i + 1;
            bool ends = false;

            while (cursor < line_length)
            {
                if (line[cursor] != DOUBLE_QUOTE)
                {
                    count++;
                    cursor++;
                }
                else
                {
                    ends = true;
                    break;
                }
            }

            if (!ends)
            {
                fprintf(stderr, "A string must be in double quote: %s\n", line);
                exit(EXIT_FAILURE);
            }

            unsigned char *string = (unsigned char *)malloc(count + sizeof(unsigned char));
            strncpy((char *)string, (const char *)(&line[i + 1]), count);
            string[count] = '\0';
            Token *token = token_new(STRING, string);
            free(string);
            add_token(tokens, token);

            i = cursor;
            continue;
        }

        // handle apostrophe
        if (line[i] == APOSTROPHE)
        {
            Token *token = token_new(PUNCTUATION, (const unsigned char *)"\'");
            add_token(tokens, token);
            continue;
        }

        // handle dot
        if (line[i] == DOT)
        {
            // TO-DO check if a identifier contains '.', such as (define a.b 1)
            Token *token = token_new(PUNCTUATION, (const unsigned char *)".");
            add_token(tokens, token);
            continue;
        }

        // handle identifier
        // use regex to recognize identifier
        {
            // racket's identifier:
            // excludes: \ ( ) [ ] { } " , ' ` ; # | 
            // can not be full of number
            // excludes whitespace
            // ^[^\\\(\)\[\]\{\}",'`;#\|\s]+
            // cant match such 1.1a or 223a
            // const char *pattern = "^[^\\\\\\(\\)\\[\\]\\{\\}\",'`;#\\|\\s]+";
            // fucked with regex T_T

            const char *pattern = "^[a-zA-Z\\+-\\*/!]+"; // ^[a-zA-Z\+-\*/!]+
            regex_t reg;
            regmatch_t match[1];
    
            int result = regcomp(&reg, pattern, REG_EXTENDED); 
            if (result != 0)
            {
                perror("Could not compile regex");
                exit(EXIT_FAILURE);
            }
            int status = regexec(&reg, (const char *)(&line[i]), 1, match, 0);

            if (status == REG_NOMATCH)
            {
                // no match
                regfree(&reg);
            }
            else if (status == 0)
            {
                // matched
                int start_index = match[0].rm_so + i;
                int finish_index = match[0].rm_eo + i;
                int identifier_len = finish_index - start_index;
                regfree(&reg);

                unsigned char *temp = (unsigned char *)malloc((identifier_len + 1) * sizeof(unsigned char));
                memcpy(temp, &line[start_index], identifier_len);
                temp[identifier_len] = '\0';
                Token *token = token_new(IDENTIFIER, temp);
                free(temp);
                add_token(tokens, token);

                i = finish_index - 1; 
                continue;
            }
            else
            {
                perror("Regex in exceptional situations, match identifier failed");
                exit(EXIT_FAILURE);
            }
        }

        // handle ...
        
    }
}