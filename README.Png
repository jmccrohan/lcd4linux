#
# $Id: README.Png,v 1.1 2001/03/02 18:06:18 reinelt Exp $
#

This is the README file for the Png display driver for lcd4linux.

Preliminarys: libgd, libpng, libz
Optional: perl, apache

The driver creates the output file specified with the -o switch. The
parameter is used as a format string for sprintf(), if you specify '%d'
in the output file, files with a sequence number will be created.

The output file is first created with a '.tmp' extension, this temporary
file will be written and closed, and finally (atomically) renamed. This way
you can be shure that you will always get a complete file, but its contents
changes every 'tick' milliseconds. 

Configuration:

The driver needs/supports the following entries in lcd4linux.conf:

Display: must be "Png"
size: [columns]x[rows], e.g. "20x4"
font: [xrex]x[yres], at the moment only "5x8" and "6x8" supported.
pixel: [pixelsize]+[pixelgap], e.g. "5+1"
gap: [row gap]x[column gap], e.g. "3x3"
border: border width
foreground: color of an active LCD Pixel, must be #rrggbb
halfground: color of an inactive LCD Pixel, must be #rrggbb
background: backlight color, must be #rrggbb

For details please look into README.Raster.

To display this png file continuosly in a web page, follow these instructions:
Copy the sample png.html to an appropriate place under your htdocs.
Copy the sample nph-png perl script into your cgi-bin directory, and adjust
png.html to contain this directory.
Adjust nph-png to contain the path/filename of the outputfile (s -o above).
Start lcd4linux -o path/filename.png.
If you are on a slow connection to your webserver you might also adjust the
$DELAY in nph-png or in lcd4linux.conf.

Note: depending on your webervers configuration, you must rename nph-png to
   nph-png.pl or npg-png.cgi.

Have fun.
