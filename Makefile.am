# Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = gnu
CLEANFILES = *~
DRIVERS=@DRIVERS@

bin_PROGRAMS = lcd4linux
#lib_LTLIBRARIES = liblcd4linux.la

AM_CFLAGS = $(X_CFLAGS) -D_GNU_SOURCE -Wall

lcd4linux_LDFLAGS = $(X_LIBS)
lcd4linux_LDADD   = @DRIVERS@ @DRVLIBS@
#lcd4linux_LDADD  = liblcd4linux.la @DRVLIBS@ 
#remove next line for liblcd4linux
lcd4linux_DEPENDENCIES = @DRIVERS@

lcd4linux_SOURCES = \
lcd4linux.c \
pid.c pid.h \
hash.c hash.h \
parser.c parser.h \
processor.c processor.h \
layout.c layout.h \
timer.c timer.h \
evaluator.c evaluator.h \
widget.c widget.h \
widget_text.c widget_text.h \
widget_bar.c widget_bar.h \
plugin.c plugin.h \
plugin_math.c \
plugin_string.c \
plugin_cfg.c \
plugin_uname.c \
plugin_loadavg.c \
plugin_proc_stat.c \
plugin_cpuinfo.c \
plugin_meminfo.c \
plugin_i2c_sensors.c \
plugin_xmms.c \
system.c system.h \
isdn.c isdn.h \
wifi.c wifi.h \
mail.c mail.h \
seti.c seti.h \
battery.c battery.h \
dvb.c dvb.h \
filter.c filter.h \
exec.c exec.h \
expr.c expr.h \
mail2.c \
socket.c socket.h \
imon.c imon.h \
\
display.c display.h \
drv.c drv.h \
drv_generic_serial.c drv_generic_serial.h \
drv_generic_text.c   drv_generic_text.h \
debug.c debug.h \
cfg.c cfg.h \
lock.c lock.h \
pixmap.c pixmap.h \
bar.c bar.h \
icon.c icon.h \
fontmap.c fontmap.h \
udelay.c udelay.h

#liblcd4linux_la_DEPENDENCIES = @DRIVERS@
#liblcd4linux_la_LDFLAGS = -version-info 9:12:9
#liblcd4linux_la_LIBADD = @DRIVERS@

#liblcd4linux_la_SOURCES = \
#display.c display.h \
#drv.c drv.h \
#debug.c debug.h \
#cfg.c cfg.h \
#lock.c lock.h \
#pixmap.c pixmap.h \
#bar.c bar.h \
#icon.c icon.h \
#fontmap.c fontmap.h \
#udelay.c udelay.h

EXTRA_lcd4linux_SOURCES= \
parport.c parport.h \
BeckmannEgle.c \
Crystalfontz.c \
Cwlinux.c \
HD44780.c \
M50530.c \
T6963.c \
USBLCD.c \
drv_MatrixOrbital.c MatrixOrbital.c \
MilfordInstruments.c \
PalmPilot.c \
Raster.c \
SIN.c \
XWindow.c \
Text.c

EXTRA_DIST = \
lcd4linux.conf.sample \
lcd4kde.conf \
lcd4linux.kdelnk \
lcd4linux.xpm \
lcd4linux.lsm \
curses.m4 \
AUTHORS \
CREDITS \
FAQ \
NEWS \
TODO \
README \
README.Rows \
README.Tokens \
README.Drivers \
README.Plugins \
README.KDE \
Skeleton.c \
plugin_sample.c
