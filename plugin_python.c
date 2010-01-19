/* $Id$
 * $URL$
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
 */

/* 
 * exported functions:
 *
 * int plugin_init_python (void)
 *  adds a python interpreter
 * 
 */

#include "config.h"
#include <Python.h>
#include "debug.h"
#include "plugin.h"

/* 
 * Executes a python function specified by function name and module.
 *
 * This method is more or less a copy of an example found in the python 
 * documentation. Kudos goes to Guido van Rossum and Fred L. Drake.
 * 
 * Fixme: this function should be able to accept and receive any types 
 * of arguments supported by the evaluator. Right now only strings are accepted.
 */

static void pyt_exec_str(RESULT * result, const char *module, const char *function, int argc, const char *argv[])
{

    PyObject *pName, *pModule, *pDict, *pFunc;
    PyObject *pArgs, *pValue;
    const char *rv = NULL;
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
		    error("Cannot convert argument \"%s\" to python format", argv[i]);
		    SetResult(&result, R_STRING, "");
		    return;
		}
		/* pValue reference stolen here: */
		PyTuple_SetItem(pArgs, i, pValue);
	    }
	    pValue = PyObject_CallObject(pFunc, pArgs);
	    Py_DECREF(pArgs);
	    if (pValue != NULL) {
		rv = PyString_AsString(pValue);
		SetResult(&result, R_STRING, rv);
		Py_DECREF(pValue);
		/* rv is now a 'dangling reference' */
		return;
	    } else {
		Py_DECREF(pModule);
		error("Python call failed (\"%s.%s\")", module, function);
		/* print traceback on stderr */
		PyErr_PrintEx(0);
		SetResult(&result, R_STRING, "");
		return;
	    }
	    /* pDict and pFunc are borrowed and must not be Py_DECREF-ed */
	} else {
	    error("Can not find python function \"%s.%s\"", module, function);
	}
	Py_DECREF(pModule);
    } else {
	error("Failed to load python module \"%s\"", module);
    }
    SetResult(&result, R_STRING, "");
    return;
}

static int python_cleanup_responsibility = 0;

static void my_exec(RESULT * result, RESULT * module, RESULT * function, RESULT * arg)
{
    /* Fixme: a plugin should be able to accept any number of arguments, don't know how
       to code that (yet) */
    const char *args[] = { R2S(arg) };
    pyt_exec_str(result, R2S(module), R2S(function), 1, args);
}

int plugin_init_python(void)
{
    if (!Py_IsInitialized()) {
	Py_Initialize();
	python_cleanup_responsibility = 1;
    }
    AddFunction("python::exec", 3, my_exec);
    return 0;
}

void plugin_exit_python(void)
{
    /* Make sure NOT to call Py_Finalize() When (and if) the entire lcd4linux process 
     * is started from inside python
     */
    if (python_cleanup_responsibility) {
	python_cleanup_responsibility = 0;
	Py_Finalize();
    }
}
