/* $Id$
 * $URL$
 *
 * generic driver helper for GPO's
 *
 * Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2005 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 *
 * exported variables:
 *
 * extern int GPIS, GPOS;    number of Inputs and Outputs
 *
 *
 * these functions must be implemented by the real driver:
 *
 * int (*drv_generic_gpio_real_set) (const int num, const int val);
 *   sets GPO num to val, returns the actal value
 *
 * int (*drv_generic_gpio_real_get) (const int num);
 *   reads GPI num
 *
 *
 * exported fuctions:
 *
 * int drv_generic_gpio_init(const char *section, const char *driver)
 *   initializes the generic GPIO driver
 *
 * int drv_generic_gpio_clear(void);
 *   resets all GPO's
 *
 * int drv_generic_gpio_get (const int num)
 *   returns value og GPI #num
 *
 * int drv_generic_gpio_draw(WIDGET * W)
 *   'draws' GPO widget
 *   calls drv_generic_gpio_real_set()
 * 
 * int drv_generic_gpio_quit(void)
 *   closes the generic GPIO driver
 *
 */

#include "config.h"

#include <stdio.h>

#include "debug.h"
#include "plugin.h"
#include "widget.h"
#include "widget_gpo.h"

#include "drv_generic_gpio.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#define MAX_GPIS 32
#define MAX_GPOS 32


static char *Section = NULL;
static char *Driver = NULL;

static int GPI[MAX_GPIS];
static int GPO[MAX_GPOS];

int GPOS = 0;
int GPIS = 0;

int (*drv_generic_gpio_real_set) () = NULL;
int (*drv_generic_gpio_real_get) () = NULL;


static void drv_generic_gpio_plugin_gpi(RESULT * result, RESULT * arg1)
{
    int num;
    double val;

    num = R2N(arg1);

    if (num <= 0 || num > GPIS) {
	error("%s::GPI(%d): GPI out of range (1..%d)", Driver, num, GPIS);
	SetResult(&result, R_STRING, "");
	return;
    }

    val = drv_generic_gpio_get(num - 1);
    SetResult(&result, R_NUMBER, &val);
}


static void drv_generic_gpio_plugin_gpo(RESULT * result, const int argc, RESULT * argv[])
{
    int num, val;
    double gpo;

    switch (argc) {
    case 1:
	num = R2N(argv[0]);
	if (num <= 0 || num > GPOS) {
	    error("%s::GPO(%d): GPO out of range (1..%d)", Driver, num, GPOS);
	    SetResult(&result, R_STRING, "");
	    return;
	}
	gpo = GPO[num - 1];
	SetResult(&result, R_NUMBER, &gpo);
	break;
    case 2:
	num = R2N(argv[0]);
	val = R2N(argv[1]);
	if (num <= 0 || num > GPOS) {
	    error("%s::GPO(%d): GPO out of range (1..%d)", Driver, num, GPOS);
	    SetResult(&result, R_STRING, "");
	    return;
	}
	if (GPO[num - 1] != val) {
	    if (drv_generic_gpio_real_set)
		GPO[num - 1] = drv_generic_gpio_real_set(num - 1, val);
	}
	gpo = GPO[num - 1];
	SetResult(&result, R_NUMBER, &gpo);
	break;
    default:
	error("%s::GPO(): wrong number of parameters", Driver);
	SetResult(&result, R_STRING, "");
    }
}


int drv_generic_gpio_init(const char *section, const char *driver)
{
    WIDGET_CLASS wc;

    Section = (char *) section;
    Driver = (char *) driver;

    info("%s: using %d GPI's and %d GPO's", Driver, GPIS, GPOS);

    /* reset all GPO's */
    drv_generic_gpio_clear();

    /* register gpo widget */
    wc = Widget_GPO;
    wc.draw = drv_generic_gpio_draw;
    widget_register(&wc);

    /* register plugins */
    AddFunction("LCD::GPI", 1, drv_generic_gpio_plugin_gpi);
    AddFunction("LCD::GPO", -1, drv_generic_gpio_plugin_gpo);

    return 0;
}


int drv_generic_gpio_clear(void)
{
    int i;

    /* clear GPI buffer */
    for (i = 0; i < MAX_GPIS; i++) {
	GPI[i] = 0;
    }

    /* clear GPO buffer */
    for (i = 0; i < MAX_GPOS; i++) {
	GPO[i] = 0;
    }

    /* really clear GPO's */
    for (i = 0; i < GPOS; i++) {
	if (drv_generic_gpio_real_set)
	    GPO[i] = drv_generic_gpio_real_set(i, 0);

    }

    return 0;
}


int drv_generic_gpio_get(const int num)
{
    int val = 0;

    if (num < 0 || num >= GPIS) {
	error("%s: gpio_get(%d): GPI out of range (0..%d)", Driver, num + 1, GPIS);
	return -1;
    }

    if (drv_generic_gpio_real_get)
	val = drv_generic_gpio_real_get(num);

    GPI[num] = val;

    return val;
}


int drv_generic_gpio_draw(WIDGET * W)
{
    WIDGET_GPO *gpo = W->data;
    int num, val;

    num = W->row;
    val = P2N(&gpo->expression);

    if (num < 0 || num >= GPOS) {
	error("%s: gpio_draw(%d): GPO out of range (0..%d)", Driver, num + 1, GPOS);
	return -1;
    }

    if (GPO[num] != val) {
	if (drv_generic_gpio_real_set)
	    GPO[num] = drv_generic_gpio_real_set(num, val);
    }

    return 0;
}


int drv_generic_gpio_quit(void)
{
    info("%s: shutting down GPIO driver.", Driver);
    drv_generic_gpio_clear();
    return 0;
}
