## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign
CLEANFILES = *~

bin_PROGRAMS = lcd4linux

AM_CFLAGS = $(X_CFLAGS) -Wall
lcd4linux_LDFLAGS = $(X_LIBS)
if WITH_X
lcd4linux_LDADD = -lX11
endif

lcd4linux_SOURCES = \
lcd4linux.c \
debug.h \
cfg.c cfg.h \
lock.c lock.h \
parser.c parser.h \
processor.c processor.h \
system.c system.h \
isdn.c isdn.h \
filter.c filter.h \
display.c display.h \
pixmap.c pixmap.h \
fontmap.c fontmap.h \
Skeleton.c \
MatrixOrbital.c \
HD44780.c \
Raster.c \
XWindow.c
