# $Id$
# $URL$

# Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = gnu
CLEANFILES = *~
DRIVERS=@DRIVERS@
PLUGINS=@PLUGINS@

bin_PROGRAMS = lcd4linux

# Fixme: -W should be renamed to -Wextra someday...
AM_CFLAGS = -D_GNU_SOURCE -Wall -W -fno-strict-aliasing

# use this for lots of warnings
#AM_CFLAGS = -D_GNU_SOURCE -std=c99 -m64 -Wall -W -pedantic -Wno-variadic-macros -fno-strict-aliasing

lcd4linux_LDFLAGS =
lcd4linux_LDADD   = @DRIVERS@ @PLUGINS@ @DRVLIBS@ @PLUGINLIBS@
lcd4linux_DEPENDENCIES = @DRIVERS@ @PLUGINS@

lcd4linux_SOURCES =           \
lcd4linux.c   svn_version.h   \
cfg.c         cfg.h           \
debug.c       debug.h         \
drv.c         drv.h           \
drv_generic.c drv_generic.h   \
evaluator.c   evaluator.h     \
property.c    property.h      \
hash.c        hash.h          \
layout.c      layout.h        \
pid.c         pid.h           \
timer.c       timer.h         \
thread.c      thread.h        \
udelay.c      udelay.h        \
qprintf.c     qprintf.h       \
rgb.c         rgb.h           \
                              \
widget.c      widget.h        \
widget_text.c widget_text.h   \
widget_bar.c  widget_bar.h    \
widget_icon.c widget_icon.h   \
widget_keypad.c widget_keypad.h \
widget_timer.c widget_timer.h \
widget_gpo.c  widget_gpo.h    \
                              \
plugin.c      plugin.h        \
plugin_cfg.c                  \
plugin_math.c                 \
plugin_string.c               \
plugin_test.c                 \
plugin_time.c

EXTRA_lcd4linux_SOURCES=      \
drv_generic_text.c            \
drv_generic_text.h            \
drv_generic_graphic.c         \
drv_generic_graphic.h         \
drv_generic_gpio.c            \
drv_generic_gpio.h            \
drv_generic_serial.c          \
drv_generic_serial.h          \
drv_generic_parport.c         \
drv_generic_parport.h         \
drv_generic_i2c.c             \
drv_generic_i2c.h             \
drv_generic_keypad.c          \
drv_generic_keypad.h          \
drv_BeckmannEgle.c            \
drv_BWCT.c                    \
drv_Crystalfontz.c            \
drv_Curses.c                  \
drv_Cwlinux.c                 \
drv_D4D.c                     \
drv_EA232graphic.c            \
drv_G15.c                     \
drv_HD44780.c                 \
drv_Image.c                   \
drv_IRLCD.c                   \
drv_LCD2USB.c                 \
drv_LCDLinux.c                \
drv_LCDTerm.c                 \
drv_LEDMatrix.c               \
drv_LPH7508.c                 \
drv_LUIse.c                   \
drv_M50530.c                  \
drv_MatrixOrbital.c           \
drv_MilfordInstruments.c      \
drv_Noritake.c                \
drv_NULL.c                    \
drv_Pertelian.c               \
drv_PHAnderson.c              \
drv_picoLCD.c                 \
drv_picoLCDGraphic.c          \
drv_RouterBoard.c             \
drv_Sample.c                  \
drv_st2205.c                  \
drv_serdisplib.c              \
drv_SimpleLCD.c               \
drv_T6963.c                   \
drv_Trefon.c                  \
drv_ULA200.c                  \
drv_USBHUB.c                  \
drv_USBLCD.c                  \
drv_WincorNixdorf.c           \
drv_X11.c                     \
                              \
font_6x8.h                    \
font_6x8_bold.h               \
widget_image.c widget_image.h \
                              \
lcd4linux_i2c.h               \
                              \
plugin_apm.c                  \
plugin_asterisk.c             \
plugin_button_exec.c          \
plugin_cpuinfo.c              \
plugin_diskstats.c            \
plugin_dvb.c                  \
plugin_exec.c                 \
plugin_fifo.c                 \
plugin_file.c                 \
plugin_gps.c                  \
plugin_hddtemp.c              \
plugin_i2c_sensors.c          \
plugin_iconv.c                \
plugin_imon.c                 \
plugin_isdn.c                 \
plugin_kvv.c                  \
plugin_loadavg.c              \
plugin_meminfo.c              \
plugin_mpd.c		      \
plugin_mysql.c                \
plugin_netdev.c               \
plugin_pop3.c                 \
plugin_ppp.c                  \
plugin_proc_stat.c            \
plugin_python.c               \
plugin_sample.c               \
plugin_seti.c                 \
plugin_statfs.c               \
plugin_uname.c                \
plugin_uptime.c               \
plugin_w1retap.c	      \
plugin_wireless.c             \
plugin_xmms.c


EXTRA_DIST =                  \
svn_version.sh                \
lcd4linux.conf.sample         \
lcd4kde.conf                  \
lcd4linux.kdelnk              \
lcd4linux.xpm                 \
lcd4linux.lsm                 \
ac_python_devel.m4            \
curses.m4                     \
drivers.m4                    \
plugins.m4                    \
AUTHORS                       \
CREDITS                       \
NEWS                          \
TODO                          \
README                        \
plugin_sample.c



# create subversion version

.PHONY: svn_version

svn_version:
	svn_version.sh

