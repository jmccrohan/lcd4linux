/* $Id: drv_generic_gpio.c,v 1.1 2005/12/18 16:18:36 reinelt Exp $
 *
 * generic driver helper for GPO's
 *
 * Copyright (C) 2005 Michael Reinelt <reinelt@eunet.at>
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
 *
 * $Log: drv_generic_gpio.c,v $
 * Revision 1.1  2005/12/18 16:18:36  reinelt
 * GPO's added again
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
 * void (*drv_generic_gpio_real_set) (const int num, const int val);
 *   sets GPO num to val
 *
 * int (*drv_generic_gpio_real_get) (const int num);
 *   reads GPI num
 *
 *
 * exported fuctions:
 *
 * int drv_generic_gpio_init(const char *section, const char *driver);
 *   initializes the generic GPIO driver
 *
 * int drv_generic_gpio_clear(void);
 *   resets all GPO's
 *
 * int drv_generic_gpio_draw(WIDGET * W);
 *   'draws' GPO widget
 *   calls drv_generic_gpio_real_set()
 * 
 * int drv_generic_gpio_quit(void);
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

#define MAX_GPOS 32


static char *Section = NULL;
static char *Driver = NULL;

static int GPO[MAX_GPOS];

int GPOS = 0;
int GPIS = 0;


int drv_generic_gpio_init(const char *section, const char *driver)
{
    WIDGET_CLASS wc;

    Section = (char *) section;
    Driver = (char *) driver;

    if (GPIS <= 0 && GPOS <= 0) {
	error("%s: Huh? gpio_init(GPIS=%d, GPOS=%d)", Driver, GPIS, GPOS);
    }

    /* reset all GPO's */
    drv_generic_gpio_clear();

    /* register gpo widget */
    wc = Widget_GPO;
    wc.draw = drv_generic_gpio_draw;
    widget_register(&wc);


    /* register plugins */
    /* Fixme */

    return 0;
}


int drv_generic_gpio_clear(void)
{
    int i;

    /* init GPO bufferr */
    for (i = 0; i < MAX_GPOS; i++) {
	GPO[i] = 0;
    }

    /* really clear GPO's */
    for (i = 0; i < GPOS; i++) {
	drv_generic_gpio_real_set(i, 0);
    }

    return 0;
}


int drv_generic_gpio_draw(WIDGET * W)
{
    WIDGET_GPO *gpo = W->data;
    int num = gpo->num;
    int val = gpo->val;

    if (num < 0 || num >= GPOS) {
	error("%s: gpio_draw(%d): GPO out of range (0..%d)", Driver, num + 1, GPOS);
	return -1;
    }

    if (GPO[num] != val) {
	drv_generic_gpio_real_set(num, val);
	GPO[num] = val;
    }

    return 0;
}


int drv_generic_gpio_quit(void)
{
    return 0;
}
