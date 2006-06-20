dnl LCD4Linux Drivers conf part
dnl
dnl Copyright (C) 1999, 2000, 2001, 2002, 2003 Michael Reinelt <reinelt@eunet.at>
dnl Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
dnl
dnl This file is part of LCD4Linux.
dnl
dnl LCD4Linux is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.
dnl
dnl LCD4Linux is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

AC_MSG_CHECKING([which drivers to compile])
AC_ARG_WITH(
  drivers, 
  [  --with-drivers=<list>   compile driver for displays in <list>,]
  [                        drivers may be separated with commas,]	
  [                        'all' (default) compiles all available drivers,]	
  [                        drivers may be excluded with 'all,!<driver>',]	
  [                        (try 'all,\!<driver>' if your shell complains...)]	
  [                        possible drivers are:]	
  [                        BeckmannEgle, BWCT, CrystalFontz, Curses, Cwlinux,]
  [                        G15, HD44780, LCD2USB LCDLinux, LCDTerm, LPH7508,]
  [                        LUIse, M50530, MatrixOrbital, MilfordInstruments,]
  [                        Noritake, NULL, PNG, PPM, RouterBoard, Sample,]
  [                        serdisplib, SimpleLCD, T6963, Trefon, USBLCD,]
  [                        WincorNixdorf, X11],
  drivers=$withval, 
  drivers=all
)

drivers=`echo $drivers|sed 's/,/ /g'`

for driver in $drivers; do

   case $driver in 
      !*) 
         val="no"
         driver=`echo $driver|cut -c 2-`
         ;;
       *) 
         val="yes"
         ;;
   esac
	
   case "$driver" in
      all)
         BECKMANNEGLE="yes"
         BWCT="yes"
         CRYSTALFONTZ="yes"
         CURSES="yes"
         CWLINUX="yes"
         G15="yes"
         HD44780="yes"
         LCD2USB="yes"
	 LCDLINUX="yes"
         LCDTERM="yes"
	 LPH7508="yes"
         LUISE="yes"
         M50530="yes"
         MATRIXORBITAL="yes"
         MILINST="yes"
         NORITAKE="yes" 
         NULL="yes" 
         PNG="yes"
         PPM="yes"
	 ROUTERBOARD="yes"
         SAMPLE="yes"
	 SERDISPLIB="yes"
         SIMPLELCD="yes"
         T6963="yes"
         Trefon="yes"
         USBLCD="yes"
	 WINCORNIXDORF="yes"
         X11="yes"
         ;;
      BeckmannEgle)
         BECKMANNEGLE=$val
         ;;
      BWCT)
         BWCT=$val
         ;;
      CrystalFontz)
         CRYSTALFONTZ=$val
         ;;
      Curses)
         CURSES=$val
         ;;
      Cwlinux)
         CWLINUX=$val
         ;;
      G15)
         G15=$val
         ;;
      HD44780)
         HD44780=$val
	 ;;
      LCD2USB)
         LCD2USB=$val
         ;;
      LCDLinux)
         LCDLINUX=$val
	 ;;
      LCDTerm)
         LCDTERM=$val
	 ;;
      LPH7508)
         LPH7508=$val
         ;;
      LUIse)
         LUISE=$val
         ;;
      M50530)
         M50530=$val
         ;;
      MatrixOrbital)
         MATRIXORBITAL=$val
         ;;
      MilfordInstruments)
         MILINST=$val
         ;;
      Noritake)
         NORITAKE=$val;
         ;;
      NULL)
         NULL=$val;
         ;;
      PNG)
         PNG=$val
         ;;
      PPM)
         PPM=$val
         ;;
      RouterBoard)
         ROUTERBOARD=$val
         ;;
      Sample)
         SAMPLE=$val
         ;;
      serdisplib)
         SERDISPLIB=$val;
         ;;
      SimpleLCD)
         SIMPLELCD=$val
         ;;
      T6963)
         T6963=$val
         ;;
      Trefon)
         Trefon=$val
         ;;
      USBLCD)
         USBLCD=$val
         ;;
      WincorNixdorf)
         WINCORNIXDORF=$val
         ;;
      X11)
         X11=$val
         ;;
      *) 	
         AC_MSG_ERROR([Unknown driver '$driver'])
         ;;
   esac
done

AC_MSG_RESULT([done])


# generic display drivers
TEXT="no"
GRAPHIC="no"
IMAGE="no"
GPIO="no"

# generiv I/O drivers
PARPORT="no"
SERIAL="no"
I2C="no"
KEYPAD="no"

# generic libraries
LIBUSB="no"

if test "$BECKMANNEGLE" = "yes"; then
   TEXT="yes"
   GPIO="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_BeckmannEgle.o"
   AC_DEFINE(WITH_BECKMANNEGLE,1,[Beckmann&Egle driver])
fi

if test "$BWCT" = "yes"; then
   if test "$has_usb" = "true"; then
      TEXT="yes"
      DRIVERS="$DRIVERS drv_BWCT.o"
      LIBUSB="yes"
      AC_DEFINE(WITH_BWCT,1,[BWCT driver])
   else
      AC_MSG_WARN(usb.h not found: BWCT driver disabled)
   fi
fi

if test "$CRYSTALFONTZ" = "yes"; then
   TEXT="yes"
   GPIO="yes"
   SERIAL="yes"
   KEYPAD="yes"
   DRIVERS="$DRIVERS drv_Crystalfontz.o"
   AC_DEFINE(WITH_CRYSTALFONTZ,1,[Crystalfontz driver])
fi

if test "$CURSES" = "yes"; then
   if test "$has_curses" = true; then
      TEXT="yes"
      DRIVERS="$DRIVERS drv_Curses.o"
      DRVLIBS="$DRVLIBS $CURSES_LIBS"
      CPPFLAGS="$CPPFLAGS $CURSES_INCLUDES"
      AC_DEFINE(WITH_CURSES,1,[Curses driver])
   else
      AC_MSG_WARN(curses not found: Curses driver disabled)
   fi   
fi

if test "$CWLINUX" = "yes"; then
   TEXT="yes"
   GPIO="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_Cwlinux.o"
   AC_DEFINE(WITH_CWLINUX,1,[CwLinux driver])
fi

if test "$G15" = "yes"; then
   if test "$has_usb" = "true"; then
      GRAPHIC="yes"
      LIBUSB="yes"
      DRIVERS="$DRIVERS drv_G15.o"
      AC_DEFINE(WITH_G15,1,[G-15 driver])
   else
      AC_MSG_WARN(usb.h not found: G15 driver disabled)
   fi
fi

if test "$HD44780" = "yes"; then
   TEXT="yes"
   PARPORT="yes"
   I2C="yes"
   GPIO="yes"
   DRIVERS="$DRIVERS drv_HD44780.o"
   AC_DEFINE(WITH_HD44780,1,[HD44780 driver])
fi

if test "$LCD2USB" = "yes"; then
   if test "$has_usb" = "true"; then
      TEXT="yes"
      SERIAL="yes"
      DRIVERS="$DRIVERS drv_LCD2USB.o"
      LIBUSB="yes"
      AC_DEFINE(WITH_LCD2USB,1,[LCD2USB driver])
   else
      AC_MSG_WARN(usb.h not found: LCD2USB driver disabled)
   fi
fi

if test "$LCDLINUX" = "yes"; then
   if test "$has_lcd_linux" = true; then
      TEXT="yes"
      DRIVERS="$DRIVERS drv_LCDLinux.o"
      AC_DEFINE(WITH_LCDLINUX,1,[LCD-Linux driver])
   else
      AC_MSG_WARN(linux/lcd-linux.h or linux/hd44780.h not found: LCD-Linux driver disabled)
   fi   
fi

if test "$LCDTERM" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_LCDTerm.o"
   AC_DEFINE(WITH_LCDTERM,1,[LCDTerm driver])
fi

if test "$LPH7508" = "yes"; then
   GRAPHICS="yes"
   GPIO="yes"
   PARPORT="yes"
   DRIVERS="$DRIVERS drv_LPH7508.o"
   AC_DEFINE(WITH_LPH7508,1,[LPH7508 driver])
fi

if test "$LUISE" = "yes"; then
   if test "$has_luise" = "true"; then
      GRAPHIC="yes"
      DRIVERS="$DRIVERS drv_LUIse.o"
      DRVLIBS="$DRVLIBS -L/usr/local/lib -lluise"
      AC_DEFINE(WITH_LUISE,1,[LUIse driver])
   else
      AC_MSG_WARN(luise.h not found: LUIse driver disabled)
   fi
fi

if test "$M50530" = "yes"; then
   TEXT="yes"
   GPIO="yes"
   PARPORT="yes"
   DRIVERS="$DRIVERS drv_M50530.o"
   AC_DEFINE(WITH_M50530,1,[M50530 driver])
fi

if test "$MATRIXORBITAL" = "yes"; then
   TEXT="yes"
   GPIO="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_MatrixOrbital.o"
   AC_DEFINE(WITH_MATRIXORBITAL,1,[MatrixOrbital driver])
fi

if test "$MILINST" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_MilfordInstruments.o"
   AC_DEFINE(WITH_MILINST,1,[Milford Instruments driver])
fi

if test "$NORITAKE" = "yes"; then
   TEXT="yes"
   GRAPHIC="yes"
   PARPORT="yes"
   DRIVERS="$DRIVERS drv_Noritake.o"
   AC_DEFINE(WITH_NORITAKE,1,[Noritake driver])
fi

if test "$NULL" = "yes"; then
   DRIVERS="$DRIVERS drv_NULL.o"
   AC_DEFINE(WITH_NULL,1,[NULL driver])
fi

if test "$PNG" = "yes"; then
   if test "$has_gd" = "true"; then
      IMAGE="yes"
      AC_DEFINE(WITH_PNG,1,[PNG driver])
   else
      AC_MSG_WARN(gd.h not found: PNG driver disabled)
   fi
fi

if test "$PPM" = "yes"; then
   IMAGE="yes"
   AC_DEFINE(WITH_PPM,1,[PPM driver])
fi

if test "$ROUTERBOARD" = "yes"; then
   TEXT="yes"
   GPIO="yes"
   DRIVERS="$DRIVERS drv_RouterBoard.o"
   AC_DEFINE(WITH_ROUTERBOARD,1,[RouterBoard driver])
fi

if test "$SAMPLE" = "yes"; then
   # select either text or graphics mode
   TEXT="yes"
   GRAPHIC="yes"
   # support for GPIO's
   GPIO="yes"
   # select bus: serial (including USB), parallel or i2c
   SERIAL="yes"
   PARPORT="yes"
   #I2C="yes"
   DRIVERS="$DRIVERS drv_Sample.o"
   AC_DEFINE(WITH_SAMPLE,1,[Sample driver])
fi

if test "$SERDISPLIB" = "yes"; then
   if test "$has_serdisplib" = "true"; then
      GRAPHIC="yes"
      DRIVERS="$DRIVERS drv_serdisplib.o"
      DRVLIBS="$DRVLIBS -L/usr/local/lib -lserdisp"
      AC_DEFINE(WITH_SERDISPLIB,1,[serdisplib driver])
      if test "$has_usb" = "true"; then
         LIBUSB="yes"
      fi
   else
      AC_MSG_WARN(serdisp.h not found: serdisplib driver disabled)
   fi
fi

if test "$SIMPLELCD" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_SimpleLCD.o"
   AC_DEFINE(WITH_SIMPLELCD,1,[SimpleLCD driver])
fi

if test "$T6963" = "yes"; then
   GRAPHIC="yes"
   PARPORT="yes"
   DRIVERS="$DRIVERS drv_T6963.o"
   AC_DEFINE(WITH_T6963,1,[T6963 driver])
fi

if test "$Trefon" = "yes"; then
   if test "$has_usb" = "true"; then
      TEXT="yes"
      DRIVERS="$DRIVERS drv_Trefon.o"
      LIBUSB="yes"
      AC_DEFINE(WITH_TREFON,1,[TREFON driver])
   else
      AC_MSG_WARN(usb.h not found: Trefon driver disabled)
   fi
fi

if test "$USBLCD" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_USBLCD.o"
   if test "$has_usb" = "true"; then
      LIBUSB="yes"
   fi
   AC_DEFINE(WITH_USBLCD,1,[USBLCD driver])
fi

if test "$WINCORNIXDORF" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_WincorNixdorf.o"
   AC_DEFINE(WITH_WINCORNIXDORF,1,[WincorNixdorf driver])
fi

if test "$X11" = "yes"; then
   if test "$no_x" = "yes"; then
      AC_MSG_WARN(X11 headers or libraries not available: X11 driver disabled)
   else
      GRAPHIC="yes"
      DRIVERS="$DRIVERS drv_X11.o"
      DRVLIBS="$DRVLIBS -L$ac_x_libraries -lX11"
      CPP_FLAGS="$CPPFLAGS $X_CFLAGS" 
      AC_DEFINE(WITH_X11, 1, [X11 driver])
   fi
fi


if test "$DRIVERS" = ""; then
   AC_MSG_ERROR([You should activate at least one driver...])
fi
   
# generic text driver
if test "$TEXT" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_text.o"
fi

# Image driver
if test "$IMAGE" = "yes"; then
   GRAPHIC="yes"
   DRIVERS="$DRIVERS drv_Image.o"
fi

# generic graphic driver
if test "$GRAPHIC" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_graphic.o"
   if test "$has_gd" = "true"; then
      DRIVERS="$DRIVERS widget_image.o"
      DRVLIBS="$DRVLIBS -lgd"
      AC_DEFINE(WITH_IMAGE, 1, [image widget])
   fi	
fi

# generic GPIO driver
if test "$GPIO" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_gpio.o"
fi

# generic parport driver
if test "$PARPORT" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_parport.o"
fi

# generic serial driver
if test "$SERIAL" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_serial.o"
fi

# generic i2c driver
if test "$I2C" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_i2c.o"
   AC_DEFINE(WITH_I2C, 1, [I2C bus driver])
fi

# generic keypad driver
if test "$KEYPAD" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_keypad.o"
fi

# libusb
if test "$LIBUSB" = "yes"; then
   DRVLIBS="$DRVLIBS -lusb"
fi

AC_SUBST(DRIVERS)
AC_SUBST(DRVLIBS)
