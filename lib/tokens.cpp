/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "tokeniser.h"
#include "tokens.h"    


TokenRegistry tokens[] = {
    "*c=", T_EQUALS,
    "*c*", T_MUL,
    "*c+", T_ADD,
    "*c/", T_DIV,
    "*c-", T_SUB,
    "*c(", T_OPREN,
    "*c)", T_CPREN,
    "*c(", T_OPREN,
    "*c;", T_SEMI,
    "*c{", T_OCURLY,
    "*c}", T_CCURLY,
    "*c,", T_COMMA,
    "*c.", T_DOT,
    "*c~", T_TILDE,
    "*c!", T_PLING,
    "*c?", T_QUESTION,
    "*c<", T_LT,
    "*c>", T_GT,
    "*c:", T_COLON,
    "*c#", T_HASH,
    "*c[", T_OSQB,
    "*c]", T_CSQB,
    "*c&", T_AMPERSAND,
    "*c|", T_PIPE,
    "*c^", T_CARET,
    "*c%", T_PERC,
    "*c@", T_AT,
    "*c`", T_BACKTICK,    
    "*c:", T_COLON,

    "*e",		T_END,
    "*s",		T_STRING,
    "*i",		T_IDENT,
    "*n",		T_INT,
    "*f",		T_FLOAT,
    
    "times",		T_TIMES,
    "for",		T_FOR,
    "if",		T_IF,
    "then",		T_THEN,
    "else",		T_ELSE,
    "leave",		T_LEAVE,
    "dup" , 		T_DUP,
    "call" , 		T_CALL,
    "global" , 		T_GLOBAL,
    "swap",		T_SWAP,
    "drop",		T_DROP,
    "not",		T_NOT,
    "and",		T_AND,
    "or",		T_OR,
    "ifleave",		T_IFLEAVE,
    "const",		T_CONST,
    "over",		T_OVER,
    "defer",		T_DEFER,
    "each",		T_EACH,
    
    NULL, -10
};

    
    
