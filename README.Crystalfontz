
This is the README file for the Crystalfontz display driver for lcd4linux

This driver supports the 632/634 LCD-Modules from Crystalfontz, but should
work for the 626 and 636 modules too. The 634 is a 20x4 character display,
while the others only display 16x2. I've written the driver using a
634 module.

The driver understands the following configuration parameters:

Display:    any of 626, 632, 634 and 636.

Port:	    serial device (i.e. ttyS0) the LCD module is connnected
            to.

Speed:      any of 1200, 2400, 9600 and 19200. By default, the driver
            uses 9600 which is the speed the LCD modules are hardwired
            at. If your module works at a different speed than 9600,
            use this parameter. Otherwise omit it (i.e. omit it when
            you have a 634).

Backlight:  controls the backlight brightness. Quote from 634.pdf from
            the Crystalfonts-Webserver[1]: "0=OFF 100=ON. Intermediate
            values vary the brightness. There are a total of 25 possible
            brightness levels."

Contrast:   controls the contrast settings. Quote[1]: "0=very light,
            100 = very dark. 50 is typical. There are a total of 25
            possible contrast levels."


Known bugs:
When you draw a bar over a previously drawn textfield, the white portion
the bar will not erase the text. Only when the black portion of the bar
has reached the full bar length, the text will be erased. I did not bother
to implement that, since in lcd4linux, the whole display-screen is erased
prior to switching to a different 'screen'. Implementing this feature would
just add to program-overhead. Yes, you guessed it: I did not use the "bar"-
command that comes with the LCD-module, but wrote my own instead.
lcd4linux also supports "split-" or "dual-bars" (two bars in one segment),
which are not available on the Crystalfontz firmware.

  [1] http://www.crystalfontz.com

