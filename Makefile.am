## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign
CLEANFILES = *~

bin_PROGRAMS = lcd4linux

AM_CFLAGS = $(X_CFLAGS) -D_GNU_SOURCE -Wall
lcd4linux_LDFLAGS = $(X_LIBS)
lcd4linux_LDADD = @DRIVERS@ @DRVLIBS@
lcd4linux_DEPENDENCIES = @DRIVERS@

lcd4linux_SOURCES = \
lcd4linux.c \
debug.c debug.h \
cfg.c cfg.h \
lock.c lock.h \
parser.c parser.h \
processor.c processor.h \
system.c system.h \
isdn.c isdn.h \
mail.c mail.h \
seti.c seti.h \
battery.c battery.h \
filter.c filter.h \
udelay.c udelay.h \
display.c display.h \
pixmap.c pixmap.h \
fontmap.c fontmap.h \
exec.c exec.h

EXTRA_lcd4linux_SOURCES= \
BeckmannEgle.c \
Crystalfontz.c Crystalfontz.h \
HD44780.c \
MatrixOrbital.c \
PalmPilot.c \
PNG.c \
PPM.c \
SIN.c \
Skeleton.c \
XWindow.c	

EXTRA_DIST = \
lcd4linux.conf.sample \
lcd4kde.conf \
lcd4linux.kdelnk \
lcd4linux.xpm \
lcd4linux.lsm \
CREDITS \
FAQ \
TODO \
README.Drivers \
README.MatrixOrbital \
README.HD44780 \
README.Crystalfontz \
README.X11 \
README.KDE \
README.Raster \
README.Png

