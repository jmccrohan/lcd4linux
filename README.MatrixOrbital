#
# $Id: README.MatrixOrbital,v 1.2 2000/03/22 07:33:50 reinelt Exp $
#

This is the README file for the MatrixOrbital display driver for lcd4linux

This driver supports the serial interface alphanumeric display modules by
Matrix Orbital Corporation (http://www.matrixorbital.com).

I could only test it with the LCD2041 model, but I think every other (LCD) model 
should work. These displays are supported:

  LCD0821: 2 lines by  8 characters
  LCD1621: 2 lines by 16 characters
  LCD2021: 2 lines by 20 characters
  LCD2041: 4 lines by 20 characters (tested)
  LCD4021: 2 lines by 40 characters

I could not test the vacuum fluorescent display models, but I think they should work, too.
There are no entries for this models in the driver table (at the bottom of MatrixOrbital.c),
but they could be easily added.

The displays come with an RS-232 and an I2C interface. The driver supports the RS-232 interface
only (because I have no idea how to find the I2C bus on my motherboard). 

Power can be applied either via an external DC power supply, a modified floppy power connector
(be aware that you can destroy your display if you get the pins wrong!) or via the RI (ring) 
signal of the RS-232 port. I choosed the latter, and modified a serial card so that it supplies 
+5V from the ISA bus to this pin (again, be aware that this is dangerous if you connect any other 
serial device to this modified port).

The driver supports vertical, horizontal and split bars (two independent bars in one line),
all bar types can be used simultanously. As the displays only have 8 user-defined characters,
the needed characters to display all the bars must be reduced to 8. This is done by replacing
characters with similar ones. To reduce flicker, a character which is displayed at the moment,
will not be redefined, even if it's not used in this run. Only if the character compaction 
fails, this characters will be redefined, too.

The displays have a GPO (general purpose output), where you can connect a LED or something. 
The driver supports controlling this GPO, but this function is unused by now.


Configuration:

The driver needs/supports the following entries in lcd4linux.conf:

Display: a valid Matrix Orbital Display name (e.g. "LCD2041")
Port: serial device the display is attached to (e.g. /dev/ttyS2)
Speed: the baud rate from the display (configured via jumpers) must match 
       this value. Possible values are 1200, 2400, 9600 and 19200
Contrast: sets the LCD display contrast to a level between 0 (light) 
          and 256 (dark). Default value: 160

