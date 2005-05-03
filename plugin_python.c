/* $Id: plugin_python.c,v 1.2 2005/05/03 11:13:24 reinelt Exp $
 *
 * Python plugin
 *
 * Copyright 2005 Dan Fritz  
 * Copyright 2005 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 *
 * $Log: plugin_python.c,v $
 * Revision 1.2  2005/05/03 11:13:24  reinelt
 * rearranged autoconf a bit,
 * libX11 will be linked only if really needed (i.e. when the X11 driver has been selected)
 * plugin_python filled with life
 *
 * Revision 1.1  2005/05/02 10:29:20  reinelt
 * preparations for python bindings and python plugin
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_python (void)
 *  adds a python interpreter
 *
 */

#include <Python.h>

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include "debug.h"
#include "plugin.h"
#include "hash.h"

/* 
 * Executes a python function specified by function name and module.
 *
 * This method is more or less a copy of an example found in the python 
 * documentation. Kudos goes to Guido van Rossum and Fred L. Drake.
 * 
 * Returns a char* directly from PyString_AsString() !!! DO NOT DEALLOCATE !!!
 */
static const char* 
pyt_exec_str(const char* module, const char* function, int argc, const char* argv[]) {

  PyObject *pName, *pModule, *pDict, *pFunc;
  PyObject *pArgs, *pValue;
  const char * rv = NULL;
  int i;

  pName = PyString_FromString(module);
  /* Error checking of pName left out */

  pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  if (pModule != NULL) {
    pDict = PyModule_GetDict(pModule);
    /* pDict is a borrowed reference */

    pFunc = PyDict_GetItemString(pDict, function);
    /* pFun: Borrowed reference */

    if (pFunc && PyCallable_Check(pFunc)) {
      pArgs = PyTuple_New(argc);
      for (i = 0; i < argc; ++i) {
        pValue = PyString_FromString(argv[i]);
        if (!pValue) {
          Py_DECREF(pArgs);
          Py_DECREF(pModule);
          fprintf(stderr, "Cannot convert argument %s\n", argv[i]);
          return NULL;
        }
        /* pValue reference stolen here: */
        PyTuple_SetItem(pArgs, i, pValue);
      }
      pValue = PyObject_CallObject(pFunc, pArgs);
      Py_DECREF(pArgs);
      if (pValue != NULL) {
        rv = PyString_AsString(pValue);
        //printf("Result of call: %s\n", rv);
        Py_DECREF(pValue);
        return rv;
      }
      else {
        Py_DECREF(pModule);
        PyErr_Print();
        fprintf(stderr,"Call failed\n");
        return NULL;
      }
      /* pDict and pFunc are borrowed and must not be Py_DECREF-ed */
    }
    else {
      if (PyErr_Occurred())
        PyErr_Print();
      fprintf(stderr, "Cannot find function \"%s\"\n", argv[2]);
    }
    Py_DECREF(pModule);
  }
  else {
    PyErr_Print();
    fprintf(stderr, "Failed to load \"%s\"\n", argv[1]);
    return NULL;
  }
  return NULL;
}

static int python_cleanup_responsibility = 0;
    
static void my_exec (RESULT *result, RESULT *module, RESULT *function, RESULT *arg )
{ 
  const char* args[] = {R2S(arg)};
  const char* value = pyt_exec_str(R2S(module),R2S(function),1,args);
  SetResult(&result, R_STRING, value); 
}

int plugin_init_python (void)
{
  if (!Py_IsInitialized()) {
    Py_Initialize();
    python_cleanup_responsibility = 1;
  }
  AddFunction ("python::exec", 3, my_exec);
  return 0;
}

void plugin_exit_python (void) 
{
  /* Make sure NOT to call Py_Finalize() When (and if) the entire lcd4linux process 
   * is started from inside python
   */
  if (python_cleanup_responsibility) {
    python_cleanup_responsibility = 0;
    Py_Finalize();
  }
}
