#
# $Id: README.Text,v 1.2 2001/03/16 09:28:08 ltoetsch Exp $
#

This is the README file for the Text display driver for lcd4linux.

This driver is mainly for debugging purposes.
It needs ncurses for display.

The driver understands the following options:

Display: must be "Text"
size: [columns]x[rows], e.g. "20x4"
TextBar: if this is set, Bars display the values max, len1 and len2.

Of course, lcd4linux should be started in the foreground with this driver.

The driver shows also a window with lcd4linux's diagnostics. In this window
CR and LF are displayed as underscores.

Example:
./lcd4linux -q -vv -F -cDisplay=Text -ctick=1000 -ctack=1000 


BUGS:
- A resize of the term window messes up the display.
- Vertical bars are not supported.
- BAR_L is ignored.

Have fun
     -lt


