/* $Id$
 * $URL$
 *
 * expression evaluation
 *
 * Copyright (C) 2003 Michael Reinelt <reinelt@eunet.at>
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


#ifndef _EVALUATOR_H_
#define _EVALUATOR_H_


/* RESULT bitmask */
#define R_NUMBER 1
#define R_STRING 2

typedef struct {
    int type;
    int size;
    double number;
    char *string;
} RESULT;

int SetVariable(const char *name, RESULT * value);
int SetVariableNumeric(const char *name, const double value);
int SetVariableString(const char *name, const char *value);

int AddFunction(const char *name, const int argc, void (*func) ());

void DeleteVariables(void);
void DeleteFunctions(void);

void DelResult(RESULT * result);
RESULT *SetResult(RESULT ** result, const int type, const void *value);

double R2N(RESULT * result);
char *R2S(RESULT * result);

int Compile(const char *expression, void **tree);
int Eval(void *tree, RESULT * result);
void DelTree(void *tree);

#endif
