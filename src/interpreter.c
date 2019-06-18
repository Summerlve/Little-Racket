#include "./interpreter.h"
#include "./load_racket_file.h"
#include "./vector.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <regex.h>
#include <stdarg.h>

// tokenizer parts macro definitions.
/* define ascii character here */
#define LANGUAGE_SIGN "lang"
#define RACKET_SIGN "racket"
#define WHITE_SPACE 0x20 // ' '
#define POUND 0x23 // '#'
#define LEFT_PAREN 0x28 // '('
#define RIGHT_PAREN 0x29 // ')'
#define LEFT_SQUARE_BRACKET 0x5b // '['
#define RIGHT_SQUARE_BRACKET 0x5d // ']'
#define APOSTROPHE 0x27 // '\''
#define DOT 0x2e // '.'
#define SEMICOLON 0x3b // ';'
#define DOUBLE_QUOTE 0x22 // '\"'
#define BACK_SLASH 0x5c // '\'

// tokenizer parts.
// number type
static Number_Type *number_type_new()
{
    Number_Type *number = (Number_Type *)malloc(sizeof(Number_Type));
    number->allocated_length = 4;
    number->logical_length = 0;
    number->contents = (char *)malloc(number->allocated_length * sizeof(char));
    if (number->contents == NULL)
    {
        perror("Number_Type::contents malloc failed");
        exit(EXIT_FAILURE);
    }

    return number;
}

static int number_type_free(Number_Type *number)
{
    // free contents
    free(number->contents);
    // free number itself
    free(number);
    return 0;
}

static int number_type_append(Number_Type *number, const char ch)
{
    // the ch must be a digit or '.'.
    // expand the Number_Type::contents.
    if (number->logical_length == number->allocated_length - 1)
    {
        number->allocated_length *= 2;
        number->contents = realloc(number->contents, number->allocated_length * sizeof(char));
        if (number->contents == NULL)
        {
            printf("errorno is: %d\n", errno);
            perror("Number_Type:contents expand failed");
            return 1;
        }
    }

    memcpy(&(number->contents[number->logical_length]), &ch, sizeof(char));
    number->logical_length ++;
    number->contents[number->logical_length] = '\0';

    return 0;
}

// remember free the char * return from here.
static char *get_number_type_contents(Number_Type *number)
{
    char *contents = (char *)malloc(number->logical_length + sizeof(char));
    strcpy(contents, number->contents);
    number_type_free(number);
    return contents;
}

// value must be a null-terminated string.
static Token *token_new(Token_Type type, const char *value)
{
    Token *token = (Token *)malloc(sizeof(Token));
    token->type = type;
    token->value = (char *)malloc(strlen(value) + 1);
    strcpy(token->value, value);
    return token;
}

static int token_free(Token *token)
{
    free(token->value);
    free(token);
    return 0;
}

static Tokens *tokens_new()
{
    Tokens *tokens = (Tokens *)malloc(sizeof(Tokens));
    tokens->allocated_length = 4; // init 4 space to store token.
    tokens->logical_length = 0;
    tokens->contents = (Token **)malloc(tokens->allocated_length * sizeof(Token *));
    return tokens;
}

static int tokens_length(Tokens *tokens)
{
    return tokens->logical_length;
}

static Token *tokens_nth(Tokens *tokens, int index)
{
    return tokens->contents[index];
}

int tokens_free(Tokens *tokens)
{
    int length = tokens->logical_length;
    for (int i = 0; i < length; i++)
    {
        Token *token = tokens_nth(tokens, i);
        token_free(token);
    }
    free(tokens->contents);
    free(tokens);
    return 0;
}

static int add_token(Tokens *tokens, Token *token)
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

        // handle language and character
        // supports only: #lang racket
        if (line[i] == POUND)
        {
            cursor = i + 1;

            // character '#\a'
            if (line[cursor] == BACK_SLASH)
            {
                // check if it is notsingle char #\aa ...
                if (isalpha(line[cursor + 2]) != 0)
                {
                    fprintf(stderr, "Character must be a single char, can not be: %s\n", &line[i]);
                    exit(EXIT_FAILURE);
                }

                const char *char_value = &line[cursor + 1];
                char *tmp = (char *)malloc(sizeof(char) * 2);
                memcpy(tmp, char_value, sizeof(char));
                tmp[1] = '\0';
                Token *token = token_new(CHARACTER, tmp);
                free(tmp);
                add_token(tokens, token);
                
                i = cursor + 1;
                continue;
            }

            // #lang
            int cmp = -1;

            cmp = strncmp(&line[cursor], LANGUAGE_SIGN, strlen(LANGUAGE_SIGN));
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
            cmp = strcmp(&line[cursor], RACKET_SIGN);
            if (cmp != 0)
            {
                fprintf(stderr, "please dont use #lang %s, supports only: #lang racket\n", &line[cursor]);
                exit(EXIT_FAILURE);
            }

            Token *token = token_new(LANGUAGE, &line[cursor]); 
            add_token(tokens, token);
            return; // go to the next line.
        }

        // handle comment
        // supports only: ; single line comment
        if (line[i] == SEMICOLON)
        {
            Token *token = token_new(COMMENT, &line[i + 1]);
            add_token(tokens, token);
            return; // go to the next line.
        }

        // handle paren --- maybe some problem? ---
        if (line[i] == LEFT_PAREN)
        {
            Token *token = token_new(PUNCTUATION, "("); 
            add_token(tokens, token); 
            continue;
        }

        if (line[i] == RIGHT_PAREN)
        {
            Token *token = token_new(PUNCTUATION, ")");
            add_token(tokens, token); 
            continue;
        }

        // handle square_bracket
        if (line[i] == LEFT_SQUARE_BRACKET)
        {
            Token *token = token_new(PUNCTUATION, "[");
            add_token(tokens, token); 
            continue;
        }

        if (line[i] == RIGHT_SQUARE_BRACKET)
        {
            Token *token = token_new(PUNCTUATION, "]");
            add_token(tokens, token); 
            continue;
        }

        // handle number
        if (isdigit(line[i]))
        {
            cursor = i + 1;
            int dot_count = 0;
            Number_Type *number = number_type_new();
            number_type_append(number, line[i]);
            bool end = false;

            while (cursor < line_length)
            {

                if (isdigit(line[cursor]))
                {
                    number_type_append(number, line[cursor]);
                    cursor ++;
                }
                else if (line[cursor] == DOT)
                {
                    number_type_append(number, line[cursor]);
                    dot_count ++;
                    cursor ++;
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

            char *contents = get_number_type_contents(number);
            Token *token = token_new(NUMBER, contents);
            free(contents);
            add_token(tokens, token);

            i = cursor - 1;
            continue;
        }

        // handle string
        if (line[i] == DOUBLE_QUOTE)
        {
            int count = 0;
            cursor = i + 1;
            bool ends = false;

            while (cursor < line_length)
            {
                if (line[cursor] != DOUBLE_QUOTE)
                {
                    count ++;
                    cursor ++;
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

            char *string = (char *)malloc(count + sizeof(char));
            strncpy(string, &line[i + 1], count);
            string[count] = '\0';
            Token *token = token_new(STRING, string);
            free(string);
            add_token(tokens, token);

            i = cursor;
            continue;
        }

        // handle character

        // handle apostrophe
        if (line[i] == APOSTROPHE)
        {
            Token *token = token_new(PUNCTUATION, "\'");
            add_token(tokens, token);
            continue;
        }

        // handle dot
        if (line[i] == DOT)
        {
            // TO-DO check if a identifier contains '.', such as (define a.b 1)
            Token *token = token_new(PUNCTUATION, ".");
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

            const char *pattern = "^[a-zA-Z\\+-\\*/]+"; // ^[a-zA-Z\+-\*/]+
            regex_t reg;
            regmatch_t match[1];
    
            int result = regcomp(&reg, pattern, REG_ENHANCED | REG_EXTENDED); 
            if (result != 0)
            {
                perror("Could not compile regex");
                exit(EXIT_FAILURE);
            }
            int status = regexec(&reg, &line[i], 1, match, 0);

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

                char *temp = malloc((identifier_len + 1) * sizeof(char));
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

Tokens *tokenizer(Raw_Code *raw_code)
{
    Tokens *tokens = tokens_new();
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

// racket value/function mapping to c native value/function.
static RacketValueBox *racket_value_box_new(const char *signature, int elem_size)
{
    RacketValueBox *rvb = (RacketValueBox *)malloc(sizeof(RacketValueBox));
    rvb->elem_size = elem_size;
    rvb->signature = (char *)malloc(strlen(signature) + 1);
    strcpy(rvb->signature, signature);
    rvb->value = malloc(elem_size);
    return rvb;
}

static int racket_value_box_free(RacketValueBox *rvb)
{
    free(rvb->signature);
    free(rvb->value);
    free(rvb);
    return 0; 
}

static void set_box_value(RacketValueBox *rvb, const void *value_addr)
{
    memcpy(rvb->value, value_addr, rvb->elem_size);
}

static void *get_box_value(RacketValueBox *rvb)
{
    return rvb->value;
}

static const char *get_box_signature(RacketValueBox *rvb)
{
    return rvb->signature;
}

// parser parts
// ast_node_new(Program)
// ast_node_new(Call_Expression, name)
// ast_node_new(Binding, name)
// ast_node_new(xxx_Literal, value)
static AST_Node *ast_node_new(AST_Node_Type type, ...)
{
    AST_Node *ast_node = (AST_Node *)malloc(sizeof(AST_Node));
    ast_node->type = type;

    // flexible args
    va_list ap;
    va_start(ap, type);

    if (ast_node->type == Program)
    {
        ast_node->contents.body = VectorNew(sizeof(AST_Node *));
        va_end(ap);
        return ast_node;
    }

    if (ast_node->type == Call_Expression)
    {
        ast_node->contents.call_expression.params = VectorNew(sizeof(AST_Node *));
        const char *name = va_arg(ap, const char *);
        ast_node->contents.call_expression.name = (char *)malloc(strlen(name) + 1);
        strcpy(ast_node->contents.call_expression.name, name);
        va_end(ap);
        return ast_node;
    }

    if (ast_node->type == Binding)
    {
        const char *name = va_arg(ap, const char *);
        ast_node->contents.name = (char *)malloc(strlen(name) + 1);
        strcpy(ast_node->contents.name, name);
        va_end(ap);
        return ast_node;
    }

    // handle literal at the rest of this function.
    const char *value = va_arg(ap, const char *);
    ast_node->contents.literal.value = (char *)malloc(strlen(value) + 1);
    strcpy(ast_node->contents.literal.value, value);
    va_end(ap);
    return ast_node;
}

static int ast_node_free(AST_Node *ast_node)
{
    if (ast_node->type == Program)
    {

    }

    if (ast_node->type == Call_Expression)
    {

    }

    // literal cases.

    return 0;
}

// recursion function <walk> walk over the tokens array, and generates a ast.
static AST_Node *walk(Tokens *tokens, int *current_p)
{
    Token *token = tokens_nth(tokens, *current_p);

    if (token->type == LANGUAGE)
    {
        (*current_p)++;
        return NULL;
    } 

    if (token->type == COMMENT)
    {
        (*current_p)++;
        return NULL;
    }

    if (token->type == IDENTIFIER)
    {
        AST_Node *ast_node = ast_node_new(Binding, token->value);
        (*current_p)++;
        return ast_node;
    }

    if (token->type == PUNCTUATION)
    {
        char token_value = (token->value)[0];

        // '(' and ')' function call
        if (token_value == LEFT_PAREN)
        {
            printf("function call\n");
            // point to the function's name.
            (*current_p)++;
            token = tokens_nth(tokens, *current_p); 

            if (token->type != IDENTIFIER) 
            {
                // check identifier
                fprintf(stderr, "Function call here, the first element of form must be a identifier\n");
                exit(EXIT_FAILURE);
            }

            AST_Node *ast_node = ast_node_new(Call_Expression, token->value);

            // point to the first argument.
            (*current_p)++; 
            token = tokens_nth(tokens, *current_p);

            while ((token->type != PUNCTUATION) ||
                   (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)
            ) 
            {
                AST_Node *param = walk(tokens, current_p);
                if (param != NULL) VectorAppend(ast_node->contents.call_expression.params, &param);
                token = tokens_nth(tokens, *current_p);
            }
            
            (*current_p)++; // skip ')'

            return ast_node;
        }

        // '\'', list or pair
        if (token_value == APOSTROPHE) 
        {
            // check '(
            (*current_p)++;
            token = tokens_nth(tokens, *current_p);
            token_value = (token->value)[0];
            if (token_value != LEFT_PAREN)
            {
                fprintf(stderr, "List or pair literal must be starts with '( \n");
                exit(EXIT_FAILURE);
            }

            // move to first element of list or pair.
            (*current_p)++;
            token = tokens_nth(tokens, *current_p);

            while ((token->type != PUNCTUATION) ||
                   (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)
            )
            {


            }

        }

        // handle ...

    }

    if (token->type == NUMBER)
    {
        AST_Node *ast_node = ast_node_new(Number_Literal, token->value);
        (*current_p)++;
        return ast_node;
    }

    if (token->type == STRING)
    {
        AST_Node *ast_node = ast_node_new(String_Literal, token->value);
        (*current_p)++;
        return ast_node;
    }

    if (token->type == CHARACTER)
    {
        AST_Node *ast_node = ast_node_new(Character_Literal, token->value);
        (*current_p)++;
        return ast_node;
    }

    // when no matches any Token_Type.
    fprintf(stderr, "walk(): can not handle token -> type: %d, value: %s\n", token->type, token->value);
    exit(EXIT_FAILURE);
}

AST parser(Tokens *tokens)
{
    AST ast = ast_node_new(Program);
    int current = 0;

    while (current < tokens_length(tokens))
    // the value of current was changed in <walk> by current_p.
    {
        AST_Node *ast_node = walk(tokens, &current);
        if (ast_node != NULL) VectorAppend(ast->contents.body, &ast_node);
        else continue;
    }
    
    return ast;
}

int ast_free(AST ast)
{
    return 0;
}

// left-sub-tree-first dfs algo. 
void traverser(AST ast, Visitor visitor, void *aux_data)
{

}

// calculator parts