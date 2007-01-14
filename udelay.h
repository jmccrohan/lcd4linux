/* $Id$
 * $URL$
 *
 * short delays 
 *
 * Copyright (C) 1999, 2000 Michael Reinelt <reinelt@eunet.at>
 * Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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

#ifndef _UDELAY_H_
#define _UDELAY_H_

/* stolen from linux/asm-i386/processor.h */
/* REP NOP (PAUSE) is a good thing to insert into busy-wait loops. */
static inline void rep_nop(void)
{
# if defined(__i386) || defined(__i386__) || defined(__AMD64__) || defined(__x86_64__) || defined(__amd64__)
    /* intel or amd64 arch, the "rep" and "nop" opcodes are available */
    __asm__ __volatile__("rep; nop");
# else
    /* other Arch, maybe add core cooldown code here, too. */
    do {
    } while (0);
# endif
}

void udelay_init(void);
unsigned long timing(const char *driver, const char *section, const char *name, const int defval, const char *unit);
void ndelay(const unsigned long nsec);

#define udelay(usec) ndelay(usec*1000)

#endif
