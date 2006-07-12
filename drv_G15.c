/* $Id: drv_G15.c,v 1.8 2006/07/12 20:47:51 reinelt Exp $
 *
 * Driver for Logitech G-15 keyboard LCD screen
 *
 * Copyright (C) 2006 Dave Ingram <dave@partis-project.net>
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
 * $Log: drv_G15.c,v $
 * Revision 1.8  2006/07/12 20:47:51  reinelt
 * indent
 *
 * Revision 1.7  2006/07/12 20:45:30  reinelt
 * G15 and thread patch by Anton
 *
 * Revision 1.6  2006/02/27 06:14:46  reinelt
 * graphic bug resulting in all black pixels solved
 *
 * Revision 1.5  2006/02/08 04:55:03  reinelt
 * moved widget registration to drv_generic_graphic
 *
 * Revision 1.4  2006/01/30 06:25:49  reinelt
 * added CVS Revision
 *
 * Revision 1.3  2006/01/30 05:47:38  reinelt
 * graphic subsystem changed to full-color RGBA
 *
 * Revision 1.2  2006/01/21 17:43:25  reinelt
 * minor cosmetic fixes
 *
 * Revision 1.1  2006/01/21 13:26:44  reinelt
 * Logitech G-15 keyboard LCD driver from Dave Ingram
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct DRIVER drv_G15
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <usb.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <signal.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "udelay.h"
#include "plugin.h"
#include "drv.h"
#include "drv_generic_graphic.h"
#include "thread.h"

#define G15_VENDOR 0x046d
#define G15_DEVICE 0xc222

#if 0
#define DEBUG(x) debug("%s(): %s", __FUNCTION__, x);
#else
#define DEBUG(x)
#endif

#define KB_UPDOWN_PRESS

static char Name[] = "G-15";

static usb_dev_handle *g15_lcd;

static unsigned char *g15_image;

unsigned char g_key_states[18];
unsigned char m_key_states[4];
unsigned char l_key_states[5];

static int uinput_fd;
static int kb_mutex;
static int kb_thread_pid;
static int kb_single_keypress = 0;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/


void drv_G15_keyDown(unsigned char scancode)
{
    struct input_event event;
    memset(&event, 0, sizeof(event));

    event.type = EV_KEY;
    event.code = scancode;
    event.value = 1;
    write(uinput_fd, &event, sizeof(event));
}
void drv_G15_keyUp(unsigned char scancode)
{
    struct input_event event;
    memset(&event, 0, sizeof(event));

    event.type = EV_KEY;
    event.code = scancode;
    event.value = 0;
    write(uinput_fd, &event, sizeof(event));
}
void drv_G15_keyDownUp(unsigned char scancode)
{
    drv_G15_keyDown(scancode);
    drv_G15_keyUp(scancode);

}
inline unsigned char drv_G15_evalScanCode(int key)
{
    // first 12 G keys produce F1 - F12, thats 0x3a + key
    if (key < 12) {
	return 0x3a + key;
    }
    // the other keys produce Key '1' (above letters) + key, thats 0x1e + key
    else {
	return 0x1e + key - 12;	// sigh, half an hour to find  -12 ....
    }
}

void drv_G15_processKeyEvent(unsigned char *buffer)
{
    const int g_scancode_offset = 167;
    const int m_scancode_offset = 187;
    const int l_scancode_offset = 191;
    int i;
    int is_set;
    unsigned char m_key_new_states[4];
    unsigned char l_key_new_states[5];
    unsigned char orig_scancode;
//   printf("%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx \n\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7],buffer[8]);
//   usleep(100);
    if (buffer[0] == 0x01) {
	DEBUG("Checking keys: ");


	for (i = 0; i < 18; ++i) {
	    orig_scancode = drv_G15_evalScanCode(i);
	    is_set = 0;

	    if (buffer[1] == orig_scancode || buffer[2] == orig_scancode || buffer[3] == orig_scancode ||
		buffer[4] == orig_scancode || buffer[5] == orig_scancode)
		is_set = 1;

	    if (!is_set && g_key_states[i] != 0) {
		// key was pressed but is no more
		if (!kb_single_keypress)
		    drv_G15_keyUp(g_scancode_offset + i);
		g_key_states[i] = 0;
		debug("G%d going up", (i + 1));
	    } else if (is_set && g_key_states[i] == 0) {
		if (!kb_single_keypress)
		    drv_G15_keyDown(g_scancode_offset + i);
		else
		    drv_G15_keyDownUp(g_scancode_offset + i);

		g_key_states[i] = 1;
		debug("G%d going down", (i + 1));
	    }
	}
    } else {
	if (buffer[0] == 0x02) {
	    memset(m_key_new_states, 0, sizeof(m_key_new_states));

	    if (buffer[6] & 0x01)
		m_key_new_states[0] = 1;
	    if (buffer[7] & 0x02)
		m_key_new_states[1] = 1;
	    if (buffer[8] & 0x04)
		m_key_new_states[2] = 1;
	    if (buffer[7] & 0x40)
		m_key_new_states[3] = 1;

	    for (i = 0; i < 4; ++i) {
		if (!m_key_new_states[i] && m_key_states[i] != 0) {
		    // key was pressed but is no more
		    if (!kb_single_keypress)
			drv_G15_keyUp(m_scancode_offset + i);
		    m_key_states[i] = 0;
		    debug("M%d going up", (i + 1));
		} else if (m_key_new_states[i] && m_key_states[i] == 0) {
		    if (!kb_single_keypress)
			drv_G15_keyDown(m_scancode_offset + i);
		    else
			drv_G15_keyDownUp(m_scancode_offset + i);
		    m_key_states[i] = 1;
		    debug("M%d going down", (i + 1));
		}
	    }

	    memset(l_key_new_states, 0, sizeof(l_key_new_states));
	    if (buffer[8] & 0x80)
		l_key_new_states[0] = 1;
	    if (buffer[2] & 0x80)
		l_key_new_states[1] = 1;
	    if (buffer[3] & 0x80)
		l_key_new_states[2] = 1;
	    if (buffer[4] & 0x80)
		l_key_new_states[3] = 1;
	    if (buffer[5] & 0x80)
		l_key_new_states[4] = 1;

	    for (i = 0; i < 5; ++i) {
		if (!l_key_new_states[i] && l_key_states[i] != 0) {
		    // key was pressed but is no more
		    if (!kb_single_keypress)
			drv_G15_keyUp(l_scancode_offset + i);
		    l_key_states[i] = 0;
		    debug("L%d going up", (i + 1));
		} else if (l_key_new_states[i] && l_key_states[i] == 0) {
		    if (!kb_single_keypress)
			drv_G15_keyDown(l_scancode_offset + i);
		    else
			drv_G15_keyDownUp(l_scancode_offset + i);
		    l_key_states[i] = 1;
		    debug("L%d going down", (i + 1));
		}
	    }

	}
    }
}

void drv_G15_closeUIDevice()
{
    DEBUG("closing device");
    ioctl(uinput_fd, UI_DEV_DESTROY);
    close(uinput_fd);
}


void drv_G15_initKeyHandling(char *device_filename)
{
    struct uinput_user_dev device;
    int i;
    DEBUG("Key Handling init")
	uinput_fd = open(device_filename, O_RDWR);

    if (uinput_fd < 0) {
	info("Error, could not open the uinput device");
	info("Compile your kernel for uinput, calling it a day now");
	info("mknod uinput c 10 223");
	abort();
    }
    memset(&device, 0, sizeof(device));
    strncpy(device.name, "G15 Keys", UINPUT_MAX_NAME_SIZE);
    device.id.bustype = BUS_USB;
    device.id.version = 4;

    ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);

    for (i = 0; i < 256; ++i)
	ioctl(uinput_fd, UI_SET_KEYBIT, i);

    write(uinput_fd, &device, sizeof(device));

    if (ioctl(uinput_fd, UI_DEV_CREATE)) {
	info("Failed to create input device");
	abort();
    }
//   atexit(&closeDevice);

    memset(g_key_states, 0, sizeof(g_key_states));
    memset(m_key_states, 0, sizeof(m_key_states));
    memset(l_key_states, 0, sizeof(l_key_states));
}


static void drv_G15_KBThread(void __attribute__ ((unused)) * notused)
{
    unsigned char buffer[9];
    int ret;
    while (1) {
	mutex_lock(kb_mutex);
	ret = usb_bulk_read(g15_lcd, 0x81, (char *) buffer, 9, 10);
//      ret = usb_interrupt_read(g15_lcd, 0x81, (char*)buffer, 9, 10);
	mutex_unlock(kb_mutex);
	if (ret == 9) {
	    drv_G15_processKeyEvent(buffer);
	}
    }
}

static int drv_G15_open()
{
    struct usb_bus *bus;
    struct usb_device *dev;
    char dname[32] = { 0 };

    g15_lcd = NULL;

    info("%s: Scanning USB for G-15 keyboard...", Name);

    usb_init();
    usb_set_debug(0);
    usb_find_busses();
    usb_find_devices();

    for (bus = usb_get_busses(); bus; bus = bus->next) {
	for (dev = bus->devices; dev; dev = dev->next) {
	    if ((g15_lcd = usb_open(dev))) {
		if ((dev->descriptor.idVendor == G15_VENDOR) && (dev->descriptor.idProduct == G15_DEVICE)) {

		    /* detach from the kernel if we need to */
		    int retval = usb_get_driver_np(g15_lcd, 0, dname, 31);
		    if (retval == 0 && strcmp(dname, "usbhid") == 0) {
			usb_detach_kernel_driver_np(g15_lcd, 0);
		    }
		    usb_set_configuration(g15_lcd, 1);
		    usleep(100);
		    usb_claim_interface(g15_lcd, 0);
		    return 0;
		} else {
		    usb_close(g15_lcd);
		}
	    }
	}
    }

    return -1;
}


static int drv_G15_close(void)
{
    usb_release_interface(g15_lcd, 0);
    if (g15_lcd)
	usb_close(g15_lcd);

    return 0;
}


static void drv_G15_update_img()
{
    int i, j, k;
    unsigned char *output = malloc(160 * 43 * sizeof(unsigned char));

    DEBUG("entered");
    if (!output)
	return;

    DEBUG("memory allocated");
    memset(output, 0, 160 * 43);
    DEBUG("memory set");
    output[0] = 0x03;
    DEBUG("first output set");

    for (k = 0; k < 6; k++) {
	for (i = 0; i < 160; i++) {
	    int maxj = (k == 5) ? 3 : 8;
	    for (j = 0; j < maxj; j++) {
		if (g15_image[(k * 1280) + i + (j * 160)])
		    output[32 + i + (k * 160)] |= (1 << j);
	    }
	}
    }

    DEBUG("output array prepared");
    mutex_lock(kb_mutex);
    usb_interrupt_write(g15_lcd, 0x02, (char *) output, 992, 1000);
    mutex_unlock(kb_mutex);
    usleep(300);

    DEBUG("data written to LCD");

    free(output);

    DEBUG("memory freed");
    DEBUG("left");
}



/* for graphic displays only */
static void drv_G15_blit(const int row, const int col, const int height, const int width)
{
    int r, c;

    DEBUG("entered");

    for (r = row; r < row + height && r < DROWS; r++) {
	for (c = col; c < col + width && c < DCOLS; c++) {
	    g15_image[r * 160 + c] = drv_generic_graphic_black(r, c);
	}
    }

    DEBUG("updating image");

    drv_G15_update_img();

    DEBUG("left");
}


/* start graphic display */
static int drv_G15_start(const char *section)
{
    char *s;

    DEBUG("entered");

    /* read display size from config */
    DROWS = 43;
    DCOLS = 160;

    DEBUG("display size set");

    s = cfg_get(section, "Font", "6x8");
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Font' entry from %s", Name, section, cfg_source());
	return -1;
    }

    XRES = -1;
    YRES = -1;
    if (sscanf(s, "%dx%d", &XRES, &YRES) != 2 || XRES < 1 || YRES < 1) {
	error("%s: bad Font '%s' from %s", Name, s, cfg_source());
	return -1;
    }

    /* Fixme: provider other fonts someday... */
    if (XRES != 6 && YRES != 8) {
	error("%s: bad Font '%s' from %s (only 6x8 at the moment)", Name, s, cfg_source());
	return -1;
    }

    DEBUG("Finished config stuff");

    DEBUG("allocating image buffer");
    /* you surely want to allocate a framebuffer or something... */
    g15_image = malloc(160 * 43 * sizeof(unsigned char));
    if (!g15_image)
	return -1;
    DEBUG("allocated");
    memset(g15_image, 0, 160 * 43);
    DEBUG("zeroed");

    /* open communication with the display */
    DEBUG("opening display...");
    if (drv_G15_open() < 0) {
	DEBUG("opening failed");
	return -1;
    }
    DEBUG("display open");

    /* reset & initialize display */
    DEBUG("clearing display");
    drv_G15_update_img();
    DEBUG("done");

    /*
       if (cfg_number(section, "Contrast", 0, 0, 255, &contrast) > 0) {
       drv_G15_contrast(contrast);
       }
     */
    s = cfg_get(section, "Uinput", "");
    if (s != NULL && *s != '\0') {
	cfg_number(section, "SingleKeyPress", 0, 0, 1, &kb_single_keypress);
	drv_G15_initKeyHandling(s);

	DEBUG("creating thread for keyboard");
	kb_mutex = mutex_create();
	kb_thread_pid = thread_create("G15_KBThread", drv_G15_KBThread, NULL);

	DEBUG("done");
    }
    DEBUG("left");

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

/* none */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_G15_list(void)
{
    printf("generic");
    return 0;
}


/* initialize driver & display */
int drv_G15_init(const char *section, const int quiet)
{
    int ret;

    info("%s: %s", Name, "$Revision: 1.8 $");

    DEBUG("entered");

    /* real worker functions */
    drv_generic_graphic_real_blit = drv_G15_blit;

    /* start display */
    if ((ret = drv_G15_start(section)) != 0)
	return ret;

    /* initialize generic graphic driver */
    if ((ret = drv_generic_graphic_init(section, Name)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];

	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_graphic_greet(buffer, NULL)) {
	    sleep(3);
	    drv_generic_graphic_clear();
	}
    }

    /* register plugins */
    /* none at the moment... */


    DEBUG("left");

    return 0;
}



/* close driver & display */
int drv_G15_quit(const int quiet)
{
    info("%s: shutting down.", Name);

    DEBUG("clearing display");
    /* clear display */
    drv_generic_graphic_clear();

    DEBUG("saying goodbye");
    /* say goodbye... */
    if (!quiet) {
	drv_generic_graphic_greet("goodbye!", NULL);
    }

    DEBUG("generic_graphic_quit()");
    drv_generic_graphic_quit();

    mutex_destroy(kb_mutex);
    usleep(10 * 1000);
    kill(kb_thread_pid, SIGKILL);

    drv_G15_closeUIDevice();
    DEBUG("closing UInputDev");


    DEBUG("closing connection");
    drv_G15_close();

    DEBUG("freeing image alloc");
    free(g15_image);

    return (0);
}


DRIVER drv_G15 = {
  name:Name,
  list:drv_G15_list,
  init:drv_G15_init,
  quit:drv_G15_quit,
};
