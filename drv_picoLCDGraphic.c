/* $Id$
 * $URL$
 *
 * driver for picoLCD Graphic(256x64) displays from mini-box.com
 *
 * Copyright (C) 2005 Michael Reinelt <michael@reinelt.co.at>
 * Copyright (C) 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * Copyright (C) 2009 Nicu Pavel, Mini-Box.com <npavel@mini-box.com>
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
 * exported fuctions:
 *
 * struct DRIVER drv_picoLCDGraphic
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <usb.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "udelay.h"
#include "plugin.h"
#include "timer.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"
#include "drv_generic_gpio.h"
#include "drv_generic_keypad.h"
#include "drv_generic_graphic.h"



#define picoLCD_VENDOR  0x04d8
#define picoLCD_DEVICE  0xc002

#define OUT_REPORT_LED_STATE		0x81
#define OUT_REPORT_LCD_BACKLIGHT	0x91
#define OUT_REPORT_LCD_CONTRAST		0x92

#define OUT_REPORT_CMD			0x94
#define OUT_REPORT_DATA			0x95
#define OUT_REPORT_CMD_DATA		0x96

#define SCREEN_H			64
#define SCREEN_W			256


#if 1
#define DEBUG(x) debug("%s(): %s", __FUNCTION__, x);
#else
#define DEBUG(x)
#endif

/* "dirty" marks the display to be redrawn from frame buffer */
static int dirty = 1;

/* timer for display redraw (set to zero for "direct updates") */
static int update = 0;

/* USB read timeout in ms (the picoLCD 256x64 times out on every read
	unless a key has been pressed!)  */
static int read_timeout = 0;

static char Name[] = "picoLCDGraphic";
static unsigned char *pLG_framebuffer;

/* used to display white text on black background or inverse */
unsigned char inverted = 0;

static unsigned int gpo = 0;

static char *Buffer;
static char *BufPtr;

static usb_dev_handle *lcd;
//extern int usb_debug;
int usb_debug;


/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_pLG_open(void)
{
    struct usb_bus *busses, *bus;
    struct usb_device *dev;
    char driver[1024];
    char product[1024];
    char manufacturer[1024];
    char serialnumber[1024];
    int ret;

    lcd = NULL;

    info("%s: scanning for picoLCD 256x64...", Name);

    usb_debug = 0;

    usb_init();
    usb_find_busses();
    usb_find_devices();
    busses = usb_get_busses();

    for (bus = busses; bus; bus = bus->next) {
	for (dev = bus->devices; dev; dev = dev->next) {
	    if ((dev->descriptor.idVendor == picoLCD_VENDOR) && (dev->descriptor.idProduct == picoLCD_DEVICE)) {

		info("%s: found picoLCD on bus %s device %s", Name, bus->dirname, dev->filename);

		lcd = usb_open(dev);

		ret = usb_get_driver_np(lcd, 0, driver, sizeof(driver));

		if (ret == 0) {
		    info("%s: interface 0 already claimed by '%s'", Name, driver);
		    info("%s: attempting to detach driver...", Name);
		    if (usb_detach_kernel_driver_np(lcd, 0) < 0) {
			error("%s: usb_detach_kernel_driver_np() failed!", Name);
			return -1;
		    }
		}

		usb_set_configuration(lcd, 1);
		usleep(100);

		if (usb_claim_interface(lcd, 0) < 0) {
		    error("%s: usb_claim_interface() failed!", Name);
		    return -1;
		}

		usb_set_altinterface(lcd, 0);

		usb_get_string_simple(lcd, dev->descriptor.iProduct, product, sizeof(product));
		usb_get_string_simple(lcd, dev->descriptor.iManufacturer, manufacturer, sizeof(manufacturer));
		usb_get_string_simple(lcd, dev->descriptor.iSerialNumber, serialnumber, sizeof(serialnumber));

		info("%s: Manufacturer='%s' Product='%s' SerialNumber='%s'", Name, manufacturer, product, serialnumber);

		return 0;
	    }
	}
    }
    error("%s: could not find a picoLCD", Name);
    return -1;
}

static int drv_pLG_read(unsigned char *data, int size)
{
    return usb_interrupt_read(lcd, USB_ENDPOINT_IN + 1, (char *) data, size, read_timeout);
}


static void drv_pLG_send(unsigned char *data, int size)
{
    int ret;
    ret = usb_interrupt_write(lcd, USB_ENDPOINT_OUT + 1, (char *) data, size, 1000);
    //fprintf(stderr, "%s written %d bytes\n", __FUNCTION__, ret);
}

static int drv_pLG_close(void)
{
    usb_release_interface(lcd, 0);
    usb_close(lcd);

    return 0;
}

static void drv_pLG_update_img()
{
    unsigned char cmd3[64] = { OUT_REPORT_CMD_DATA };	/* send command + data */
    unsigned char cmd4[64] = { OUT_REPORT_DATA };	/* send data only */

    int index, bit, x, y;
    unsigned char cs, line;
    unsigned char pixel;

    /* do not redraw display if frame buffer has not changed, unless
       "direct updates" have been requested (update is zero) */
    if ((!dirty) && (update > 0)) {
	debug("Skipping %s\n", __FUNCTION__);
	return;
    }

    info("In %s\n", __FUNCTION__);

    for (cs = 0; cs < 4; cs++) {
	unsigned char chipsel = (cs << 2);	//chipselect
	for (line = 0; line < 8; line++) {
	    //ha64_1.setHIDPkt(OUT_REPORT_CMD_DATA, 8+3+32, 8, chipsel, 0x02, 0x00, 0x00, 0xb8|j, 0x00, 0x00, 0x40);
	    cmd3[0] = OUT_REPORT_CMD_DATA;
	    cmd3[1] = chipsel;
	    cmd3[2] = 0x02;
	    cmd3[3] = 0x00;
	    cmd3[4] = 0x00;
	    cmd3[5] = 0xb8 | line;
	    cmd3[6] = 0x00;
	    cmd3[7] = 0x00;
	    cmd3[8] = 0x40;
	    cmd3[9] = 0x00;
	    cmd3[10] = 0x00;
	    cmd3[11] = 32;

	    //ha64_2.setHIDPkt(OUT_REPORT_DATA, 4+32, 4, chipsel | 0x01, 0x00, 0x00, 32);

	    cmd4[0] = OUT_REPORT_DATA;
	    cmd4[1] = chipsel | 0x01;
	    cmd4[2] = 0x00;
	    cmd4[3] = 0x00;
	    cmd4[4] = 32;

	    for (index = 0; index < 32; index++) {
		pixel = 0x00;

		for (bit = 0; bit < 8; bit++) {
		    x = cs * 64 + index;
		    y = (line * 8 + bit + 0) % SCREEN_H;

		    if (pLG_framebuffer[y * 256 + x] ^ inverted)
			pixel |= (1 << bit);
		}
		cmd3[12 + index] = pixel;
	    }

	    for (index = 32; index < 64; index++) {
		pixel = 0x00;

		for (bit = 0; bit < 8; bit++) {
		    x = cs * 64 + index;
		    y = (line * 8 + bit + 0) % SCREEN_H;
		    if (pLG_framebuffer[y * 256 + x] ^ inverted)
			pixel |= (1 << bit);
		}

		cmd4[5 + (index - 32)] = pixel;
	    }

	    drv_pLG_send(cmd3, 44);
	    drv_pLG_send(cmd4, 38);
	}
    }

    /* mark display as up-to-date */
    dirty = 0;
    //drv_pLG_clear();
}


/* for graphic displays only */
static void drv_pLG_blit(const int row, const int col, const int height, const int width)
{
    int r, c;

    //DEBUG(fprintf(stderr, "In %s called with row %d col %d height %d width %d\n", __FUNCTION__, row, col, height, width));

    for (r = row; r < row + height; r++) {
	for (c = col; c < col + width; c++) {
	    pLG_framebuffer[r * 256 + c] = drv_generic_graphic_black(r, c);
	    //fprintf(stderr, "%d", pLG_framebuffer[r * 256 + c]);
	}
	//fprintf(stderr, "\n");
    }

    /*
       for (r = 0; r < 64; r++) {
       for(c = 0; c < 256; c++) {
       fprintf(stderr, "%d", pLG_framebuffer[r * 256 + c]); 
       }
       fprintf(stderr, "\n");
       }
     */

    /* display needs to be redrawn from frame buffer */
    dirty = 1;

    /* if "direct updates" have been requested, redraw now */
    if (update <= 0)
	drv_pLG_update_img();
}


void drv_pLG_clear(void)
{
    unsigned char cmd[3] = { 0x93, 0x01, 0x00 };	/* init display */
    unsigned char cmd2[9] = { OUT_REPORT_CMD };	/* init display */
    unsigned char cmd3[64] = { OUT_REPORT_CMD_DATA };	/* clear screen */
    unsigned char cmd4[64] = { OUT_REPORT_CMD_DATA };	/* clear screen */

    int init, index;
    unsigned char cs, line;


    info("In %s\n", __FUNCTION__);
    drv_pLG_send(cmd, 3);

    for (init = 0; init < 4; init++) {
	unsigned char cs = ((init << 2) & 0xFF);

	cmd2[0] = OUT_REPORT_CMD;
	cmd2[1] = cs;
	cmd2[2] = 0x02;
	cmd2[3] = 0x00;
	cmd2[4] = 0x64;
	cmd2[5] = 0x3F;
	cmd2[6] = 0x00;
	cmd2[7] = 0x64;
	cmd2[8] = 0xC0;

	drv_pLG_send(cmd2, 9);
    }


    for (cs = 0; cs < 4; cs++) {
	unsigned char chipsel = (cs << 2);	//chipselect
	for (line = 0; line < 8; line++) {
	    //ha64_1.setHIDPkt(OUT_REPORT_CMD_DATA, 8+3+32, 8, cs, 0x02, 0x00, 0x00, 0xb8|j, 0x00, 0x00, 0x40);
	    cmd3[0] = OUT_REPORT_CMD_DATA;
	    cmd3[1] = chipsel;
	    cmd3[2] = 0x02;
	    cmd3[3] = 0x00;
	    cmd3[4] = 0x00;
	    cmd3[5] = 0xb8 | line;
	    cmd3[6] = 0x00;
	    cmd3[7] = 0x00;
	    cmd3[8] = 0x40;
	    cmd3[9] = 0x00;
	    cmd3[10] = 0x00;
	    cmd3[11] = 32;

	    unsigned char temp = 0;

	    for (index = 0; index < 32; index++) {
		cmd3[12 + index] = temp;
	    }

	    drv_pLG_send(cmd3, 64);

	    //ha64_2.setHIDPkt(OUT_REPORT_DATA, 4+32, 4, cs | 0x01, 0x00, 0x00, 32);

	    cmd4[0] = OUT_REPORT_DATA;
	    cmd4[1] = chipsel | 0x01;
	    cmd4[2] = 0x00;
	    cmd4[3] = 0x00;
	    cmd4[4] = 32;

	    for (index = 32; index < 64; index++) {
		temp = 0x00;
		cmd4[5 + (index - 32)] = temp;
	    }
	    drv_pLG_send(cmd4, 64);
	}
    }
}

static int drv_pLG_contrast(int contrast)
{
    unsigned char cmd[2] = { 0x92 };	/* set contrast */

    if (contrast < 0)
	contrast = 0;
    if (contrast > 255)
	contrast = 255;

    cmd[1] = contrast;
    drv_pLG_send(cmd, 2);

    return contrast;
}


static int drv_pLG_backlight(int backlight)
{
    unsigned char cmd[2] = { 0x91 };	/* set backlight */

    if (backlight < 0)
	backlight = 0;
    if (backlight >= 1)
	backlight = 200;

    cmd[1] = backlight;
    drv_pLG_send(cmd, 2);

    return backlight;
}

#define _USBLCD_MAX_DATA_LEN          24
#define IN_REPORT_KEY_STATE           0x11
static int drv_pLG_gpi( __attribute__ ((unused))
		       int num)
{
    int ret;
    unsigned char read_packet[_USBLCD_MAX_DATA_LEN];
    ret = drv_pLG_read(read_packet, _USBLCD_MAX_DATA_LEN);
    if ((ret > 0) && (read_packet[0] == IN_REPORT_KEY_STATE)) {
	debug("picoLCD: pressed key= 0x%02x\n", read_packet[1]);
	return read_packet[1];
    }
    return 0;
}


static int drv_pLG_gpo(int num, int val)
{
    unsigned char cmd[2] = { 0x81 };	/* set GPO */

    if (num < 0)
	num = 0;
    if (num > 7)
	num = 7;

    if (val < 0)
	val = 0;
    if (val > 1)
	val = 1;

    /* set led bit to 1 or 0 */
    if (val)
	gpo |= 1 << num;
    else
	gpo &= ~(1 << num);

    cmd[1] = gpo;
    drv_pLG_send(cmd, 2);

    return val;
}

static int drv_pLG_start(const char *section, const int quiet)
{
    int rows = -1, cols = -1;
    int value;
    char *s;

    /* set display redraw interval (set to zero for "direct updates") */
    cfg_number(section, "update", 200, 0, -1, &update);

    /* USB read timeout in ms (the picoLCD 256x64 times out on every
       read unless a key has been pressed!)  */
    cfg_number(section, "Timeout", 5, 1, 1000, &read_timeout);

    s = cfg_get(section, "Size", NULL);
    if (s == NULL || *s == '\0') {
	error("%s: no '%s.Size' entry from %s", Name, section, cfg_source());
	return -1;
    }
    if (sscanf(s, "%dx%d", &cols, &rows) != 2 || rows < 1 || cols < 1) {
	error("%s: bad %s.Size '%s' from %s", Name, section, s, cfg_source());
	free(s);
	return -1;
    }

    if (cfg_number(section, "Inverted", 0, 0, 1, &value) > 0) {
	info("Setting display inverted to %d", value);
	inverted = value;
    }

    DROWS = SCREEN_H;
    DCOLS = SCREEN_W;

    if (drv_pLG_open() < 0) {
	return -1;
    }

    /* Init the command buffer */
    Buffer = (char *) malloc(1024);
    if (Buffer == NULL) {
	error("%s: coommand buffer could not be allocated: malloc() failed", Name);
	return -1;
    }
    BufPtr = Buffer;

    /* Init framebuffer buffer */
    pLG_framebuffer = malloc(SCREEN_W * SCREEN_H * sizeof(unsigned char));
    if (!pLG_framebuffer)
	return -1;

    DEBUG("allocated");
    memset(pLG_framebuffer, 0, SCREEN_W * SCREEN_H);
    DEBUG("zeroed");

    if (cfg_number(section, "Contrast", 0, 0, 255, &value) > 0) {
	info("Setting contrast to %d", value);
	drv_pLG_contrast(value);
    }

    if (cfg_number(section, "Backlight", 0, 0, 1, &value) > 0) {
	info("Setting backlight to %d", value);
	drv_pLG_backlight(value);
    }

    drv_pLG_clear();		/* clear display */

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, SCREEN_W, SCREEN_H);
	if (drv_generic_graphic_greet(buffer, "http://www.picolcd.com")) {
	    sleep(3);
	    drv_pLG_clear();
	}
    }

    /* setup a timer that regularly redraws the display from the frame
       buffer (unless "direct updates" have been requested */
    if (update > 0)
	timer_add(drv_pLG_update_img, NULL, update, 0);

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

static void plugin_contrast(RESULT * result, RESULT * arg1)
{
    double contrast;

    contrast = drv_pLG_contrast(R2N(arg1));
    SetResult(&result, R_NUMBER, &contrast);
}

static void plugin_backlight(RESULT * result, RESULT * arg1)
{
    double backlight;

    backlight = drv_pLG_backlight(R2N(arg1));
    SetResult(&result, R_NUMBER, &backlight);
}

static void plugin_gpo(RESULT * result, RESULT * argv[])
{
    double gpo;
    gpo = drv_pLG_gpo(R2N(argv[0]), R2N(argv[1]));
    SetResult(&result, R_NUMBER, &gpo);
}


/****************************************/
/***        widget callbacks          ***/
/****************************************/


/* using drv_generic_text_draw(W) */
/* using drv_generic_text_icon_draw(W) */
/* using drv_generic_text_bar_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_pLG_list(void)
{
    printf("picoLCD 256x64 Graphic LCD");
    return 0;
}


/* initialize driver & display */
int drv_pLG_init(const char *section, const int quiet)
{
    int ret;

    info("%s: %s", Name, "$Rev$");

    info("PICOLCD Graphic initialization\n");

    /* display preferences */
    XRES = 6;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    GPOS = 8;
    GPIS = 1;
    /* real worker functions */
    drv_generic_graphic_real_blit = drv_pLG_blit;
    drv_generic_gpio_real_set = drv_pLG_gpo;
    drv_generic_gpio_real_get = drv_pLG_gpi;


    /* start display */
    if ((ret = drv_pLG_start(section, quiet)) != 0)
	return ret;


    /* initialize generic graphic driver */
    if ((ret = drv_generic_graphic_init(section, Name)) != 0)
	return ret;

    /* GPO's init */

    if ((ret = drv_generic_gpio_init(section, Name)) != 0)
	return ret;

    /* register plugins */

    AddFunction("LCD::contrast", -1, plugin_contrast);
    AddFunction("LCD::backlight", -1, plugin_backlight);
    AddFunction("LCD::gpo", -1, plugin_gpo);

    return 0;
}


/* close driver & display */
int drv_pLG_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    /* clear display */
    drv_pLG_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_graphic_greet("goodbye!", NULL);
    }

    drv_pLG_close();

    if (Buffer) {
	free(Buffer);
	BufPtr = NULL;
    }

    drv_generic_graphic_quit();

    return (0);
}


DRIVER drv_picoLCDGraphic = {
    .name = Name,
    .list = drv_pLG_list,
    .init = drv_pLG_init,
    .quit = drv_pLG_quit,
};
