/* $Id: socket.h,v 1.2 2003/10/05 17:58:50 reinelt Exp $
 *
 * simple socket functions
 *
 * Copyright 2001 Leopold Tötsch <lt@toetsch.at>
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
 * $Log: socket.h,v $
 * Revision 1.2  2003/10/05 17:58:50  reinelt
 * libtool junk; copyright messages cleaned up
 *
 * Revision 1.1  2001/03/14 13:19:29  ltoetsch
 * Added pop3/imap4 mail support
 *
 *
 */

#ifndef __SOCKET_H_
#define __SOCKET_H_

int open_socket(char *machine, int port);
int read_socket(int fd, char *buf, size_t size);
int read_socket_match(int fd, char *buf, size_t size, char *match);
int write_socket(int fd, char *buf);

#endif



