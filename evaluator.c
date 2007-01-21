/* $Id$
 * $URL$
 *
 * expression evaluation
 *
 * Copyright (C) 1999, 2000 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * This file is part of LCD4Linux.
 *
 * LCD4Linux is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * LCD4Linux is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


/* 
 * exported functions:
 *
 * int SetVariable (char *name, RESULT *value)
 *   adds a generic variable to the evaluator
 *
 * int SetVariableNumeric (char *name, double value)
 *   adds a numerical variable to the evaluator
 *
 * int SetVariableString (char *name, char *value)
 *   adds a numerical variable to the evaluator
 *
 * int AddFunction (char *name, int argc, void (*func)())
 *   adds a function to the evaluator
 *
 * void DeleteVariables    (void);
 *   frees all allocated variables
 *
 * void DeleteFunctions    (void);
 *   frees all allocated functions
 *
 * void DelResult (RESULT *result)
 *   sets a result to none
 *   frees a probably allocated memory
 *
 * RESULT* SetResult (RESULT **result, int type, void *value)
 *   initializes a result
 *
 * double R2N (RESULT *result)
 *   converts a result into a number
 *
 * char* R2S (RESULT *result)
 *   converts a result into a string
 *
 *
 * int Compile (char* expression, void **tree)
 *   compiles a expression into a tree
 * 
 * int Eval (void *tree, RESULT *result)
 *   evaluates an expression
 *
 * void DelTree (void *tree)
 *   frees a compiled tree
 */


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <setjmp.h>

#include "debug.h"
#include "evaluator.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

/* string buffer chunk size */
#define CHUNK_SIZE 16

typedef enum {
    T_UNDEF,
    T_NAME,
    T_NUMBER,
    T_STRING,
    T_OPERATOR,
    T_VARIABLE,
    T_FUNCTION
} TOKEN;

typedef enum {
    O_UNDEF,			/* undefined */
    O_LST,			/* expression lists */
    O_SET,			/* variable assignements */
    O_CND,			/* conditional a?b:c */
    O_COL,			/* colon in a?b:c */
    O_OR,			/* logical OR */
    O_AND,			/* logical AND */
    O_EQ,			/* equal */
    O_NE,			/* not equal */
    O_LT,			/* less than */
    O_LE,			/* less or equal */
    O_GT,			/* greater than */
    O_GE,			/* greater or equal */
    O_ADD,			/* addition */
    O_SUB,			/* subtraction */
    O_SGN,			/* sign '-' */
    O_CAT,			/* string concatenation */
    O_MUL,			/* multiplication */
    O_DIV,			/* division */
    O_MOD,			/* modulo */
    O_POW,			/* power */
    O_NOT,			/* logical NOT */
    O_BRO,			/* open brace */
    O_COM,			/* comma (argument seperator) */
    O_BRC			/* closing brace */
} OPERATOR;

typedef struct {
    char *pattern;
    int len;
    OPERATOR op;
} PATTERN;

typedef struct {
    char *name;
    RESULT *value;
} VARIABLE;

typedef struct {
    char *name;
    int argc;
    void (*func) ();
} FUNCTION;

typedef struct _NODE {
    TOKEN Token;
    OPERATOR Operator;
    RESULT *Result;
    VARIABLE *Variable;
    FUNCTION *Function;
    int Children;
    struct _NODE **Child;
} NODE;



/* operators */
/* IMPORTANT! list must be sorted by length! */
static PATTERN Pattern[] = {
    {";", 1, O_LST},		/* expression lists */
    {"=", 1, O_SET},		/* variable assignements */
    {"?", 1, O_CND},		/* conditional a?b:c */
    {":", 1, O_COL},		/* colon a?b:c */
    {"|", 1, O_OR},		/* logical OR */
    {"&", 1, O_AND},		/* logical AND */
    {"<", 1, O_LT},		/* less than */
    {">", 1, O_GT},		/* greater than */
    {"+", 1, O_ADD},		/* addition */
    {"-", 1, O_SUB},		/* subtraction or sign */
    {".", 1, O_CAT},		/* string concatenation */
    {"*", 1, O_MUL},		/* multiplication */
    {"/", 1, O_DIV},		/* division */
    {"%", 1, O_MOD},		/* modulo */
    {"^", 1, O_POW},		/* power */
    {"!", 1, O_NOT},		/* logical NOT */
    {"(", 1, O_BRO},		/* open brace */
    {",", 1, O_COM},		/* comma (argument seperator) */
    {")", 1, O_BRC},		/* closing brace */
    {"==", 2, O_EQ},		/* equal */
    {"!=", 2, O_NE},		/* not equal */
    {"<=", 2, O_LE},		/* less or equal */
    {">=", 2, O_GE}		/* greater or equal */
};


static char *Expression = NULL;
static char *ExprPtr = NULL;
static char *Word = NULL;
static TOKEN Token = T_UNDEF;
static OPERATOR Operator = O_UNDEF;

static VARIABLE Variable[255];
static unsigned int nVariable = 0;

static FUNCTION *Function = NULL;
static unsigned int nFunction = 0;


void DelResult(RESULT * result)
{
    result->type = 0;
    result->size = 0;
    result->number = 0.0;
    if (result->string) {
	free(result->string);
	result->string = NULL;
    }
}


static void FreeResult(RESULT * result)
{
    if (result != NULL) {
	DelResult(result);
	free(result);
    }
}


static RESULT *NewResult(void)
{
    RESULT *result = malloc(sizeof(RESULT));
    if (result == NULL) {
	error("Evaluator: cannot allocate result: out of memory!");
	return NULL;
    }
    result->type = 0;
    result->size = 0;
    result->number = 0.0;
    result->string = NULL;

    return result;
}


RESULT *SetResult(RESULT ** result, const int type, const void *value)
{
    if (*result == NULL) {
	if ((*result = NewResult()) == NULL)
	    return NULL;
    } else if (type == R_NUMBER) {
	DelResult(*result);
    }

    if (type == R_NUMBER) {
	(*result)->type = R_NUMBER;
	(*result)->size = 0;
	(*result)->number = *(double *) value;
	(*result)->string = NULL;
    }

    else if (type == R_STRING) {
	int len = strlen((char *) value);
	(*result)->type = R_STRING;
	(*result)->number = 0.0;
	if ((*result)->string == NULL || len >= (*result)->size) {
	    /* buffer is either empty or too small */
	    if ((*result)->string)
		free((*result)->string);
	    /* allocate memory in multiples of CHUNK_SIZE */
	    (*result)->size = CHUNK_SIZE * ((len + 1) / CHUNK_SIZE + 1);
	    (*result)->string = malloc((*result)->size);
	}
	strcpy((*result)->string, value);
    } else {
	error("Evaluator: internal error: invalid result type %d", type);
	return NULL;
    }

    return *result;
}


RESULT *CopyResult(RESULT ** result, RESULT * value)
{
    if (*result == NULL) {
	if ((*result = NewResult()) == NULL)
	    return NULL;
    }

    (*result)->type = value->type;
    (*result)->number = value->number;

    if (value->string == NULL) {
	(*result)->size = 0;
	if ((*result)->string)
	    free((*result)->string);
	(*result)->string = NULL;
    } else {
	/* is buffer large enough? */
	if ((*result)->string == NULL || value->size > (*result)->size) {
	    if ((*result)->string)
		free((*result)->string);
	    (*result)->size = value->size;
	    (*result)->string = malloc((*result)->size);
	}
	strcpy((*result)->string, value->string);
    }
    return *result;
}


double R2N(RESULT * result)
{
    if (result == NULL) {
	error("Evaluator: internal error: NULL result");
	return 0.0;
    }

    if (result->type == 0) {
	return 0.0;
    }

    if (result->type & R_NUMBER) {
	return result->number;
    }

    if (result->type & R_STRING) {
	result->type |= R_NUMBER;
	result->number = atof(result->string);
	return result->number;
    }

    error("Evaluator: internal error: invalid result type %d", result->type);
    return 0.0;
}


char *R2S(RESULT * result)
{
    if (result == NULL) {
	error("Evaluator: internal error: NULL result");
	return NULL;
    }

    if (result->type == 0) {
	return NULL;
    }

    if (result->type & R_STRING) {
	return result->string;
    }

    if (result->type & R_NUMBER) {
	result->type |= R_STRING;
	if (result->string)
	    free(result->string);
	result->size = CHUNK_SIZE;
	result->string = malloc(result->size);
	snprintf(result->string, result->size, "%g", result->number);
	return result->string;
    }

    error("Evaluator: internal error: invalid result type %d", result->type);
    return NULL;

}


static VARIABLE *FindVariable(const char *name)
{
    unsigned int i;

    for (i = 0; i < nVariable; i++) {
	if (strcmp(name, Variable[i].name) == 0) {
	    return &Variable[i];
	}
    }
    return NULL;
}


int SetVariable(const char *name, RESULT * value)
{
    VARIABLE *V;

    V = FindVariable(name);
    if (V != NULL) {
	CopyResult(&V->value, value);
	return 1;
    }

    if (nVariable >= sizeof(Variable) / sizeof(Variable[0])) {
	error("Evaluator: cannot set variable <%s>: out of slots", name);
	return -1;
    }

    nVariable++;
    Variable[nVariable - 1].name = strdup(name);
    Variable[nVariable - 1].value = NULL;
    CopyResult(&Variable[nVariable - 1].value, value);

    return 0;
}


int SetVariableNumeric(const char *name, const double value)
{
    RESULT result = { 0, 0, 0, NULL };
    RESULT *rp = &result;

    SetResult(&rp, R_NUMBER, &value);

    return SetVariable(name, rp);
}


int SetVariableString(const char *name, const char *value)
{
    RESULT result = { 0, 0, 0, NULL };
    RESULT *rp = &result;

    SetResult(&rp, R_STRING, value);

    return SetVariable(name, rp);
}


void DeleteVariables(void)
{
    unsigned int i;

    for (i = 0; i < nVariable; i++) {
	free(Variable[i].name);
	FreeResult(Variable[i].value);
    }
    nVariable = 0;
}


/* bsearch compare function for functions */
static int LookupFunction(const void *a, const void *b)
{
    char *n = (char *) a;
    FUNCTION *f = (FUNCTION *) b;

    return strcmp(n, f->name);
}


/* qsort compare function for functions */
static int SortFunction(const void *a, const void *b)
{
    FUNCTION *fa = (FUNCTION *) a;
    FUNCTION *fb = (FUNCTION *) b;

    return strcmp(fa->name, fb->name);
}


static FUNCTION *FindFunction(const char *name)
{
    return bsearch(name, Function, nFunction, sizeof(FUNCTION), LookupFunction);
}


int AddFunction(const char *name, const int argc, void (*func) ())
{
    nFunction++;
    Function = realloc(Function, nFunction * sizeof(FUNCTION));
    Function[nFunction - 1].name = strdup(name);
    Function[nFunction - 1].argc = argc;
    Function[nFunction - 1].func = func;

    qsort(Function, nFunction, sizeof(FUNCTION), SortFunction);

    return 0;
}


void DeleteFunctions(void)
{
    unsigned int i;

    for (i = 0; i < nFunction; i++) {
	free(Function[i].name);
    }
    free(Function);
    Function = NULL;
    nFunction = 0;
}


#define is_space(c)  ((c) == ' ' || (c) == '\t')
#define is_digit(c)  ((c) >= '0' && (c) <= '9')
#define is_alpha(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z') || ((c) == '_'))
#define is_alnum(c) (is_alpha(c) || is_digit(c))

static void Parse(void)
{
    Token = T_UNDEF;
    Operator = O_UNDEF;

    if (Word) {
	free(Word);
	Word = NULL;
    }

    /* NULL expression? */
    if (ExprPtr == NULL) {
	Word = strdup("");
	return;
    }

    /* skip leading whitespace */
    while (is_space(*ExprPtr))
	ExprPtr++;

    /* names */
    if (is_alpha(*ExprPtr)) {
	char *start = ExprPtr;
	while (is_alnum(*ExprPtr))
	    ExprPtr++;
	if (*ExprPtr == ':' && *(ExprPtr + 1) == ':' && is_alpha(*(ExprPtr + 2))) {
	    ExprPtr += 3;
	    while (is_alnum(*ExprPtr))
		ExprPtr++;
	}
	Word = strndup(start, ExprPtr - start);
	Token = T_NAME;
    }

    /* numbers */
    else if (is_digit(*ExprPtr) || (*ExprPtr == '.' && is_digit(*(ExprPtr + 1)))) {
	char *start = ExprPtr;
	while (is_digit(*ExprPtr))
	    ExprPtr++;
	if (*ExprPtr == '.') {
	    ExprPtr++;
	    while (is_digit(*ExprPtr))
		ExprPtr++;
	}
	Word = strndup(start, ExprPtr - start);
	Token = T_NUMBER;
    }

    /* strings */
    else if (*ExprPtr == '\'') {
	char *start = ++ExprPtr;
	while (*ExprPtr != '\0' && *ExprPtr != '\'')
	    ExprPtr++;
	Word = strndup(start, ExprPtr - start);
	Token = T_STRING;
	if (*ExprPtr == '\'')
	    ExprPtr++;
    }

    /* operators */
    else {
	int i;
	for (i = sizeof(Pattern) / sizeof(Pattern[0]) - 1; i >= 0; i--) {
	    int len = Pattern[i].len;
	    if (strncmp(ExprPtr, Pattern[i].pattern, Pattern[i].len) == 0) {
		Word = strndup(ExprPtr, len);
		Token = T_OPERATOR;
		Operator = Pattern[i].op;
		ExprPtr += len;
		break;
	    }
	}
    }

    /* syntax check */
    if (Token == T_UNDEF && *ExprPtr != '\0') {
	error("Evaluator: parse error in <%s>: garbage <%s>", Expression, ExprPtr);
    }

    /* skip trailing whitespace */
    while (is_space(*ExprPtr))
	ExprPtr++;

    /* empty token */
    if (Word == NULL)
	Word = strdup("");
}


static NODE *NewNode(NODE * Child)
{
    NODE *N;

    N = malloc(sizeof(NODE));
    if (N == NULL)
	return NULL;

    memset(N, 0, sizeof(NODE));
    N->Token = Token;
    N->Operator = Operator;

    if (Child != NULL) {
	N->Children = 1;
	N->Child = malloc(sizeof(NODE *));
	N->Child[0] = Child;
    }

    return N;

}


static NODE *JunkNode(void)
{
    NODE *Junk;

    Junk = NewNode(NULL);
    Junk->Token = T_STRING;
    SetResult(&Junk->Result, R_STRING, "");

    return Junk;
}


static void LinkNode(NODE * Root, NODE * Child)
{

    if (Child == NULL)
	return;

    Root->Children++;
    Root->Child = realloc(Root->Child, Root->Children * sizeof(NODE *));
    if (Root->Child == NULL)
	return;
    Root->Child[Root->Children - 1] = Child;
}


/* forward declaration */
static NODE *Level01(void);


/* literal numbers, variables, functions */
static NODE *Level12(void)
{
    NODE *Root = NULL;

    if (Token == T_OPERATOR && Operator == O_BRO) {
	Parse();
	Root = Level01();
	if (Token != T_OPERATOR || Operator != O_BRC) {
	    error("Evaluator: unbalanced parentheses in <%s>", Expression);
	    LinkNode(Root, JunkNode());
	}
    }

    else if (Token == T_NUMBER) {
	double value = atof(Word);
	Root = NewNode(NULL);
	SetResult(&Root->Result, R_NUMBER, &value);
    }

    else if (Token == T_STRING) {
	Root = NewNode(NULL);
	SetResult(&Root->Result, R_STRING, Word);
    }

    else if (Token == T_NAME) {

	/* look-ahead for opening brace */
	if (*ExprPtr == '(') {
	    int argc = 0;
	    Root = NewNode(NULL);
	    Root->Token = T_FUNCTION;
	    Root->Result = NewResult();
	    Root->Function = FindFunction(Word);
	    if (Root->Function == NULL) {
		error("Evaluator: unknown function '%s' in <%s>", Word, Expression);
		Root->Token = T_STRING;
		SetResult(&Root->Result, R_STRING, "");
	    }

	    /* opening brace */
	    Parse();
	    do {
		Parse();	/* read argument */
		if (Token == T_OPERATOR && Operator == O_BRC) {
		    break;
		} else if (Token == T_OPERATOR && Operator == O_COM) {
		    error("Evaluator: empty argument in <%s>", Expression);
		    LinkNode(Root, JunkNode());
		} else {
		    LinkNode(Root, Level01());
		}
		argc++;
	    } while (Token == T_OPERATOR && Operator == O_COM);

	    /* check for closing brace */
	    if (Token != T_OPERATOR || Operator != O_BRC) {
		error("Evaluator: missing closing brace in <%s>", Expression);
	    }

	    /* check number of arguments */
	    if (Root->Function != NULL && Root->Function->argc >= 0 && Root->Function->argc != argc) {
		error("Evaluator: wrong number of arguments in <%s>", Expression);
		while (argc < Root->Function->argc) {
		    LinkNode(Root, JunkNode());
		    argc++;
		}
	    }

	} else {
	    Root = NewNode(NULL);
	    Root->Token = T_VARIABLE;
	    Root->Result = NewResult();
	    Root->Variable = FindVariable(Word);
	    if (Root->Variable == NULL) {
		SetVariableString(Word, "");
		Root->Variable = FindVariable(Word);
	    }
	}
    }

    else {
	error("Evaluator: syntax error in <%s>: <%s>", Expression, Word);
	Root = NewNode(NULL);
	Root->Token = T_STRING;
	SetResult(&Root->Result, R_STRING, "");
    }

    Parse();
    return Root;

}


/* unary + or - signs or logical 'not' */
static NODE *Level11(void)
{
    NODE *Root;
    TOKEN sign = T_UNDEF;

    if (Token == T_OPERATOR && (Operator == O_ADD || Operator == O_SUB || Operator == O_NOT)) {
	sign = Operator;
	if (sign == O_SUB)
	    sign = O_SGN;
	Parse();
    }

    Root = Level12();

    if (sign == O_SGN || sign == O_NOT) {
	Root = NewNode(Root);
	Root->Token = T_OPERATOR;
	Root->Operator = sign;
    }

    return Root;
}


/* x^y */
static NODE *Level10(void)
{
    NODE *Root;

    Root = Level11();

    while (Token == T_OPERATOR && Operator == O_POW) {
	Root = NewNode(Root);
	Parse();
	LinkNode(Root, Level11());
    }

    return Root;
}


/* multiplication, division, modulo */
static NODE *Level09(void)
{
    NODE *Root;

    Root = Level10();

    while (Token == T_OPERATOR && (Operator == O_MUL || Operator == O_DIV || Operator == O_MOD)) {
	Root = NewNode(Root);
	Parse();
	LinkNode(Root, Level10());
    }

    return Root;
}


/* addition, subtraction, string concatenation */
static NODE *Level08(void)
{
    NODE *Root;

    Root = Level09();

    while (Token == T_OPERATOR && (Operator == O_ADD || Operator == O_SUB || Operator == O_CAT)) {
	Root = NewNode(Root);
	Parse();
	LinkNode(Root, Level09());
    }

    return Root;
}


/* relational operators */
static NODE *Level07(void)
{
    NODE *Root;

    Root = Level08();

    while (Token == T_OPERATOR && (Operator == O_GT || Operator == O_GE || Operator == O_LT || Operator == O_LE)) {
	Root = NewNode(Root);
	Parse();
	LinkNode(Root, Level08());
    }

    return Root;
}


/* equal, not equal */
static NODE *Level06(void)
{
    NODE *Root;

    Root = Level07();

    while (Token == T_OPERATOR && (Operator == O_EQ || Operator == O_NE)) {
	Root = NewNode(Root);
	Parse();
	LinkNode(Root, Level07());
    }

    return Root;
}

/* logical 'and' */
static NODE *Level05(void)
{
    NODE *Root;

    Root = Level06();

    while (Token == T_OPERATOR && Operator == O_AND) {
	Root = NewNode(Root);
	Parse();
	LinkNode(Root, Level06());
    }

    return Root;
}


/* logical 'or' */
static NODE *Level04(void)
{
    NODE *Root;

    Root = Level05();

    while (Token == T_OPERATOR && Operator == O_OR) {
	Root = NewNode(Root);
	Parse();
	LinkNode(Root, Level05());
    }

    return Root;
}


/* conditional expression a?b:c */
static NODE *Level03(void)
{
    NODE *Root;

    Root = Level04();

    if (Token == T_OPERATOR && Operator == O_CND) {
	Root = NewNode(Root);
	Parse();
	LinkNode(Root, Level04());
	if (Token == T_OPERATOR && Operator == O_COL) {
	    Parse();
	    LinkNode(Root, Level04());
	} else {
	    error("Evaluator: syntax error in <%s>: expecting ':' got '%s'", Expression, Word);
	    LinkNode(Root, JunkNode());
	}
    }

    return Root;
}


/* variable assignments */
static NODE *Level02(void)
{
    NODE *Root;

    /* we have to do a look-ahead if it's really an assignment */
    if ((Token == T_NAME) && (*ExprPtr == '=') && (*(ExprPtr + 1) != '=')) {
	char *name = strdup(Word);
	VARIABLE *V = FindVariable(name);
	if (V == NULL) {
	    SetVariableString(name, "");
	    V = FindVariable(name);
	}
	Parse();
	Root = NewNode(NULL);
	Root->Variable = V;
	Parse();
	LinkNode(Root, Level03());
	free(name);
    } else {
	Root = Level03();
    }

    return Root;
}


/* expression lists */
static NODE *Level01(void)
{
    NODE *Root;

    Root = Level02();

    while (Token == T_OPERATOR && Operator == O_LST) {
	Root = NewNode(Root);
	Parse();
	LinkNode(Root, Level02());
    }

    return Root;
}


static int EvalTree(NODE * Root)
{
    int i;
    int argc;
    int type = -1;
    double number = 0.0;
    double dummy;
    int freeme = 0;
    char *string = NULL;
    char *s1, *s2;
    RESULT *param[10];

    for (i = 0; i < Root->Children; i++) {
	EvalTree(Root->Child[i]);
    }

    switch (Root->Token) {

    case T_NUMBER:
    case T_STRING:
	/* Root->Result already contains the value */
	return 0;

    case T_VARIABLE:
	CopyResult(&Root->Result, Root->Variable->value);
	return 0;

    case T_FUNCTION:
	DelResult(Root->Result);
	/* prepare parameter list */
	argc = Root->Children;
	if (argc > 10)
	    argc = 10;
	for (i = 0; i < argc; i++) {
	    param[i] = Root->Child[i]->Result;
	}
	if (Root->Function->argc < 0) {
	    /* Function with variable argument list:  */
	    /* pass number of arguments as first parameter */
	    Root->Function->func(Root->Result, argc, &param);
	} else {
	    Root->Function->func(Root->Result, param[0], param[1], param[2], param[3], param[4], param[5], param[6],
				 param[7], param[8], param[9]);
	}
	return 0;

    case T_OPERATOR:
	switch (Root->Operator) {

	case O_LST:		/* expression list: result is last expression */
	    i = Root->Children - 1;
	    type = Root->Child[i]->Result->type;
	    number = Root->Child[i]->Result->number;
	    string = Root->Child[i]->Result->string;
	    break;

	case O_SET:		/* variable assignment */
	    CopyResult(&Root->Variable->value, Root->Child[0]->Result);
	    type = Root->Child[0]->Result->type;
	    number = Root->Child[0]->Result->number;
	    string = Root->Child[0]->Result->string;
	    break;

	case O_CND:		/* conditional expression */
	    i = 1 + (R2N(Root->Child[0]->Result) == 0.0);
	    type = Root->Child[i]->Result->type;
	    number = Root->Child[i]->Result->number;
	    string = Root->Child[i]->Result->string;
	    break;

	case O_OR:		/* logical OR */
	    type = R_NUMBER;
	    number = ((R2N(Root->Child[0]->Result) != 0.0) || (R2N(Root->Child[1]->Result) != 0.0));
	    break;

	case O_AND:		/* logical AND */
	    type = R_NUMBER;
	    number = ((R2N(Root->Child[0]->Result) != 0.0) && (R2N(Root->Child[1]->Result) != 0.0));
	    break;

	case O_EQ:		/* numeric equal */
	    type = R_NUMBER;
	    number = (R2N(Root->Child[0]->Result) == R2N(Root->Child[1]->Result));
	    break;

	case O_NE:		/* numeric not equal */
	    type = R_NUMBER;
	    number = (R2N(Root->Child[0]->Result) != R2N(Root->Child[1]->Result));
	    break;

	case O_LT:		/* numeric less than */
	    type = R_NUMBER;
	    number = (R2N(Root->Child[0]->Result) < R2N(Root->Child[1]->Result));
	    break;

	case O_LE:		/* numeric less equal */
	    type = R_NUMBER;
	    number = (R2N(Root->Child[0]->Result) <= R2N(Root->Child[1]->Result));
	    break;

	case O_GT:		/* numeric greater than */
	    type = R_NUMBER;
	    number = (R2N(Root->Child[0]->Result) > R2N(Root->Child[1]->Result));
	    break;

	case O_GE:		/* numeric greater equal */
	    type = R_NUMBER;
	    number = (R2N(Root->Child[0]->Result) >= R2N(Root->Child[1]->Result));
	    break;

	case O_ADD:		/* addition */
	    type = R_NUMBER;
	    number = R2N(Root->Child[0]->Result) + R2N(Root->Child[1]->Result);
	    break;

	case O_SUB:		/* subtraction */
	    type = R_NUMBER;
	    number = R2N(Root->Child[0]->Result) - R2N(Root->Child[1]->Result);
	    break;

	case O_SGN:		/* sign */
	    type = R_NUMBER;
	    number = -R2N(Root->Child[0]->Result);
	    break;

	case O_CAT:		/* string concatenation */
	    type = R_STRING;
	    s1 = R2S(Root->Child[0]->Result);
	    s2 = R2S(Root->Child[1]->Result);
	    string = malloc(strlen(s1) + strlen(s2) + 1);
	    strcpy(string, s1);
	    strcat(string, s2);
	    freeme = 1;
	    break;

	case O_MUL:		/* multiplication */
	    type = R_NUMBER;
	    number = R2N(Root->Child[0]->Result) * R2N(Root->Child[1]->Result);
	    break;

	case O_DIV:		/* division */
	    type = R_NUMBER;
	    dummy = R2N(Root->Child[1]->Result);
	    if (dummy == 0) {
		error("Evaluator: warning: division by zero");
		number = 0.0;
	    } else {
		number = R2N(Root->Child[0]->Result) / R2N(Root->Child[1]->Result);
	    }
	    break;

	case O_MOD:		/* modulo */
	    type = R_NUMBER;
	    dummy = R2N(Root->Child[1]->Result);
	    if (dummy == 0) {
		error("Evaluator: warning: division by zero");
		number = 0.0;
	    } else {
		number = fmod(R2N(Root->Child[0]->Result), R2N(Root->Child[1]->Result));
	    }
	    break;

	case O_POW:		/* x^y */
	    type = R_NUMBER;
	    number = pow(R2N(Root->Child[0]->Result), R2N(Root->Child[1]->Result));
	    break;

	case O_NOT:		/* logical NOT */
	    type = R_NUMBER;
	    number = (R2N(Root->Child[0]->Result) == 0.0);
	    break;

	default:
	    error("Evaluator: internal error: unhandled operator <%d>", Root->Operator);
	    SetResult(&Root->Result, R_STRING, "");
	    return -1;
	}

	if (type == R_NUMBER) {
	    SetResult(&Root->Result, R_NUMBER, &number);
	    return 0;
	}
	if (type == R_STRING) {
	    SetResult(&Root->Result, R_STRING, string);
	    if (freeme)
		free(string);
	    return 0;
	}
	error("Evaluator: internal error: unhandled type <%d>", type);
	SetResult(&Root->Result, R_STRING, "");
	return -1;

    default:
	error("Evaluator: internal error: unhandled token <%d>", Root->Token);
	SetResult(&Root->Result, R_STRING, "");
	return -1;

    }

    return 0;
}


int Compile(const char *expression, void **tree)
{
    NODE *Root;

    *tree = NULL;

    Expression = (char *) expression;
    ExprPtr = Expression;

    Parse();
    if (*Word == '\0') {
	/* error ("Evaluator: empty expression <%s>", Expression); */
	free(Word);
	Word = NULL;
	return -1;
    }

    Root = Level01();

    if (*Word != '\0') {
	error("Evaluator: syntax error in <%s>: garbage <%s>", Expression, Word);
	free(Word);
	Word = NULL;
	return -1;
    }

    free(Word);
    Word = NULL;

    *(NODE **) tree = Root;

    return 0;
}


int Eval(void *tree, RESULT * result)
{
    int ret;
    NODE *Tree = (NODE *) tree;

    DelResult(result);

    if (Tree == NULL) {
	SetResult(&result, R_STRING, "");
	return 0;
    }

    ret = EvalTree(Tree);

    result->type = Tree->Result->type;
    result->size = Tree->Result->size;
    result->number = Tree->Result->number;
    if (result->size > 0) {
	result->string = malloc(result->size);
	if (Tree->Result->string != NULL) {
	    strcpy(result->string, Tree->Result->string);
	} else
	    result->string[0] = '\0';
    } else {
	result->string = NULL;
    }

    return ret;
}


void DelTree(void *tree)
{
    int i;
    NODE *Tree = (NODE *) tree;

    if (Tree == NULL)
	return;

    for (i = 0; i < Tree->Children; i++) {
	DelTree(Tree->Child[i]);
    }

    if (Tree->Result)
	FreeResult(Tree->Result);
    free(Tree);
}
