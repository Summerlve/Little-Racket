#include "./interpreter.h"
#include "./load_racket_file.h"
#include "./vector.h"
#include "./racket_built_in.h"
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
static Number_Type *number_type_new(void)
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
    if (number == NULL) return 1;
    // free contents
    free(number->contents);
    // free number itself
    free(number);
    return 0;
}

static int number_type_append(Number_Type *number, const char ch)
{
    // the ch must be a digit or '.'.
    if (isdigit(ch) == 0 && ch != DOT)
    {
        fprintf(stderr, "number_type_append(): the ch must be a digit number or '.'.\n");
        exit(EXIT_FAILURE);
    }

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
    if (token == NULL) return 1;
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
    if (tokens == NULL) return 1;
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
        // language: supports only: #lang racket
        if (line[i] == POUND)
        {
            cursor = i + 1;

            // handle character such as: '#\a'
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

        // handle paren
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

// parser parts

// ast_node_new(Program)
// ast_node_new(Call_Expression, name)
// ast_node_new(Local_Binding_Form, LET/LET_STAR/LETREC/DEFINE)
// ast_node_new(Local_Binding_Form, DEFINE, char *name, AST_Node *value)
// ast_node_new(Binding, name, AST_Node *value)
// ast_node_new(List or Pair)
// ast_node_new(xxx_Literal, value)
// ast_node_new(Procedure, name, required_params_count, params, body_exprs, c_native_function)
AST_Node *ast_node_new(AST_Node_Type type, ...)
{
    AST_Node *ast_node = (AST_Node *)malloc(sizeof(AST_Node));
    ast_node->type = type;
    ast_node->parent = NULL;
    ast_node->context = NULL; 
    bool matched = false;

    // flexible args
    va_list ap;
    va_start(ap, type);

    if (ast_node->type == Program)
    {
        matched = true;
        ast_node->context = VectorNew(sizeof(AST_Node *));
        ast_node->contents.program.body = VectorNew(sizeof(AST_Node *));
        ast_node->contents.program.built_in_bindings = VectorNew(sizeof(AST_Node *));
    }

    if (ast_node->type == Call_Expression)
    {
        matched = true;
        ast_node->contents.call_expression.params = VectorNew(sizeof(AST_Node *));
        const char *name = va_arg(ap, const char *);
        ast_node->contents.call_expression.name = (char *)malloc(strlen(name) + 1);
        strcpy(ast_node->contents.call_expression.name, name);
    }

    if (ast_node->type == Procedure)
    {
        matched = true;
        const char *name = va_arg(ap, const char *);
        ast_node->contents.procedure.name = (char *)malloc(strlen(name) + 1);
        strcpy(ast_node->contents.procedure.name, name);
        ast_node->contents.procedure.required_params_count = va_arg(ap, int);
        ast_node->contents.procedure.params = va_arg(ap, Vector *);
        ast_node->contents.procedure.body_exprs = va_arg(ap, Vector *);
        ast_node->contents.procedure.c_native_function = va_arg(ap, Function);
    }

    if (ast_node->type == Local_Binding_Form)
    {
        matched = true;
        Local_Binding_Form_Type local_binding_form_type = va_arg(ap, Local_Binding_Form_Type);
        ast_node->contents.local_binding_form.type = local_binding_form_type;

        if (local_binding_form_type == LET ||
            local_binding_form_type == LET_STAR ||
            local_binding_form_type == LETREC)
        {
            ast_node->contents.local_binding_form.contents.lets.bindings = VectorNew(sizeof(AST_Node *));
            ast_node->contents.local_binding_form.contents.lets.body_exprs = VectorNew(sizeof(AST_Node *));
        }

        if (local_binding_form_type == DEFINE)
        {
            const char *name = va_arg(ap, const char *);
            AST_Node *value = va_arg(ap, AST_Node *);
            ast_node->contents.local_binding_form.contents.define.binding = ast_node_new(Binding, name, value);
        }
    }

    if (ast_node->type == Binding)
    {
        matched = true;
        ast_node->contents.binding.name = NULL;
        const char *name = va_arg(ap, const char *);
        if (name != NULL)
        {
            ast_node->contents.binding.name = (char *)malloc(strlen(name) + 1);
            strcpy(ast_node->contents.binding.name, name);
        }
        AST_Node *value = va_arg(ap, AST_Node *);
        ast_node->contents.binding.value = value;
    }

    if (ast_node->type == List_Literal)
    {
        matched = true;
        ast_node->contents.literal.value = VectorNew(sizeof(AST_Node *));
        ast_node->contents.literal.c_native_value = NULL;
    }

    if (ast_node->type == Pair_Literal)
    {
        matched = true;
        ast_node->contents.literal.value = VectorNew(sizeof(AST_Node *));
        ast_node->contents.literal.c_native_value = NULL;
    }

    if (ast_node->type == Number_Literal)
    {
        matched = true;
        const char *value = va_arg(ap, const char *);
        ast_node->contents.literal.value = malloc(strlen(value) + 1);
        strcpy((char *)ast_node->contents.literal.value, value);
        // check '.' to decide use int or double.
        if (strchr(ast_node->contents.literal.value, '.') == NULL)
        {
            // convert string to int.
            int c_native_value = atoi(ast_node->contents.literal.value);
            ast_node->contents.literal.c_native_value = malloc(sizeof(int));
            memcpy(ast_node->contents.literal.c_native_value, &c_native_value, sizeof(int));
        }
        else
        {
            // convert string to double.
            double c_native_value = strtod(ast_node->contents.literal.value, (char **)NULL);
            ast_node->contents.literal.c_native_value = malloc(sizeof(double));
            memcpy(ast_node->contents.literal.c_native_value, &c_native_value, sizeof(double));
        }
    }

    if (ast_node->type == String_Literal)
    {
        matched = true;
        const char *value = va_arg(ap, const char *);
        ast_node->contents.literal.value = malloc(strlen(value) + 1);
        strcpy((char *)ast_node->contents.literal.value, value);
    }

    if (ast_node->type == Character_Literal)
    {
        matched = true;
        const char *character = va_arg(ap, const char *);
        ast_node->contents.literal.value = malloc(sizeof(char));
        memcpy((char *)ast_node->contents.literal.value, character, sizeof(char));
    }

    va_end(ap);

    if (!matched)
    {
        // when no matches any AST_Node_Type.
        fprintf(stderr, "ast_node_new(): can not handle AST_Node_Type: %d\n", ast_node->type);
        exit(EXIT_FAILURE);
    }
  
    return ast_node;
}

int ast_node_free(AST_Node *ast_node)
{
    if (ast_node == NULL) return 1;

    bool matched = false;

    // free context itself only.
    Vector *context = ast_node->context;
    if (context != NULL) VectorFree(ast_node->context, NULL, NULL);
    
    if (ast_node->type == Program)
    {
        matched = true;
        Vector *body = ast_node->contents.program.body;
        for (int i = 0; i < VectorLength(body); i++)
        {
            AST_Node *sub_node = *(AST_Node **)VectorNth(body, i);
            ast_node_free(sub_node);
        }
        VectorFree(body, NULL, NULL);
        Vector *built_in_bindings = ast_node->contents.program.built_in_bindings;
        for (int i = 0; i < VectorLength(built_in_bindings); i++)
        {
            AST_Node *binding = *(AST_Node **)VectorNth(built_in_bindings, i);
            ast_node_free(binding);
        }
        VectorFree(built_in_bindings, NULL, NULL);
        free(ast_node);
    }

    if (ast_node->type == Call_Expression)
    {
        matched = true;
        Vector *params = ast_node->contents.call_expression.params;
        for (int i = 0; i < VectorLength(params); i++)
        {
            AST_Node *sub_node = *(AST_Node **)VectorNth(params, i);
            ast_node_free(sub_node);
        }
        VectorFree(params, NULL, NULL);
        free(ast_node->contents.call_expression.name);
        free(ast_node);
    }

    if (ast_node->type == Procedure)
    {
        matched = true;
        char *name = ast_node->contents.procedure.name;
        if (name != NULL) free(ast_node->contents.procedure.name);
        Vector *params = ast_node->contents.procedure.params;
        if (params != NULL)
        {
            for (int i = 0; i < VectorLength(params); i++)
            {
                AST_Node *param = *(AST_Node **)VectorNth(params, i);
                ast_node_free(param);
            }
            VectorFree(params, NULL, NULL);
        }
        Vector *body_exprs = ast_node->contents.procedure.body_exprs;
        if (body_exprs != NULL)
        {
            for (int i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                ast_node_free(body_expr);
            }
            VectorFree(body_exprs, NULL, NULL);
        }
        free(ast_node);
    }

    if (ast_node->type == Local_Binding_Form)
    {
        matched = true;
        Local_Binding_Form_Type Local_binding_form_type = ast_node->contents.local_binding_form.type;

        if (Local_binding_form_type == LET ||
            Local_binding_form_type == LET_STAR ||
            Local_binding_form_type == LETREC)
        {
            Vector *bindings = ast_node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = ast_node->contents.local_binding_form.contents.lets.body_exprs;
            for (int i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                ast_node_free(binding);
            }
            for (int i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                ast_node_free(body_expr);
            }
            VectorFree(bindings, NULL, NULL);
            VectorFree(body_exprs, NULL, NULL);
            free(ast_node);
        }
       
        if (Local_binding_form_type == DEFINE) 
        {
            ast_node_free(ast_node->contents.local_binding_form.contents.define.binding);
            free(ast_node);
        }
    }

    if (ast_node->type == Binding)
    {
        matched = true;
        const char *name = ast_node->contents.binding.name;
        if (name != NULL) free((void *)name);
        AST_Node *value = ast_node->contents.binding.value;
        if (value != NULL) ast_node_free(value);
        free(ast_node);
    }

    if (ast_node->type == List_Literal)
    {
        matched = true;
        Vector *elements = (Vector *)(ast_node->contents.literal.value);
        for (int i = 0; i < VectorLength(elements); i++)
        {
            AST_Node *element = *(AST_Node **)VectorNth(elements, i);
            ast_node_free(element);
        }
        VectorFree(elements, NULL, NULL);
        free(ast_node);
    }

    if (ast_node->type == Pair_Literal)
    {
        matched = true;
        Vector *elements = (Vector *)(ast_node->contents.literal.value);
        for (int i = 0; i < VectorLength(elements); i++)
        {
            AST_Node *element = *(AST_Node **)VectorNth(elements, i);
            ast_node_free(element);
        }
        VectorFree(elements, NULL, NULL);
        free(ast_node);
    }

    if (ast_node->type == Number_Literal)
    {
        matched = true;
        free(ast_node->contents.literal.value);
        free(ast_node->contents.literal.c_native_value);
        free(ast_node);
    }

    if (ast_node->type == String_Literal)
    {
        matched = true;
        free(ast_node->contents.literal.value);
        free(ast_node);
    }

    if (ast_node->type == Character_Literal)
    {
        matched = true;
        free(ast_node->contents.literal.value);
        free(ast_node);
    }

    if (!matched)
    {
        // when no matches any AST_Node_Type.
        fprintf(stderr, "ast_node_free(): can not handle AST_Node_Type: %d\n", ast_node->type);
        exit(EXIT_FAILURE);
    }

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
        AST_Node *ast_node = ast_node_new(Binding, token->value, NULL);
        (*current_p)++;
        return ast_node;
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

    if (token->type == PUNCTUATION)
    {
        char token_value = (token->value)[0];

        // '(' and ')' normally function call or each kind of form such as let let* if cond etc.
        if (token_value == LEFT_PAREN)
        {
            // point to the function's name.
            (*current_p)++;
            token = tokens_nth(tokens, *current_p); 

            // handle Local_Binding_Form
            // handle 'let' 'let*' 'letrec' contains '[' and ']'
            if ((strcmp(token->value, "let") == 0) ||
                (strcmp(token->value, "let*") == 0) ||
                (strcmp(token->value, "letrec") == 0))
            {
                AST_Node *ast_node;
                if (strcmp(token->value, "let") == 0) ast_node = ast_node_new(Local_Binding_Form, LET);
                if (strcmp(token->value, "let*") == 0) ast_node = ast_node_new(Local_Binding_Form, LET_STAR);
                if (strcmp(token->value, "letrec") == 0) ast_node = ast_node_new(Local_Binding_Form, LETREC);

                // point to the bindings form starts '('
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);
                if ((token->type != PUNCTUATION) ||
                    (token->type == PUNCTUATION && token->value[0] != LEFT_PAREN))
                {
                    fprintf(stderr, "walk(): let expression here, check the syntax\n");
                    exit(EXIT_FAILURE);
                }

                // move to first binding. '['
                (*current_p)++; 
                token = tokens_nth(tokens, *current_p);

                // collect bindings
                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)) 
                {
                    // check '['
                    if ((token->type != PUNCTUATION) ||
                         (token->type == PUNCTUATION && token->value[0] != LEFT_SQUARE_BRACKET))
                    {
                        fprintf(stderr, "walk(): let expression here, check the syntax\n");
                        exit(EXIT_FAILURE);
                    }

                    // move to binding's name
                    (*current_p)++; 

                    AST_Node *binding = walk(tokens, current_p); 
                    VectorAppend(ast_node->contents.local_binding_form.contents.lets.bindings, &binding);
                     
                    AST_Node *value = walk(tokens, current_p);
                    binding->contents.binding.value = value;

                    // check ']'
                    token = tokens_nth(tokens, *current_p);
                    if ((token->type != PUNCTUATION) ||
                        (token->type == PUNCTUATION && token->value[0] != RIGHT_SQUARE_BRACKET))
                    {
                        fprintf(stderr, "walk(): let expression here, check the syntax\n");
                        exit(EXIT_FAILURE);
                    }

                    // move to next '[' or ')' that completeing the binding form.
                    (*current_p)++;
                    token = tokens_nth(tokens, *current_p);
                }

                /*
                    collect body_exprs
                */
                // move to next token.
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);
                
                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)) 
                {
                    AST_Node *body_expr = walk(tokens, current_p);
                    VectorAppend(ast_node->contents.local_binding_form.contents.lets.body_exprs, &body_expr);
                    token = tokens_nth(tokens, *current_p);
                }

                (*current_p)++; // skip ')' of let expression
                return ast_node;
            }

            // handle 'define' 
            if (strcmp(token->value, "define") == 0)
            {
                // move to binding's name.
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);

                // check token's type if or not identifier.
                if (token->type != IDENTIFIER)
                {
                    fprintf(stderr, "walk(): plz: (define xxx xxx), check the syntax\n");
                    exit(EXIT_FAILURE);
                }

                // move to binding's value
                (*current_p)++;
                AST_Node *value = walk(tokens, current_p);
                AST_Node *ast_node = ast_node_new(Local_Binding_Form, DEFINE, token->value, value);

                (*current_p)++; // skip ')' of let expression
                return ast_node;
            }

            // handle ... 
            
            // handle normally function call
            // check identifier, it's must be a normally function name.
            if (token->type != IDENTIFIER) 
            {
                fprintf(stderr, "walk(): Function call here, the first element of form must be a identifier\n");
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

        // '\'' and '.', list or pair
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

            // check list or pair '.', dont move current_p, use a tmp value instead.
            bool is_pair = false;
            int cursor = *current_p + 1;
            Token *tmp = tokens_nth(tokens, cursor);
            char tmp_value = tmp->value[0];
            if (tmp_value == DOT) is_pair = true;

            AST_Node *ast_node = NULL;
            if (is_pair)
            {
                ast_node = ast_node_new(Pair_Literal);

                AST_Node *car = walk(tokens, current_p);
                (*current_p)++; // skip '.'
                AST_Node *cdr = walk(tokens, current_p);
                if (car == NULL || cdr == NULL)
                {
                    fprintf(stderr, "Pair literal should have two values\n");
                    exit(EXIT_FAILURE);
                }
                VectorAppend((Vector*)(ast_node->contents.literal.value), &car);
                VectorAppend((Vector*)(ast_node->contents.literal.value), &cdr);

                (*current_p)++; // skip ')'
                return ast_node;
            }
            else
            {
                ast_node = ast_node_new(List_Literal);

                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)
                )
                {
                    AST_Node *element = walk(tokens, current_p);
                    if (element != NULL) VectorAppend((Vector *)(ast_node->contents.literal.value), &element);
                    token = tokens_nth(tokens, *current_p);
                }

                (*current_p)++; // skip ')'
                return ast_node;
            }
        }

        // handle PUNCTUATION ...

    }

    // handle ...
    
    // when no matches any Token_Type.
    fprintf(stderr, "walk(): can not handle token -> type: %d, value: %s\n", token->type, token->value);
    exit(EXIT_FAILURE);
}

AST parser(Tokens *tokens)
{
    AST ast = ast_node_new(Program);
    int current = 0;

    while (current < tokens_length(tokens))
    // the value of 'current' was changed in walk() by current_p.
    {
        AST_Node *ast_node = walk(tokens, &current);
        if (ast_node != NULL) VectorAppend(ast->contents.program.body, &ast_node);
        else continue;
    }
    
    return ast;
}

int ast_free(AST ast)
{
    return ast_node_free(ast);
}

Visitor visitor_new()
{
    return VectorNew(sizeof(AST_Node_Handler *));
}

AST_Node_Handler *ast_node_handler_new(AST_Node_Type type, VisitorFunction enter, VisitorFunction exit)
{
    AST_Node_Handler *handler = (AST_Node_Handler *)malloc(sizeof(AST_Node_Handler));
    handler->type = type;
    handler->enter = enter;
    handler->exit = exit;
    return handler;
}

int ast_node_handler_free(AST_Node_Handler *handler)
{
    free(handler);
    return 0;
}

static void visitor_free_helper(void *value_addr, void *aux_data)
{
    AST_Node_Handler *handler = *(AST_Node_Handler **)value_addr;
    ast_node_handler_free(handler);
}

int visitor_free(Visitor visitor)
{
    if (visitor == NULL) return 1;
    VectorFree(visitor, visitor_free_helper, NULL);
    return 0;
}

int ast_node_handler_append(Visitor visitor, AST_Node_Type type, VisitorFunction enter, VisitorFunction exit)
{
    AST_Node_Handler *handler = ast_node_handler_new(type, enter, exit);
    VectorAppend(visitor, &handler);
    return 0;
}

AST_Node_Handler *find_ast_node_handler(Visitor visitor, AST_Node_Type type)
{
    int l = VectorLength(visitor);
    for (int i = 0; i < l; i++)
    {
        AST_Node_Handler *handler = *(AST_Node_Handler **)VectorNth(visitor, i);
        if (handler->type == type) return handler;
    }
    return NULL;
}

// traverser help function.
static void traverser_node(AST_Node *node, AST_Node *parent, Visitor visitor, void *aux_data)
{
    AST_Node_Handler *handler = find_ast_node_handler(visitor, node->type);

    if (handler == NULL)
    {
        fprintf(stderr, "traverser_node(): can not find handler for AST_Node_Type: %d\n", node->type);
        exit(EXIT_FAILURE);
    }

    // enter
    if (handler->enter != NULL) handler->enter(node, parent, aux_data);

    // left sub-tree first dfs algorithm.
    if (node->type == Program)  
    {
        Vector *body = node->contents.program.body;
        for (int i = 0; i < VectorLength(body); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(body, i);
            traverser_node(ast_node, node, visitor, aux_data);
        }
    }

    if (node->type == Call_Expression)
    {
        Vector *params = node->contents.call_expression.params;
        for (int i = 0; i < VectorLength(params); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(params, i);
            traverser_node(ast_node, node, visitor, aux_data);
        }
    }

    if (node->type == Local_Binding_Form)
    {
        Local_Binding_Form_Type local_binding_form_type = node->contents.local_binding_form.type;

        if (local_binding_form_type == LET ||
            local_binding_form_type == LET_STAR ||
            local_binding_form_type == LETREC)
        {
            Vector *bindings = node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = node->contents.local_binding_form.contents.lets.body_exprs;
            for (int i = 0; i < VectorLength(bindings); i++)            
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                traverser_node(binding, node, visitor, aux_data);
            }
            for (int i = 0; i < VectorLength(body_exprs); i++)            
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                traverser_node(body_expr, node, visitor, aux_data);
            }
        }

        if (local_binding_form_type == DEFINE)
        {
            AST_Node *binding = node->contents.local_binding_form.contents.define.binding;
            traverser_node(binding, node, visitor, aux_data);
        }
    }

    if (node->type == List_Literal)
    {
        Vector *value = (Vector *)(node->contents.literal.value);
        for (int i = 0; i < VectorLength(value); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(value, i);
            traverser_node(ast_node, node, visitor, aux_data);
        }
    }

    if (node->type == Pair_Literal)
    {
        Vector *value = (Vector *)(node->contents.literal.value);
        for (int i = 0; i < VectorLength(value); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(value, i);
            traverser_node(ast_node, node, visitor, aux_data);
        }
    }

    if (node->type == Binding)
    {
        AST_Node *value = node->contents.binding.value;
        if (value != NULL)
        {
            traverser_node(value, node, visitor, aux_data);
        }
    }

    // exit
    if(handler->exit != NULL) handler->exit(node, parent, aux_data);
}

// defult AST_Node handler for calculator parts
Visitor get_defult_visitor(void)
{
    Visitor visitor = visitor_new();
    return visitor;
}

// left-sub-tree-first dfs algo. 
void traverser(AST ast, Visitor visitor, void *aux_data)
{
    if (visitor == NULL) visitor = get_defult_visitor();
    traverser_node(ast, NULL, visitor, aux_data);
}

// finally, the contextable will not be null, the top Program node always have context.
static AST_Node *find_contextable_node(AST_Node *current_node)
{
    AST_Node *contextable = NULL;
    if (current_node->context == NULL)
    {
        contextable = current_node->parent;
        while (contextable->context == NULL) 
        {
            contextable = contextable->parent;
        }
    }
    else
    {
        contextable = current_node;
    }
    return contextable;
}

static void generate_context(AST_Node *node, AST_Node *parent, void *aux_data)
{
    node->parent = parent;

    if (node->type == Program)
    {
        // only one Program Node in AST, so the following code will run only once.
        Vector *built_in_bindings = generate_built_in_bindings(); // add built-in binding to Program.
        for (int i = 0; i < VectorLength(built_in_bindings); i++)
        {
            AST_Node *binding = *(AST_Node **)VectorNth(built_in_bindings, i);
            VectorAppend(node->contents.program.built_in_bindings, &binding);
        }
        free_built_in_bindings(built_in_bindings, NULL); // free 'built_in_bindings' itself only.
        Vector *body = node->contents.program.body;
        for (int i = 0; i < VectorLength(body); i++)
        {
            AST_Node *sub_node = *(AST_Node **)(VectorNth(body, i));
            generate_context(sub_node, node, aux_data);
        }
    }

    if (node->type == Local_Binding_Form)
    {
        if (node->contents.local_binding_form.type == DEFINE)
        {
            AST_Node *binding = node->contents.local_binding_form.contents.define.binding;
            AST_Node *contextable = find_contextable_node(node);
            VectorAppend(contextable->context, &binding);
            generate_context(binding, node, aux_data);
        }

        if (node->contents.local_binding_form.type == LET)
        {
            Vector *bindings = node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = node->contents.local_binding_form.contents.lets.body_exprs;
            // append bindings to every expr in body_exprs, even a Number_Literal ast node.
            for (int i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i); 
                if (body_expr->context == NULL)
                {
                    body_expr->context = VectorNew(sizeof(AST_Node *));
                }
                for(int j = 0; j < VectorLength(bindings); j++)
                {
                    AST_Node *binding = *(AST_Node **)VectorNth(bindings, j);
                    VectorAppend(body_expr->context, &binding);
                }
            }
            for (int i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                generate_context(binding, node, aux_data);
            }
            for (int i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                generate_context(body_expr, node, aux_data);
            }
        }

        if (node->contents.local_binding_form.type == LET_STAR)
        {
            Vector *bindings = node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = node->contents.local_binding_form.contents.lets.body_exprs;

            int i = 0;
            while (i + 1 < VectorLength(body_exprs))
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i + 1);
                if (body_expr->context == NULL)
                {
                    body_expr->context = VectorNew(sizeof(AST_Node *));
                }
                int j = i; // j = i;
                while (j >= 0)
                {
                    AST_Node *binding = *(AST_Node **)VectorNth(bindings, j);
                    VectorAppend(body_expr->context, &binding);
                    j--;
                }
                i++;
            }
            
            for (int i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                generate_context(binding, node, aux_data);
            }
            for (int i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                generate_context(body_expr, node, aux_data);
            }
        }

        if (node->contents.local_binding_form.type == LETREC)
        {
            Vector *bindings = node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = node->contents.local_binding_form.contents.lets.body_exprs;

            int i = 0;
            while (i + 1 < VectorLength(body_exprs))
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i + 1);
                if (body_expr->context == NULL)
                {
                    body_expr->context = VectorNew(sizeof(AST_Node *));
                }
                int j = i + 1; // j = i + 1;
                while (j >= 0)
                {
                    AST_Node *binding = *(AST_Node **)VectorNth(bindings, j);
                    VectorAppend(body_expr->context, &binding);
                    j--;
                }
                i++;
            }
            
            for (int i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                generate_context(binding, node, aux_data);
            }
            for (int i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                generate_context(body_expr, node, aux_data);
            }
        }
    }

    if (node->type == Call_Expression)
    {
        Vector *params = node->contents.call_expression.params;
        for (int i = 0; i < VectorLength(params); i++)
        {
            AST_Node *param = *(AST_Node **)VectorNth(params, i);
            generate_context(param, node, aux_data);
        }
    }

    if (node->type == Binding)
    {
        AST_Node *value = node->contents.binding.value;
        generate_context(value, node, aux_data);
    }
    
    if (node->type == Procedure)
    {
        return;
    }
    
    if (node->type == Number_Literal ||
        node->type == String_Literal ||
        node->type == Character_Literal)
    {
        return;
    }

    if (node->type == List_Literal)
    {
        // symbol doesnt impled right now, so '((+ 1 2)) will not be supported.
        // only literal will appear in list literal.the 
        Vector *elems = (Vector *)node->contents.literal.value;
        for (int i = 0; i < VectorLength(elems); i++)
        {
            AST_Node *elem = *(AST_Node **)VectorNth(elems, i);
            generate_context(elem, node, aux_data);
        }
    }

    if (node->type == Pair_Literal)
    {
        // same situation with List_Literal.
        Vector *elems = (Vector *)node->contents.literal.value;
        for (int i = 0; i < VectorLength(elems); i++)
        {
            AST_Node *elem = *(AST_Node **)VectorNth(elems, i);
            generate_context(elem, node, aux_data);
        }
    }
}

static Result eval(AST_Node *ast_node)
{
    return NULL;
}

// calculator parts
Result calculator(AST ast)
{
    generate_context(ast, NULL, NULL); // generate context 
    Result result = eval(ast); // eval
    return result;
}

int result_free(Result result)
{
    return ast_node_free(result);
}