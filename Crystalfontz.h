/*
 * driver for display modules from Crystalfontz
 *
 * Copyright 2000 by Herbert Rosmanith (herp@wildsau.idv.uni-linz.ac.at)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


/* see: 634full.pdf, avail. from www.crystalfontz.com
 * values here are octal
 */

#define	CRYFON_NULL			"\000"
#define	CRYFON_CURSOR_HOME		"\001"
#define	CRYFON_HOME			CRYFON_CURSOR_HOME
#define	CRYFON_HIDE_DISPLAY		"\002"
#define	CRYFON_RESTORE_DISPLAY		"\003"
#define	CRYFON_HIDE_CURSOR		"\004"
#define	CRYFON_SHOW_UL_CURSOR		"\005"
#define	CRYFON_SHOW_BLK_CURSOR		"\006"
#define	CRYFON_SHOW_INV_CURSOR		"\007"
#define	CRYFON_BACKSPACE		"\010"
/*not used: 9 (tab), 011 octal */
#define	CRYFON_LF			"\012"
#define	CRYFON_DEL_IN_PLACE		"\013"
#define	CRYFON_FF			"\014"
#define	CRYFON_CR			"\015"
#define	CRYFON_BACKLIGHT_CTRL		"\016"
#define	CRYFON_CONTRAST_CTRL		"\017"
/*not used: 16, 020 octal */
#define	CRYFON_SET_CURSOR_POS		"\021"
#define	CRYFON_GOTO			CRYFON_SET_CURSOR_POS
#define	CRYFON_HORIZONTAL_BAR		"\022"
#define	CRYFON_SCROLL_ON		"\023"
#define	CRYFON_SCROLL_OFF		"\024"
#define	CRYFON_SET_SCROLL_MARQU		"\025"
#define	CRYFON_ENABLE_SCROLL_MARQU	"\026"
#define	CRYFON_WRAP_ON			"\027"
#define	CRYFON_WRAP_OFF			"\030"
#define	CRYFON_SET_CUSTOM_CHAR_BITMAP	"\031"
#define	CRYFON_REBOOT			"\032"
#define	CRYFON_ESC			"\033"
#define	CRYFON_LARGE_BLK_NUM		"\034"
/*not used: 29, 035 octal*/
#define	CRYFON_SEND_DATA_DIRECT		"\036"
#define	CRYFON_SHOW_INFO		"\037"
#define	CRYFON_CUST_CHAR0		"\200"
#define	CRYFON_CUST_CHAR1		"\201"
#define	CRYFON_CUST_CHAR2		"\202"
#define	CRYFON_CUST_CHAR3		"\203"
#define	CRYFON_CUST_CHAR4		"\204"
#define	CRYFON_CUST_CHAR5		"\205"
#define	CRYFON_CUST_CHAR6		"\206"
#define	CRYFON_CUST_CHAR7		"\207"

