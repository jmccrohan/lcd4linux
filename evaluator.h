/* $Id: evaluator.h,v 1.2 2003/10/11 06:01:53 reinelt Exp $
 *
 * expression evaluation
 *
 * based on EE (Expression Evaluator) which is 
 * (c) 1992 Mark Morley <morley@Camosun.BC.CA>
 * 
 * heavily modified 2003 by Michael Reinelt <reinelt@eunet.at>
 *
 * FIXME: GPL or not GPL????
 *
 * $Log: evaluator.h,v $
 * Revision 1.2  2003/10/11 06:01:53  reinelt
 *
 * renamed expression.{c,h} to client.{c,h}
 * added config file client
 * new functions 'AddNumericVariable()' and 'AddStringVariable()'
 * new parameter '-i' for interactive mode
 *
 * Revision 1.1  2003/10/06 04:34:06  reinelt
 * expression evaluator added
 *
 */


/***************************************************************************
 **                                                                       **
 ** EE.C         Expression Evaluator                                     **
 **                                                                       **
 ** AUTHOR:      Mark Morley                                              **
 ** COPYRIGHT:   (c) 1992 by Mark Morley                                  **
 ** DATE:        December 1991                                            **
 ** HISTORY:     Jan 1992 - Made it squash all command line arguments     **
 **                         into one big long string.                     **
 **                       - It now can set/get VMS symbols as if they     **
 **                         were variables.                               **
 **                       - Changed max variable name length from 5 to 15 **
 **              Jun 1992 - Updated comments and docs                     **
 **                                                                       **
 ** You are free to incorporate this code into your own works, even if it **
 ** is a commercial application.  However, you may not charge anyone else **
 ** for the use of this code!  If you intend to distribute your code,     **
 ** I'd appreciate it if you left this message intact.  I'd like to       **
 ** receive credit wherever it is appropriate.  Thanks!                   **
 **                                                                       **
 ** I don't promise that this code does what you think it does...         **
 **                                                                       **
 ** Please mail any bug reports/fixes/enhancments to me at:               **
 **      morley@camosun.bc.ca                                             **
 ** or                                                                    **
 **      Mark Morley                                                      **
 **      3889 Mildred Street                                              **
 **      Victoria, BC  Canada                                             **
 **      V8Z 7G1                                                          **
 **      (604) 479-7861                                                   **
 **                                                                       **
 ***************************************************************************/


#ifndef _EVALUATOR_H_
#define _EVALUATOR_H_


// RESULT bitmask
#define R_NUMBER 1
#define R_STRING 2

typedef struct {
  int type;
  double number;
  char  *string;
} RESULT;


// error codes
#define E_OK      0 /* Successful evaluation */
#define E_SYNTAX  1 /* Syntax error */
#define E_UNBALAN 2 /* Unbalanced parenthesis */
#define E_DIVZERO 3 /* Attempted division by zero */
#define E_UNKNOWN 4 /* Reference to unknown variable */
#define E_BADFUNC 5 /* Unrecognised function */
#define E_NUMARGS 6 /* Wrong number of arguments to function */
#define E_NOARG   7 /* Missing an argument to a function */
#define E_EMPTY   8 /* Empty expression */


int AddNumericVariable (char *name, double value);
int AddStringVariable  (char *name, char *value);
int AddFunction        (char *name, int args, void (*func)());

RESULT* SetResult (RESULT **result, int type, void *value);

double R2N (RESULT *result);
char*  R2S (RESULT *result);

int Eval (char* expression, RESULT *result);

#endif
