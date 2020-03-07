#include "../include/interpreter.h"
#include "../include/load_racket_file.h"
#include "../include/vector.h"
#include "../include/racket_built_in.h"
#include "../include/custom_handler.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <regex.h>
#include <stdarg.h>
#include <stddef.h>

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
#define BAR 0x2d // '-'

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
    // the ch must be a digit or '.' or '-' when a negative number.
    if (isdigit(ch) == 0 && ch != DOT && ch != BAR)
    {
        fprintf(stderr, "number_type_append(): the ch must be a digit number or '.' or '-'.\n");
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

static size_t tokens_length(Tokens *tokens)
{
    return tokens->logical_length;
}

static Token *tokens_nth(Tokens *tokens, size_t index)
{
    return tokens->contents[index];
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
    size_t line_length = strlen(line);    
    size_t cursor = 0;

    for(size_t i = 0; i < line_length; i++)
    {
        // handle whitespace.
        if (line[i] == WHITE_SPACE)
        {
            continue;
        }

        // handle language, character, boolean.
        // language: supports only: #lang racket
        if (line[i] == POUND)
        {
            cursor = i + 1;

            // handle character such as: '#\a'
            if (line[cursor] == BACK_SLASH)
            {
                // check if it is not single char #\aa ...
                // !bug here #\11 will not be resolved correctly. 
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

            // #t #f
            if (line[cursor] == 't' || line[cursor] == 'f')
            {
                const char *char_value = &line[cursor];
                char *tmp = (char *)malloc(sizeof(char) * 2);
                memcpy(tmp, char_value, sizeof(char));
                tmp[1] = '\0';
                Token *token = token_new(BOOLEAN, tmp);
                free(tmp);
                add_token(tokens, token);
                
                i = cursor;
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

        // handle number or negative nubmer.
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

            const char *pattern = "^[a-zA-Z\\+-\\*/!]+"; // ^[a-zA-Z\+-\*/!]+
            regex_t reg;
            regmatch_t match[1];
    
            int result = regcomp(&reg, pattern, REG_EXTENDED); 
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
    size_t length = tokens->logical_length;

    for (size_t i = 0; i < length; i++)
    {
        const Token *token = tokens_nth(tokens, i);
        map(token, aux_data);
    }
}

// ast_node_new(tag, Program, body/NULL, built_in_bindings/NULL)
// ast_node_new(tag, Call_Expression, name/NULL, anonymous_procedure/NULL, params/NULL)
// ast_node_new(tag, Local_Binding_Form, Local_Binding_Form_Type, ...)
//   ast_node_new(tag, Local_Binding_Form, DEFINE, char *name, AST_Node *value)
//   ast_node_new(tag, Local_Binding_Form, LET/LET_STAR/LETREC, bindings/NULL, body_exprs/NULL)
// ast_node_new(tag, Binding, name, AST_Node *value/NULL)
// ast_node_new(tag, List or Pair, Vector *value/NULL)
// ast_node_new(tag, xxx_Literal, value)
// ast_node_new(tag, Procedure, name/NULL, required_params_count, params, body_exprs, c_native_function/NULL)
// ast_node_new(tag, Conditional_Form, Conditional_Form_Type, ...)
//   ast_node_new(tag, Conditional_Form, IF, test_expr, then_expr, else_expr)
//   ast_node_new(tag, Conditional_Form, AND, Vector *exprs/NULL)
//   ast_node_new(tag, Conditional_Form, NOT, AST_Node *expr/NULL)
//   ast_node_new(tag, Conditional_Form, OR, Vector *exprs/NULL)
//   ast_node_new(tag, Conditional_Form, COND, Vector *cond_clauses/NULL)
//   ast_node_new(tag, Cond_Clause, Cond_Clause_Type type, AST_Node *test_expr/NULL, Vector *then_bodies/NULL, AST_Node *proc_expr/NULL)
//      ast_node_new(tag, Cond_Clause, TEST_EXPR_WITH_THENBODY, test_expr, then_bodies, NULL)
//      ast_node_new(tag, Cond_Clause, ELSE_STATEMENT, NULL, then_bodies, NULL)
// ast_node_new(tag, Lambda_Form, params, body_exprs)
// ast_node_new(tag, Set_Form, id/NULL, expr/NULL)
// ast_node_new(tag, NULL_Expression)
// ast_node_new(tag, EMPTY_Expression)
AST_Node *ast_node_new(AST_Node_Tag tag, AST_Node_Type type, ...)
{
    AST_Node *ast_node = (AST_Node *)malloc(sizeof(AST_Node));
    ast_node->tag = tag;
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
        Vector *body = va_arg(ap, Vector *);
        if (body == NULL) body = VectorNew(sizeof(AST_Node *));
        Vector *built_in_bindings = va_arg(ap, Vector *);
        if (built_in_bindings == NULL) built_in_bindings = VectorNew(sizeof(AST_Node *));
        ast_node->contents.program.body = body;
        ast_node->contents.program.built_in_bindings = built_in_bindings;
    }

    if (ast_node->type == Call_Expression)
    {
        matched = true;
        char *name = va_arg(ap, char *);
        if (name == NULL)
        {
            ast_node->contents.call_expression.name = name;
        }
        else if (name != NULL)
        {
            ast_node->contents.call_expression.name = (char *)malloc(strlen(name) + 1);
            strcpy(ast_node->contents.call_expression.name, name);
        }
        ast_node->contents.call_expression.anonymous_procedure = va_arg(ap, AST_Node *);
        Vector *params = va_arg(ap, Vector *);
        if (params == NULL) params = VectorNew(sizeof(AST_Node *));
        ast_node->contents.call_expression.params = params;
    }

    if (ast_node->type == Procedure)
    {
        matched = true;
        ast_node->contents.procedure.name = NULL;
        const char *name = va_arg(ap, const char *);
        if (name != NULL)
        {
            ast_node->contents.procedure.name = (char *)malloc(strlen(name) + 1);
            strcpy(ast_node->contents.procedure.name, name);
        }
        ast_node->contents.procedure.required_params_count = va_arg(ap, int);
        ast_node->contents.procedure.params = va_arg(ap, Vector *);
        ast_node->contents.procedure.body_exprs = va_arg(ap, Vector *);
        ast_node->contents.procedure.c_native_function = va_arg(ap, Function);
    }

    if (ast_node->type == Lambda_Form)
    {
        matched = true;
        ast_node->contents.lambda_form.params = va_arg(ap, Vector *);
        ast_node->contents.lambda_form.body_exprs = va_arg(ap, Vector *);
    }

    if (ast_node->type == Local_Binding_Form)
    {
        Local_Binding_Form_Type local_binding_form_type = va_arg(ap, Local_Binding_Form_Type);
        ast_node->contents.local_binding_form.type = local_binding_form_type;

        if (local_binding_form_type == LET ||
            local_binding_form_type == LET_STAR ||
            local_binding_form_type == LETREC)
        {
            matched = true;
            Vector *bindings = va_arg(ap, Vector *);
            if (bindings == NULL) bindings = VectorNew(sizeof(AST_Node *));
            Vector *body_exprs = va_arg(ap, Vector *);
            if (body_exprs == NULL) body_exprs = VectorNew(sizeof(AST_Node *));
            ast_node->contents.local_binding_form.contents.lets.bindings = bindings;
            ast_node->contents.local_binding_form.contents.lets.body_exprs = body_exprs;
        }

        if (local_binding_form_type == DEFINE)
        {
            matched = true;
            const char *name = va_arg(ap, const char *);
            AST_Node *value = va_arg(ap, AST_Node *);
            ast_node->contents.local_binding_form.contents.define.binding = ast_node_new(ast_node->tag, Binding, name, value);
        }
    }

    if (ast_node->type == Set_Form)
    {
        matched = true;
        ast_node->contents.set_form.id = va_arg(ap, AST_Node *);
        ast_node->contents.set_form.expr = va_arg(ap, AST_Node *);
    }

    if (ast_node->type == Conditional_Form)
    {
        Conditional_Form_Type conditional_form_type = va_arg(ap, Conditional_Form_Type);
        ast_node->contents.conditional_form.type = conditional_form_type;

        if (conditional_form_type == IF)
        {
            matched = true;
            ast_node->contents.conditional_form.contents.if_expression.test_expr = va_arg(ap, AST_Node *);
            ast_node->contents.conditional_form.contents.if_expression.then_expr = va_arg(ap, AST_Node *);
            ast_node->contents.conditional_form.contents.if_expression.else_expr = va_arg(ap, AST_Node *);
        }

        if (conditional_form_type == COND)
        {
            matched = true;
            ast_node->contents.conditional_form.contents.cond_expression.cond_clauses = va_arg(ap, Vector *);
        }

        if (conditional_form_type == AND)
        {
            matched = true;
            ast_node->contents.conditional_form.contents.and_expression.exprs = va_arg(ap, Vector *);
        }

        if (conditional_form_type == NOT)
        {
            matched = true;
            ast_node->contents.conditional_form.contents.not_expression.expr = va_arg(ap, AST_Node *);
        }

        if (conditional_form_type == OR)
        {
            matched = true;
            ast_node->contents.conditional_form.contents.or_expression.exprs = va_arg(ap, Vector *);
        }
    }

    if (ast_node->type == Cond_Clause)
    {
        matched = true;
        ast_node->contents.cond_clause.type = va_arg(ap, Cond_Clause_Type);
        ast_node->contents.cond_clause.test_expr = va_arg(ap, AST_Node *);
        ast_node->contents.cond_clause.then_bodies = va_arg(ap, Vector *);
        ast_node->contents.cond_clause.proc_expr = va_arg(ap, AST_Node *);
    }

    if (ast_node->type == Binding)
    {
        matched = true;
        const char *name = va_arg(ap, const char *);
        ast_node->contents.binding.name = (char *)malloc(strlen(name) + 1);
        strcpy(ast_node->contents.binding.name, name);
        ast_node->contents.binding.value = va_arg(ap, AST_Node *);
    }

    if (ast_node->type == List_Literal)
    {
        matched = true;
        Vector *value = va_arg(ap, Vector *);
        if (value == NULL) value = VectorNew(sizeof(AST_Node *));
        ast_node->contents.literal.value = value; 
        ast_node->contents.literal.c_native_value = NULL;
    }

    if (ast_node->type == Pair_Literal)
    {
        matched = true;
        Vector *value = va_arg(ap, Vector *);
        if (value == NULL) value = VectorNew(sizeof(AST_Node *));
        ast_node->contents.literal.value = value; 
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
            // convert string to long long int.
            char *endptr = NULL;
            long long int c_native_value = strtoll(ast_node->contents.literal.value, &endptr, 10);
            ast_node->contents.literal.c_native_value = malloc(sizeof(long long int));
            memcpy(ast_node->contents.literal.c_native_value, &c_native_value, sizeof(long long int));
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
        strcpy((char *)(ast_node->contents.literal.value), value);
        ast_node->contents.literal.c_native_value = NULL;
    }

    if (ast_node->type == Character_Literal)
    {
        matched = true;
        const char *character = va_arg(ap, const char *);
        ast_node->contents.literal.value = malloc(sizeof(char));
        memcpy(ast_node->contents.literal.value, character, sizeof(char));
        ast_node->contents.literal.c_native_value = NULL;
    }

    if (ast_node->type == Boolean_Literal)
    {
        matched = true;
        Boolean_Type *value = va_arg(ap, Boolean_Type *);
        ast_node->contents.literal.value = malloc(sizeof(Boolean_Type));
        memcpy(ast_node->contents.literal.value, value, sizeof(Boolean_Type));
        ast_node->contents.literal.c_native_value = NULL;
    }

    if (ast_node->type == NULL_Expression)
    {
        matched = true;
        ast_node->contents.null_expression.value = NULL;
    }

    if (ast_node->type == EMPTY_Expression)
    {
        matched = true;
        ast_node->contents.empty_expression.value = NULL;
    }

    va_end(ap);

    if (matched == false)
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
        for (size_t i = 0; i < VectorLength(body); i++)
        {
            AST_Node *sub_node = *(AST_Node **)VectorNth(body, i);
            ast_node_free(sub_node);
        }
        VectorFree(body, NULL, NULL);
        Vector *built_in_bindings = ast_node->contents.program.built_in_bindings;
        for (size_t i = 0; i < VectorLength(built_in_bindings); i++)
        {
            AST_Node *binding = *(AST_Node **)VectorNth(built_in_bindings, i);
            ast_node_free(binding);
        }
        VectorFree(built_in_bindings, NULL, NULL);
    }

    if (ast_node->type == Call_Expression)
    {
        matched = true;
        Vector *params = ast_node->contents.call_expression.params;
        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *sub_node = *(AST_Node **)VectorNth(params, i);
            ast_node_free(sub_node);
        }
        VectorFree(params, NULL, NULL);
        free(ast_node->contents.call_expression.name);
    }

    if (ast_node->type == Procedure)
    {
        matched = true;
        char *name = ast_node->contents.procedure.name;
        if (name != NULL) free(ast_node->contents.procedure.name);
        Vector *params = ast_node->contents.procedure.params;
        if (params != NULL)
        {
            for (size_t i = 0; i < VectorLength(params); i++)
            {
                AST_Node *param = *(AST_Node **)VectorNth(params, i);
                ast_node_free(param);
            }
            VectorFree(params, NULL, NULL);
        }
        Vector *body_exprs = ast_node->contents.procedure.body_exprs;
        if (body_exprs != NULL)
        {
            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                ast_node_free(body_expr);
            }
            VectorFree(body_exprs, NULL, NULL);
        }
    }

    if (ast_node->type == Lambda_Form)
    {
        matched = true;

        Vector *params = ast_node->contents.lambda_form.params;
        Vector *body_exprs = ast_node->contents.lambda_form.body_exprs;

        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *param = *(AST_Node **)VectorNth(params, i);
            ast_node_free(param);
        }
        VectorFree(params, NULL, NULL);

        for (size_t i = 0; i < VectorLength(body_exprs); i++)
        {
            AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
            ast_node_free(body_expr);
        }
        VectorFree(body_exprs, NULL, NULL);
    }

    if (ast_node->type == Local_Binding_Form)
    {
        Local_Binding_Form_Type Local_binding_form_type = ast_node->contents.local_binding_form.type;

        if (Local_binding_form_type == LET ||
            Local_binding_form_type == LET_STAR ||
            Local_binding_form_type == LETREC)
        {
            matched = true;
            Vector *bindings = ast_node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = ast_node->contents.local_binding_form.contents.lets.body_exprs;
            for (size_t i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                ast_node_free(binding);
            }
            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                ast_node_free(body_expr);
            }
            VectorFree(bindings, NULL, NULL);
            VectorFree(body_exprs, NULL, NULL);
        }
       
        if (Local_binding_form_type == DEFINE) 
        {
            matched = true;
            ast_node_free(ast_node->contents.local_binding_form.contents.define.binding);
        }
    }

    if (ast_node->type == Set_Form)
    {
        matched = true;
        AST_Node *id = ast_node->contents.set_form.id;
        AST_Node *expr = ast_node->contents.set_form.expr;
        ast_node_free(id);
        ast_node_free(expr);
    }

    if (ast_node->type == Conditional_Form)
    {
        Conditional_Form_Type conditional_form_type = ast_node->contents.conditional_form.type;

        if (conditional_form_type == IF)
        {
            matched = true;
            AST_Node *test_expr = ast_node->contents.conditional_form.contents.if_expression.test_expr;
            AST_Node *then_expr = ast_node->contents.conditional_form.contents.if_expression.then_expr;
            AST_Node *else_expr = ast_node->contents.conditional_form.contents.if_expression.else_expr;
            ast_node_free(test_expr);
            ast_node_free(then_expr);
            ast_node_free(else_expr);
        }

        if (conditional_form_type == COND)
        {
            matched = true;
            Vector *cond_clauses = ast_node->contents.conditional_form.contents.cond_expression.cond_clauses;

            for (size_t i = 0; i < VectorLength(cond_clauses); i++)
            {
                // free each cond_clause
                AST_Node *cond_clause = *(AST_Node **)VectorNth(cond_clauses, i);
                ast_node_free(cond_clause);
            }

            VectorFree(cond_clauses, NULL, NULL);
        }

        if (conditional_form_type == AND)
        {
            matched = true;
            Vector *exprs = ast_node->contents.conditional_form.contents.and_expression.exprs;

            for (size_t i = 0; i < VectorLength(exprs); i++)
            {
                AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                ast_node_free(expr);
            }

            VectorFree(exprs, NULL, NULL);
        }

        if (conditional_form_type == NOT)
        {
            matched = true;
            AST_Node *expr = ast_node->contents.conditional_form.contents.not_expression.expr;
            ast_node_free(expr);
        }

        if (conditional_form_type == OR)
        {
            matched = true;
            Vector *exprs = ast_node->contents.conditional_form.contents.or_expression.exprs;

            for (size_t i = 0; i < VectorLength(exprs); i++)
            {
                AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                ast_node_free(expr);
            }

            VectorFree(exprs, NULL, NULL);
        }

        // ...
    }

    if (ast_node->type == Cond_Clause)
    {
        Cond_Clause_Type cond_clause_type = ast_node->contents.cond_clause.type;

        if (cond_clause_type == TEST_EXPR_WITH_THENBODY)
        {
            matched = true;
            // [test-expr then-body ...+]
            // needs to free test_expr and then_bodies
            AST_Node *test_expr = ast_node->contents.cond_clause.test_expr;
            ast_node_free(test_expr);

            Vector *then_bodies = ast_node->contents.cond_clause.then_bodies;
            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                ast_node_free(then_body);
            }
            VectorFree(then_bodies, NULL, NULL);
        }
        else if (cond_clause_type == ELSE_STATEMENT)
        {
            matched = true;
            // [else then-body ...+]
            // needs to free then_bodies
            Vector *then_bodies = ast_node->contents.cond_clause.then_bodies;
            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                ast_node_free(then_body);
            }
            VectorFree(then_bodies, NULL, NULL);
        }
        else if (cond_clause_type == TEST_EXPR_WITH_PROC)
        {
        }
        else if (cond_clause_type == SINGLE_TEST_EXPR)
        {
        }
        else
        {
            // something wrong here
            fprintf(stderr, "ast_node_free(): can not handle cond_clause_type: %d\n", cond_clause_type);
            exit(EXIT_FAILURE);
        }
    }

    if (ast_node->type == Binding)
    {
        matched = true;
        const char *name = ast_node->contents.binding.name;
        if (name != NULL) free((void *)name);
        AST_Node *value = ast_node->contents.binding.value;
        if (value != NULL) ast_node_free(value);
    }

    if (ast_node->type == List_Literal)
    {
        matched = true;
        Vector *elements = (Vector *)(ast_node->contents.literal.value);
        for (size_t i = 0; i < VectorLength(elements); i++)
        {
            AST_Node *element = *(AST_Node **)VectorNth(elements, i);
            ast_node_free(element);
        }
        VectorFree(elements, NULL, NULL);
    }

    if (ast_node->type == Pair_Literal)
    {
        matched = true;
        Vector *elements = (Vector *)(ast_node->contents.literal.value);
        for (size_t i = 0; i < VectorLength(elements); i++)
        {
            AST_Node *element = *(AST_Node **)VectorNth(elements, i);
            ast_node_free(element);
        }
        VectorFree(elements, NULL, NULL);
    }

    if (ast_node->type == Number_Literal)
    {
        matched = true;
        free(ast_node->contents.literal.value);
        free(ast_node->contents.literal.c_native_value);
    }

    if (ast_node->type == String_Literal)
    {
        matched = true;
        free(ast_node->contents.literal.value);
    }

    if (ast_node->type == Character_Literal)
    {
        matched = true;
        free(ast_node->contents.literal.value);
    }

    if (ast_node->type == Boolean_Literal)
    {
        matched = true;
        free(ast_node->contents.literal.value);
    }

    if (ast_node->type == NULL_Expression)
    {
        matched = true;
        if (ast_node->contents.null_expression.value != NULL)
            ast_node_free(ast_node->contents.null_expression.value);
    }

    if (ast_node->type == EMPTY_Expression)
    {
        matched = true;
        if (ast_node->contents.empty_expression.value != NULL)
            ast_node_free(ast_node->contents.empty_expression.value);
    }

    if (matched == false)
    {
        // when no matches any AST_Node_Type.
        fprintf(stderr, "ast_node_free(): can not handle AST_Node_Type: %d\n", ast_node->type);
        exit(EXIT_FAILURE);
    }

    // free ast_node itself.
    free(ast_node);

    return 0;
}

void ast_node_set_tag(AST_Node *ast_node, AST_Node_Tag tag)
{
    if (ast_node == NULL)
    {
        fprintf(stderr, "ast_node_set_tag(): ast_node is NULL\n");
        exit(EXIT_FAILURE);
    }

    ast_node->tag = tag;
}

AST_Node_Tag ast_node_get_tag(AST_Node *ast_node)
{
    if (ast_node == NULL)
    {
        fprintf(stderr, "ast_node_get_tag(): ast_node is NULL\n");
        exit(EXIT_FAILURE);
    }

    return ast_node->tag;
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
        // handle null
        if (strcmp(token->value, "null") == 0)
        {
            AST_Node *null_expr = ast_node_new(IN_AST, NULL_Expression);
            (*current_p)++; // skip null itself
            return null_expr;
        }

        // handle empty
        if (strcmp(token->value, "empty") == 0)
        {
            AST_Node *empty_expr = ast_node_new(IN_AST, EMPTY_Expression);
            (*current_p)++; // skip empty itself
            return empty_expr;
        }

        // handle normally identifier 
        AST_Node *ast_node = ast_node_new(IN_AST, Binding, token->value, NULL);
        (*current_p)++;
        return ast_node;
    }

    if (token->type == NUMBER)
    {
        AST_Node *ast_node = ast_node_new(IN_AST, Number_Literal, token->value);
        (*current_p)++;
        return ast_node;
    }

    if (token->type == STRING)
    {
        AST_Node *ast_node = ast_node_new(IN_AST, String_Literal, token->value);
        (*current_p)++;
        return ast_node;
    }

    if (token->type == CHARACTER)
    {
        AST_Node *ast_node = ast_node_new(IN_AST, Character_Literal, token->value);
        (*current_p)++;
        return ast_node;
    }

    if (token->type == BOOLEAN)
    {
        Boolean_Type *boolean_type = malloc(sizeof(Boolean_Type));
        if (strchr(token->value, 't') != NULL)
        {
            *boolean_type = R_TRUE;
        }
        if (strchr(token->value, 'f') != NULL)
        {
            *boolean_type = R_FALSE;
        }
        AST_Node *ast_node = ast_node_new(IN_AST, Boolean_Literal, boolean_type);
        (*current_p)++;
        return ast_node;
    }

    if (token->type == PUNCTUATION)
    {
        char token_value = (token->value)[0];

        // '(' and ')' normally function call or each kind of form such as let let* if cond etc
        if (token_value == LEFT_PAREN)
        {
            // point to the function's name
            (*current_p)++;
            token = tokens_nth(tokens, *current_p); 

            // handle Local_Binding_Form
            // handle 'let' 'let*' 'letrec' contains '[' and ']'
            if ((strcmp(token->value, "let") == 0) ||
                (strcmp(token->value, "let*") == 0) ||
                (strcmp(token->value, "letrec") == 0))
            {
                Token *name_token = token;
                Vector *bindings = VectorNew(sizeof(AST_Node *));
                Vector *body_exprs = VectorNew(sizeof(AST_Node *));

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
                    VectorAppend(bindings, &binding);
                     
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

                    // move to next '[' or ')' that completeing the binding form
                    (*current_p)++;
                    token = tokens_nth(tokens, *current_p);
                }

                /*
                    collect body_exprs
                */
                // move to next token
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);
                
                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)) 
                {
                    AST_Node *body_expr = walk(tokens, current_p);
                    VectorAppend(body_exprs, &body_expr);
                    token = tokens_nth(tokens, *current_p);
                }

                AST_Node *ast_node = NULL;
                if (strcmp(name_token->value, "let") == 0) ast_node = ast_node_new(IN_AST, Local_Binding_Form, LET, bindings, body_exprs);
                if (strcmp(name_token->value, "let*") == 0) ast_node = ast_node_new(IN_AST, Local_Binding_Form, LET_STAR, bindings, body_exprs);
                if (strcmp(name_token->value, "letrec") == 0) ast_node = ast_node_new(IN_AST, Local_Binding_Form, LETREC, bindings, body_exprs); 
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

                AST_Node *ast_node = ast_node_new(IN_AST, Local_Binding_Form, DEFINE, token->value, value);
                (*current_p)++; // skip ')' of let expression
                return ast_node;
            }

            // handle 'lambda'
            if (strcmp(token->value, "lambda") == 0)
            {
                Vector *params = VectorNew(sizeof(AST_Node *));
                Vector *body_exprs = VectorNew(sizeof(AST_Node *));

                // arguments list
                // move to '('    
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);
                if ((token->value)[0] != LEFT_PAREN)
                {
                    fprintf(stderr, "walk(): lambda: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                // move to first arg of argument-list 
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);

                // collect arguments
                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)) 
                {
                    AST_Node *param = walk(tokens, current_p);
                    VectorAppend(params, &param);
                    token = tokens_nth(tokens, *current_p);
                }

                // check ')' of argument-list
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): lambda: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                // move to first body_expr
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);

                // collect body expressions
                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)) 
                {
                    AST_Node *body_expr = walk(tokens, current_p);
                    VectorAppend(body_exprs, &body_expr);
                    token = tokens_nth(tokens, *current_p);
                }

                // check ')' of body_exprs
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): lambda: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                AST_Node *lambda = ast_node_new(IN_AST, Lambda_Form, params, body_exprs);
                (*current_p)++; // skip ')' of lambda expression 
                return lambda;
            }

            // handle 'if'
            if (strcmp(token->value, "if") == 0)
            {
                // move to test_expr
                (*current_p)++;

                AST_Node *test_expr = walk(tokens, current_p);
                AST_Node *then_expr = walk(tokens, current_p);
                AST_Node *else_expr = walk(tokens, current_p);

                // check ')'
                token = tokens_nth(tokens, *current_p);
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): if: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                AST_Node *if_expr = ast_node_new(IN_AST, Conditional_Form, IF, test_expr, then_expr, else_expr);
                (*current_p)++; // skip the ')' of if expression
                return if_expr;
            }
            
            // handle 'and'
            if (strcmp(token->value, "and") == 0)
            {
                // move to first expr or ')'
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);

                Vector *exprs = VectorNew(sizeof(AST_Node *));

                // check ')'
                // (and) -> #t
                if ((token->value)[0] == RIGHT_PAREN)
                {
                    AST_Node *and_expr = ast_node_new(IN_AST, Conditional_Form, AND, exprs);
                    (*current_p)++; // skip the ')' of and expression
                    return and_expr;
                }

                // when have some exprs
                // (and 1), (and #t #f), ...
                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)) 
                {
                    AST_Node *expr = walk(tokens, current_p);
                    VectorAppend(exprs, &expr);
                    token = tokens_nth(tokens, *current_p);
                }

                // check ')' of and expression
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): and: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                AST_Node *and_expr = ast_node_new(IN_AST, Conditional_Form, AND, exprs);
                (*current_p)++; // skip the ')' of and expression
                return and_expr;
            }

            // handle 'not'
            if (strcmp(token->value, "not") == 0)
            {
                // move to expr
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);

                AST_Node *expr = walk(tokens, current_p);

                // check ')' of not expression
                token = tokens_nth(tokens, *current_p);
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): not: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                AST_Node *not_expr = ast_node_new(IN_AST, Conditional_Form, NOT, expr);
                (*current_p)++; // skip the ')' of not expression
                return not_expr;
            }

            // handle 'or'
            if (strcmp(token->value, "or") == 0)
            {
                // move to first expr or ')'
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);

                Vector *exprs = VectorNew(sizeof(AST_Node *));

                // check ')'
                // (or) -> #f 
                if ((token->value)[0] == RIGHT_PAREN)
                {
                    AST_Node *or_expr = ast_node_new(IN_AST, Conditional_Form, OR, exprs);
                    (*current_p)++; // skip the ')' of and expression
                    return or_expr;
                } 

                // when have some exprs
                // (or 1) -> 1, (or #f ...) -> ...
                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)) 
                {
                    AST_Node *expr = walk(tokens, current_p);
                    VectorAppend(exprs, &expr);
                    token = tokens_nth(tokens, *current_p);
                }

                // check ')' of or expression
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): or: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                AST_Node *or_expr = ast_node_new(IN_AST, Conditional_Form, OR, exprs);
                (*current_p)++; // skip the ')' of and expression
                return or_expr;
            }

            // handle cond
            if (strcmp(token->value, "cond") == 0)
            {
                Vector *cond_clauses = VectorNew(sizeof(AST_Node *));
                int else_statement_counter = 0;

                // move to first cond clause
                (*current_p)++;
                token = tokens_nth(tokens, *current_p);

                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)) 
                {
                    AST_Node *cond_clause = NULL;

                    // check '['
                    if ((token->value)[0] != LEFT_SQUARE_BRACKET)
                    {
                        fprintf(stderr, "walk(): cond: bad syntax\n");
                        exit(EXIT_FAILURE);
                    }

                    // move to test-expr or else
                    (*current_p)++;
                    token = tokens_nth(tokens, *current_p);

                    if (strcmp(token->value, "else") == 0)
                    {
                        // else statement
                        Vector *then_bodies = VectorNew(sizeof(AST_Node *));

                        // move to first then_body
                        (*current_p)++;
                        token = tokens_nth(tokens, *current_p);

                        while ((token->type != PUNCTUATION) ||
                               (token->type == PUNCTUATION && (token->value)[0] != RIGHT_SQUARE_BRACKET)) 
                        {
                            AST_Node *then_body = walk(tokens, current_p);
                            VectorAppend(then_bodies, &then_body);
                            token = tokens_nth(tokens, *current_p);
                        }
                        
                        else_statement_counter ++;
                        cond_clause = ast_node_new(IN_AST, Cond_Clause, ELSE_STATEMENT, NULL, then_bodies, NULL);
                    }
                    else
                    {
                        // other, actually TEST_EXPR_WITH_THENBODY only right now
                        Vector *then_bodies = VectorNew(sizeof(AST_Node *));
                        AST_Node *test_expr = walk(tokens, current_p);
                        token = tokens_nth(tokens, *current_p);

                        while ((token->type != PUNCTUATION) ||
                               (token->type == PUNCTUATION && (token->value)[0] != RIGHT_SQUARE_BRACKET)) 
                        {
                            AST_Node *then_body = walk(tokens, current_p);
                            VectorAppend(then_bodies, &then_body);
                            token = tokens_nth(tokens, *current_p);
                        }

                        cond_clause = ast_node_new(IN_AST, Cond_Clause, TEST_EXPR_WITH_THENBODY, test_expr, then_bodies, NULL);
                    }

                    // check ']'
                    if ((token->value)[0] != RIGHT_SQUARE_BRACKET)
                    {
                        fprintf(stderr, "walk(): cond: bad syntax\n");
                        exit(EXIT_FAILURE);
                    }

                    VectorAppend(cond_clauses, &cond_clause);
                    (*current_p)++; // skip ']'
                    token = tokens_nth(tokens, *current_p);
                }

                // check ')' of cond expression
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): cond: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                // check else statement situation 
                if (else_statement_counter == 1)
                {
                    // check the last element of cond_clauses if it is a ELSE_STATEMENT or not
                    AST_Node *else_statment = *(AST_Node **)VectorNth(cond_clauses, VectorLength(cond_clauses) - 1);
                    if (else_statment->contents.cond_clause.test_expr != NULL)
                    {
                        fprintf(stderr, "walk(): cond: bad syntax\n");
                        exit(EXIT_FAILURE);
                    }
                }
                else if (else_statement_counter > 1)
                {
                    fprintf(stderr, "walk(): cond: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                AST_Node *cond_expr = ast_node_new(IN_AST, Conditional_Form, COND, cond_clauses);
                (*current_p)++; // skip the ')' of cond expression
                return cond_expr;
            }

            // handle set!
            if (strcmp(token->value, "set!") == 0)
            {
                // move to id.
                (*current_p)++;

                AST_Node *id = walk(tokens, current_p);
                AST_Node *expr = walk(tokens, current_p);

                // check ')'
                token = tokens_nth(tokens, *current_p);
                if ((token->value)[0] != RIGHT_PAREN)
                {
                    fprintf(stderr, "walk(): set!: bad syntax\n");
                    exit(EXIT_FAILURE);
                }

                AST_Node *set_expr = ast_node_new(IN_AST, Set_Form, id, expr);
                (*current_p)++; // skip the ')' of and expression
                return set_expr;
            }

            // handle ... 
            
            // handle normally function call
            struct {
                bool is_name;
                union {
                    Token *name_token;
                    AST_Node *lambda;
                } value;
            } name_or_lambda = {.is_name = false, .value = {NULL}};

            if (token->type == IDENTIFIER)
            {
                name_or_lambda.is_name = true;
                name_or_lambda.value.name_token = token;
            }

            if (token->type != IDENTIFIER)
            {
                // ((lambda (x) x) x) 
                name_or_lambda.is_name = false;
                name_or_lambda.value.lambda = walk(tokens, current_p); 

                // check lambda form
                if (name_or_lambda.value.lambda->type != Lambda_Form)
                {
                    fprintf(stderr, "walk(): call expression: bad syntax\n");
                    exit(EXIT_FAILURE);
                }
            }

            Vector *params = VectorNew(sizeof(AST_Node *));

            // point to the first argument when name only
            if (name_or_lambda.is_name == true)
            {
                (*current_p)++; 
                token = tokens_nth(tokens, *current_p);
            }

            while ((token->type != PUNCTUATION) ||
                   (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)
            ) 
            {
                AST_Node *param = walk(tokens, current_p);
                if (param != NULL) VectorAppend(params, &param);
                token = tokens_nth(tokens, *current_p);
            }

            AST_Node *ast_node = NULL;

            if (name_or_lambda.is_name == true) 
            {
                ast_node = ast_node_new(IN_AST, Call_Expression, name_or_lambda.value.name_token->value, NULL, params);
            }

            if (name_or_lambda.is_name == false) 
            {
                ast_node = ast_node_new(IN_AST, Call_Expression, NULL, name_or_lambda.value.lambda, params);
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

            // move to first element of list or pair
            // or ) for '() empty list
            (*current_p)++;
            token = tokens_nth(tokens, *current_p);
            if ((token->value)[0] == RIGHT_PAREN)
            {
                // '() empty list here
                Vector *value = VectorNew(sizeof(AST_Node *));
                AST_Node *empty_list = ast_node_new(IN_AST, List_Literal, value);
                (*current_p)++; // skip ')'
                return empty_list;
            }

            // check list or pair '.', dont move current_p, use a tmp value instead
            bool is_pair = false;
            int cursor = *current_p + 1;
            Token *tmp = tokens_nth(tokens, cursor);
            char tmp_value = tmp->value[0];
            if (tmp_value == DOT) is_pair = true;

            AST_Node *ast_node = NULL;
            if (is_pair)
            {
                // pair here
                AST_Node *car = walk(tokens, current_p);
                (*current_p)++; // skip '.'
                AST_Node *cdr = walk(tokens, current_p);
                if (car == NULL || cdr == NULL)
                {
                    fprintf(stderr, "Pair literal should have two values\n");
                    exit(EXIT_FAILURE);
                }

                Vector *value = VectorNew(sizeof(AST_Node *));
                VectorAppend(value, &car);
                VectorAppend(value, &cdr);

                ast_node = ast_node_new(IN_AST, Pair_Literal, value);
            }
            else
            {
                // non-empty list here
                Vector *value = VectorNew(sizeof(AST_Node *));

                while ((token->type != PUNCTUATION) ||
                       (token->type == PUNCTUATION && (token->value)[0] != RIGHT_PAREN)
                )
                {
                    AST_Node *element = walk(tokens, current_p);
                    if (element != NULL) VectorAppend(value, &element);
                    token = tokens_nth(tokens, *current_p);
                }
                
                ast_node = ast_node_new(IN_AST, List_Literal, value);
            }

            (*current_p)++; // skip ')'
            return ast_node;
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
    AST ast = ast_node_new(IN_AST, Program, NULL, NULL);
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

AST_Node *ast_node_deep_copy(AST_Node *ast_node, void *aux_data)
{
    /*
        AST_Node::parent: can not copy, you should use generate_context() or xxx->parent = yyy to set parent of an ast copy.
        AST_Node::context: can not copy, you should use generate_context() to generate context of an ast copy.
        AST_Node::tag: copy tag
    **/

    if (ast_node == NULL)
    {
        fprintf(stderr, "ast_node_deep_copy(): can not copy NULL\n");
        exit(EXIT_FAILURE); 
    }

    AST_Node *copy = NULL;
    bool matched = false;

    if (ast_node->type == Number_Literal)
    {
        matched = true;
        copy = ast_node_new(ast_node->tag, Number_Literal, ast_node->contents.literal.value);
    }

    if (ast_node->type == String_Literal)
    {
        matched = true;
        copy = ast_node_new(ast_node->tag, String_Literal, ast_node->contents.literal.value);
    }

    if (ast_node->type == Character_Literal)
    {
        matched = true;
        copy = ast_node_new(ast_node->tag, Character_Literal, ast_node->contents.literal.value);
    }

    if (ast_node->type == List_Literal)
    {
        matched = true;
        Vector *value = ast_node->contents.literal.value;
        Vector *value_copy = VectorNew(sizeof(AST_Node *));

        for (size_t i = 0; i < VectorLength(value); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(value, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            VectorAppend(value_copy, &node_copy);
        }

        copy = ast_node_new(ast_node->tag, List_Literal, value_copy);
    }

    if (ast_node->type == Pair_Literal)
    {
        matched = true;
        Vector *value = ast_node->contents.literal.value;
        Vector *value_copy = VectorNew(sizeof(AST_Node *));

        for (size_t i = 0; i < VectorLength(value); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(value, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            VectorAppend(value_copy, &node_copy);
        }

        copy = ast_node_new(ast_node->tag, Pair_Literal, value_copy);
    }

    if (ast_node->type == Boolean_Literal)
    {
        matched = true;
        copy = ast_node_new(ast_node->tag, Boolean_Literal, ast_node->contents.literal.value);
    }

    if (ast_node->type == NULL_Expression)
    {
        matched = true;
        copy = ast_node_new(ast_node->tag, NULL_Expression);
        AST_Node *value = ast_node->contents.null_expression.value;
        if (value != NULL)
            copy->contents.null_expression.value = ast_node_deep_copy(value, aux_data);
    }

    if (ast_node->type == EMPTY_Expression)
    {
        matched = true;
        copy = ast_node_new(ast_node->tag, EMPTY_Expression);
        AST_Node *value = ast_node->contents.empty_expression.value;
        if (value != NULL)
            copy->contents.empty_expression.value = ast_node_deep_copy(value, aux_data);
    }
    
    if (ast_node->type == Local_Binding_Form)
    {
        Local_Binding_Form_Type local_binding_form_type = ast_node->contents.local_binding_form.type;

        if (local_binding_form_type == DEFINE)
        {
            matched = true;
            AST_Node *binding = ast_node->contents.local_binding_form.contents.define.binding;
            char *name = binding->contents.binding.name;
            AST_Node *value = binding->contents.binding.value;

            AST_Node *value_copy = ast_node_deep_copy(value, aux_data);
            copy = ast_node_new(ast_node->tag, Local_Binding_Form, DEFINE, name, value_copy);
        }

        if (local_binding_form_type == LET ||
            local_binding_form_type == LET_STAR ||
            local_binding_form_type == LETREC)
        {
            matched = true;
            Vector *bindings = ast_node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = ast_node->contents.local_binding_form.contents.lets.body_exprs;

            Vector *bindings_copy = VectorNew(sizeof(AST_Node *));
            Vector *body_exprs_copy = VectorNew(sizeof(AST_Node *));

            for (size_t i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *node = *(AST_Node **)VectorNth(bindings, i);
                AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
                VectorAppend(bindings_copy, &node_copy);
            }

            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *node = *(AST_Node **)VectorNth(body_exprs, i);
                AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
                VectorAppend(body_exprs_copy, &node_copy);
            }

            if (local_binding_form_type == LET)
                copy = ast_node_new(ast_node->tag, Local_Binding_Form, LET, bindings_copy, body_exprs_copy);
            if (local_binding_form_type == LET_STAR)
                copy = ast_node_new(ast_node->tag, Local_Binding_Form, LET_STAR, bindings_copy, body_exprs_copy);
            if (local_binding_form_type == LETREC)
                copy = ast_node_new(ast_node->tag, Local_Binding_Form, LETREC, bindings_copy, body_exprs_copy);
        }
    }

    if (ast_node->type == Set_Form)
    {
        matched = true;
        AST_Node *id = ast_node->contents.set_form.id;
        AST_Node *expr = ast_node->contents.set_form.expr;

        AST_Node *id_copy = ast_node_deep_copy(id, aux_data);
        AST_Node *expr_copy = ast_node_deep_copy(expr, aux_data);

        copy = ast_node_new(ast_node->tag, Set_Form, id_copy, expr_copy);
    }

    if (ast_node->type == Conditional_Form)
    {
        Conditional_Form_Type conditional_form_type = ast_node->contents.conditional_form.type;

        if (conditional_form_type == IF)
        {
            matched = true;
            AST_Node *test_expr = ast_node->contents.conditional_form.contents.if_expression.test_expr;
            AST_Node *then_expr = ast_node->contents.conditional_form.contents.if_expression.then_expr;
            AST_Node *else_expr = ast_node->contents.conditional_form.contents.if_expression.else_expr; 

            AST_Node *test_expr_copy = ast_node_deep_copy(test_expr, aux_data);
            AST_Node *then_expr_copy = ast_node_deep_copy(then_expr, aux_data);
            AST_Node *else_expr_copy = ast_node_deep_copy(else_expr, aux_data);

            copy = ast_node_new(ast_node->tag, Conditional_Form, IF, test_expr_copy, then_expr_copy, else_expr_copy);
        }

        if (conditional_form_type == COND)
        {
            matched = true;
            Vector *cond_clauses = ast_node->contents.conditional_form.contents.cond_expression.cond_clauses;
            Vector *cond_clauses_copy = VectorNew(sizeof(AST_Node *));

            for (size_t i = 0; i < VectorLength(cond_clauses); i++)
            {
                AST_Node *cond_clause = *(AST_Node **)VectorNth(cond_clauses, i);
                AST_Node *cond_clause_copy = ast_node_deep_copy(cond_clause, aux_data);
                VectorAppend(cond_clauses_copy, &cond_clause_copy);
            }

            copy = ast_node_new(ast_node->tag, Conditional_Form, COND, cond_clauses_copy);
        }

        if (conditional_form_type == AND)
        {
            matched = true;
            Vector *exprs = ast_node->contents.conditional_form.contents.and_expression.exprs;
            Vector *exprs_copy = VectorNew(sizeof(AST_Node *));

            for (size_t i = 0; i < VectorLength(exprs); i++)
            {
                AST_Node *node = *(AST_Node **)VectorNth(exprs, i);
                AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
                VectorAppend(exprs_copy, &node_copy);
            }

            copy = ast_node_new(ast_node->tag, Conditional_Form, AND, exprs_copy);
        }

        if (conditional_form_type == NOT)
        {
            matched = true;
            AST_Node *expr = ast_node->contents.conditional_form.contents.not_expression.expr;
            AST_Node *expr_copy = ast_node_deep_copy(expr, aux_data);
            copy = ast_node_new(ast_node->tag, Conditional_Form, NOT, expr_copy);
        }

        if (conditional_form_type == OR)
        {
            matched = true;
            Vector *exprs = ast_node->contents.conditional_form.contents.or_expression.exprs;
            Vector *exprs_copy = VectorNew(sizeof(AST_Node *));

            for (size_t i = 0; i < VectorLength(exprs); i++)
            {
                AST_Node *node = *(AST_Node **)VectorNth(exprs, i);
                AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
                VectorAppend(exprs_copy, &node_copy);
            }

            copy = ast_node_new(ast_node->tag, Conditional_Form, OR, exprs_copy);
        }
    }

    if (ast_node->type == Cond_Clause)
    {
        matched = true;
        
        if (ast_node->contents.cond_clause.type == TEST_EXPR_WITH_THENBODY)
        {
            // [test-expr then-body ...+]
            AST_Node *test_expr = ast_node->contents.cond_clause.test_expr;
            AST_Node *test_expr_copy = ast_node_deep_copy(test_expr, aux_data);

            Vector *then_bodies = ast_node->contents.cond_clause.then_bodies;
            Vector *then_bodies_copy = VectorNew(sizeof(AST_Node *));
            
            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                AST_Node *then_body_copy = ast_node_deep_copy(then_body, aux_data);
                VectorAppend(then_bodies_copy, &then_body_copy);
            }

            copy = ast_node_new(ast_node->tag, Cond_Clause, TEST_EXPR_WITH_THENBODY, test_expr_copy, then_bodies_copy, NULL);
        }
        else if (ast_node->contents.cond_clause.type == ELSE_STATEMENT)
        {
            // [else then-body ...+]
            Vector *then_bodies = ast_node->contents.cond_clause.then_bodies;
            Vector *then_bodies_copy = VectorNew(sizeof(AST_Node *));
            
            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                AST_Node *then_body_copy = ast_node_deep_copy(then_body, aux_data);
                VectorAppend(then_bodies_copy, &then_body_copy);
            }

            copy = ast_node_new(ast_node->tag, Cond_Clause, ELSE_STATEMENT, NULL, then_bodies_copy, NULL);
        }
        else if (ast_node->contents.cond_clause.type == TEST_EXPR_WITH_PROC)
        {
        }
        else if (ast_node->contents.cond_clause.type == SINGLE_TEST_EXPR)
        {
        }
        else
        {
            // something wrong here
            fprintf(stderr, "ast_node_deep_copy(): can not copy Cond_Clause_Type: %d\n", ast_node->contents.cond_clause.type);
            exit(EXIT_FAILURE); 
        }
    }
    
    if (ast_node->type == Lambda_Form)
    {
        matched = true;
        Vector *params = ast_node->contents.lambda_form.params;
        Vector *body_exprs = ast_node->contents.lambda_form.body_exprs; 

        Vector *params_copy = VectorNew(sizeof(AST_Node *));
        Vector *body_exprs_copy = VectorNew(sizeof(AST_Node *));

        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(params, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            VectorAppend(params_copy, &node_copy);
        }

        for (size_t i = 0; i < VectorLength(body_exprs); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(body_exprs, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            VectorAppend(body_exprs_copy, &node_copy);
        }

        copy = ast_node_new(ast_node->tag, Lambda_Form, params_copy, body_exprs_copy);
    }

    if (ast_node->type == Call_Expression)
    {
        matched = true;
        const char *name = ast_node->contents.call_expression.name;
        AST_Node *anonymous_procedure = ast_node->contents.call_expression.anonymous_procedure;
        Vector *params = ast_node->contents.call_expression.params;

        Vector *params_copy = VectorNew(sizeof(AST_Node *));

        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(params, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            VectorAppend(params_copy, &node_copy);
        }

        copy = ast_node_new(ast_node->tag, Call_Expression, name, anonymous_procedure, params_copy);
    }

    if (ast_node->type == Binding)
    {
        matched = true;
        char *name = ast_node->contents.binding.name;
        AST_Node *value = ast_node->contents.binding.value;
        AST_Node *value_copy = NULL;
        if (value != NULL) value_copy = ast_node_deep_copy(value, aux_data);

        copy = ast_node_new(ast_node->tag, Binding, name, value_copy);
    }

    if (ast_node->type == Procedure)
    {
        matched = true;
        char *name = ast_node->contents.procedure.name;
        int required_params_count = ast_node->contents.procedure.required_params_count; 
        Vector *params = ast_node->contents.procedure.params; 
        Vector *body_exprs = ast_node->contents.procedure.body_exprs; 
        Function c_native_function = ast_node->contents.procedure.c_native_function;

        Vector *params_copy = VectorNew(sizeof(AST_Node *));
        Vector *body_exprs_copy = VectorNew(sizeof(AST_Node *));
        
        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(params, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            VectorAppend(params_copy, &node_copy);
        }

        for (size_t i = 0; i < VectorLength(body_exprs); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(body_exprs, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            VectorAppend(body_exprs_copy, &node_copy);
        }

        copy = ast_node_new(ast_node->tag, Procedure, name, required_params_count, params_copy, body_exprs_copy, c_native_function);
    }

    if (ast_node->type == Program)
    {
        matched = true;
        Vector *body = ast_node->contents.program.body;
        Vector *built_in_bindings = ast_node->contents.program.built_in_bindings;

        Vector *body_copy = VectorNew(sizeof(AST_Node *));
        Vector *built_in_bindings_copy = VectorNew(sizeof(AST_Node *));

        for (size_t i = 0; i < VectorLength(body); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(body, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            VectorAppend(body_copy, &node_copy);
        }

        for (size_t i = 0; i < VectorLength(built_in_bindings); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(built_in_bindings, i);
            VectorAppend(built_in_bindings_copy, &node);
        }

        copy = ast_node_new(ast_node->tag, Program, body_copy, built_in_bindings_copy);
    }

    if (matched == false)
    {
        // when no matches any AST_Node_Type.
        fprintf(stderr, "ast_node_deep_copy(): can not copy AST_Node_Type: %d\n", ast_node->type);
        exit(EXIT_FAILURE);
    }

    // copy AST_Node::tag
    copy->tag = ast_node->tag;
    
    return copy;
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

static void visitor_free_helper(void *value_addr, size_t index, Vector *vector, void *aux_data)
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

int ast_node_handler_append(Visitor visitor, AST_Node_Handler *handler)
{
    VectorAppend(visitor, &handler);
    return 0;
}

AST_Node_Handler *find_ast_node_handler(Visitor visitor, AST_Node_Type type)
{
    for (size_t i = 0; i < VectorLength(visitor); i++)
    {
        AST_Node_Handler *handler = *(AST_Node_Handler **)VectorNth(visitor, i);
        if (handler->type == type) return handler;
    }
    return NULL;
}

// traverser helper function.
static void traverser_helper(AST_Node *node, AST_Node *parent, Visitor visitor, void *aux_data)
{
    if (node == NULL)
    {
        fprintf(stdout, "works out no value.\n");
        return;
    }

    AST_Node_Handler *handler = find_ast_node_handler(visitor, node->type);

    if (handler == NULL)
    {
        fprintf(stderr, "traverser_helper(): can not find handler for AST_Node_Type: %d\n", node->type);
        exit(EXIT_FAILURE);
    }

    // enter
    if (handler->enter != NULL) handler->enter(node, parent, aux_data);

    // left sub-tree first dfs algorithm.
    if (node->type == Program)  
    {
        Vector *body = node->contents.program.body;
        for (size_t i = 0; i < VectorLength(body); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(body, i);
            traverser_helper(ast_node, node, visitor, aux_data);
        }
    }

    if (node->type == Call_Expression)
    {
        Vector *params = node->contents.call_expression.params;
        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(params, i);
            traverser_helper(ast_node, node, visitor, aux_data);
        }
    }

    if (node->type == Lambda_Form)
    {
        Vector *params = node->contents.lambda_form.params;
        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(params, i);
            traverser_helper(ast_node, node, visitor, aux_data);
        }
        Vector *body_exprs = node->contents.lambda_form.body_exprs;
        for (size_t i = 0; i < VectorLength(body_exprs); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(body_exprs, i);
            traverser_helper(ast_node, node, visitor, aux_data);
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
            for (size_t i = 0; i < VectorLength(bindings); i++)            
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                traverser_helper(binding, node, visitor, aux_data);
            }
            for (size_t i = 0; i < VectorLength(body_exprs); i++)            
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                traverser_helper(body_expr, node, visitor, aux_data);
            }
        }

        if (local_binding_form_type == DEFINE)
        {
            AST_Node *binding = node->contents.local_binding_form.contents.define.binding;
            traverser_helper(binding, node, visitor, aux_data);
        }
    }

    if (node->type == Set_Form)
    {
        AST_Node *id = node->contents.set_form.id;
        AST_Node *expr = node->contents.set_form.expr;
        traverser_helper(id, node, visitor, aux_data);
        traverser_helper(expr, node, visitor, aux_data);
    }

    if (node->type == Conditional_Form)
    {
        Conditional_Form_Type conditional_form_type = node->contents.conditional_form.type;
        
        if (conditional_form_type == IF)
        {
            AST_Node *test_expr = node->contents.conditional_form.contents.if_expression.test_expr;
            AST_Node *then_expr = node->contents.conditional_form.contents.if_expression.then_expr;
            AST_Node *else_expr = node->contents.conditional_form.contents.if_expression.else_expr;
            traverser_helper(test_expr, node, visitor, aux_data);
            traverser_helper(then_expr, node, visitor, aux_data);
            traverser_helper(else_expr, node, visitor, aux_data);
        }

        if (conditional_form_type == AND)
        {
            Vector *exprs = node->contents.conditional_form.contents.and_expression.exprs;
            for (size_t i = 0; i < VectorLength(exprs); i++)
            {
                AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                traverser_helper(expr, node, visitor, aux_data);
            }
        }

        if (conditional_form_type == NOT)
        {
            AST_Node *expr = node->contents.conditional_form.contents.not_expression.expr;
            traverser_helper(expr, node, visitor, aux_data);
        }

        if (conditional_form_type == OR)
        {
            Vector *exprs = node->contents.conditional_form.contents.or_expression.exprs;
            for (size_t i = 0; i < VectorLength(exprs); i++)
            {
                AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                traverser_helper(expr, node, visitor, aux_data);
            }
        }

        if (conditional_form_type == COND)
        {
            Vector *cond_clauses = node->contents.conditional_form.contents.cond_expression.cond_clauses;
            for (size_t i = 0; i < VectorLength(cond_clauses); i++)
            {
                AST_Node *cond_clause = *(AST_Node **)VectorNth(cond_clauses, i);
                traverser_helper(cond_clause, node, visitor, aux_data);
            }
        }
    }

    if (node->type == Cond_Clause)
    {
        if (node->contents.cond_clause.type == TEST_EXPR_WITH_THENBODY)
        {
            // [test-expr then-body ...+]
            AST_Node *test_expr = node->contents.cond_clause.test_expr;
            Vector *then_bodies = node->contents.cond_clause.then_bodies;
            
            traverser_helper(test_expr, node, visitor, aux_data);

            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                traverser_helper(then_body, node, visitor, aux_data);
            }
        }
        else if (node->contents.cond_clause.type == ELSE_STATEMENT)
        {
            // [else then-body ...+]
            Vector *then_bodies = node->contents.cond_clause.then_bodies;

            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                traverser_helper(then_body, node, visitor, aux_data);
            }
        }
        else if (node->contents.cond_clause.type == TEST_EXPR_WITH_PROC)
        {
        }
        else if (node->contents.cond_clause.type == SINGLE_TEST_EXPR)
        {
        }
        else
        {
            // something wrong here
            fprintf(stderr, "traverser_helper(): can not handle Cond_Clause_Type: %d\n", node->contents.cond_clause.type);
            exit(EXIT_FAILURE);
        }
    }

    if (node->type == List_Literal)
    {
        Vector *value = (Vector *)(node->contents.literal.value);
        for (size_t i = 0; i < VectorLength(value); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(value, i);
            traverser_helper(ast_node, node, visitor, aux_data);
        }
    }

    if (node->type == Pair_Literal)
    {
        Vector *value = (Vector *)(node->contents.literal.value);
        for (size_t i = 0; i < VectorLength(value); i++)
        {
            AST_Node *ast_node = *(AST_Node **)VectorNth(value, i);
            traverser_helper(ast_node, node, visitor, aux_data);
        }
    }

    if (node->type == Binding)
    {
        AST_Node *value = node->contents.binding.value;
        if (value != NULL)
        {
            traverser_helper(value, node, visitor, aux_data);
        }
    }

    // exit
    if(handler->exit != NULL) handler->exit(node, parent, aux_data);
}

Visitor get_defult_visitor(void)
{
    return NULL;
}

// left-sub-tree-first dfs algo. 
void traverser(AST ast, Visitor visitor, void *aux_data)
{
    if (visitor == NULL) visitor = get_defult_visitor();
    traverser_helper(ast, NULL, visitor, aux_data);
}

// find the nearly parent contextable node.
static AST_Node *find_contextable_node(AST_Node *current_node)
{
    if (current_node == NULL) return NULL;

    AST_Node *contextable = NULL;

    if (current_node->context == NULL)
    {
        contextable = current_node->parent;
        while (contextable != NULL &&
               contextable->context == NULL) 
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

void generate_context(AST_Node *node, AST_Node *parent, void *aux_data)
{
    node->parent = parent;

    if (node->type == Program)
    {
        if (node->context == NULL)
        {
            node->context = VectorNew(sizeof(AST_Node *));
        }

        // only one Program Node in AST, so the following code will run only once.
        Vector *built_in_bindings = generate_built_in_bindings(); // add built-in binding to Program.
        for (size_t i = 0; i < VectorLength(built_in_bindings); i++)
        {
            AST_Node *binding = *(AST_Node **)VectorNth(built_in_bindings, i);
            generate_context(binding, node, aux_data); // generate context for built-in bindings.
            VectorAppend(node->contents.program.built_in_bindings, &binding);
        }

        free_built_in_bindings(built_in_bindings, NULL); // free 'built_in_bindings' itself only.

        // generate context for body.
        Vector *body = node->contents.program.body;
        for (size_t i = 0; i < VectorLength(body); i++)
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
            AST_Node *contextable = find_contextable_node(node); // find the nearly parent contextable node.
            if (contextable == NULL)
            {
                fprintf(stderr, "generate_context(): something wrong here, the contextable will not be null forever.\n");
                exit(EXIT_FAILURE);
            }
            else
            {
                VectorAppend(contextable->context, &binding);
            } 
            generate_context(binding, node, aux_data);
        }

        if (node->contents.local_binding_form.type == LET)
        {
            Vector *bindings = node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = node->contents.local_binding_form.contents.lets.body_exprs;

            for (size_t i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                generate_context(binding, node, aux_data);
            }

            // append bindings to every expr in body_exprs, even a Number_Literal ast node.
            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i); 
                if (body_expr->context == NULL)
                {
                    body_expr->context = VectorNew(sizeof(AST_Node *));
                }
                for(size_t j = 0; j < VectorLength(bindings); j++)
                {
                    AST_Node *binding = *(AST_Node **)VectorNth(bindings, j);
                    VectorAppend(body_expr->context, &binding);
                }
            }

            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                generate_context(body_expr, node, aux_data);
            }
        }

        if (node->contents.local_binding_form.type == LET_STAR)
        {
            Vector *bindings = node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = node->contents.local_binding_form.contents.lets.body_exprs;
            
            for (size_t i = 0; i < VectorLength(bindings) - 1; i++)
            {
                AST_Node *nxt_binding = *(AST_Node **)VectorNth(bindings, i + 1);
                AST_Node *value = nxt_binding->contents.binding.value;
                if (value->context == NULL)
                {
                    value->context = VectorNew(sizeof(AST_Node *));
                }
                for (size_t j = 0; j < i + 1; j++)
                {
                    AST_Node *binding = *(AST_Node **)VectorNth(bindings, j);
                    VectorAppend(value->context, &binding);
                }
            }

            for (size_t i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                generate_context(binding, node, aux_data);
            }
 
            // append bindings to every expr in body_exprs, even a Number_Literal ast node.
            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i); 
                if (body_expr->context == NULL)
                {
                    body_expr->context = VectorNew(sizeof(AST_Node *));
                }
                for(size_t j = 0; j < VectorLength(bindings); j++)
                {
                    AST_Node *binding = *(AST_Node **)VectorNth(bindings, j);
                    VectorAppend(body_expr->context, &binding);
                }
            }

            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                generate_context(body_expr, node, aux_data);
            }
        }

        if (node->contents.local_binding_form.type == LETREC)
        {
            Vector *bindings = node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = node->contents.local_binding_form.contents.lets.body_exprs;

            if (VectorLength(bindings) == 1)
            {
                AST_Node *single_binding = *(AST_Node **)VectorNth(bindings, 0);
                AST_Node *value = single_binding->contents.binding.value;

                if (value->context == NULL)
                {
                    value->context = VectorNew(sizeof(AST_Node *));
                }

                VectorAppend(value->context, &single_binding);
            }

            for (size_t i = 0; i < VectorLength(bindings) - 1; i++)
            {
                AST_Node *nxt_binding = *(AST_Node **)VectorNth(bindings, i + 1);
                AST_Node *value = nxt_binding->contents.binding.value;

                if (value->context == NULL)
                {
                    value->context = VectorNew(sizeof(AST_Node *));
                }
                
                for (size_t j = 0; j <= i + 1; j++)
                {
                    AST_Node *binding = *(AST_Node **)VectorNth(bindings, j);
                    VectorAppend(value->context, &binding);
                }
            }

            for (size_t i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                generate_context(binding, node, aux_data);
            } 

            // append bindings to every expr in body_exprs, even a Number_Literal ast node.
            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i); 
                if (body_expr->context == NULL)
                {
                    body_expr->context = VectorNew(sizeof(AST_Node *));
                }
                for(size_t j = 0; j < VectorLength(bindings); j++)
                {
                    AST_Node *binding = *(AST_Node **)VectorNth(bindings, j);
                    VectorAppend(body_expr->context, &binding);
                }
            }

            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                generate_context(body_expr, node, aux_data);
            }
        }
    }

    if (node->type == Set_Form)
    {
        AST_Node *id = node->contents.set_form.id;
        AST_Node *expr = node->contents.set_form.expr;
        generate_context(id, node, aux_data);
        generate_context(expr, node, aux_data);
    }

    if (node->type == Conditional_Form)
    {
        if (node->contents.conditional_form.type == IF)
        {
            AST_Node *test_expr = node->contents.conditional_form.contents.if_expression.test_expr;
            AST_Node *then_expr = node->contents.conditional_form.contents.if_expression.then_expr;
            AST_Node *else_expr = node->contents.conditional_form.contents.if_expression.else_expr;
            generate_context(test_expr, node, aux_data);
            generate_context(then_expr, node, aux_data);
            generate_context(else_expr, node, aux_data);
        }

        if (node->contents.conditional_form.type == AND)
        {
            Vector *exprs = node->contents.conditional_form.contents.and_expression.exprs;
            for (size_t i = 0; i < VectorLength(exprs); i++) 
            {
                AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                generate_context(expr, node, aux_data);
            }
        }

        if (node->contents.conditional_form.type == NOT)
        {
            AST_Node *expr = node->contents.conditional_form.contents.not_expression.expr;
            generate_context(expr, node, aux_data);
        }

        if (node->contents.conditional_form.type == OR)
        {
            Vector *exprs = node->contents.conditional_form.contents.or_expression.exprs;
            for (size_t i = 0; i < VectorLength(exprs); i++) 
            {
                AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                generate_context(expr, node, aux_data);
            }
        }

        if (node->contents.conditional_form.type == COND)
        {
            Vector *cond_clauses = node->contents.conditional_form.contents.cond_expression.cond_clauses;
            for (size_t i = 0; i < VectorLength(cond_clauses); i++)
            {
                AST_Node *cond_clause = *(AST_Node **)VectorNth(cond_clauses, i);
                generate_context(cond_clause, node, aux_data);
            }
        }
    }

    if (node->type == Cond_Clause)
    {
        Cond_Clause_Type cond_clause_type = node->contents.cond_clause.type;

        if (cond_clause_type == TEST_EXPR_WITH_THENBODY)
        {
            // [test-expr then-body ...+]
            // needs to free test_expr and then_bodies
            AST_Node *test_expr = node->contents.cond_clause.test_expr;
            generate_context(test_expr, node, aux_data);

            Vector *then_bodies = node->contents.cond_clause.then_bodies;
            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                generate_context(then_body, node, aux_data);
            }
        }
        else if (cond_clause_type == ELSE_STATEMENT)
        {
            // [else then-body ...+]
            // needs to free then_bodies
            Vector *then_bodies = node->contents.cond_clause.then_bodies;
            for (size_t i = 0; i < VectorLength(then_bodies); i++)
            {
                AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, i);
                generate_context(then_body, node, aux_data);
            }
        }
        else if (cond_clause_type == TEST_EXPR_WITH_PROC)
        {
        }
        else if (cond_clause_type == SINGLE_TEST_EXPR)
        {
        }
        else
        {
            // something wrong here
            fprintf(stderr, "generate_context(): can not handle cond_clause_type: %d\n", cond_clause_type);
            exit(EXIT_FAILURE);
        }
    }

    if (node->type == Call_Expression)
    {
        Vector *params = node->contents.call_expression.params;
        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *param = *(AST_Node **)VectorNth(params, i);
            generate_context(param, node, aux_data);
        }
    }

    if (node->type == Binding)
    {
        AST_Node *value = node->contents.binding.value;
        // such call_expression (+ a b), 'a' and 'b' is binding, but dont have value.
        // it should be search the value for 'a' in eval();
        if (value != NULL) generate_context(value, node, aux_data); 
    }
    
    if (node->type == Lambda_Form) 
    {
        Vector *params = node->contents.lambda_form.params;
        Vector *body_exprs = node->contents.lambda_form.body_exprs;

        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *param = *(AST_Node **)VectorNth(params, i);
            generate_context(param, node, aux_data);
        }

        // append param to every expr in body_exprs, even a Number_Literal ast node.
        for (size_t i = 0; i < VectorLength(body_exprs); i++)
        {
            AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i); 
            if (body_expr->context == NULL)
            {
                body_expr->context = VectorNew(sizeof(AST_Node *));
            }
            for(size_t j = 0; j < VectorLength(params); j++)
            {
                AST_Node *param = *(AST_Node **)VectorNth(params, j);
                VectorAppend(body_expr->context, &param);
            }
        }

        for (size_t i = 0; i < VectorLength(body_exprs); i++)
        {
            AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
            generate_context(body_expr, node, aux_data);
        }
    }

    if (node->type == Procedure)
    {
        // built-in procedure.
        if (node->contents.procedure.c_native_function != NULL)
        {

        }

        // programmer defined procedure.
        if (node->contents.procedure.c_native_function == NULL)
        {
            Vector *params = node->contents.procedure.params;
            Vector *body_exprs = node->contents.procedure.body_exprs;

            for (size_t i = 0; i < VectorLength(params); i++)
            {
                AST_Node *param = *(AST_Node **)VectorNth(params, i);
                generate_context(param, node, aux_data);
            }

            // append param to every expr in body_exprs, even a Number_Literal ast node.
            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i); 
                if (body_expr->context == NULL)
                {
                    body_expr->context = VectorNew(sizeof(AST_Node *));
                }
                for(size_t j = 0; j < VectorLength(params); j++)
                {
                    AST_Node *param = *(AST_Node **)VectorNth(params, j);
                    VectorAppend(body_expr->context, &param);
                }
            }

            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                generate_context(body_expr, node, aux_data);
            } 
        }
    }
    
    if (node->type == Number_Literal ||
        node->type == String_Literal ||
        node->type == Character_Literal ||
        node->type == Boolean_Literal)
    {
        return;
    }

    if (node->type == NULL_Expression)
    {
        AST_Node *value = node->contents.null_expression.value;
        if (value != NULL)
            generate_context(value, node, aux_data);
    }

    if (node->type == EMPTY_Expression)
    {
        AST_Node *value = node->contents.empty_expression.value;
        if (value != NULL)
            generate_context(value, node, aux_data);
    }

    if (node->type == List_Literal)
    {
        // symbol doesnt impled right now, so '((+ 1 2)) will not be supported.
        // only literal will appear in list literal. 
        // nothing will happend for follow code.
        Vector *elems = (Vector *)node->contents.literal.value;
        for (size_t i = 0; i < VectorLength(elems); i++)
        {
            AST_Node *elem = *(AST_Node **)VectorNth(elems, i);
            generate_context(elem, node, aux_data);
        }
    }

    if (node->type == Pair_Literal)
    {
        // same situation with List_Literal.
        Vector *elems = (Vector *)node->contents.literal.value;
        for (size_t i = 0; i < VectorLength(elems); i++)
        {
            AST_Node *elem = *(AST_Node **)VectorNth(elems, i);
            generate_context(elem, node, aux_data);
        }
    }
}

// param: AST_Node *(type: Binding) has just a name.
// return: AST_Node *(type: Binding) contains value.
static AST_Node *search_binding_value(AST_Node *binding)
{
    if (binding == NULL)
    {
        fprintf(stderr, "can not search binding value for NULL\n");
        exit(EXIT_FAILURE);
    }

    // if current binding node has value.
    if (binding->contents.binding.value != NULL) return binding;

    AST_Node *binding_contains_value = NULL;
    AST_Node *contextable = find_contextable_node(binding);

    // search binding's value by 'name'
    while (contextable != NULL)
    {
        Vector *context = contextable->context;
        Vector *built_in_bindings = NULL;

        if (contextable->type == Program) built_in_bindings = contextable->contents.program.built_in_bindings;
        
        for (size_t i = VectorLength(context); i > 0; i--)
        {
            AST_Node *node = *(AST_Node **)VectorNth(context, i - 1);
            #ifdef DEBUG_MODE
            printf("searching for name: %s, cur node's name: %s\n", binding->contents.binding.name, node->contents.binding.name);
            #endif
            if (strcmp(binding->contents.binding.name, node->contents.binding.name) == 0)
            {
                binding_contains_value = node;

                // check value
                AST_Node *value = binding_contains_value->contents.binding.value;
                if (value == NULL)
                {
                    fprintf(stderr, "search_binding_value(): unbound identifier: %s\n", binding->contents.binding.name);
                    exit(EXIT_FAILURE);
                }

                goto found;
            }
        }

        if (built_in_bindings != NULL)
        {
            for (size_t i = VectorLength(built_in_bindings); i > 0; i--)
            {
                AST_Node *node = *(AST_Node **)VectorNth(built_in_bindings, i - 1);
                #ifdef DEBUG_MODE
                printf("searching for name: %s, cur node's name: %s\n", binding->contents.binding.name, node->contents.binding.name);
                #endif
                if (strcmp(binding->contents.binding.name, node->contents.binding.name) == 0)
                {
                    binding_contains_value = node;

                    // check value
                    AST_Node *value = binding_contains_value->contents.binding.value;
                    if (value == NULL)
                    {
                        fprintf(stderr, "search_binding_value(): unbound identifier: %s\n", binding->contents.binding.name);
                        exit(EXIT_FAILURE);
                    }

                    goto found;
                }
            }
        }
        
        contextable = find_contextable_node(contextable->parent);
    }

    if (binding_contains_value == NULL)
    {
        fprintf(stderr, "search_binding_value(): unbound identifier: %s\n", binding->contents.binding.name);
        exit(EXIT_FAILURE);
    }

    found: return binding_contains_value;
}

static void *context_copy_helper(void *value_addr, size_t index, Vector *original_vector, Vector *new_vector, void *aux_data)
{
    AST_Node *binding = *(AST_Node **)value_addr;
    return value_addr;
}

// return null when work out no value.
// this function will make some ast_node not in ast, should be free manually.
Result eval(AST_Node *ast_node, void *aux_data)
{
    if (ast_node == NULL) return NULL;
    Result result = NULL;
    bool matched = false;

    if (ast_node->type == Call_Expression)
    {
        matched = true;
        
        const char *name = ast_node->contents.call_expression.name;
        AST_Node *anonymous_procedure = ast_node->contents.call_expression.anonymous_procedure;
        AST_Node *procedure = NULL;

        // anonymous procedure call or ((lambda (x) x) 1)
        if (name == NULL && anonymous_procedure != NULL)
        {
            procedure = anonymous_procedure;
            if (procedure->type == Lambda_Form)
                procedure = eval(procedure, aux_data);
        }
        // named procedure call
        else if (name != NULL && anonymous_procedure == NULL)
        {
            AST_Node *binding = ast_node_new(NOT_IN_AST, Binding, name, NULL);
            generate_context(binding, ast_node, NULL);
            AST_Node *binding_contains_value = search_binding_value(binding);
            ast_node_free(binding); // free the tmp binding.

            if (binding_contains_value == NULL)
            {
                fprintf(stderr, "eval(): unbound identifier: %s\n", name);
                exit(EXIT_FAILURE);
            }

            procedure = binding_contains_value->contents.binding.value;
        }
        else {
            fprintf(stderr, "eval(): call expression error\n");
            exit(EXIT_FAILURE);
        }

        if (procedure->type != Procedure)
        {
            fprintf(stderr, "eval(): not a procedure: %s\n", name);
            exit(EXIT_FAILURE); 
        }

        // built-in procedure.
        if (procedure->contents.procedure.c_native_function != NULL)
        {
            Vector *params = ast_node->contents.call_expression.params;
            Vector *operands = VectorNew(sizeof(AST_Node *));

            // eval out operands.
            for (size_t i = 0; i < VectorLength(params); i++)
            {
                AST_Node *param = *(AST_Node **)VectorNth(params, i);
                AST_Node *operand = eval(param, aux_data);
                VectorAppend(operands, &operand);
            }

            Function c_native_function = procedure->contents.procedure.c_native_function;
            result = ((AST_Node *(*)(AST_Node *procedure, Vector *operands))c_native_function)(procedure, operands);

            VectorFree(operands, NULL, NULL);
        }

        // programmer defined procedure.
        if (procedure->contents.procedure.c_native_function == NULL)
        {
            Vector *params = ast_node->contents.call_expression.params;
            Vector *operands = VectorNew(sizeof(AST_Node *)); 

            // eval out operands.
            for (size_t i = 0; i < VectorLength(params); i++)
            {
                AST_Node *param = *(AST_Node **)VectorNth(params, i);
                AST_Node *operand = eval(param, aux_data);
                VectorAppend(operands, &operand);
            }

            // check arity.
            int required_params_count = procedure->contents.procedure.required_params_count;
            size_t operands_count = VectorLength(operands);
            if (operands_count != required_params_count)
            {
                if (procedure->contents.procedure.name == NULL)
                {
                    fprintf(stderr, "anomyous procedure: arity mismatch;\n"
                                    "the expected number of arguments does not match the given number\n"
                                    "expected: %d\n"
                                    "given: %zu\n", required_params_count, operands_count);
                    exit(EXIT_FAILURE); 
                }
                else if (procedure->contents.procedure.name != NULL)
                {
                    fprintf(stderr, "%s: arity mismatch;\n"
                                    "the expected number of arguments does not match the given number\n"
                                    "expected: %d\n"
                                    "given: %zu\n", procedure->contents.procedure.name, required_params_count, operands_count);
                    exit(EXIT_FAILURE); 
                }
            }

            // generate a environment for every function call.
            Vector *virtual_params = procedure->contents.procedure.params;
            Vector *body_exprs = procedure->contents.procedure.body_exprs;

            Vector *virtual_params_copy = VectorNew(sizeof(AST_Node *));
            Vector *body_exprs_copy = VectorNew(sizeof(AST_Node *));
            
            for (size_t i = 0; i < VectorLength(virtual_params); i++)
            {
                AST_Node *node = *(AST_Node **)VectorNth(virtual_params, i);
                AST_Node *node_copy = ast_node_deep_copy(node, aux_data);

                if (node->context != NULL)
                {
                    node_copy->context = VectorCopy(node->context, context_copy_helper, NULL);
                }

                generate_context(node_copy, node->parent, NULL);
                VectorAppend(virtual_params_copy, &node_copy);
            }

            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *node = *(AST_Node **)VectorNth(body_exprs, i);
                AST_Node *node_copy = ast_node_deep_copy(node, aux_data);

                if (node->context != NULL)
                {
                    node_copy->context = VectorNew(sizeof(AST_Node *));
                    for(size_t j = 0; j < VectorLength(virtual_params_copy); j++)
                    {
                        AST_Node *virtual_param_copy = *(AST_Node **)VectorNth(virtual_params_copy, j);
                        VectorAppend(node_copy->context, &virtual_param_copy);
                    }
                }

                generate_context(node_copy, node->parent, NULL);
                VectorAppend(body_exprs_copy, &node_copy);
            }

            // binding virtual params copy to actual params.
            for (size_t i = 0; i < VectorLength(virtual_params_copy); i++)
            {
                AST_Node *virtual_param_copy = *(AST_Node **)VectorNth(virtual_params_copy, i);
                AST_Node *operand = *(AST_Node **)VectorNth(operands, i);
                virtual_param_copy->contents.binding.value = operand;
            }

            // eval
            size_t last = VectorLength(body_exprs_copy) - 1;
            for (size_t i = 0; i < VectorLength(body_exprs_copy); i++)
            {
                AST_Node *body_expr_copy = *(AST_Node **)VectorNth(body_exprs_copy, i);
                result = eval(body_expr_copy, aux_data);
                if (i != last && result != NULL && ast_node_get_tag(result) == NOT_IN_AST)
                {
                    ast_node_free(result);
                }
            }

            // set virtual_param_copy's value to null.
            for (size_t i = 0; i < VectorLength(virtual_params_copy); i++)
            {
                AST_Node *virtual_param_copy = *(AST_Node **)VectorNth(virtual_params_copy, i);
                virtual_param_copy->contents.binding.value = NULL; 
            }

            // free operands
            VectorFree(operands, NULL, NULL);

            // free virtual_params_copy.
            for (size_t i = 0; i < VectorLength(virtual_params_copy); i++)
            {
                AST_Node *virtual_param_copy = *(AST_Node **)VectorNth(virtual_params_copy, i);
                ast_node_free(virtual_param_copy);
            }
            VectorFree(virtual_params_copy, NULL, NULL);

            // free body_exprs_copy
            // TO-DO solve problem here !
            VectorFree(body_exprs_copy, NULL, NULL);
        }
    }
    
    if (ast_node->type == Local_Binding_Form)
    {
        Local_Binding_Form_Type local_binding_form_type = ast_node->contents.local_binding_form.type;

        if (local_binding_form_type == DEFINE)
        {
            // define works out no value.
            // eval the init value of binding into a new ast_node, and instead of the init value node by new ast_node.
            matched = true;
            AST_Node *binding = ast_node->contents.local_binding_form.contents.define.binding;
            AST_Node *init_value = binding->contents.binding.value;
            AST_Node *eval_value = eval(init_value, aux_data);
            binding->contents.binding.value = eval_value;

            if (eval_value != init_value)
            {
                if (eval_value != NULL && ast_node_get_tag(eval_value) == IN_AST)
                {
                    binding->contents.binding.value = ast_node_deep_copy(eval_value, NULL);
                }
                else if (eval_value != NULL && ast_node_get_tag(eval_value) == NOT_IN_AST)
                {
                    ast_node_set_tag(binding->contents.binding.value, IN_AST);
                }
                /**
                 *  give up middle garbage release that generated by eval().
                 *  ast_node_free(init_value);
                 */
                generate_context(binding->contents.binding.value, binding, NULL);
            }

            if (binding->contents.binding.value->type == Procedure)
            {
                AST_Node *procedure = binding->contents.binding.value;
                procedure->contents.procedure.name = malloc(strlen(binding->contents.binding.name) + 1);
                strcpy(procedure->contents.procedure.name, binding->contents.binding.name);
            }
        }
        
        if (local_binding_form_type == LET ||
            local_binding_form_type == LET_STAR ||
            local_binding_form_type == LETREC)
        {
            matched = true;
            Vector *bindings = ast_node->contents.local_binding_form.contents.lets.bindings;
            Vector *body_exprs = ast_node->contents.local_binding_form.contents.lets.body_exprs;

            // eval bindings
            for (size_t i = 0; i < VectorLength(bindings); i++)
            {
                AST_Node *binding = *(AST_Node **)VectorNth(bindings, i);
                AST_Node *init_value = binding->contents.binding.value;
                AST_Node *eval_value = eval(init_value, aux_data);
                binding->contents.binding.value = eval_value;

                if (eval_value != init_value)
                {
                    if (eval_value != NULL && ast_node_get_tag(eval_value) == IN_AST)
                    {
                        binding->contents.binding.value = ast_node_deep_copy(eval_value, NULL);
                    }
                    else if (eval_value != NULL && ast_node_get_tag(eval_value) == NOT_IN_AST)
                    {
                        ast_node_set_tag(binding->contents.binding.value, IN_AST);
                    }
                    /**
                     *   give up middle garbage release that generated by eval().
                     *   ast_node_free(init_value);
                     */
                    generate_context(binding->contents.binding.value, binding, NULL);
                }

                if (binding->contents.binding.value->type == Procedure)
                {
                    AST_Node *procedure = binding->contents.binding.value;
                    procedure->contents.procedure.name = malloc(strlen(binding->contents.binding.name) + 1);
                    strcpy(procedure->contents.procedure.name, binding->contents.binding.name);
                }
            }

            // eval body_exprs
            size_t last = VectorLength(body_exprs) - 1;
            for (size_t i = 0; i < VectorLength(body_exprs); i++)
            {
                AST_Node *body_expr = *(AST_Node **)VectorNth(body_exprs, i);
                result = eval(body_expr, aux_data); // the last body_expr's result will be returned;
                if (i != last) 
                {
                    if (result != NULL && ast_node_get_tag(result) == NOT_IN_AST)
                    {
                        /**
                         *   give up middle garbage release that generated by eval().
                         *   ast_node_free(result);
                         */
                    }
                }
            }
        }
    }

    if (ast_node->type == Set_Form)
    {
        matched = true;
        AST_Node *id = ast_node->contents.set_form.id;
        AST_Node *expr = ast_node->contents.set_form.expr;

        AST_Node *binding = search_binding_value(id);
        AST_Node *expr_val = eval(expr, aux_data);

        AST_Node *old_value = binding->contents.binding.value;
        ast_node_free(old_value);

        binding->contents.binding.value = ast_node_deep_copy(expr_val, aux_data);
        binding->contents.binding.value->tag = IN_AST;
        if (binding->contents.binding.value->type == Procedure)
        {
            AST_Node *procedure = binding->contents.binding.value;
            procedure->contents.procedure.name = malloc(strlen(binding->contents.binding.name) + 1);
            strcpy(procedure->contents.procedure.name, binding->contents.binding.name);
        }
        generate_context(binding->contents.binding.value, ast_node, aux_data);

        result = NULL;
    } 

    if (ast_node->type == Conditional_Form)
    {
        Conditional_Form_Type conditional_form_type = ast_node->contents.conditional_form.type;

        if (conditional_form_type == IF)
        {
            matched = true;
            AST_Node *test_expr = ast_node->contents.conditional_form.contents.if_expression.test_expr;
            AST_Node *then_expr = ast_node->contents.conditional_form.contents.if_expression.then_expr;
            AST_Node *else_expr = ast_node->contents.conditional_form.contents.if_expression.else_expr;

            AST_Node *test_val = eval(test_expr, aux_data);
            if (test_val == NULL)
            {
                fprintf(stderr, "eval(): if: bad syntax\n");
                exit(EXIT_FAILURE); 
            }

            Boolean_Type val = R_TRUE; // true by default.
            if (test_val->type == Boolean_Literal &&
               *(Boolean_Type *)(test_val->contents.literal.value) == R_FALSE)
            {
                val = R_FALSE;
            }

            if (ast_node_get_tag(test_val) == NOT_IN_AST)
            {
                ast_node_free(test_val);
            }

            if (val == R_TRUE)
            {
                // excute then expr.
                // only (define) will work out NULL, and (define) can not be in (if).
                result = eval(then_expr, aux_data);
                if (result == NULL)
                {
                    fprintf(stderr, "eval(): if: bad syntax\n");
                    exit(EXIT_FAILURE);
                }
            }

            if (val == R_FALSE)
            {
                // excute else expr.
                // only (define) will work out NULL, and (define) can not be in (if).
                result = eval(else_expr, aux_data);
                if (result == NULL)
                {
                    fprintf(stderr, "eval(): if: bad syntax\n");
                    exit(EXIT_FAILURE);
                }
            }
        }

        if (conditional_form_type == COND)
        {
            matched = true;
            Vector *cond_clauses = ast_node->contents.conditional_form.contents.cond_expression.cond_clauses;

            for (size_t i = 0; i < VectorLength(cond_clauses); i++)
            {
                AST_Node *cond_clause = *(AST_Node **)VectorNth(cond_clauses, i);

                if (cond_clause->contents.cond_clause.type == TEST_EXPR_WITH_THENBODY)
                {
                    // [test-expr then-body ...+]
                    AST_Node *test_expr = cond_clause->contents.cond_clause.test_expr;

                    AST_Node *test_val = eval(test_expr, aux_data);
                    if (test_val == NULL)
                    {
                        fprintf(stderr, "eval(): cond: bad syntax\n");
                        exit(EXIT_FAILURE); 
                    }

                    if (test_val->type != Boolean_Literal ||
                        (test_val->type == Boolean_Literal && (*(Boolean_Type *)(test_val->contents.literal.value) == R_TRUE)))

                    {
                        Vector *then_bodies = cond_clause->contents.cond_clause.then_bodies;
                        for (size_t j = 0; j < VectorLength(then_bodies); j++)
                        {
                            AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, j);
                            result = eval(then_body, aux_data);
                        }
                        break;
                    }

                    if (ast_node_get_tag(test_val) == NOT_IN_AST)
                    {
                        ast_node_free(test_val);
                    } 
                }
                else if (cond_clause->contents.cond_clause.type == ELSE_STATEMENT)
                {
                    // [else then-body ...+]
                    Vector *then_bodies = cond_clause->contents.cond_clause.then_bodies;
                    for (size_t j = 0; j < VectorLength(then_bodies); j++)
                    {
                        AST_Node *then_body = *(AST_Node **)VectorNth(then_bodies, j);
                        result = eval(then_body, aux_data);
                    }
                    break;
                }
                else if (cond_clause->contents.cond_clause.type == TEST_EXPR_WITH_PROC)
                {
                }
                else if (cond_clause->contents.cond_clause.type == SINGLE_TEST_EXPR)
                {
                }
                else
                {
                    // something wrong here
                    fprintf(stderr, "eval(): can not handle Cond_Clause_Type: %d\n", cond_clause->contents.cond_clause.type);
                    exit(EXIT_FAILURE);
                }
            }
        }

        if (conditional_form_type == AND)
        {
            matched = true;
            Vector *exprs = ast_node->contents.conditional_form.contents.and_expression.exprs;

            if (VectorLength(exprs) == 0)
            {
                Boolean_Type *value = malloc(sizeof(Boolean_Type)); 
                *value = R_TRUE;
                result = ast_node_new(NOT_IN_AST, Boolean_Literal, value);
            }

            if (VectorLength(exprs) > 0)
            {
                for (size_t i = 0; i < VectorLength(exprs); i++)
                {
                    AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                    AST_Node *expr_val = eval(expr, aux_data);

                    if (expr_val->type == Boolean_Literal &&
                        *(Boolean_Type *)(expr_val->contents.literal.value) == R_FALSE)
                    {
                        Boolean_Type *value = malloc(sizeof(Boolean_Type)); 
                        *value = R_FALSE;
                        result = ast_node_new(NOT_IN_AST, Boolean_Literal, value);
                        break;
                    }

                    result = expr_val;
                }
            }
        }

        if (conditional_form_type == NOT)
        {
            matched = true;

            AST_Node *expr = ast_node->contents.conditional_form.contents.not_expression.expr;
            AST_Node *expr_val = eval(expr, aux_data);            

            Boolean_Type *value = malloc(sizeof(Boolean_Type));

            if (expr_val->type == Boolean_Literal && *(Boolean_Type *)(expr_val->contents.literal.value) == R_FALSE)
                *value = R_TRUE; 
            else
                *value = R_FALSE; 

            result = ast_node_new(NOT_IN_AST, Boolean_Literal, value);
        }

        if (conditional_form_type == OR)
        {
            matched = true;
            Vector *exprs = ast_node->contents.conditional_form.contents.or_expression.exprs;
            
            if (VectorLength(exprs) == 0)
            {
                Boolean_Type *value = malloc(sizeof(Boolean_Type));
                *value = R_FALSE;
                result = ast_node_new(NOT_IN_AST, Boolean_Literal, value);
            }
            else if (VectorLength(exprs) == 1)
            {
                AST_Node *expr = *(AST_Node **)VectorNth(exprs, 0);
                AST_Node *expr_val = eval(expr, aux_data);
                result = expr_val;
            }
            else if (VectorLength(exprs) > 1)
            {
                for (size_t i = 0; i < VectorLength(exprs); i++)
                {
                    AST_Node *expr = *(AST_Node **)VectorNth(exprs, i);
                    AST_Node *expr_val = eval(expr, aux_data);

                    if (expr_val->type != Boolean_Literal ||
                        (expr_val->type == Boolean_Literal && *(Boolean_Type *)(expr_val->contents.literal.value) == R_TRUE))
                    {
                        result = expr_val;
                        break;
                    } 

                    result = expr_val;
                }
            }
        }
    }

    if (ast_node->type == Binding)
    { 
        // return the value of Binding.
        matched = true;
        AST_Node *value = ast_node->contents.binding.value;
        if (value == NULL)
        {
            AST_Node *binding_contains_value = search_binding_value(ast_node);
            if (binding_contains_value == NULL)
            {
                fprintf(stderr, "eval(): unbound identifier: %s\n", ast_node->contents.binding.name);
                exit(EXIT_FAILURE);
            }
            value = binding_contains_value->contents.binding.value;
        }
        result = value;
    }

    // these kind of AST_Node_Type works out them self.
    if (ast_node->type == Number_Literal ||
        ast_node->type == String_Literal ||
        ast_node->type == Character_Literal ||
        ast_node->type == Boolean_Literal)
    {
        matched = true;
        result = ast_node;
    }

    if (ast_node->type == NULL_Expression ||
        ast_node->type == EMPTY_Expression)
    {
        // return '()
        // the element's size in list means nothing, so use 1 byte(sizeof(char))
        Vector *empty_vector = VectorNew(sizeof(char));
        AST_Node *empty_list = ast_node_new(IN_AST, List_Literal, empty_vector);
        return empty_list;
    }

    // List_Literal works out itself.
    if (ast_node->type == List_Literal)
    {
        matched = true;
        result = ast_node;
    }

    // Pair_Literal works out itself.
    if (ast_node->type == Pair_Literal)
    {
        matched = true;
        result = ast_node;
    }

    // Procedure works out itself.
    if (ast_node->type == Procedure)
    {
        matched = true;
        result = ast_node;
    }

    if (ast_node->type == Lambda_Form)
    {
        matched = true;
        Vector *params = ast_node->contents.lambda_form.params;
        Vector *body_exprs = ast_node->contents.lambda_form.body_exprs;

        result = ast_node_new(NOT_IN_AST, Procedure, NULL, -1, NULL, NULL, NULL);

        Vector *params_copy = VectorNew(sizeof(AST_Node *));
        Vector *body_exprs_copy = VectorNew(sizeof(AST_Node *));

        for (size_t i = 0; i < VectorLength(params); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(params, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            if (node->context != NULL)
            {
                Vector *context_copy = VectorCopy(node->context, context_copy_helper, NULL);
                node_copy->context = context_copy;
            }
            generate_context(node_copy, result, NULL);
            VectorAppend(params_copy, &node_copy);
        }

        for (size_t i = 0; i < VectorLength(body_exprs); i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(body_exprs, i);
            AST_Node *node_copy = ast_node_deep_copy(node, aux_data);
            if (node_copy->context == NULL)
            {
                node_copy->context = VectorNew(sizeof(AST_Node *));
            }
            for(size_t j = 0; j < VectorLength(params_copy); j++)
            {
                AST_Node *param_copy = *(AST_Node **)VectorNth(params_copy, j);
                VectorAppend(node_copy->context, &param_copy);
            }
            generate_context(node_copy, result, NULL);
            VectorAppend(body_exprs_copy, &node_copy);
        }

        result->contents.procedure.required_params_count = VectorLength(params_copy);
        result->contents.procedure.params = params_copy;
        result->contents.procedure.body_exprs = body_exprs_copy;

        // if lambda_form itself has context, it should be inherit to the procedure.
        if (ast_node->context != NULL)
        {
            result->context = VectorCopy(ast_node->context, context_copy_helper, NULL);
        }

        generate_context(result, ast_node->parent, NULL);
    }

    if (matched == false)
    {
        // when no matches any AST_Node_Type.
        fprintf(stderr, "eval(): can not eval AST_Node_Type: %d\n", ast_node->type);
        exit(EXIT_FAILURE);
    }

    return result;
}

// calculator parts
// return: Vector *(Result)
Vector *calculator(AST ast, void *aux_data)
{
    generate_context(ast, NULL, NULL); // generate context 

    Vector *body = ast->contents.program.body;
    Vector *results = VectorNew(sizeof(AST_Node *));

    for (size_t i = 0; i < VectorLength(body); i++)
    {
        AST_Node *sub_node = *(AST_Node **)VectorNth(body, i);
        Result result = eval(sub_node, aux_data);
        if (result != NULL)
        {
            if (ast_node_get_tag(result) == IN_AST)
            {
                AST_Node *result_copy = ast_node_deep_copy(result, NULL);
                result_copy->tag = NOT_IN_AST;
                VectorAppend(results, &result_copy);
            }
            else if (ast_node_get_tag(result) == NOT_IN_AST)
            {
                VectorAppend(results, &result);
            }
        }
    }

    return results;
}

static int result_free(Result result)
{
    if (result == NULL) return 1;
    return ast_node_free(result);
}

int results_free(Vector *results)
{
    int error = 0;

    for (size_t i = 0; i < VectorLength(results); i++)
    {
        AST_Node *result = *(AST_Node **)VectorNth(results, i);
        error = error | result_free(result);
    }

    return error;
}

static void output_result(Result result, void *aux_data)
{
    bool matched = false;

    if (result->type == Number_Literal)
    {
        matched = true;
        fprintf(stdout, "%s", (char *)(result->contents.literal.value));
    }

    if (result->type ==  String_Literal)
    {
        matched = true;
        fprintf(stdout, "\"%s\"", (char *)(result->contents.literal.value));
    }

    if (result->type ==  Character_Literal)
    {
        matched = true;
        fprintf(stdout, "#\\%c", *(char *)(result->contents.literal.value));
    }

    if (result->type ==  List_Literal)
    {
        matched = true;

        Vector *value = (Vector *)(result->contents.literal.value);

        size_t length = VectorLength(value);
        size_t last = length - 1;

        if (aux_data != NULL && strcmp(aux_data, "in_list_or_in_pair") == 0)
        {
            fprintf(stdout, "(");
        }
        else
        {
            fprintf(stdout, "'(");
        }
        
        for (size_t i = 0; i < length; i++)
        {
            AST_Node *node = *(AST_Node **)VectorNth(value, i);
            output_result(node, "in_list_or_in_pair");
            if (i != last) fprintf(stdout, " ");
        }
        fprintf(stdout, ")");
    }

    if (result->type == Pair_Literal)
    {
        matched = true;

        Vector *value = (Vector *)(result->contents.literal.value);
        AST_Node *car = *(AST_Node **)VectorNth(value, 0);
        AST_Node *cdr = *(AST_Node **)VectorNth(value, 1);

        int length = 2; // a dot pair always has two elements.

        if (aux_data != NULL && strcmp(aux_data, "in_list_or_in_pair") == 0)
        {
            fprintf(stdout, "(");
        }
        else
        {
            fprintf(stdout, "'(");
        }
        output_result(car, "in_list_or_in_pair");
        fprintf(stdout, " . ");
        output_result(cdr, "in_list_or_in_pair");
        fprintf(stdout, ")");
    }

    if (result->type == Boolean_Literal)
    {
        matched = true;

        Boolean_Type *value = (Boolean_Type *)(result->contents.literal.value);

        if (*value == R_TRUE)
            fprintf(stdout, "#t");
        else if (*value == R_FALSE)
            fprintf(stdout, "#f");
    }

    if (result->type == Procedure)
    {
        matched = true;

        char *name = result->contents.procedure.name;

        if (name == NULL)
            fprintf(stdout, "#<procedure>");
        else if (name != NULL)
            fprintf(stdout, "#<procedure:%s>", name);
    }

    if (matched == false)
    {
        // when no matches any AST_Node_Type.
        fprintf(stderr, "output_result(): can not output AST_Node_Type: %d\n", result->type);
        exit(EXIT_FAILURE);
    }
}

void output_results(Vector *results, void *aux_data)
{
    for (size_t i = 0; i < VectorLength(results); i++)
    {
        Result result = *(AST_Node **)VectorNth(results, i);
        output_result(result, aux_data);
        fprintf(stdout, "\n");
    }
}