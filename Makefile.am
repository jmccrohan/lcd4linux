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

lcd4linux_SOURCES =         \
lcd4linux.c                 \
cfg.c         cfg.h         \
debug.c       debug.h       \
drv.c         drv.h         \
evaluator.c   evaluator.h   \
hash.c        hash.h        \
layout.c      layout.h      \
lock.c 	      lock.h        \
pid.c         pid.h         \
timer.c       timer.h       \
thread.c      thread.h      \
udelay.c      udelay.h      \
qprintf.c     qprintf.h     \
                            \
widget.c      widget.h      \
widget_text.c widget_text.h \
widget_bar.c  widget_bar.h  \
widget_icon.c widget_icon.h \
                            \
plugin.c      plugin.h      \
plugin_math.c               \
plugin_string.c             \
plugin_cfg.c                \
plugin_uname.c              \
plugin_loadavg.c            \
plugin_proc_stat.c          \
plugin_cpuinfo.c            \
plugin_meminfo.c            \
plugin_netdev.c             \
plugin_ppp.c                \
plugin_dvb.c                \
plugin_apm.c                \
plugin_i2c_sensors.c        \
plugin_imon.c               \
plugin_exec.c               \
plugin_xmms.c               \
plugin_mysql.c              \
plugin_wireless.c           \
plugin_seti.c

#liblcd4linux_la_DEPENDENCIES = @DRIVERS@
#liblcd4linux_la_LDFLAGS = -version-info 9:12:9
#liblcd4linux_la_LIBADD = @DRIVERS@

#liblcd4linux_la_SOURCES =

EXTRA_lcd4linux_SOURCES= \
drv_generic_text.c    drv_generic_text.h \
drv_generic_graphic.c drv_generic_graphic.h \
drv_generic_serial.c  drv_generic_serial.h \
drv_generic_parport.c drv_generic_parport.h \
BeckmannEgle.c \
drv_Crystalfontz.c \
drv_Cwlinux.c \
drv_HD44780.c \
drv_M50530.c \
drv_T6963.c \
drv_USBLCD.c \
drv_MatrixOrbital.c \
MilfordInstruments.c \
PalmPilot.c \
Raster.c \
drv_X11.c \
Text.c \
font_6x8.h

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
plugin_sample.c
