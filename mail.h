/* $Id: mail.h,v 1.4 2001/03/15 14:25:05 ltoetsch Exp $
 *
 * email specific functions
 *
 * Copyright 2001 by Axel Ehnert <Axel@Ehnert.net>
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
 * $Log: mail.h,v $
 * Revision 1.4  2001/03/15 14:25:05  ltoetsch
 * added unread/total news
 *
 * Revision 1.3  2001/03/14 13:19:29  ltoetsch
 * Added pop3/imap4 mail support
 *
 * Revision 1.2  2001/03/08 09:02:04  reinelt
 *
 * seti client cleanup
 *
 * Revision 1.1  2001/02/18 22:11:34  reinelt
 * *** empty log message ***
 *
 */

#ifndef _MAIL_H
#define _MAIL_H_

#define MAILBOXES 9

int Mail (int index, int *num, int *unseen);
int Mail_pop_imap_news(char *mbx, int *num, int *unseen); /* mail2.c */
#endif
