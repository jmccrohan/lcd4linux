/* $Id: cfg.h,v 1.5 2003/09/09 06:54:43 reinelt Exp $
 *
 * config file stuff
 *
 * Copyright 1999, 2000 by Michael Reinelt (reinelt@eunet.at)
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
 *
 * $Log: cfg.h,v $
 * Revision 1.5  2003/09/09 06:54:43  reinelt
 * new function 'cfg_number()'
 *
 * Revision 1.4  2003/08/24 05:17:58  reinelt
 * liblcd4linux patch from Patrick Schemitz
 *
 * Revision 1.3  2003/02/22 07:53:10  reinelt
 * cfg_get(key,defval)
 *
 * Revision 1.2  2000/04/03 04:46:38  reinelt
 *
 * added '-c key=val' option
 *
 * Revision 1.1  2000/03/10 11:40:47  reinelt
 * *** empty log message ***
 *
 * Revision 1.2  2000/03/06 06:04:06  reinelt
 *
 * minor cleanups
 *
 *
 */

#ifndef _CFG_H_
#define _CFG_H_

extern int   (*cfg_init)   (char *source);
extern char *(*cfg_source) (void);
extern int   (*cfg_cmd)    (char *arg);
extern char *(*cfg_get)    (char *key, char *defval);
extern int   (*cfg_number) (char *key, int   defval, 
			    int min, int max, int *value);

int   l4l_cfg_init   (char *file);
char *l4l_cfg_source (void);
int   l4l_cfg_cmd    (char *arg);
char *l4l_cfg_get    (char *key, char *defval);
int   l4l_cfg_number (char *key, int   defval, 
		      int min, int max, int *value);

#endif
